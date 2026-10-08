#define SDL_MAIN_USE_CALLBACKS
#include <charconv>
#include <exception>
#include <memory>
#include <optional>
#include <SDL3/SDL_main.h>
#include <string_view>

#include "desktop/DesktopApp.hpp"

SDL_AppResult SDL_AppInit(void** state, int argc, char** argv) {
    *state = nullptr;
    try {
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) return SDL_APP_FAILURE;
        // SDL owns the callback token until SDL_AppQuit reclaims it.
        bool structural = false;
        std::optional<std::size_t> scalePopulation;
        std::optional<std::size_t> workers, partitions;
        auto window_mode = crucible::desktop::DesktopApp::WindowMode::windowed;
        auto diagnostics = crucible::runtime::InspectorSession::Diagnostics::disabled;
        for (int i = 1; i < argc; ++i) {
            if (std::string_view{argv[i]} == "--structural") structural = true;
            else if (std::string_view{argv[i]} == "--fullscreen")
                window_mode = crucible::desktop::DesktopApp::WindowMode::fullscreen;
            else if (std::string_view{argv[i]} == "--diagnostics")
                diagnostics = crucible::runtime::InspectorSession::Diagnostics::bounded;
            else if ((std::string_view{argv[i]} == "--workers" || std::string_view{argv[i]} == "--partitions") && i + 1 < argc) {
                auto& selected = std::string_view{argv[i]} == "--workers" ? workers : partitions;
                const std::string_view text{argv[++i]};
                std::size_t value{};
                const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
                if (selected || result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
                    SDL_Log("Worker/partition options require one unsigned integer each"); return SDL_APP_FAILURE;
                }
                selected = value;
            }
            else if (std::string_view{argv[i]} == "--scale" && i + 1 < argc) {
                const std::string_view value{argv[++i]};
                if (scalePopulation || (value != "100000" && value != "150000")) {
                    SDL_Log("--scale requires one of 100000 or 150000"); return SDL_APP_FAILURE;
                }
                scalePopulation = value == "100000" ? 100000 : 150000;
            }
            else { SDL_Log("Unknown option: %s", argv[i]); return SDL_APP_FAILURE; }
        }
        crucible::desktop::DesktopApp::StartupSettings settings;
        settings.rowExecution = {workers.value_or(1), partitions.value_or(0)};
        settings.diagnostics = diagnostics;
        settings.windowMode = window_mode;
        if (scalePopulation) {
            if (structural) {
                SDL_Log("--scale and --structural select different scenarios"); return SDL_APP_FAILURE;
            }
            settings.scenario.population = *scalePopulation;
            settings.scenario.grid = *scalePopulation == 100000
                ? crucible::GridConfig{400, 250, 1.0F} : crucible::GridConfig{500, 300, 1.0F};
            settings.mission.reset();
            settings.toolRadius = 16.0F;
            settings.viewPolicy = crucible::presentation::desktop::ScenePainter::ViewPolicy::densityOverview;
#if defined(_WIN32)
            settings.presentationMode = crucible::desktop::DesktopApp::PresentationMode::checkedD3D11;
#endif
        } else {
            if (structural) settings.scenario.structural = crucible::StructuralSettings{};
        }
        *state = std::make_unique<crucible::desktop::DesktopApp>(settings).release();
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
