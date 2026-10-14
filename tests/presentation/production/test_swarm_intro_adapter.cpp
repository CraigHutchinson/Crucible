#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <SwarmIntro.hpp>

#include "crucible/presentation/production/swarm_intro_adapter.hpp"

namespace
{
using crucible::presentation::production::SwarmIntroAdapter;
void require(bool value, const char* reason) { if (!value) throw std::runtime_error(reason); }
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
        crucible::spike::swarm::SwarmIntro reference{settings};
        require(reference.trySetViewport(2.0F / 3.0F, 640), "Independent source viewport refused");
        for (unsigned tick = 0; tick < 13; ++tick) reference.tick();
        require(adapter.getProgress().choreographySeconds_ == reference.time() &&
            adapter.getProgress().choreographySeconds_ != 13.0F / 60, "Presentation time reinterpreted as choreography time");
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
