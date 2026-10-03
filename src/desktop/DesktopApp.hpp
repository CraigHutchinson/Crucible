#pragma once
#include <crucible/runtime/InspectorSession.hpp>
#include <crucible/presentation/Camera2D.hpp>
#include <crucible/presentation/FieldTool.hpp>
#include <crucible/presentation/desktop/ScenePainter.hpp>
#include <SDL3/SDL.h>
#include <chrono>
#include <memory>
#include <string_view>
namespace crucible::desktop {
/** Concrete main-thread SDL lifetime and event adapter. No producer threads.
 * SDL callbacks guard thread identity before borrowing this instance.
 */
class DesktopApp {
public:
    /** Allocates the reference challenge and SDL window/renderer.
     * @param[in] mission Positive quota/deadline tuning within startup substrate stock.
     * @throws std::invalid_argument Invalid mission; startup/allocation errors propagate.
     */
    explicit DesktopApp(ReclamationMissionSettings mission = {});
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
    [[nodiscard]] std::optional<FieldEdit> GetPreview() const noexcept { return preview_; }
    [[nodiscard]] SDL_Window& GetWindow() const noexcept { return *window_; }
private:
    struct WindowDelete { void operator()(SDL_Window* p) const noexcept { SDL_DestroyWindow(p); } };
    struct RendererDelete { void operator()(SDL_Renderer* p) const noexcept { SDL_DestroyRenderer(p); } };
    enum class Action { attract, repel, erase, slot, pause, restart, fit };
    void Act(Action action);
    void Admit(FieldEdit edit);
    void Preview(presentation::ScreenPoint point);
    void Suspend(bool value);
    [[nodiscard]] presentation::ScreenPoint ToLogical(float x, float y) const;
    std::unique_ptr<SDL_Window, WindowDelete> window_;
    std::unique_ptr<SDL_Renderer, RendererDelete> renderer_;
    runtime::InspectorSession session_;
    presentation::Camera2D camera_{{64, 32, 1.0F}, {24, 96, 1232, 520}};
    presentation::desktop::ScenePainter painter_{2048, 2048};
    presentation::FieldTool tool_{presentation::FieldTool::attract};
    std::size_t slot_{};
    std::optional<FieldEdit> preview_;
    std::optional<runtime::CommandIngress::Admission> admission_;
    presentation::ScreenPoint last_pointer_{};
    bool dragging_{}, suspended_{}, restore_running_{}, background_{}, minimized_{};
    std::string_view message_{"Recover biomass before the deadline - click world to steer"};
    std::chrono::steady_clock::time_point baseline_{std::chrono::steady_clock::now()};
};
}
