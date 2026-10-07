#include "DesktopApp.hpp"
#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <crucible/runtime/HeadlessSession.hpp>
#include <crucible/simulation.hpp>
#include <algorithm>
#include <crucible/presentation/desktop/SceneUi.hpp>
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
void Release(crucible::desktop::DesktopApp& app, float x, float y, Uint8 button = SDL_BUTTON_LEFT) {
    SDL_Event e{}; e.type = SDL_EVENT_MOUSE_BUTTON_UP;
    e.button.x = x; e.button.y = y; e.button.button = button; Send(app, e);
}
void Move(crucible::desktop::DesktopApp& app, float x, float y) {
    SDL_Event e{}; e.type = SDL_EVENT_MOUSE_MOTION; e.motion.x = x; e.motion.y = y; Send(app, e);
}
void ReceiveGestureReplay(const crucible::desktop::DesktopApp& app) {
    using namespace crucible;
    const auto& actual = app.GetSession().GetSnapshot();
    Simulation simulation{2048, {{64, 32, 1}, 4, SteeringSettings{}, ResourceSettings{}}};
    runtime::HeadlessSession replay{simulation, {64, 4096}};
    Require(replay.TryReplay(app.GetSession().GetTrace(), actual.GetInfo()->completed_tick).status ==
        runtime::HeadlessSession::StepStatus::advanced, "actual SDL gesture trace replays");
    presentation::ScenarioSnapshot expected{2048, 4, 2048};
    Require(expected.TryCapture(simulation), "desktop replay snapshot capture");
    const auto a = *actual.GetInfo(), b = *expected.GetInfo();
    Require(a.completed_tick == b.completed_tick && a.grid.columns == b.grid.columns &&
        a.grid.rows == b.grid.rows && a.grid.cell_size == b.grid.cell_size && a.samples == b.samples &&
        a.fields == b.fields && a.cells == b.cells && a.biomass == b.biomass, "SDL replay metadata and ledger");
    Require(std::ranges::equal(actual.GetSamples(), expected.GetSamples(), [](const auto& p, const auto& q) {
        return p.id == q.id && p.position.x == q.position.x && p.position.y == q.position.y &&
            p.velocity.x == q.velocity.x && p.velocity.y == q.velocity.y;
    }) && std::ranges::equal(actual.GetFields(), expected.GetFields(), [](const auto& p, const auto& q) {
        return p.kind == q.kind && p.slot == q.slot && p.center.x == q.center.x && p.center.y == q.center.y &&
            p.radius == q.radius && p.strength == q.strength && p.end.x == q.end.x && p.end.y == q.end.y;
    }) && std::ranges::equal(actual.GetBlight(), expected.GetBlight()) &&
        std::ranges::equal(actual.GetStocks(), expected.GetStocks()), "SDL full-state replay includes flow endpoints");
}
void FlowEvents() {
    using crucible::FieldEditKind;
    crucible::desktop::DesktopApp app;
    Key(app, SDLK_SPACE);
    // The new button is appended; existing PAUSE/RESTART/FIT semantic indices stay fixed.
    const auto flow_button = crucible::presentation::desktop::ToolbarButton(7);
    Click(app, static_cast<float>(flow_button.x + 10), static_cast<float>(flow_button.y + 10));
    Click(app, 500, 350); Move(app, 700, 350);
    Require(app.GetSession().GetSummary().ingress_pending == 0 && app.GetPreview() &&
        app.GetPreview()->kind == FieldEditKind::set_flow && app.GetPreview()->end.x > app.GetPreview()->center.x,
        "owned drag preview does not admit or mutate fields");
    Require(app.GetSession().GetSnapshot().GetFields()[0].kind == FieldEditKind::remove, "preview is uncommitted");
    const auto expected = *app.GetPreview();
    Release(app, 700, 350); Release(app, 700, 350);
    Require(app.GetSession().GetSummary().ingress_pending == 1, "one release admits one flow");
    Require(app.Iterate() == SDL_APP_CONTINUE && app.GetSession().GetTrace().empty() &&
        app.GetPreview() && app.GetPreview()->end.x == expected.end.x, "paused flow remains queued and visible");
    Key(app, SDLK_SPACE); SDL_Delay(20);
    Require(app.Iterate() == SDL_APP_CONTINUE && app.GetSession().GetTrace().size() == 1 && !app.GetPreview(),
        "flow application confirms only the accepted preview");
    const auto applied = app.GetSession().GetSnapshot().GetFields()[0];
    Require(applied.kind == FieldEditKind::set_flow && applied.center.x == expected.center.x &&
        applied.end.x == expected.end.x && applied.end.y == expected.end.y, "completed snapshot retains endpoints");
    Key(app, SDLK_SPACE); Key(app, SDLK_1); Click(app, 640, 350);
    Key(app, SDLK_4); Click(app, 500, 350); Move(app, 700, 350);
    const auto fresh = *app.GetPreview();
    Key(app, SDLK_SPACE); SDL_Delay(20);
    Require(app.Iterate() == SDL_APP_CONTINUE && app.GetSession().GetTrace().size() == 2 &&
        app.GetPreview() && app.GetPreview()->kind == FieldEditKind::set_flow &&
        app.GetPreview()->center.x == fresh.center.x && app.GetPreview()->end.x == fresh.end.x,
        "old radial confirmation preserves unfinished flow preview");
    Release(app, 700, 350); SDL_Delay(20);
    Require(app.Iterate() == SDL_APP_CONTINUE && app.GetSession().GetTrace().size() == 3 &&
        app.GetSession().GetSnapshot().GetFields()[0].kind == FieldEditKind::set_flow,
        "flow atomically replaces radial in the selected slot");
    ReceiveGestureReplay(app);
    Key(app, SDLK_DELETE); SDL_Delay(20);
    Require(app.Iterate() == SDL_APP_CONTINUE && app.GetSession().GetTrace().size() == 4 &&
        app.GetSession().GetSnapshot().GetFields()[0].kind == FieldEditKind::remove && !app.GetPreview(),
        "delete erases applied flow through the same boundary");
    ReceiveGestureReplay(app);
    Key(app, SDLK_R); Key(app, SDLK_SPACE); Key(app, SDLK_4);
    Click(app, 500, 350); Release(app, 500, 350);
    Require(app.GetSession().GetSummary().ingress_pending == 0 && !app.GetPreview(), "zero-length flow cancels");
    Click(app, 500, 350); Move(app, 700, 350); Release(app, 700, 40);
    Require(app.GetSession().GetSummary().ingress_pending == 0 && !app.GetPreview(), "outside release cancels");
    // Each cancellation must prevent a later lost release from admitting the old gesture.
    for (const auto key : {SDLK_ESCAPE, SDLK_TAB, SDLK_1, SDLK_F, SDLK_R}) {
        Key(app, SDLK_4); Click(app, 500, 350); Move(app, 700, 350); Key(app, key); Release(app, 700, 350);
        Require(app.GetSession().GetSummary().ingress_pending == 0, "key cancellation prevents admission");
    }
    Key(app, SDLK_SPACE); // restart above leaves the run running
    for (const auto type : {SDL_EVENT_WINDOW_FOCUS_LOST, SDL_EVENT_WILL_ENTER_BACKGROUND}) {
        Key(app, SDLK_4); Click(app, 500, 350); Move(app, 700, 350);
        SDL_Event e{}; e.type = type; Send(app, e);
        if (type == SDL_EVENT_WILL_ENTER_BACKGROUND) { e.type = SDL_EVENT_DID_ENTER_FOREGROUND; Send(app, e); }
        Release(app, 700, 350);
        Require(app.GetSession().GetSummary().ingress_pending == 0 && !app.GetPreview(), "focus/suspend cancel flow");
    }
    Key(app, SDLK_4); Click(app, 500, 350); Move(app, 700, 350);
    Click(app, 640, 350, SDL_BUTTON_MIDDLE);
    Click(app, 500, 350); Move(app, 700, 350); Release(app, 700, 350);
    Require(!app.GetPreview() && app.GetSession().GetSummary().ingress_pending == 0,
        "left press cannot start a competing flow during camera pan");
    Release(app, 640, 350, SDL_BUTTON_MIDDLE); Release(app, 700, 350);
    Require(app.GetSession().GetSummary().ingress_pending == 0, "pan gesture cancels flow");
    Click(app, 500, 350); Move(app, 700, 350);
    SDL_Event wheel{}; wheel.type = SDL_EVENT_MOUSE_WHEEL; wheel.wheel.mouse_x = 640; wheel.wheel.mouse_y = 350; wheel.wheel.y = 1;
    Send(app, wheel); Release(app, 700, 350);
    Require(app.GetSession().GetSummary().ingress_pending == 0, "zoom gesture cancels flow");
    Key(app, SDLK_F);
    Require(SDL_SetWindowSize(&app.GetWindow(), 800, 800) && SDL_SyncWindow(&app.GetWindow()), "flow letterbox resize");
    SDL_PumpEvents();
    Click(app, 300, 393.75F); Move(app, 500, 393.75F); Release(app, 500, 393.75F);
    Require(app.GetSession().GetSummary().ingress_pending == 1 && app.GetPreview()->kind == FieldEditKind::set_flow &&
        app.GetPreview()->center.x < 32 && app.GetPreview()->end.x > 32, "flow endpoints use native letterbox conversion");
    Key(app, SDLK_R); Key(app, SDLK_SPACE);
    Require(SDL_SetWindowSize(&app.GetWindow(), 1280, 720) && SDL_SyncWindow(&app.GetWindow()), "restore fixture window");
    SDL_PumpEvents(); Key(app, SDLK_4);
    for (int i = 0; i < 65; ++i) { Click(app, 500, 350); Move(app, 700, 350); Release(app, 700, 350); }
    Require(app.GetSession().GetSummary().ingress_pending == 64 && app.GetPreview() &&
        app.GetPreview()->kind == FieldEditKind::set_flow && app.GetSession().GetTrace().empty(), "full queue retains refused owned flow");
    const auto refused = *app.GetPreview();
    Require(app.Iterate() == SDL_APP_CONTINUE && app.GetPreview() &&
        app.GetPreview()->center.x == refused.center.x && app.GetPreview()->end.x == refused.end.x,
        "full admission preserves owned flow while paused");
    Key(app, SDLK_SPACE); SDL_Delay(20);
    Require(app.Iterate() == SDL_APP_CONTINUE && app.GetSession().GetTrace().size() == 64 && app.GetPreview() &&
        app.GetPreview()->end.x == refused.end.x, "completed accepted edits do not erase refused preview");
    ReceiveGestureReplay(app);
    Key(app, SDLK_ESCAPE);
    Require(!app.GetPreview(), "cancelling refusal cannot reveal an already applied queued preview");
    Key(app, SDLK_R);
    Require(!app.GetPreview() && app.GetSession().GetSummary().ingress_pending == 0, "restart clears all flow feedback");
}
void Check() {
    using Status = crucible::runtime::ClockDriver::Status;
    crucible::desktop::DesktopApp app;
    Require(app.Iterate() == SDL_APP_CONTINUE, "initial draw");
    Require(app.GetSession().GetMission().has_value() &&
        app.GetSession().GetMission()->settings.target_reclaimed == 1780 &&
        app.GetSession().GetMission()->settings.deadline_ticks == 900, "production reference challenge selected");
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

void MissionEvents() {
    using Status = crucible::runtime::ClockDriver::Status;
    using Outcome = crucible::ReclamationMissionOutcome;
    for (const auto target : {5ULL, 6ULL}) {
        crucible::desktop::DesktopApp app{{target, 1}};
        SDL_Delay(20);
        Require(app.Iterate() == SDL_APP_CONTINUE, "terminal mission draws through native adapter");
        const auto mission = app.GetSession().GetMission();
        Require(mission && mission->completed_tick == 1 &&
            mission->outcome == (target == 5 ? Outcome::won : Outcome::lost) &&
            app.GetSession().GetStatus() == Status::closed, "actual terminal boundary and outcome");
        Click(app, 640, 350);
        Key(app, SDLK_4); Click(app, 500, 350); Move(app, 700, 350); Release(app, 700, 350);
        Require(app.GetSession().GetSummary().ingress_pending == 0, "terminal radial/flow edit denied");
        Key(app, SDLK_SPACE);
        Require(app.GetSession().GetStatus() == Status::closed, "terminal pause button cannot revive run");
        SDL_Event suspend{}; suspend.type = SDL_EVENT_WILL_ENTER_BACKGROUND; Send(app, suspend);
        suspend.type = SDL_EVENT_DID_ENTER_FOREGROUND; Send(app, suspend);
        Require(app.GetSession().GetStatus() == Status::closed, "foreground cannot revive outcome");
        Require(app.Iterate() == SDL_APP_CONTINUE && !app.GetPreview() &&
            app.GetSession().GetSnapshot().GetInfo()->completed_tick == 1,
            "terminal scene stays frozen and rejected preview clears");
        Key(app, SDLK_R);
        Require(app.GetSession().GetStatus() == Status::running && app.GetSession().GetTrace().empty() &&
            app.GetSession().GetMission()->outcome == Outcome::active &&
            app.GetSession().GetSnapshot().GetInfo()->completed_tick == 0 && !app.GetPreview(),
            "terminal restart reconstructs production challenge");
    }
}

void ExportMission(const char* path) {
    using namespace crucible;
    runtime::InspectorSession run{2048, {64, 4096}, ReclamationMissionSettings{}};
    for (const auto edit : {
            FieldEdit{FieldEditKind::set, 0, {24, 16}, 8, 4},
            FieldEdit{FieldEditKind::set, 1, {48, 16}, 8, -4}})
        Require(run.TryAdmitFieldEdit(edit).status == runtime::CommandIngress::AdmissionStatus::accepted,
            "mission export field admission");
    for (int i = 0; i < 20; ++i)
        Require(run.TryPump(std::chrono::milliseconds{50}).advanced_ticks == 3, "mission export completed ticks");
    std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface{
        SDL_CreateSurface(1280, 720, SDL_PIXELFORMAT_RGBA32), SDL_DestroySurface};
    Require(surface != nullptr, "mission export surface");
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer{
        SDL_CreateSoftwareRenderer(surface.get()), SDL_DestroyRenderer};
    Require(renderer != nullptr, "mission export renderer");
    presentation::desktop::ScenePainter painter{2048, 2048};
    presentation::Camera2D camera{{64, 32, 1}, {24, 96, 1232, 520}};
    presentation::desktop::SceneUi ui{};
    ui.mission = run.GetMission();
    ui.message = "Actual owned mission frame / two applied fields / conserved biomass";
    Require(painter.TryDraw(*renderer, run.GetSnapshot(), camera, ui) &&
        SDL_FlushRenderer(renderer.get()) && SDL_SaveBMP(surface.get(), path), "mission export draw/save");
}
}
int main(int argc, char** argv) {
    const bool native = argc > 1 && std::string_view{argv[1]} == "--native";
    if (!native) { SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy"); SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software"); }
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) { std::cerr << SDL_GetError(); return 1; }
    int result{};
    try {
        Check(); FlowEvents(); MissionEvents();
        if (argc == 3 && std::string_view{argv[1]} == "--export-mission") ExportMission(argv[2]);
    } catch (const std::exception& e) { std::cerr << e.what() << ": " << SDL_GetError() << '\n'; result = 1; }
    SDL_Quit();
    return result;
}
