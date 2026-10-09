#pragma once

#include <cstdint>
#include <variant>

#include "crucible/application/frontend_view.hpp"

namespace crucible::application
{
/// Navigation requests whose legality is checked against the originating screen.
enum class NavigationAction
{
    continueRun, newRun, showMissions, showOptions, back, beginMission, pause, resume,
    retryMission, nextMission, requestQuit, confirmQuit, cancelQuit, replayIntro,
    skipIntro, retrySave, continueWithoutSaving
};

/// Requests a selected mission without attaching unrelated option values.
struct SelectMission { MissionId id_{MissionId::reclaimFront}; };
/// Changes only the reduced-motion preference.
struct SetReducedMotion { bool value_{}; };
/// Changes only the fullscreen preference.
struct SetFullscreen { bool value_{}; };
/// Changes only the finite text-scale preference.
struct SetTextScale { TextScale value_{TextScale::standard}; };

/** Returns one correlated player action without mutating authoritative state.
 * @note Coordinator-only. Root rejects stale screen/serial values before any
 * allocation, session replacement, native setting change or persistence write.
 */
struct FrontendIntent
{
    FrontendScreen originScreen_{FrontendScreen::intro};
    std::uint64_t transitionSerial_{1};
    std::variant<NavigationAction, SelectMission, SetReducedMotion, SetFullscreen,
        SetTextScale> action_{NavigationAction::skipIntro};
};
}
