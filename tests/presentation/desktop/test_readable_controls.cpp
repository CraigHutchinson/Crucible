#include <filesystem>
#include <iostream>
#include <memory>
#include <SDL3/SDL.h>
#include <stdexcept>

#include "crucible/presentation/desktop/SceneUi.hpp"
#include "crucible/presentation/ScenarioSnapshot.hpp"
#include "desktop/DesktopApp.hpp"

namespace
{
using namespace crucible;
using namespace crucible::presentation::desktop;

void require(bool value, const char* reason)
{
    if (!value) throw std::runtime_error(reason);
}

void key(desktop::DesktopApp& app, SDL_Keycode code)
{
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = code;
    require(app.HandleEvent(event) == SDL_APP_CONTINUE, "key keeps desktop live");
}

void clickAction(desktop::DesktopApp& app, ToolbarAction action)
{
    const auto bounds = toolbarButton(static_cast<std::size_t>(action));
    auto* renderer = SDL_GetRenderer(&app.GetWindow());
    SDL_Event event{};
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.button = SDL_BUTTON_LEFT;
    require(SDL_RenderCoordinatesToWindow(renderer, static_cast<float>(bounds.x + 10),
        static_cast<float>(bounds.y + 10), &event.button.x, &event.button.y), "shared target maps to window");
    require(app.HandleEvent(event) == SDL_APP_CONTINUE, "toolbar event keeps desktop live");
}

void receiveControls()
{
    for (std::size_t i = 0; i < ToolbarButtonCount; ++i)
    {
        const auto bounds = toolbarButton(i);
        require(bounds.x >= 0 && bounds.y >= 0 && bounds.x + bounds.width <= CanvasWidth &&
            bounds.y + bounds.height <= CanvasHeight, "every drawn target fits the canvas");
        require(tryToolbarAction({bounds.x + bounds.width / 2, bounds.y + bounds.height / 2}, true) ==
            static_cast<ToolbarAction>(i), "drawn center receives matching action");
        require(!tryToolbarAction({bounds.x + bounds.width, bounds.y + 10}, true), "target right edge is a gap");
    }
    require(!tryToolbarAction({24, 640}, true), "relay status never admits a toolbar action");
    require(!tryToolbarAction({toolbarButton(8).x + 10, toolbarButton(8).y + 10}, false),
        "legacy fuse target is disabled");
    desktop::DesktopApp app{{}, true};
    key(app, SDLK_SPACE);
    clickAction(app, ToolbarAction::flow);
    clickAction(app, ToolbarAction::slot);
    clickAction(app, ToolbarAction::fuse);
    require(app.GetSession().GetSummary().ingress_pending == 1 && app.GetSession().GetTrace().empty(),
        "visible fuse queues while paused");
    const auto initial = *app.GetSession().GetSnapshot().GetInfo();
    const auto scale = app.GetCamera().GetScale();
    int width{}, height{};
    require(SDL_GetWindowSize(&app.GetWindow(), &width, &height), "windowed size");
    clickAction(app, ToolbarAction::fullscreen);
    require(SDL_SyncWindow(&app.GetWindow()), "fullscreen request completes");
    require((SDL_GetWindowFlags(&app.GetWindow()) & SDL_WINDOW_FULLSCREEN) != 0, "dummy receives borderless fullscreen");
    require(app.Iterate() == SDL_APP_CONTINUE && app.GetSession().GetTrace().empty() &&
        app.GetSession().GetSummary().ingress_pending == 1 && app.GetCamera().GetScale() == scale &&
        app.GetSession().GetSnapshot().GetInfo()->biomass == initial.biomass, "fullscreen preserves pause/receipt/camera/ledger");
    key(app, SDLK_ESCAPE);
    require(SDL_SyncWindow(&app.GetWindow()), "escape window request completes");
    int restoredWidth{}, restoredHeight{};
    require(SDL_GetWindowSize(&app.GetWindow(), &restoredWidth, &restoredHeight) &&
        restoredWidth == width && restoredHeight == height &&
        (SDL_GetWindowFlags(&app.GetWindow()) & SDL_WINDOW_FULLSCREEN) == 0, "escape restores windowed geometry");
    for (int i = 0; i < 3; ++i)
    {
        key(app, SDLK_F11); require(SDL_SyncWindow(&app.GetWindow()), "repeated fullscreen entry");
        key(app, SDLK_F11); require(SDL_SyncWindow(&app.GetWindow()), "repeated fullscreen exit");
    }
    // Resizing or changing DPI cannot complete a drag that began in old coordinates.
    SDL_Event press{};
    press.type = SDL_EVENT_MOUSE_BUTTON_DOWN; press.button.button = SDL_BUTTON_LEFT;
    press.button.x = 500; press.button.y = 350;
    require(app.HandleEvent(press) == SDL_APP_CONTINUE, "begin flow after toggles");
    for (const auto type : {SDL_EVENT_WINDOW_RESIZED, SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED,
        SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED})
    {
        require(app.HandleEvent(press) == SDL_APP_CONTINUE, "begin resize cancellation gesture");
        SDL_Event resize{}; resize.type = type;
        require(app.HandleEvent(resize) == SDL_APP_CONTINUE, "receive coordinate change");
        SDL_Event release = press; release.type = SDL_EVENT_MOUSE_BUTTON_UP; release.button.x = 700;
        require(app.HandleEvent(release) == SDL_APP_CONTINUE && app.GetSession().GetSummary().ingress_pending == 1,
            "changed coordinates cancel only unfinished input; accepted receipt survives");
    }
    clickAction(app, ToolbarAction::restart);
    clickAction(app, ToolbarAction::pause);
    clickAction(app, ToolbarAction::shatter);
    require(app.GetSession().GetSummary().ingress_pending == 1 && app.GetSession().GetTrace().empty(),
        "restart cancels old receipt; visible shatter queues");
    desktop::DesktopApp startup{{}, true, runtime::InspectorSession::Diagnostics::disabled,
        desktop::DesktopApp::WindowMode::fullscreen};
    require(SDL_SyncWindow(&startup.GetWindow()) &&
        (SDL_GetWindowFlags(&startup.GetWindow()) & SDL_WINDOW_FULLSCREEN) != 0, "fullscreen startup is consumed");
    key(startup, SDLK_ESCAPE);
    desktop::DesktopApp legacy;
    key(legacy, SDLK_SPACE);
    clickAction(legacy, ToolbarAction::fuse);
    clickAction(legacy, ToolbarAction::shatter);
    require(legacy.GetSession().GetSummary().ingress_pending == 0, "disabled structural controls submit nothing");
}

void capture(const std::filesystem::path& directory, int width, int height)
{
    runtime::InspectorSession session{2048, {64, 4096}, ReclamationMissionSettings{}, true};
    session.Pause();
    require(session.TryAdmitFieldEdit({FieldEditKind::set_flow, 0, {12, 12}, 8, 4, {44, 12}}).status ==
        runtime::CommandIngress::AdmissionStatus::accepted, "capture admits queued flow");
    std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface{
        SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA32), SDL_DestroySurface};
    require(surface != nullptr, "sized software output");
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer{
        SDL_CreateSoftwareRenderer(surface.get()), SDL_DestroyRenderer};
    require(renderer != nullptr && SDL_SetRenderLogicalPresentation(renderer.get(), CanvasWidth,
        CanvasHeight, SDL_LOGICAL_PRESENTATION_LETTERBOX), "sized logical presentation");
    presentation::Camera2D camera{{64, 32, 1}, {24, 96, 1232, 520}};
    ScenePainter painter{2048, 2048};
    SceneUi ui{};
    ui.paused = true; ui.tool = presentation::FieldTool::flow; ui.mission = session.GetMission();
    ui.fullscreen_ = width == 2048; // Presentation fixture, not a physical fullscreen claim.
    ui.preview = FieldEdit{FieldEditKind::set_flow, 0, {12, 12}, 8, 4, {44, 12}};
    ui.message = "Queued - waiting for completed boundary. Resume to apply; dashed preview is not committed state.";
    require(painter.TryDraw(*renderer, session.GetSnapshot(), camera, ui) && SDL_FlushRenderer(renderer.get()),
        "actual production HUD at receiving output size");
    // Independent pixel coverage: the scaled body row must span at least twelve physical rows.
    SDL_FRect logical{};
    require(SDL_GetRenderLogicalPresentationRect(renderer.get(), &logical), "physical letterbox bounds");
    const auto scale = logical.h / CanvasHeight;
    int occupiedRows = 0;
    for (int y = static_cast<int>(logical.y + 34 * scale); y < logical.y + 54 * scale; ++y)
    {
        bool occupied = false;
        for (int x = static_cast<int>(logical.x + 24 * scale); x < logical.x + 1000 * scale; ++x)
        {
            Uint8 r{}, g{}, b{}, a{};
            require(SDL_ReadSurfacePixel(surface.get(), x, y, &r, &g, &b, &a), "body text pixel read");
            occupied = occupied || (r > 180 && g > 180 && b > 180);
        }
        if (occupied) ++occupiedRows;
    }
    require(occupiedRows >= 12, "body text has twelve visible physical rows at supported sizes");
    if (!directory.empty())
    {
        std::filesystem::create_directories(directory);
        require(SDL_SaveBMP(surface.get(), (directory / (std::to_string(width) + "x" +
            std::to_string(height) + ".bmp")).string().c_str()), "sized evidence export");
    }
    std::cout << "software output=" << width << 'x' << height << " logical=1280x864 bodyRows=" << occupiedRows
        << " tick=0 paused=1 queued=1 structural=1\n";
}
}

int main(int argc, char** argv)
{
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) return 1;
    int result = 0;
    try
    {
        receiveControls();
        const auto directory = argc == 2 ? std::filesystem::path{argv[1]} : std::filesystem::path{};
        capture(directory, 1026, 607);
        capture(directory, 2048, 1280);
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << ": " << SDL_GetError() << '\n'; result = 1;
    }
    SDL_Quit();
    return result;
}
