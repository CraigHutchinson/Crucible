#include <chrono>
#include <iostream>
#include <stdexcept>

#include "crucible/application/game_journey.hpp"
#include "crucible/runtime/ReferenceMissionRoute.hpp"

namespace
{
using namespace crucible;
using namespace crucible::application;
using namespace std::chrono_literals;

void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

void navigate(GameJourney& journey, NavigationAction action)
{
    const auto view = journey.getFrontendView();
    require(journey.tryApplyIntent({view.screen_, view.transitionSerial_, action}),
        "Legal navigation was rejected");
}

void checkNavigationAndClock()
{
    GameJourney journey;
    const auto intro = journey.getFrontendView();
    require(intro.screen_ == FrontendScreen::intro && !intro.canContinue_ &&
        journey.getAttemptId() == 0 && !journey.getSnapshot(), "Unexpected startup state");
    require(!journey.tryPump(1h), "Intro advanced Runtime");
    navigate(journey, NavigationAction::skipIntro);
    require(!journey.tryApplyIntent({intro.screen_, intro.transitionSerial_, NavigationAction::newRun}),
        "Consumed intro event activated another screen");
    navigate(journey, NavigationAction::showMissions);
    auto view = journey.getFrontendView();
    require(!journey.tryApplyIntent({view.screen_, view.transitionSerial_, SelectMission{MissionId::secureRelay}}),
        "Locked relay could be selected");
    require(!journey.tryApplyIntent({view.screen_, view.transitionSerial_,
        SelectMission{static_cast<MissionId>(99)}}), "Invalid mission enum was accepted");
    navigate(journey, NavigationAction::back);
    navigate(journey, NavigationAction::newRun);
    navigate(journey, NavigationAction::beginMission);
    require(journey.getAttemptId() == 1 && journey.getSnapshot(), "First attempt was not constructed");
    const auto fractional = journey.tryPump(8ms);
    require(fractional && fractional->advanced_ticks == 0, "Fractional clock unexpectedly advanced");
    navigate(journey, NavigationAction::pause);
    const auto paused = journey.getFrontendView();
    navigate(journey, NavigationAction::showOptions);
    view = journey.getFrontendView();
    require(journey.tryApplyIntent({view.screen_, view.transitionSerial_, SetReducedMotion{true}}),
        "Consumed preference was rejected");
    require(!journey.tryApplyIntent({view.screen_, view.transitionSerial_, SetFullscreen{true}}),
        "Stale option serial was accepted");
    view = journey.getFrontendView();
    require(!journey.tryApplyIntent({view.screen_, view.transitionSerial_,
        SetTextScale{static_cast<TextScale>(99)}}), "Invalid typography enum was accepted");
    require(!journey.tryPump(1h), "Options advanced Runtime");
    navigate(journey, NavigationAction::back);
    require(journey.getFrontendView().screen_ == FrontendScreen::paused &&
        journey.getFrontendView().options_.reducedMotion_, "Options lost the paused return state");
    require(!journey.tryApplyIntent({paused.screen_, paused.transitionSerial_, NavigationAction::resume}),
        "Old paused intent resumed the session");
    navigate(journey, NavigationAction::resume);
    const auto freshFraction = journey.tryPump(9ms);
    require(freshFraction && freshFraction->advanced_ticks == 0,
        "Suspended clock fraction leaked into resumed play");
    const auto advanced = journey.tryPump(8ms);
    require(advanced && advanced->advanced_ticks == 1, "Resumed clock failed to advance normally");
    navigate(journey, NavigationAction::requestQuit);
    require(!journey.tryPump(1h), "Quit confirmation advanced Runtime");
    navigate(journey, NavigationAction::cancelQuit);
    require(journey.getFrontendView().screen_ == FrontendScreen::playing,
        "Quit cancellation lost the original playing screen");
    navigate(journey, NavigationAction::requestQuit);
    navigate(journey, NavigationAction::confirmQuit);
    require(journey.isQuitRequested() && !journey.tryPump(1h), "Confirmed exit still advances play");
}

void checkRealMissionProgression()
{
    GameJourney journey;
    navigate(journey, NavigationAction::skipIntro);
    navigate(journey, NavigationAction::newRun);
    navigate(journey, NavigationAction::beginMission);
    for (std::size_t boundary = 0; boundary < 900; ++boundary)
    {
        const auto view = journey.getFrontendView();
        if (view.screen_ == FrontendScreen::results) break;
        require(view.progress_.has_value(), "Real mission lost progress");
        if (const auto edit = runtime::GetReferenceMissionRouteEdit(view.progress_->completed_tick))
        {
            const auto admission = journey.tryAdmitFieldEdit(*edit);
            require(admission && admission->status == runtime::CommandIngress::AdmissionStatus::accepted,
                "Production route command was rejected");
        }
        const auto pump = journey.tryPump(16'666'667ns);
        require(pump && pump->advanced_ticks == 1, "Real mission failed a boundary");
    }
    const auto result = journey.getFrontendView();
    require(result.screen_ == FrontendScreen::results && result.progress_ &&
        result.progress_->outcome == ReclamationMissionOutcome::won && result.cards_[1].enabled_,
        "Actual mission completion did not unlock the relay");
    const auto terminalSerial = result.transitionSerial_;
    require(!journey.tryPump(1h) && journey.getFrontendView().transitionSerial_ == terminalSerial,
        "Repeated completion changed the terminal result");
    navigate(journey, NavigationAction::nextMission);
    require(journey.getFrontendView().selectedMission_ == MissionId::secureRelay,
        "Next mission did not select the unlocked recipe");
    navigate(journey, NavigationAction::beginMission);
    require(journey.getAttemptId() == 2 && journey.getFrontendView().progress_->completed_tick == 0,
        "A new session reused the application attempt identity or old progress");
    navigate(journey, NavigationAction::pause);
    navigate(journey, NavigationAction::retryMission);
    require(journey.getAttemptId() == 3, "Retry did not establish an independent attempt");
    navigate(journey, NavigationAction::pause);
    navigate(journey, NavigationAction::back);
    require(journey.getFrontendView().screen_ == FrontendScreen::menu && !journey.getSnapshot() &&
        !journey.tryPump(1h), "Return to menu retained or advanced an abandoned attempt");
}
}

int main()
{
    try
    {
        checkNavigationAndClock();
        checkRealMissionProgression();
        std::cout << "Application journey: real mission progression, stale intent rejection,"
                     " independent attempts and suspended clocks passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
