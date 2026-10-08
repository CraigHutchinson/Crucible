#pragma once
#include <chrono>
#include <cstdint>
#include <memory>
#include <SDL3/SDL.h>
#include <string_view>

#include "crucible/presentation/Camera2D.hpp"
#include "crucible/presentation/desktop/checked_presentation.hpp"
#include "crucible/presentation/desktop/ScenePainter.hpp"
#include "crucible/presentation/desktop/SceneUi.hpp"
#include "crucible/presentation/FieldTool.hpp"
#include "crucible/runtime/InspectorSession.hpp"

namespace crucible::desktop {
/** Concrete main-thread SDL lifetime and event adapter. No producer threads.
 * SDL callbacks guard thread identity before borrowing this instance.
 */
class DesktopApp {
public:
    enum class WindowMode { windowed, fullscreen };
    /// Legacy SDL calls or the received concrete Windows native handoff path.
    enum class PresentationMode { sdl, checkedD3D11 };
    /** Owns consumed desktop startup policy around the platform-free scenario.
     * Omit mission for a continuously evolving inspector. Restart retains this policy.
     */
    struct StartupSettings {
        ScenarioSettings scenario{}; ///< Authoritative population/geometry/capacity.
        std::optional<ReclamationMissionSettings> mission{ReclamationMissionSettings{}}; ///< Session quota policy.
        runtime::InspectorSession::Diagnostics diagnostics{runtime::InspectorSession::Diagnostics::disabled}; ///< Optional outcome sink.
        WindowMode windowMode{WindowMode::windowed}; ///< Reversible initial display policy.
        float toolRadius{8.0F}; ///< Radial radius and FLOW corridor half-width in world units.
        float toolMagnitude{4.0F}; ///< Shared live/script force magnitude.
        presentation::desktop::ScenePainter::ViewPolicy viewPolicy{presentation::desktop::ScenePainter::ViewPolicy::exact}; ///< Consumed view-only representation.
        PresentationMode presentationMode{PresentationMode::sdl}; ///< Checked mode requires the concrete native capability.
    };
    /** Copies timing from one production iteration without retaining frame storage.
     * Durations describe CPU call intervals only; GPU completion is separate.
     */
    struct FrameStatistics {
        std::uint64_t frameId{}, runId{}, completedTick{};
        std::chrono::nanoseconds service{}, pump{}, draw{}, present{};
        std::size_t advancedTicks{};
        bool presented{}; ///< Native presentation call returned; not successful scanout proof.
        std::optional<presentation::desktop::CheckedPresentation::Receipt> nativeReceipt{}; ///< Explicit native outcome in checked mode only.
    };
    /** Allocates the reference challenge and SDL window/renderer.
     * @param[in] mission Positive quota/deadline tuning within startup substrate stock.
     * @param[in] structural Enables the fixed relay challenge.
     * @param[in] diagnostics Optional bounded outcome log, decoded to stdout at shutdown.
     * Requires CRUCIBLE_ENABLE_DIAGNOSTICS; allocation failure is reported and disables logging.
     * @param[in] window_mode Requests borderless fullscreen or the default resizable window.
     * Fullscreen failure is reported and falls back to a playable window.
     * @throws std::invalid_argument Invalid mission; startup/allocation errors propagate.
     */
    explicit DesktopApp(ReclamationMissionSettings mission = {}, bool structural = false,
        runtime::InspectorSession::Diagnostics diagnostics = runtime::InspectorSession::Diagnostics::disabled,
        WindowMode window_mode = WindowMode::windowed);
    /** Allocates receivers from the same validated scenario used by the session.
     * @param settings Scenario, optional mission and consumed tool/window policy.
     * @throws std::invalid_argument Invalid scenario or tool settings; startup errors propagate.
     * @note Coordinator-owned SDL lifetime; the toolbar currently requires four field slots.
     */
    explicit DesktopApp(StartupSettings settings);
    ~DesktopApp();
    DesktopApp(const DesktopApp&) = delete;
    DesktopApp& operator=(const DesktopApp&) = delete;
    /** Converts native window coordinates before input; copies edits through ingress.
     * @param[in] event Main-thread event borrow, valid only during this call.
     * @return Continue, or success after closing ingress on quit.
     * @throws std::runtime_error SDL conversion/output errors; restart errors propagate.
     */
    [[nodiscard]] SDL_AppResult HandleEvent(const SDL_Event& event);
    /** Advances bounded fixed ticks and draws the latest owned frame.
     * @return Continue after rendering or skipping suspended/zero-output presentation.
     * @throws std::runtime_error SDL errors; stopped simulation errors propagate.
     */
    [[nodiscard]] SDL_AppResult Iterate();
    /// Coordinator-only observations used by actual drawing and event acceptance fixtures.
    [[nodiscard]] const runtime::InspectorSession& GetSession() const noexcept { return session_; }
    [[nodiscard]] const presentation::Camera2D& GetCamera() const noexcept { return camera_; }
    [[nodiscard]] std::optional<FieldEdit> GetPreview() const noexcept { return preview_ ? preview_ : admitted_preview_; }
    [[nodiscard]] SDL_Window& GetWindow() const noexcept { return *window_; }
    /** Returns the native renderer for the coordinator's completion-observation receiver.
     * @return Renderer owned by this app, valid until app destruction.
     * @note No worker may retain or access this borrow.
     */
    [[nodiscard]] SDL_Renderer& getRenderer() const noexcept { return *renderer_; }
    /** Copies the latest iteration's CPU timings and frame identity.
     * @return Completed call intervals, with presented false for skipped iterations.
     * @note Coordinator-only; GPU completion and physical scanout are not inferred.
     */
    [[nodiscard]] FrameStatistics getFrameStatistics() const noexcept { return frameStatistics_; }
    /** Copies representation counts from the latest successful production draw.
     * @return Authoritative, individual, aggregated and hidden population counts.
     * @note Coordinator-only; counts never alter simulation participation.
     */
    [[nodiscard]] presentation::desktop::ScenePainter::DrawStatistics getDrawStatistics() const noexcept {
        return painter_.getDrawStatistics();
    }
private:
    struct WindowDelete { void operator()(SDL_Window* p) const noexcept { SDL_DestroyWindow(p); } };
    struct RendererDelete { void operator()(SDL_Renderer* p) const noexcept { SDL_DestroyRenderer(p); } };
    using Action = presentation::desktop::ToolbarAction;
    void Act(Action action);
    void setFullscreen(bool enabled);
    void Admit(FieldEdit edit);
    void Preview(presentation::ScreenPoint point);
    void CancelGesture() noexcept;
    void Suspend(bool value);
    [[nodiscard]] presentation::ScreenPoint ToLogical(float x, float y) const;
    std::unique_ptr<SDL_Window, WindowDelete> window_;
    std::unique_ptr<SDL_Renderer, RendererDelete> renderer_;
    std::unique_ptr<presentation::desktop::CheckedPresentation> checkedPresentation_;
    runtime::InspectorSession session_;
    presentation::Camera2D camera_;
    presentation::desktop::ScenePainter painter_;
    const float toolRadius_, toolMagnitude_;
    FrameStatistics frameStatistics_{};
    std::uint64_t frameId_{};
    presentation::FieldTool tool_{presentation::FieldTool::attract};
    std::size_t slot_{};
    std::optional<FieldEdit> preview_;
    std::optional<FieldEdit> admitted_preview_;
    std::uint64_t admitted_sequence_{};
    std::optional<Position> flow_start_;
    std::optional<runtime::CommandIngress::Admission> admission_;
    std::optional<runtime::CommandIngress::Admission> structural_admission_;
    const bool structural_;
    presentation::ScreenPoint last_pointer_{};
    bool dragging_{}, suspended_{}, restore_running_{}, background_{}, minimized_{};
    std::string_view message_{"Recover biomass before the deadline - click world to steer"};
    std::chrono::steady_clock::time_point baseline_{std::chrono::steady_clock::now()};
};
}
