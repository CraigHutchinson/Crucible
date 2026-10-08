#include <chrono>
#include <cstdio>
#include <SDL3/SDL.h>
#include <stdexcept>
#include <string_view>

#include "crucible/presentation/ScenarioSnapshot.hpp"
#include "crucible/presentation/desktop/frame_completion_observer.hpp"
#include "desktop/DesktopApp.hpp"

namespace {
void require(bool value, const char* reason) {
    if (!value) throw std::runtime_error(reason);
}
struct Video {
    explicit Video(bool native) {
        require(SDL_SetHint(SDL_HINT_VIDEO_DRIVER, native ? "windows" : "dummy"), "video selection");
        require(SDL_SetHint(SDL_HINT_RENDER_DRIVER, native ? "direct3d11" : "software"), "renderer selection");
        require(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS), "video initialization");
    }
    ~Video() { SDL_Quit(); }
};
void key(crucible::desktop::DesktopApp& app, SDL_Keycode code) {
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = code;
    require(app.HandleEvent(event) == SDL_APP_CONTINUE, "key keeps run alive");
}
void receive(bool native, crucible::ScenarioSettings scenario) {
    using App = crucible::desktop::DesktopApp;
    using Observer = crucible::presentation::desktop::FrameCompletionObserver;
    using Presentation = crucible::presentation::desktop::CheckedPresentation;
    App::StartupSettings settings;
    settings.scenario = scenario;
    settings.mission.reset();
    settings.toolRadius = 16;
    settings.viewPolicy = crucible::presentation::desktop::ScenePainter::ViewPolicy::densityOverview;
    if (native) settings.presentationMode = App::PresentationMode::checkedD3D11;
    App app{settings};
    Observer observer{app.getRenderer(), 3};
    if (native) require(observer.getCapability() == Observer::Capability::direct3d11, "actual completion capability");
    std::uint64_t frameId{}, tick{};
    const auto draw = [&] {
        SDL_Event event{};
        while (SDL_PollEvent(&event)) require(app.HandleEvent(event) == SDL_APP_CONTINUE, "native window event");
        SDL_Delay(18);
        require(app.Iterate() == SDL_APP_CONTINUE, "production frame");
        const auto frame = app.getFrameStatistics();
        const auto counts = app.getDrawStatistics();
        require(frame.frameId > frameId && frame.runId == app.GetSession().getRunId(), "owned frame identity");
        require(counts.authoritativeMobile == scenario.population && counts.authoritativeMobile ==
            counts.individualSamples + counts.aggregatedSamples + counts.hiddenSamples, "closed representation count");
        require(frame.completedTick == app.GetSession().GetSnapshot().GetInfo()->completed_tick, "same completed state");
        if (native) {
            require(frame.nativeReceipt && frame.nativeReceipt->status == Presentation::Status::handedOff &&
                frame.presented, "explicit native handoff");
            require(observer.recordFrame({frame.runId, frame.frameId, frame.completedTick}) ==
                Observer::RecordStatus::recorded, "post-present native marker");
            require(observer.tryDrain(), "join before window replacement");
        } else require(frame.presented && !frame.nativeReceipt, "software call remains separately classified");
        frameId = frame.frameId;
        tick = frame.completedTick;
    };
    draw();
    require(tick != 0 && !app.GetSession().GetMission(), "scale run evolves without quota cutoff");
    key(app, SDLK_SPACE);
    draw();
    const auto pausedTick = tick;
    require(SDL_SetWindowSize(&app.GetWindow(), 1100, 700) && SDL_SyncWindow(&app.GetWindow()), "resize completes");
    draw();
    require(tick == pausedTick, "resize preserves paused boundary");
    if (native) {
        key(app, SDLK_F11);
        require(SDL_SyncWindow(&app.GetWindow()), "fullscreen entry");
        draw();
        require(tick == pausedTick, "fullscreen preserves pause");
        key(app, SDLK_ESCAPE);
        require(SDL_SyncWindow(&app.GetWindow()), "fullscreen exit");
        draw();
    }
    key(app, SDLK_R);
    key(app, SDLK_SPACE);
    draw();
    require(app.GetSession().getRunId() == 2 && tick == 0 && app.GetSession().GetTrace().empty(), "restart retires old identity/state");
    key(app, SDLK_SPACE);
    draw();
    require(tick != 0, "restarted scale evolves");
    SDL_Event close{};
    close.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
    require(app.HandleEvent(close) == SDL_APP_SUCCESS && observer.tryDrain(), "close joins completed work");
}
}
int main(int argc, char** argv) {
    try {
        const bool native = argc == 2 && std::string_view{argv[1]} == "--native";
        require(argc == 1 || native, "expected no arguments or --native");
        Video video{native};
        if (native) {
            receive(true, {.population = 100000, .grid = {400, 250, 1}});
            receive(true, {.population = 150000, .grid = {500, 300, 1}});
        } else receive(false, {.population = 64, .grid = {8, 8, 1}});
        std::puts("Production scale lifecycle received; no performance or scanout claim");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s: %s\n", error.what(), SDL_GetError());
        return 1;
    }
}
