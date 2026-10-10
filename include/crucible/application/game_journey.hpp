#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>

#include "crucible/application/frontend_intent.hpp"
#include "crucible/application/frontend_view.hpp"
#include "crucible/application/profile_store.hpp"
#include "crucible/runtime/ClockDriver.hpp"
#include "crucible/runtime/CommandIngress.hpp"

namespace crucible::presentation { class ScenarioSnapshot; }
namespace crucible::runtime { class InspectorSession; }

namespace crucible::application
{
/** Coordinates player screens around complete, owned mission sessions.
 * Only the playing screen pumps Runtime. Each fresh mission has a monotonic
 * application attempt identity, independent of the session-local run ID.
 * @note Coordinator-only. An optional profile stores mission-boundary continuation
 * and preferences, never live simulation state or a mid-mission checkpoint.
 */
class GameJourney
{
public:
    /** Loads the optional profile before exposing a player view.
     * @param profilePath Explicit destination, or omission for an in-memory journey.
     * @throws std::exception Cold profile/path allocation failure.
     * @note Invalid/unsupported files remain untouched and are reported in the view.
     */
    explicit GameJourney(std::optional<std::filesystem::path> profilePath = std::nullopt);
    ~GameJourney();
    GameJourney(const GameJourney&) = delete;
    GameJourney& operator=(const GameJourney&) = delete;

    /** Applies one correlated player action, with all legality checks first.
     * @param intent Action originating from the current screen and serial.
     * @return True when applied; false for stale, invalid or unavailable actions.
     * @throws std::exception Mission construction failures preserve the old session.
     */
    [[nodiscard]] bool tryApplyIntent(const FrontendIntent& intent);
    /** Advances only an active mission; screen transitions discard clock debt.
     * @param elapsed Time since the last active call, excluding suspended time.
     * @return Actual Runtime result, or no result when not playing.
     * @throws std::exception Runtime errors propagate without fabricating a result.
     */
    [[nodiscard]] std::optional<runtime::ClockDriver::PumpResult> tryPump(
        std::chrono::nanoseconds elapsed);
    /** Copies a field command only while the player is actively playing.
     * @param edit Owned bounded field edit; acceptance is not application.
     * @return Admission result, or no result outside active play.
     */
    [[nodiscard]] std::optional<runtime::CommandIngress::Admission> tryAdmitFieldEdit(
        const FieldEdit& edit);
    /** Borrows labels/cards and copies completed-boundary progress for UI drawing.
     * @param safeContent Logical content bounds supplied by the platform owner.
     * @return Synchronous view; string/card borrows expire on mutation/destruction.
     */
    [[nodiscard]] FrontendView getFrontendView(SafeContentRect safeContent = {}) const noexcept;
    /** Borrows the latest owned mission snapshot without exposing Runtime mutation.
     * @return Null without a retained mission; otherwise a synchronous snapshot borrow.
     * @note Borrow expires on successful pump, session replacement or destruction.
     */
    [[nodiscard]] const presentation::ScenarioSnapshot* getSnapshot() const noexcept;
    /** Identifies the last constructed application attempt.
     * @return Zero before play; increases only after successful replacement.
     */
    [[nodiscard]] constexpr std::uint64_t getAttemptId() const noexcept { return attemptId_; }
    /** Reports the user's confirmed exit without conflating it with navigation.
     * @return True only after a legal quit confirmation.
     */
    [[nodiscard]] constexpr bool isQuitRequested() const noexcept { return quitRequested_; }

private:
    [[nodiscard]] bool tryNavigate(NavigationAction action);
    [[nodiscard]] bool tryBeginMission();
    void saveProfileFor(FrontendScreen returnScreen);
    void transitionTo(FrontendScreen screen) noexcept;

    std::unique_ptr<runtime::InspectorSession> session_;
    std::array<MissionCard, 2> cards_;
    FrontendScreen screen_{FrontendScreen::intro};
    FrontendScreen returnScreen_{FrontendScreen::menu};
    FrontendScreen quitReturnScreen_{FrontendScreen::menu};
    MissionId selectedMission_{MissionId::reclaimFront};
    std::optional<MissionId> continuation_;
    FrontendOptions options_{};
    std::optional<ProfileStore> profileStore_;
    std::optional<ProfileStore::Error> profileError_;
    FrontendScreen saveReturnScreen_{FrontendScreen::results};
    std::uint64_t transitionSerial_{1}, attemptId_{};
    bool quitRequested_{};
    bool loadRefused_{}, unsavedProfile_{};
};
}
