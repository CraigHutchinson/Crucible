#include "DesktopApp.hpp"
#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <SDL3/SDL.h>
#include <iostream>
#include <stdexcept>
#include <string_view>
namespace {
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void Send(crucible::desktop::DesktopApp& app, const SDL_Event& event) {
    Require(app.HandleEvent(event) == SDL_APP_CONTINUE, "event stays live");
}
void Key(crucible::desktop::DesktopApp& app, SDL_Keycode key) {
    SDL_Event e{}; e.type = SDL_EVENT_KEY_DOWN; e.key.key = key; Send(app, e);
}
void Click(crucible::desktop::DesktopApp& app, float x, float y, Uint8 button = SDL_BUTTON_LEFT) {
    SDL_Event e{}; e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    e.button.x = x; e.button.y = y; e.button.button = button; Send(app, e);
}
void Check() {
    using Status = crucible::runtime::ClockDriver::Status;
    crucible::desktop::DesktopApp app;
    Require(app.Iterate() == SDL_APP_CONTINUE, "initial draw");
    Click(app, 630, 646); // toolbar pause
    Require(app.GetSession().GetStatus() == Status::paused, "pointer pause");
    const auto initial = app.GetSession().GetSummary().completed_tick;
    Click(app, 30, 40); // header never admits an edit
    Require(app.GetSession().GetSummary().ingress_pending == 0, "HUD rejected");
    Key(app, SDLK_2);
    Click(app, 640, 350);
    Require(app.GetSession().GetSummary().ingress_pending == 1 && app.GetPreview()->strength < 0,
        "world input becomes signed queued command");
    SDL_Delay(20);
    Require(app.Iterate() == SDL_APP_CONTINUE && app.GetSession().GetSummary().completed_tick == initial &&
        app.GetSession().GetTrace().empty(), "pause keeps queued input uncommitted");
    Key(app, SDLK_SPACE);
    SDL_Delay(20);
    Require(app.Iterate() == SDL_APP_CONTINUE && app.GetSession().GetTrace().size() == 1 &&
        !app.GetPreview(), "boundary confirms input");
    SDL_Event hover{}; hover.type = SDL_EVENT_MOUSE_MOTION; hover.motion.x = 600; hover.motion.y = 350;
    Send(app, hover);
    Require(app.Iterate() == SDL_APP_CONTINUE && app.GetPreview().has_value(), "new preview survives old application feedback");
    SDL_Event wheel{}; wheel.type = SDL_EVENT_MOUSE_WHEEL;
    wheel.wheel.mouse_x = 640; wheel.wheel.mouse_y = 350; wheel.wheel.y = 2;
    const auto fit = app.GetCamera().GetScale(); Send(app, wheel);
    Require(app.GetCamera().GetScale() > fit, "wheel zoom");
    Click(app, 640, 350, SDL_BUTTON_MIDDLE);
    SDL_Event motion{}; motion.type = SDL_EVENT_MOUSE_MOTION; motion.motion.x = 700; motion.motion.y = 350;
    Send(app, motion);
    const auto dragged = app.GetCamera().TryToWorld({640, 350});
    SDL_Event focus{}; focus.type = SDL_EVENT_WINDOW_FOCUS_LOST; Send(app, focus);
    motion.motion.x = 760; Send(app, motion);
    const auto after_focus = app.GetCamera().TryToWorld({640, 350});
    Require(dragged->x == after_focus->x && dragged->y == after_focus->y, "focus loss cancels lost-release drag");
    Key(app, SDLK_F);
    Require(app.GetCamera().GetScale() == fit, "fit resets camera");
    Key(app, SDLK_SPACE);
    SDL_Event suspend{}; suspend.type = SDL_EVENT_WILL_ENTER_BACKGROUND; Send(app, suspend);
    Click(app, 640, 350);
    Require(app.GetSession().GetSummary().ingress_pending == 0, "background rejects input");
    suspend.type = SDL_EVENT_WINDOW_MINIMIZED; Send(app, suspend);
    suspend.type = SDL_EVENT_DID_ENTER_FOREGROUND; Send(app, suspend);
    Require(app.GetSession().GetStatus() == Status::paused, "foreground cannot bypass minimized gate");
    suspend.type = SDL_EVENT_WINDOW_RESTORED; Send(app, suspend);
    Require(app.GetSession().GetStatus() == Status::paused, "foreground preserves user pause");
    Click(app, 780, 646); // toolbar restart
    Require(app.GetSession().GetSnapshot().GetInfo()->completed_tick == 0 && app.GetSession().GetTrace().empty() &&
        !app.GetPreview() && app.GetSession().GetStatus() == Status::running, "pointer restart fresh run");
    suspend.type = SDL_EVENT_WILL_ENTER_BACKGROUND; Send(app, suspend);
    suspend.type = SDL_EVENT_WINDOW_MINIMIZED; Send(app, suspend);
    suspend.type = SDL_EVENT_DID_ENTER_FOREGROUND; Send(app, suspend);
    Require(app.GetSession().GetStatus() == Status::paused, "running run waits for every suspension reason");
    suspend.type = SDL_EVENT_WINDOW_RESTORED; Send(app, suspend);
    Require(app.GetSession().GetStatus() == Status::running, "aggregate restore resumes prior running state");
    Key(app, SDLK_SPACE);
    Require(SDL_SetWindowSize(&app.GetWindow(), 800, 800), "resize native window");
    Require(SDL_SyncWindow(&app.GetWindow()), "resize completes before coordinate assertions");
    SDL_PumpEvents();
    Require(app.Iterate() == SDL_APP_CONTINUE, "resized logical draw");
    Click(app, 400, 100);
    Require(app.GetSession().GetSummary().ingress_pending == 0, "resized presentation letterbox rejects commands");
    Click(app, 400, 393.75F);
    Require(app.GetSession().GetSummary().ingress_pending == 1 && app.GetPreview()->center.x == 32,
        "resized native window maps through logical presentation");
    SDL_Event quit{}; quit.type = SDL_EVENT_QUIT;
    Require(app.HandleEvent(quit) == SDL_APP_SUCCESS && app.GetSession().GetStatus() == Status::closed, "close ingress before teardown");
}
}
int main(int argc, char** argv) {
    const bool native = argc > 1 && std::string_view{argv[1]} == "--native";
    if (!native) { SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy"); SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software"); }
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) { std::cerr << SDL_GetError(); return 1; }
    int result{};
    try { Check(); } catch (const std::exception& e) { std::cerr << e.what() << ": " << SDL_GetError() << '\n'; result = 1; }
    SDL_Quit();
    return result;
}
