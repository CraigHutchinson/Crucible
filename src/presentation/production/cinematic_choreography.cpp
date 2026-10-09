#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <stdexcept>

#include "crucible/presentation/production/cinematic_choreography.hpp"

namespace crucible::presentation::production
{
namespace
{
struct Stroke
{
    std::array<float, 2> from_, to_;
};

// Authored vector strokes preserve open counters; no raster/font masking enters the scene.
constexpr std::array cStrokes{
    Stroke{{6, 8}, {2, 8}}, Stroke{{2, 8}, {0, 6}}, Stroke{{0, 6}, {0, 2}},
    Stroke{{0, 2}, {2, 0}}, Stroke{{2, 0}, {6, 0}}};
constexpr std::array rStrokes{
    Stroke{{0, 0}, {0, 8}}, Stroke{{0, 8}, {4, 8}}, Stroke{{4, 8}, {6, 6}},
    Stroke{{6, 6}, {6, 5}}, Stroke{{6, 5}, {4, 3.8F}},
    Stroke{{4, 3.8F}, {0, 3.8F}}, Stroke{{3, 3.8F}, {6, 0}}};
constexpr std::array uStrokes{
    Stroke{{0, 8}, {0, 2}}, Stroke{{0, 2}, {2, 0}}, Stroke{{2, 0}, {4, 0}},
    Stroke{{4, 0}, {6, 2}}, Stroke{{6, 2}, {6, 8}}};
constexpr std::array iStrokes{
    Stroke{{0, 8}, {6, 8}}, Stroke{{3, 8}, {3, 0}}, Stroke{{0, 0}, {6, 0}}};
constexpr std::array bStrokes{
    Stroke{{0, 0}, {0, 8}}, Stroke{{0, 8}, {4, 8}}, Stroke{{4, 8}, {6, 6}},
    Stroke{{6, 6}, {4, 4}}, Stroke{{4, 4}, {0, 4}},
    Stroke{{4, 4}, {6, 2}}, Stroke{{6, 2}, {4, 0}}, Stroke{{4, 0}, {0, 0}}};
constexpr std::array lStrokes{Stroke{{0, 8}, {0, 0}}, Stroke{{0, 0}, {6, 0}}};
constexpr std::array eStrokes{
    Stroke{{6, 8}, {0, 8}}, Stroke{{0, 8}, {0, 0}}, Stroke{{0, 0}, {6, 0}},
    Stroke{{0, 4}, {4.8F, 4}}};
constexpr std::array<std::span<const Stroke>, 8> letters{
    cStrokes, rStrokes, uStrokes, cStrokes, iStrokes, bStrokes, lStrokes, eStrokes};

constexpr float smooth(float value) noexcept
{
    value = std::clamp(value, 0.0F, 1.0F);
    return value * value * (3.0F - 2.0F * value);
}

constexpr float cue(float time, float start, float end) noexcept
{
    return smooth((time - start) / (end - start));
}

constexpr std::array<float, 3> mix(const std::array<float, 3>& from,
    const std::array<float, 3>& to, float amount) noexcept
{
    return {std::lerp(from[0], to[0], amount), std::lerp(from[1], to[1], amount),
        std::lerp(from[2], to[2], amount)};
}

std::array<float, 4> mixOrientation(const std::array<float, 4>& from,
    const std::array<float, 4>& to, float amount) noexcept
{
    float dot{};
    for (std::size_t i = 0; i < from.size(); ++i)
    {
        dot += from[i] * to[i];
    }
    const float sign = dot < 0 ? -1.0F : 1.0F;
    std::array<float, 4> result{};
    float squared{};
    for (std::size_t i = 0; i < result.size(); ++i)
    {
        result[i] = std::lerp(from[i], sign * to[i], amount);
        squared += result[i] * result[i];
    }
    const float inverse = 1.0F / std::sqrt(squared);
    for (auto& component : result)
    {
        component *= inverse;
    }
    return result;
}

constexpr std::array<float, 3> sourcePosition(std::size_t index) noexcept
{
    const auto lane = index % 12;
    return {-10.0F + static_cast<float>(lane % 3) * 1.35F
            + static_cast<float>(lane / 3) * 4.4F,
        0.23F, 4.0F + static_cast<float>(index / 12) * 0.43F};
}

CinematicCamera fitCamera(float aspect, float fraction, float distance,
    std::array<float, 3> target) noexcept
{
    float fov = 0.62F;
    const float needed = 68.0F / (2.0F * std::tan(fov * 0.5F) * aspect * fraction);
    distance = std::clamp(std::max(distance, needed), 70.0F, 340.0F);
    if (needed > 340.0F)
    {
        fov = 2.0F * std::atan(68.0F / (2.0F * distance * aspect * fraction));
    }
    return {{target[0], target[1] + 8.0F, target[2] + distance}, target, fov};
}

CinematicCamera mixCamera(const CinematicCamera& from, const CinematicCamera& to,
    float amount) noexcept
{
    return {mix(from.eye_, to.eye_, amount), mix(from.target_, to.target_, amount),
        std::lerp(from.verticalFovRadians_, to.verticalFovRadians_, amount)};
}
}

CinematicChoreography::CinematicChoreography()
{
    settled_.reserve(CinematicFrame::maxInstances);
    letters_.reserve(CinematicFrame::maxInstances);
    constexpr float pitch = 0.44F;
    constexpr float halfTurn = std::numbers::pi_v<float> * 0.5F;
    constexpr float rootHalf = std::numbers::sqrt2_v<float> * 0.5F;
    for (std::size_t letter = 0; letter < letters.size(); ++letter)
    {
        for (const auto& stroke : letters[letter])
        {
            const float dx = stroke.to_[0] - stroke.from_[0];
            const float dy = stroke.to_[1] - stroke.from_[1];
            const float length = std::hypot(dx, dy);
            const auto steps = static_cast<std::size_t>(std::ceil(length / pitch));
            const float angle = (std::atan2(dy, dx) + halfTurn) * 0.5F;
            const float sine = std::sin(angle), cosine = std::cos(angle);
            const std::array<float, 4> orientation{
                rootHalf * cosine, rootHalf * sine, rootHalf * sine, rootHalf * cosine};
            for (std::size_t step = 0; step < steps; ++step)
            {
                const float along = (static_cast<float>(step) + 0.5F)
                    / static_cast<float>(steps);
                for (const float offset : {-0.39F, 0.0F, 0.39F})
                {
                    if (settled_.size() == CinematicFrame::maxInstances)
                    {
                        throw std::length_error("Authored cinematic exceeds instance capacity");
                    }
                    settled_.push_back({{
                        -31.0F + static_cast<float>(letter) * 8.0F
                            + std::lerp(stroke.from_[0], stroke.to_[0], along) - dy / length * offset,
                        2.0F + std::lerp(stroke.from_[1], stroke.to_[1], along) + dx / length * offset,
                        0.08F}, orientation, 0.43F, 0.85F});
                    letters_.push_back(static_cast<std::uint8_t>(letter));
                }
            }
        }
    }
    poses_.resize(settled_.size());
}

CinematicFrame CinematicChoreography::evaluate(std::chrono::milliseconds elapsed,
    float drawableAspect)
{
    if (!std::isfinite(drawableAspect) || drawableAspect < 1.0F / 32.0F || drawableAspect > 32.0F)
    {
        throw std::invalid_argument("Cinematic drawable aspect is outside [1/32,32]");
    }
    const float time = static_cast<float>(std::clamp(elapsed, std::chrono::milliseconds{0}, duration).count());
    constexpr std::array<float, 4> floorOrientation{0, 0, 0, 1};
    for (std::size_t index = 0; index < settled_.size(); ++index)
    {
        const auto& destination = settled_[index];
        auto& pose = poses_[index];
        pose = destination;
        const float letter = static_cast<float>(letters_[index]);
        const float start = 3300.0F + letter * 260.0F + static_cast<float>(index % 13) * 26.0F;
        const float assembly = cue(time, start, start + 2700.0F);
        if (assembly < 1.0F)
        {
            const auto origin = sourcePosition(index);
            const std::array<float, 3> lane{
                destination.position_[0] * 0.4F, 0.23F, 7.0F + static_cast<float>(index % 11) * 0.43F};
            const auto departure = mix(origin, lane, cue(time, 1800.0F, 3600.0F));
            pose.position_ = mix(departure, destination.position_, assembly);
            // A smooth airborne arc belongs only to the decorative title scene.
            pose.position_[1] += 3.6F * assembly * (1.0F - assembly);
            pose.orientation_ = mixOrientation(floorOrientation, destination.orientation_, assembly);
            const float wake = cue(time, static_cast<float>(index % 112) * 5.0F, 800.0F + static_cast<float>(index % 112) * 5.0F);
            const float arrival = smooth((assembly - 0.72F) / 0.18F) - smooth((assembly - 0.94F) / 0.06F);
            pose.coreIntensity_ = 0.08F + wake * 0.9F + arrival * 1.1F;
        }
    }

    const auto first = sourcePosition(0);
    const float macroDistance = std::max(2.8F, 1.4F / (2.0F * std::tan(0.275F) * drawableAspect * 0.85F));
    const CinematicCamera macro{{first[0] + 1.1F, 1.05F, first[2] + macroDistance}, first, 0.55F};
    const auto flowing = fitCamera(drawableAspect, 0.9F, 116.0F, {0, 2, 18});
    const auto hero = fitCamera(drawableAspect, 0.9F, 84.0F, {0, 6, 0});
    const bool wide = drawableAspect >= 1.2F;
    auto menu = fitCamera(drawableAspect, wide ? 0.64F : 0.86F, 116.0F,
        {wide ? -17.0F : 0.0F, 6.0F, 0});
    if (!wide)
    {
        const float verticalSpan = 2.0F * (menu.eye_[2] - menu.target_[2])
            * std::tan(menu.verticalFovRadians_ * 0.5F);
        const float shift = std::min(240.0F, verticalSpan * 0.23F);
        menu.eye_[1] -= shift;
        menu.target_[1] -= shift;
    }
    auto camera = mixCamera(macro, flowing, cue(time, 1800, 4200));
    camera = mixCamera(camera, hero, cue(time, 4200, 6800));
    camera = mixCamera(camera, menu, cue(time, 10000, 12000));
    return {poses_, camera, std::lerp(1.6F, 4.2F, cue(time, 1800, 6800)),
        std::lerp(0.25F, 0.8F, cue(time, 0, 1800))};
}
}
