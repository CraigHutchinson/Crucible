#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <SwarmIntro.hpp>

#include "crucible/presentation/production/nanite_model.hpp"
#include "crucible/presentation/production/swarm_intro_adapter.hpp"

namespace
{
using crucible::presentation::production::SwarmIntroAdapter;
void require(bool value, const char* reason) { if (!value) throw std::runtime_error(reason); }

// Independent source observers receive the new profile's contact/stacking behavior;
// these are the existing spike's gates, not geometry-generator mirror assertions.
struct SourceReceiving
{
    std::uint64_t unsupported_{}, offSurface_{}, climberTicks_{};
    double worstOverlapRatio_{};
    float minimumGap_{std::numeric_limits<float>::infinity()};
};
void observeSource(const crucible::spike::swarm::SwarmIntro& source, SourceReceiving& receiving)
{
    const auto& camera = source.camera();
    require(std::isfinite(camera.cx) && std::isfinite(camera.cy) && std::isfinite(camera.halfWidth) &&
        camera.halfWidth > 0 && std::isfinite(camera.yaw), "New-height source produced a nonfinite camera");
    const auto& metrics = source.metrics();
    require(std::isfinite(metrics.time) && std::isfinite(metrics.visibleMinGap), "New-height source contact observation is nonfinite");
    receiving.unsupported_ += metrics.unsupported;
    receiving.offSurface_ += metrics.offSurface;
    receiving.climberTicks_ += metrics.climbers;
    const double ratio = metrics.visibleMovers ? static_cast<double>(metrics.visibleOverlaps) / metrics.visibleMovers : 0;
    receiving.worstOverlapRatio_ = std::max(receiving.worstOverlapRatio_, ratio);
    if (metrics.visibleMinGap >= 0) receiving.minimumGap_ = std::min(receiving.minimumGap_, metrics.visibleMinGap);
}
void finishSourceReceiving(crucible::spike::swarm::SwarmIntro& source, const crucible::spike::swarm::IntroSettings& settings,
    SourceReceiving& receiving, unsigned presentedTicks)
{
    const auto complete = [&] {
        return source.time() >= source.sequenceSeconds() && source.metrics().locked == source.assignedSlots() &&
            source.metrics().raised == source.panelCount();
    };
    while (!complete() && presentedTicks < 4096)
    {
        source.tick();
        ++presentedTicks;
        observeSource(source, receiving);
    }
    std::cout << "New-height Scaffold source: aspect=" << source.camera().aspect << " ticks=" << presentedTicks
        << " height=" << settings.body.height << " unsupported=" << receiving.unsupported_
        << " off_surface=" << receiving.offSurface_ << " climber_ticks=" << receiving.climberTicks_
        << " worst_overlap_ratio=" << receiving.worstOverlapRatio_ << " minimum_gap=" << receiving.minimumGap_ << '\n';
    require(complete(), "New-height source failed to complete within4096presentation ticks");
    require(receiving.unsupported_ == 0, "New-height source made an unsupported lock");
    require(static_cast<double>(receiving.offSurface_) <= .005 * static_cast<double>(receiving.climberTicks_),
        "New-height source exceeded its off-surface contact gate");
    require(receiving.worstOverlapRatio_ <= .03, "New-height source exceeded its visible overlap gate");
    require(std::isfinite(receiving.minimumGap_) && receiving.minimumGap_ >= .5F * settings.diameter,
        "New-height source violated minimum mover separation");
}
}

