#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <new>
#include <stdexcept>
#include <vector>

#include "crucible/presentation/production/cinematic_choreography.hpp"

namespace
{
std::atomic<bool> countAllocations{};
std::atomic<std::size_t> allocationCalls{};

void require(bool condition, const char* reason)
{
    if (!condition)
    {
        throw std::runtime_error(reason);
    }
}
}

void* operator new(std::size_t size)
{
    if (countAllocations.load(std::memory_order_relaxed))
    {
        allocationCalls.fetch_add(1, std::memory_order_relaxed);
    }
    if (auto* memory = std::malloc(std::max(size, std::size_t{1})))
    {
        return memory;
    }
    throw std::bad_alloc{};
}
void* operator new[](std::size_t size)
{
    return ::operator new(size);
}
void operator delete(void* memory) noexcept
{
    std::free(memory);
}
void operator delete[](void* memory) noexcept
{
    std::free(memory);
}
void operator delete(void* memory, std::size_t) noexcept
{
    std::free(memory);
}
void operator delete[](void* memory, std::size_t) noexcept
{
    std::free(memory);
}

namespace
{
using namespace crucible::presentation::production;
using namespace std::chrono_literals;
using Point = std::array<double, 3>;

Point subtract(const std::array<float, 3>& from, const std::array<float, 3>& to)
{
    return {static_cast<double>(from[0]) - to[0], static_cast<double>(from[1]) - to[1],
        static_cast<double>(from[2]) - to[2]};
}

double dot(const Point& left, const Point& right)
{
    return left[0] * right[0] + left[1] * right[1] + left[2] * right[2];
}

Point normalized(Point value)
{
    const double inverse = 1 / std::sqrt(dot(value, value));
    for (auto& component : value)
    {
        component *= inverse;
    }
    return value;
}

void samePoses(std::span<const NanitePose> left, std::span<const NanitePose> right)
{
    require(left.size() == right.size(), "stable pose count");
    for (std::size_t index = 0; index < left.size(); ++index)
    {
        require(left[index].position_ == right[index].position_
            && left[index].orientation_ == right[index].orientation_
            && left[index].scale_ == right[index].scale_
            && left[index].coreIntensity_ == right[index].coreIntensity_, "repeatable owned pose fields");
    }
}

void validFrame(const CinematicFrame& frame)
{
    require(!frame.poses_.empty() && frame.poses_.size() <= CinematicFrame::maxInstances, "instance capacity");
    for (const auto& pose : frame.poses_)
    {
        for (const auto component : pose.position_)
        {
            require(std::isfinite(component) && std::abs(component) <= 512, "finite bounded position");
        }
        double squared{};
        for (const auto component : pose.orientation_)
        {
            require(std::isfinite(component), "finite orientation");
            squared += static_cast<double>(component) * component;
        }
        require(std::abs(squared - 1) <= 0.002, "unit XYZW orientation");
        require(pose.scale_ > 0 && pose.scale_ <= 16 && std::isfinite(pose.scale_), "scale bound");
        require(pose.coreIntensity_ >= 0 && pose.coreIntensity_ <= 4, "core bound");
    }
    for (const auto value : frame.camera_.eye_)
    {
        require(std::isfinite(value) && std::abs(value) <= 512, "camera eye bound");
    }
    for (const auto value : frame.camera_.target_)
    {
        require(std::isfinite(value) && std::abs(value) <= 512, "camera target bound");
    }
    require(frame.camera_.verticalFovRadians_ > 0.1F && frame.camera_.verticalFovRadians_ < 2.8F,
        "perspective field of view");
    const auto sight = subtract(frame.camera_.target_, frame.camera_.eye_);
    require(dot(sight, sight) >= 0.01, "nondegenerate view");
    const auto forward = normalized(sight);
    require(std::sqrt(forward[0] * forward[0] + forward[2] * forward[2]) >= 0.01, "stable +Y up");
    require(frame.keyIntensity_ >= 0 && frame.keyIntensity_ <= 8
        && frame.glowIntensity_ >= 0 && frame.glowIntensity_ <= 4, "lighting bounds");
}

void fittedWord(const CinematicFrame& frame, float aspect)
{
    // Independent right-handed look-at projection; include a conservative mesh radius.
    const auto forward = normalized(subtract(frame.camera_.target_, frame.camera_.eye_));
    const auto right = normalized(Point{-forward[2], 0, forward[0]});
    const Point up{right[1] * forward[2] - right[2] * forward[1],
        right[2] * forward[0] - right[0] * forward[2],
        right[0] * forward[1] - right[1] * forward[0]};
    const double tangent = std::tan(frame.camera_.verticalFovRadians_ * 0.5);
    for (const auto& pose : frame.poses_)
    {
        const auto relative = subtract(pose.position_, frame.camera_.eye_);
        const auto depth = dot(relative, forward);
        require(depth > 0.6 && depth < 2047.5, "settled mesh inside depth planes");
        const double horizontal = (std::abs(dot(relative, right)) + 0.5) / ((depth - 0.5) * tangent * aspect);
        const double vertical = (std::abs(dot(relative, up)) + 0.5) / ((depth - 0.5) * tangent);
        require(horizontal < 1 && vertical < 1, "whole word including mesh radius fits portrait/landscape");
    }
}

void authoredWord(std::span<const NanitePose> poses)
{
    std::array<std::size_t, 8> counts{};
    for (const auto& pose : poses)
    {
        bool assigned{};
        for (std::size_t letter = 0; letter < counts.size(); ++letter)
        {
            const float origin = -31 + static_cast<float>(letter) * 8;
            if (pose.position_[0] >= origin - 0.5F && pose.position_[0] <= origin + 6.5F)
            {
                ++counts[letter];
                assigned = true;
            }
        }
        require(assigned && pose.position_[1] >= 1.5F && pose.position_[1] <= 10.5F
            && pose.position_[2] == 0.08F, "authored separated letter envelopes");
    }
    require(std::all_of(counts.begin(), counts.end(), [](auto count) { return count > 50; }), "eight populated letters");
    require(counts[0] == counts[3], "two C silhouettes have the same geometry count");
    constexpr std::array<std::array<float, 2>, 6> counters{{
        {-27, 6}, {-20, 8}, {-12, 9}, {-3, 6}, {11, 8}, {11, 4}}};
    for (const auto& counter : counters)
    {
        require(std::none_of(poses.begin(), poses.end(), [&](const NanitePose& pose)
            { return std::hypot(pose.position_[0] - counter[0], pose.position_[1] - counter[1]) < 0.7F; }),
            "C/R/U/B counters remain open instead of generic particle blocks");
    }
}
}

