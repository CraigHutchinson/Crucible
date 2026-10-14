#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <SwarmIntro.hpp>
#include <vector>

#include "crucible/presentation/production/detail/nanite_proxy_mesh.hpp"
#include "crucible/presentation/production/swarm_intro_adapter.hpp"

namespace crucible::presentation::production
{
struct SwarmIntroAdapter::State
{
    spike::swarm::IntroSettings settings_{};
    std::unique_ptr<spike::swarm::SwarmIntro> source_;
    std::vector<spike::swarm::RenderRow> rows_;
    detail::NaniteProxyMesh mesh_;
    AdvanceResult progress_{};
    std::int64_t debtUnits_{}; // One unit is1/60nanosecond; avoids rounding the60Hz tick.
    float aspect_{16.0F / 9.0F}, pixelsWide_{1920};
    bool frameAvailable_{}, refresh_{true};

    State()
    {
        settings_.build = spike::swarm::Build::Scaffold;
        settings_.nanites = 1536;
        rows_.reserve(NaniteMeshFrame::maxUnits);
        source_ = std::make_unique<spike::swarm::SwarmIntro>(settings_);
        if (!source_->trySetViewport(aspect_, pixelsWide_)) throw std::runtime_error("Default prototype viewport refused");
    }
    void updateProgress()
    {
        progress_.choreographySeconds_ = source_->time();
        progress_.sequenceFinished_ = source_->time() >= source_->sequenceSeconds();
        progress_.staticReady_ = progress_.sequenceFinished_ && source_->metrics().locked == source_->assignedSlots() &&
            source_->metrics().raised == source_->panelCount();
        if (!std::isfinite(progress_.choreographySeconds_)) throw std::runtime_error("Prototype choreography clock is nonfinite");
    }
    void tick()
    {
        source_->tick();
        ++progress_.totalPresentedTicks_;
        ++progress_.advancedTicks_;
        refresh_ = true;
        updateProgress();
    }
    void produce()
    {
        source_->snapshot(rows_);
        if (rows_.size() != settings_.nanites || rows_.capacity() > NaniteMeshFrame::maxUnits ||
            !mesh_.tryBuild(rows_, settings_.body, source_->camera(), settings_.viewTilt, source_->time(), settings_.diameter))
            throw std::runtime_error("Source nanite proxy mesh refused");
        frameAvailable_ = true;
        refresh_ = false;
    }
};

SwarmIntroAdapter::SwarmIntroAdapter() : state_(std::make_unique<State>()) {}
SwarmIntroAdapter::~SwarmIntroAdapter() = default;

bool SwarmIntroAdapter::trySetViewport(float aspect, float pixelsWide) noexcept
{
    if (!std::isfinite(aspect) || !std::isfinite(pixelsWide) || aspect < 1.0F / 32 || aspect > 32 || pixelsWide < 1 || pixelsWide > 4096)
        return false;
    if (aspect == state_->aspect_ && pixelsWide == state_->pixelsWide_) return true;
    if (!state_->source_->trySetViewport(aspect, pixelsWide)) return false;
    state_->aspect_ = aspect;
    state_->pixelsWide_ = pixelsWide;
    state_->refresh_ = true;
    return true;
}

SwarmIntroAdapter::AdvanceResult SwarmIntroAdapter::advance(std::chrono::nanoseconds elapsed, PlaybackMode mode)
{
    if (elapsed < std::chrono::nanoseconds::zero() || elapsed > std::chrono::hours{24} ||
        (mode != PlaybackMode::animated && mode != PlaybackMode::settling))
        throw std::invalid_argument("Unsupported decorative elapsed time or playback mode");
    auto& state = *state_;
    state.progress_.advancedTicks_ = 0;
    state.progress_.droppedPresentationTime_ = {};
    if (mode == PlaybackMode::settling)
    {
        state.debtUnits_ = 0;
        state.frameAvailable_ = state.progress_.staticReady_;
        for (unsigned tick = 0; tick < 4 && !state.progress_.staticReady_; ++tick)
        {
            if (state.progress_.totalPresentedTicks_ >= 4096)
                throw std::runtime_error("Default swarm did not settle within4096actual presentation ticks");
            state.tick();
        }
    }
    else if (!state.progress_.staticReady_)
    {
        constexpr std::int64_t maxDebtUnits = 250'000'000LL * 60;
        const auto accepted = std::min(elapsed.count(), std::int64_t{250'000'000});
        const auto combined = state.debtUnits_ + accepted * 60;
        const auto discardedUnits = std::max(combined - maxDebtUnits, std::int64_t{0});
        state.progress_.droppedPresentationTime_ = std::chrono::nanoseconds{elapsed.count() - accepted + discardedUnits / 60};
        state.debtUnits_ = std::min(combined, maxDebtUnits);
        for (unsigned tick = 0; tick < 4 && state.debtUnits_ >= 1'000'000'000 && !state.progress_.staticReady_; ++tick)
        {
            if (state.progress_.totalPresentedTicks_ >= 4096)
                throw std::runtime_error("Default swarm did not complete within4096actual presentation ticks");
            state.debtUnits_ -= 1'000'000'000;
            state.tick();
        }
    }
    if ((mode == PlaybackMode::animated && state.progress_.totalPresentedTicks_ != 0) || state.progress_.staticReady_)
    {
        if (state.refresh_ || !state.frameAvailable_) state.produce();
        state.frameAvailable_ = true;
    }
    return state.progress_;
}

void SwarmIntroAdapter::suspend() noexcept { state_->debtUnits_ = 0; }

void SwarmIntroAdapter::replay()
{
    auto replacement = std::make_unique<spike::swarm::SwarmIntro>(state_->settings_);
    if (!replacement->trySetViewport(state_->aspect_, state_->pixelsWide_))
        throw std::runtime_error("Prototype replay viewport refused");
    state_->source_.swap(replacement);
    state_->rows_.clear();
    state_->debtUnits_ = 0;
    state_->progress_ = {};
    state_->frameAvailable_ = false;
    state_->refresh_ = true;
}

bool SwarmIntroAdapter::trySeekPresentedTicks(std::uint32_t targetTicks)
{
    auto& state = *state_;
    if (targetTicks > 2700 || targetTicks < state.progress_.totalPresentedTicks_) return false;
    state.debtUnits_ = 0;
    state.progress_.advancedTicks_ = 0;
    state.progress_.droppedPresentationTime_ = {};
    while (state.progress_.totalPresentedTicks_ < targetTicks) state.tick();
    state.produce();
    return true;
}

std::optional<NaniteMeshFrame> SwarmIntroAdapter::getFrame() const noexcept
{
    return state_->frameAvailable_ ? std::optional{state_->mesh_.getFrame()} : std::nullopt;
}

SwarmIntroAdapter::AdvanceResult SwarmIntroAdapter::getProgress() const noexcept { return state_->progress_; }
}