int main()
{
    try
    {
        SwarmIntroAdapter adapter;
        require(!adapter.getFrame() && adapter.getProgress().totalPresentedTicks_ == 0, "Cold adapter fabricated a frame/tick");
        require(!adapter.trySetViewport(0, 640) && !adapter.trySetViewport(1, 4097) &&
            !adapter.trySetViewport(std::numeric_limits<float>::quiet_NaN(), 640), "Invalid viewport admitted");
        require(adapter.trySetViewport(2.0F / 3.0F, 640), "Portrait viewport refused");
        require(adapter.trySeekPresentedTicks(0) && adapter.getFrame(), "Explicit zero-tick receiving unavailable");
        require(adapter.getFrame()->camera_.orthographic_.has_value(), "Prototype camera replaced with perspective");
        const float receivedHeight = crucible::presentation::production::naniteCompactEnvelope()[3][1] * 1.10F;
        float highestInitial{};
        for (const auto& vertex : adapter.getFrame()->vertices_) highestInitial = std::max(highestInitial, vertex.position_[1]);
        require(highestInitial > receivedHeight * .8F && highestInitial <= receivedHeight + .003F,
            "Production source still uses the legacy proxy stacking height");
        const auto overloaded = adapter.advance(std::chrono::seconds{1});
        require(overloaded.advancedTicks_ == 4 && overloaded.totalPresentedTicks_ == 4 &&
            overloaded.droppedPresentationTime_ >= std::chrono::milliseconds{750}, "Catch-up/debt cap missing");
        require(adapter.advance(std::chrono::nanoseconds{0}).advancedTicks_ == 4, "Graphics-skip debt was silently discarded");
        adapter.suspend();
        require(adapter.advance(std::chrono::nanoseconds{0}).advancedTicks_ == 0, "Suspended debt leaked into playback");
        adapter.replay();
        require(!adapter.getFrame() && adapter.getProgress().totalPresentedTicks_ == 0, "Replay retained old frame/clock");
        require(!adapter.trySeekPresentedTicks(2701) && adapter.getProgress().totalPresentedTicks_ == 0, "Out-of-range seek advanced state");
        require(adapter.trySeekPresentedTicks(13), "Explicit receiving seek refused");
        require(!adapter.trySeekPresentedTicks(12) && adapter.getProgress().totalPresentedTicks_ == 13, "Backward seek mutated state");
        crucible::spike::swarm::IntroSettings settings;
        settings.build = crucible::spike::swarm::Build::Scaffold;
        settings.body.height = crucible::presentation::production::naniteCompactEnvelope()[3][1] * settings.body.span;
        settings.body.lean = 0;
        crucible::spike::swarm::SwarmIntro reference{settings};
        require(reference.trySetViewport(2.0F / 3.0F, 640), "Independent source viewport refused");
        SourceReceiving portraitReceiving;
        for (unsigned tick = 0; tick < 13; ++tick)
        {
            reference.tick();
            observeSource(reference, portraitReceiving);
        }
        require(adapter.getProgress().choreographySeconds_ == reference.time() &&
            adapter.getProgress().choreographySeconds_ != 13.0F / 60, "Presentation time reinterpreted as choreography time");
        finishSourceReceiving(reference, settings, portraitReceiving, 13);
        // The source's camera/decision LOD observes a different viewport on the
        // native desktop receiver; receive that profile separately from portrait.
        crucible::spike::swarm::SwarmIntro desktopReference{settings};
        require(desktopReference.trySetViewport(1280.0F / 864.0F, 1280), "Native desktop source viewport refused");
        SourceReceiving desktopReceiving;
        finishSourceReceiving(desktopReference, settings, desktopReceiving, 0);
        adapter.replay();
        const auto preparing = adapter.advance({}, SwarmIntroAdapter::PlaybackMode::settling);
        require(preparing.advancedTicks_ == 4 && !preparing.staticReady_ && !adapter.getFrame(), "Hidden preparation displayed animated swarm");
        for (unsigned pass = 0; pass < 1024 && !adapter.getProgress().staticReady_; ++pass)
        {
            const auto progress = adapter.advance({}, SwarmIntroAdapter::PlaybackMode::settling);
            require(progress.advancedTicks_ <= 4 && progress.totalPresentedTicks_ <= 4096, "Hidden settling exceeded coordinator bounds");
            if (!progress.staticReady_) require(!adapter.getFrame(), "Unsettled hidden frame became visible");
        }
        require(adapter.getProgress().staticReady_ && adapter.getProgress().sequenceFinished_ && adapter.getFrame(), "Default static preparation did not complete");
        const auto finishedTicks = adapter.getProgress().totalPresentedTicks_;
        require(adapter.trySetViewport(16.0F / 9.0F, 1280), "Settled viewport change refused");
        const auto held = adapter.advance(std::chrono::seconds{5});
        require(held.advancedTicks_ == 0 && held.totalPresentedTicks_ == finishedTicks && adapter.getFrame(), "Menu hold advanced source simulation");
        const auto frame = *adapter.getFrame();
        require(adapter.trySetViewport(16.0F / 9.0F, 1280), "Equal settled viewport refused");
        (void)adapter.advance({});
        require(adapter.getFrame()->vertices_.data() == frame.vertices_.data(), "Equal viewport changed the settled borrow storage");
        require(std::abs(frame.camera_.orthographic_->halfHeight_ -
            frame.camera_.orthographic_->halfWidth_ / (16.0F / 9.0F) / std::sqrt(1 + .4F * .4F)) < 1e-5,
            "Settled camera ignored actual drawable viewport");
        bool refused{};
        try { (void)adapter.advance(std::chrono::nanoseconds{-1}); } catch (const std::invalid_argument&) { refused = true; }
        require(refused && adapter.getProgress().totalPresentedTicks_ == finishedTicks, "Invalid elapsed time mutated state");
        std::cout << "Scaffold native source clocks/viewport/preparation received; actual settled presentation ticks=" << finishedTicks << '\n';
        return 0;
    }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