int main()
{
    try
    {
        CinematicChoreography producer;
        allocationCalls = 0;
        countAllocations = true;
        const auto first = producer.evaluate(0ms, 1.5F);
        countAllocations = false;
        require(allocationCalls == 0, "first valid evaluation ordinary allocation calls are zero");
        const auto* storage = first.poses_.data();
        const std::vector<NanitePose> opening(first.poses_.begin(), first.poses_.end());
        samePoses(opening, producer.evaluate(-100ms, 1.5F).poses_);
        const auto final = producer.evaluate(12000ms, 1.5F);
        const std::vector<NanitePose> settled(final.poses_.begin(), final.poses_.end());
        authoredWord(settled);
        samePoses(settled, producer.evaluate(std::chrono::milliseconds::max(), 1.5F).poses_);
        samePoses(settled, producer.evaluate(9000ms, 1.5F).poses_);
        for (const float aspect : {1.0F / 32, 0.5F, 2.0F / 3, 1.0F, 1.5F, 2.0F, 32.0F})
        {
            for (int time = 0; time <= 12000; time += 73)
            {
                const auto frame = producer.evaluate(std::chrono::milliseconds{time}, aspect);
                validFrame(frame);
                require(frame.poses_.data() == storage && frame.poses_.size() == settled.size(), "reused stable span");
            }
            for (const int boundary : {0, 1, 1800, 3300, 3600, 4200, 6000, 6800, 8132, 10000, 12000})
            {
                validFrame(producer.evaluate(std::chrono::milliseconds{boundary}, aspect));
            }
            fittedWord(producer.evaluate(9000ms, aspect), aspect);
            fittedWord(producer.evaluate(12000ms, aspect), aspect);
        }
        const auto repeated = producer.evaluate(5300ms, 0.5F);
        const auto camera = repeated.camera_;
        const std::vector<NanitePose> repeatPoses(repeated.poses_.begin(), repeated.poses_.end());
        (void)producer.evaluate(1700ms, 2.0F);
        const auto again = producer.evaluate(5300ms, 0.5F);
        samePoses(repeatPoses, again.poses_);
        require(camera.eye_ == again.camera_.eye_ && camera.target_ == again.camera_.target_
            && camera.verticalFovRadians_ == again.camera_.verticalFovRadians_, "no history-dependent camera");
        for (const float invalid : {0.0F, -1.0F, 0.01F, 33.0F, std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::quiet_NaN()})
        {
            bool rejected{};
            try
            {
                (void)producer.evaluate(0ms, invalid);
            }
            catch (const std::invalid_argument&)
            {
                rejected = true;
            }
            require(rejected, "invalid aspect rejected");
            samePoses(repeatPoses, again.poses_);
        }
        allocationCalls = 0;
        countAllocations = true;
        for (int time = 0; time <= 12000; time += 13)
        {
            (void)producer.evaluate(std::chrono::milliseconds{time}, 0.5F);
        }
        countAllocations = false;
        require(allocationCalls == 0, "valid evaluation ordinary allocation calls are zero");
        std::printf("cinematic choreography PASS: %zu authored instances, all aspect/cue bounds, stable borrows, 0 ordinary allocations\n", settled.size());
        return 0;
    }
    catch (const std::exception& error)
    {
        countAllocations = false;
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
