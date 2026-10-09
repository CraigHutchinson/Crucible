#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

#include "crucible/contracts/ReclamationMission.hpp"

namespace crucible::application
{
/// The two received mission recipes; progression policy is application-owned.
enum class MissionId { reclaimFront = 0, secureRelay = 1 };

/// Player screens; only playing permits the application to advance Runtime.
enum class FrontendScreen
{
    intro, menu, missions, briefing, playing, paused, options, results,
    saveFailed, quitConfirmation
};

/// Finite typography choices, received separately from framebuffer density.
enum class TextScale { standard = 0, large = 1, extraLarge = 2 };

/// Consumed player preferences; sound controls await a real audio consumer.
struct FrontendOptions
{
    bool reducedMotion_{};
    bool fullscreen_{};
    TextScale textScale_{TextScale::standard};
};

/// Logical safe-content coordinates; desktop and mobile layouts use the same value.
struct SafeContentRect
{
    float x_{}, y_{}, width_{1280.0F}, height_{864.0F};
};

/// One mission card borrowing bounded UTF-8 labels until the frontend draw returns.
struct MissionCard
{
    MissionId id_{MissionId::reclaimFront};
    std::string_view title_{};
    std::string_view objective_{};
    bool enabled_{true};
    std::string_view disabledReason_{};
};

/** Synchronous player view with owned progress and borrowed presentation labels.
 * Cards and strings expire after draw; consumers retain only IDs/focus metadata.
 * Card count is at most two. Labels are valid UTF-8 without embedded nulls:
 * title <=96 bytes, objective/reason <=256, status/error <=512.
 * @note Coordinator-only. transitionSerial_ correlates a returned intent with
 * this particular screen state; it is independent of Runtime's session-local ID.
 */
struct FrontendView
{
    FrontendScreen screen_{FrontendScreen::intro};
    std::uint64_t transitionSerial_{1};
    MissionId selectedMission_{MissionId::reclaimFront};
    std::span<const MissionCard> cards_{}; ///< Non-owning until draw returns.
    bool canContinue_{};
    std::string_view continueReason_{};
    FrontendOptions options_{};
    std::string_view status_{};
    std::string_view error_{};
    std::optional<ReclamationMissionProgress> progress_{};
    SafeContentRect safeContent_{};
};
}
