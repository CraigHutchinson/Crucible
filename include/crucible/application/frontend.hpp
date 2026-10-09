#pragma once

#include <filesystem>
#include <memory>
#include <optional>

#include "crucible/application/frontend_intent.hpp"

struct ImGuiContext;

namespace crucible::application
{
/** Presents composed production screens and correlated player intentions.
 * @note Coordinator-owned. The root-created ImGui context outlives this object;
 * platform events, frame sequencing, authoritative state and persistence belong
 * to the application. No SDL renderer or graphics device is borrowed here.
 */
class Frontend
{
public:
    /** Loads the two trusted bundled fonts and installs the Crucible theme.
     * @param context Root-owned context, already current on the coordinator.
     * @param fontDirectory Asset directory containing rajdhani and source-sans.
     * @throws std::invalid_argument The context is not current or its atlas is locked.
     * @throws std::runtime_error A required bounded font file cannot be read.
     * @throws std::bad_alloc Cold frontend/font storage cannot be allocated.
     * @note Loaded font data transfers to the context's atlas. Its matched
     * graphics adapter owns texture uploads; later glyph baking is not free.
     * Allocation failure after registration can leave atlas entries; the caller
     * retires a failed startup context before retrying frontend construction.
     * @pre Cold startup with a mutable atlas, outside an active ImGui frame.
     */
    Frontend(ImGuiContext& context, const std::filesystem::path& fontDirectory);
    /// Releases local focus/style metadata; the borrowed context is not destroyed.
    ~Frontend();
    Frontend(const Frontend&) = delete;
    Frontend& operator=(const Frontend&) = delete;
    Frontend(Frontend&&) = delete;
    Frontend& operator=(Frontend&&) = delete;

    /** Draws one validated view using real ImGui controls.
     * @param view Synchronous bounded labels/cards and copied player state.
     * @return One correlated action, or nullopt when no control was activated.
     * @throws std::invalid_argument Invalid view, context or supported dimensions.
     * @pre The same context is current inside a root-created active ImGui frame.
     * @note Coordinator-only. No view borrow survives; this does not call
     * NewFrame/Render, route native events or apply any authoritative mutation.
     */
    [[nodiscard]] std::optional<FrontendIntent> draw(const FrontendView& view);
private:
    struct State;
    std::unique_ptr<State> state_;
};
}
