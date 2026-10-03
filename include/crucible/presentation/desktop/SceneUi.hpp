#pragma once

#include <crucible/presentation/FieldTool.hpp>
#include <crucible/presentation/Camera2D.hpp>
#include <cstddef>
#include <optional>
#include <string_view>

namespace crucible::presentation::desktop {
/** Display-only controls and feedback, distinct from committed simulation fields.
 * Message storage is borrowed only during a painter call; no UI values enter ECS.
 */
/// Shared fixed prototype layout; input and paint use the same button geometry.
[[nodiscard]] constexpr ScreenRect ToolbarButton(std::size_t index) noexcept {
    return {24.0 + 148.0 * static_cast<double>(index), 632, 136, 32};
}
inline constexpr std::size_t ToolbarButtonCount = 7;
struct SceneUi {
    bool paused{};
    bool blocked{};
    FieldTool tool{FieldTool::attract};
    std::size_t selected_slot{};
    std::optional<FieldEdit> preview{};
    std::string_view message{};
};
}
