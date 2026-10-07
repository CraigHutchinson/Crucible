#pragma once

#include <cstddef>
#include <optional>
#include <string_view>

#include "crucible/contracts/ReclamationMission.hpp"
#include "crucible/presentation/Camera2D.hpp"
#include "crucible/presentation/FieldTool.hpp"

namespace crucible::presentation::desktop {
inline constexpr int CanvasWidth = 1280;
inline constexpr int CanvasHeight = 864;
inline constexpr float BodyTextScale = 2.5F;
/// Stable toolbar actions shared by the painter and main-thread input adapter.
enum class ToolbarAction { attract, repel, erase, slot, pause, restart, fit, flow, fuse, shatter, fullscreen };
inline constexpr std::size_t ToolbarButtonCount = 11;
/** Returns shared logical-pixel toolbar geometry for painting and input routing.
 * @param[in] index Zero-based position below ToolbarButtonCount.
 * @return Logical-pixel button rectangle used by painting and input.
 */
[[nodiscard]] constexpr ScreenRect toolbarButton(std::size_t index) noexcept {
    return {24.0 + 206.0 * static_cast<double>(index % 6),
        650.0 + 48.0 * static_cast<double>(index / 6), 196, 40};
}
/** Reports whether the current scenario can consume a toolbar action.
 * @param action Toolbar action from the shared enum.
 * @param structural Whether this run has a relay lattice.
 * @return True for an enabled control; false for legacy fuse/shatter.
 */
[[nodiscard]] constexpr bool isToolbarActionEnabled(ToolbarAction action, bool structural) noexcept {
    return structural || (action != ToolbarAction::fuse && action != ToolbarAction::shatter);
}
/** Resolves a pointer to a toolbar action, excluding gaps and disabled structure controls.
 * @param[in] point Logical canvas coordinate, valid only for this call.
 * @param[in] structural Whether this run supports fuse/shatter.
 * @return Matching action, or no action outside an enabled button.
 */
[[nodiscard]] constexpr std::optional<ToolbarAction> tryToolbarAction(ScreenPoint point, bool structural) noexcept {
    for (std::size_t index = 0; index < ToolbarButtonCount; ++index) {
        const auto bounds = toolbarButton(index);
        const auto action = static_cast<ToolbarAction>(index);
        if (!isToolbarActionEnabled(action, structural)) continue;
        if (point.x >= bounds.x && point.x < bounds.x + bounds.width &&
            point.y >= bounds.y && point.y < bounds.y + bounds.height) return action;
    }
    return std::nullopt;
}
static_assert(static_cast<std::size_t>(ToolbarAction::fullscreen) + 1 == ToolbarButtonCount);
/** Display-only controls and feedback, distinct from committed simulation fields.
 * Message storage is borrowed only during a painter call; no UI values enter ECS.
 */
struct SceneUi {
    bool paused{};
    bool blocked{};
    FieldTool tool{FieldTool::attract};
    std::size_t selected_slot{};
    std::optional<FieldEdit> preview{};
    std::string_view message{};
    std::optional<ReclamationMissionProgress> mission{}; ///< Owned progress for this captured boundary.
    bool fullscreen_{}; ///< Actual window state; never enters simulation.
};
}
