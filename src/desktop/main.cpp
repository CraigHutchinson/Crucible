#define SDL_MAIN_USE_CALLBACKS
#include <exception>
#include <memory>
#include <SDL3/SDL_main.h>
#include <string_view>

#include "desktop/DesktopApp.hpp"

SDL_AppResult SDL_AppInit(void** state, int argc, char** argv) {
    *state = nullptr;
    try {
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) return SDL_APP_FAILURE;
        // SDL owns the callback token until SDL_AppQuit reclaims it.
        bool structural = false;
        auto window_mode = crucible::desktop::DesktopApp::WindowMode::windowed;
        auto diagnostics = crucible::runtime::InspectorSession::Diagnostics::disabled;
        for (int i = 1; i < argc; ++i) {
            if (std::string_view{argv[i]} == "--structural") structural = true;
            else if (std::string_view{argv[i]} == "--fullscreen")
                window_mode = crucible::desktop::DesktopApp::WindowMode::fullscreen;
            else if (std::string_view{argv[i]} == "--diagnostics")
                diagnostics = crucible::runtime::InspectorSession::Diagnostics::bounded;
            else { SDL_Log("Unknown option: %s", argv[i]); return SDL_APP_FAILURE; }
        }
        *state = std::make_unique<crucible::desktop::DesktopApp>(crucible::ReclamationMissionSettings{}, structural, diagnostics, window_mode).release();
        return SDL_APP_CONTINUE;
    } catch (const std::exception& e) { SDL_Log("Crucible startup: %s", e.what()); return SDL_APP_FAILURE; }
}
SDL_AppResult SDL_AppEvent(void* state, SDL_Event* event) {
    // SDL can deliver pushed events on their producer thread. No ECS/SDL owner
    // borrow is permitted there; this prototype has no event-producing workers.
    if (!SDL_IsMainThread()) return SDL_APP_FAILURE;
    try { return static_cast<crucible::desktop::DesktopApp*>(state)->HandleEvent(*event); }
    catch (const std::exception& e) { SDL_Log("Crucible input: %s", e.what()); return SDL_APP_FAILURE; }
}
SDL_AppResult SDL_AppIterate(void* state) {
    try { return static_cast<crucible::desktop::DesktopApp*>(state)->Iterate(); }
    catch (const std::exception& e) { SDL_Log("Crucible frame: %s", e.what()); return SDL_APP_FAILURE; }
}
void SDL_AppQuit(void* state, SDL_AppResult) {
    std::unique_ptr<crucible::desktop::DesktopApp> owned{static_cast<crucible::desktop::DesktopApp*>(state)};
    owned.reset();
    SDL_Quit();
}
