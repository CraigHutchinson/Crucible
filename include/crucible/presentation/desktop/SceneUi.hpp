#pragma once

#include <crucible/contracts/ReclamationMission.hpp>
#include <crucible/presentation/FieldTool.hpp>
#include <crucible/presentation/Camera2D.hpp>
#include <cstddef>
#include <optional>
#include <string_view>

namespace crucible::presentation::desktop {
/** Returns shared logical-pixel toolbar geometry for painting and input routing.
 * @param[in] index Zero-based position below ToolbarButtonCount.
 * @return Logical-pixel button rectangle used by painting and input.
 */
[[nodiscard]] constexpr ScreenRect ToolbarButton(std::size_t index) noexcept {
    return {24.0 + 148.0 * static_cast<double>(index), 632, 136, 32};
}
/// Seven existing controls retain their positions; FLOW is appended at index seven.
inline constexpr std::size_t ToolbarButtonCount = 8;
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
};
}
