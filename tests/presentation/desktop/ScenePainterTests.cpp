#include <crucible/presentation/desktop/ScenePainter.hpp>
#include <crucible/presentation/desktop/SceneUi.hpp>
#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <crucible/presentation/Camera2D.hpp>
#include <crucible/simulation.hpp>
#include <SDL3/SDL.h>

#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <cstdio>
#include <string_view>

namespace {
using namespace crucible;
using namespace crucible::presentation;
using namespace crucible::presentation::desktop;
void Require(bool value, const char* reason) { if (!value) throw std::runtime_error(reason); }
struct RendererDelete { void operator()(SDL_Renderer* p) const noexcept { SDL_DestroyRenderer(p); } };
struct SurfaceDelete { void operator()(SDL_Surface* p) const noexcept { SDL_DestroySurface(p); } };
struct SoftwareCanvas {
    std::unique_ptr<SDL_Surface, SurfaceDelete> surface{SDL_CreateSurface(1280, 720, SDL_PIXELFORMAT_RGBA32)};
    std::unique_ptr<SDL_Renderer, RendererDelete> renderer;
    SoftwareCanvas() {
        Require(surface != nullptr, "software surface");
        renderer.reset(SDL_CreateSoftwareRenderer(surface.get()));
        Require(renderer != nullptr, "software renderer");
        Require(SDL_SetRenderLogicalPresentation(renderer.get(), 1280, 720, SDL_LOGICAL_PRESENTATION_LETTERBOX), "logical canvas");
    }
    std::array<std::uint8_t, 4> Pixel(int x, int y) {
        Require(SDL_FlushRenderer(renderer.get()), "flush for independent pixel read");
        std::array<std::uint8_t, 4> pixel{};
        Require(SDL_ReadSurfacePixel(surface.get(), x, y, &pixel[0], &pixel[1], &pixel[2], &pixel[3]), "pixel read");
        return pixel;
    }
};
constexpr ScreenRect View{24, 96, 1232, 520};
constexpr std::array<std::uint8_t, 4> Red{159, 69, 77, 255}, Ochre{108, 86, 47, 255};
constexpr std::array<std::uint8_t, 4> Green{33, 59, 55, 255}, Cyan{119, 221, 255, 255};
constexpr std::array<std::uint8_t, 4> Background{11, 19, 32, 255};

void PaletteAndRetained(SoftwareCanvas& canvas) {
    const GridConfig grid{1, 1, 1};
    Simulation stocked{0, {grid, 4, {}, ResourceSettings{}}};
    ScenarioSnapshot frame{1, 4, 1};
    Camera2D camera{grid, View};
    ScenePainter painter{1, 1};
    Require(frame.TryCapture(stocked), "stocked capture");
    Require(painter.TryDraw(*canvas.renderer, frame, camera, {}), "stocked draw");
    // Independent fit: square world occupies x380..900, y96..616, center640,356.
    Require(canvas.Pixel(640, 356) == Red, "infected stock is red");
    Require(canvas.Pixel(300, 356) == Background, "fit letterbox stays background");
    Require(canvas.Pixel(640, 617) == Background, "world viewport clips bottom");
    Simulation empty{0, {grid, 4, {}, ResourceSettings{0, 1, 0, 0}}};
    Require(frame.TryCapture(empty), "empty infected capture");
    Require(painter.TryDraw(*canvas.renderer, frame, camera, {}), "empty draw");
    Require(canvas.Pixel(640, 356) == Ochre, "infected depleted state distinct");
    Simulation reclaim{1, {grid, 4, {}, ResourceSettings{1, 1, 0, 1}}};
    Require(frame.TryCapture(reclaim), "nanite capture");
    Require(painter.TryDraw(*canvas.renderer, frame, camera, {}), "nanite draw");
    Require(canvas.Pixel(640, 356) == Cyan, "nanite quad drawn over cell");
    reclaim.tick();
    Require(painter.TryDraw(*canvas.renderer, frame, camera, {}), "retained draw");
    Require(canvas.Pixel(640, 356) == Cyan && canvas.Pixel(400, 116) == Red, "live mutation cannot change retained image");
    Require(frame.TryCapture(reclaim), "reclaimed capture");
    Require(painter.TryDraw(*canvas.renderer, frame, camera, {}), "reclaimed draw");
    Require(canvas.Pixel(400, 116) == Green, "cleared exhausted cell green");
}

void StructuralPixels(SoftwareCanvas& canvas) {
    const GridConfig grid{16, 8, 1};
    StructuralSettings settings{}; settings.relay_center = {8.5F, 4.5F}; settings.eligibility_radius = 32;
    Simulation simulation{128, {grid, 4, {}, ResourceSettings{0, 1, 0, 0}, settings}};
    ScenarioSnapshot frame{128, 4, 128};
    Camera2D camera{grid, View}; ScenePainter painter{128, 128};
    Require(frame.TryCapture(simulation) && painter.TryDraw(*canvas.renderer, frame, camera, {}), "structural startup pixels");
    // Independent fit for16x8: scale65, origin120,96. ID1 center152.5,128.5.
    Require(canvas.Pixel(152, 128) == Cyan, "mobile startup identity drawn");
    Require(simulation.TryFuseRelay() == StructuralCommandResult::applied && frame.TryCapture(simulation) &&
        painter.TryDraw(*canvas.renderer, frame, camera, {}), "primitive fuse frame");
    Require(canvas.Pixel(152, 128) != Cyan, "anchored ID1 is removed from mobile drawing");
    // Relay center672.5,388.5 is crossed by occupied amber lattice.
    const auto amber = canvas.Pixel(672, 388);
    Require(amber[0] > 200 && amber[1] > 140 && amber[2] < 120, "occupied lattice amber crossbars");
    const auto generation = frame.GetInfo()->structural->generation;
    Require(simulation.TryShatterRelay(generation) == StructuralCommandResult::applied, "pixel shatter transition");
    Require(painter.TryDraw(*canvas.renderer, frame, camera, {}) && canvas.Pixel(672, 388) == amber,
        "retained occupied image survives later live shatter");
    Require(frame.TryCapture(simulation) && painter.TryDraw(*canvas.renderer, frame, camera, {}), "new shatter frame");
    Require(canvas.Pixel(672, 388) == Cyan && !frame.GetInfo()->structural->occupied &&
        frame.GetInfo()->biomass->lost_mass == 16, "returned particles replace occupied crossbars; lost remain hidden");
}

void RejectionBeforeDraw(SoftwareCanvas& canvas) {
    const GridConfig grid{2, 1, 1};
    Simulation simulation{1, {grid, 4, {}, ResourceSettings{}}};
    ScenarioSnapshot frame{1, 4, 2};
    Camera2D camera{grid, View};
    ScenePainter too_small{0, 2}, too_few_cells{1, 1}, painter{1, 2};
    Require(SDL_SetRenderDrawColor(canvas.renderer.get(), 200, 1, 2, 255) && SDL_RenderClear(canvas.renderer.get()), "sentinel clear");
    const auto sentinel = canvas.Pixel(640, 356);
    Require(!painter.TryDraw(*canvas.renderer, frame, camera, {}), "uncaptured rejected");
    Require(frame.TryCapture(simulation), "capacity fixture capture");
    Require(!too_small.TryDraw(*canvas.renderer, frame, camera, {}), "sample cap rejected");
    Require(!too_few_cells.TryDraw(*canvas.renderer, frame, camera, {}), "cell cap rejected");
    Camera2D wrong_geometry{{1, 1, 1}, View};
    Camera2D wrong_view{grid, {0, 0, 1280, 720}};
    Require(!painter.TryDraw(*canvas.renderer, frame, wrong_geometry, {}), "wrong geometry rejected");
    Require(!painter.TryDraw(*canvas.renderer, frame, wrong_view, {}), "wrong viewport rejected");
    SceneUi invalid{};
    invalid.preview = FieldEdit{FieldEditKind::set, 4, {.5F, .5F}, 1, 1};
    Require(!painter.TryDraw(*canvas.renderer, frame, camera, invalid), "invalid preview rejected");
    Require(canvas.Pixel(640, 356) == sentinel, "all preflight rejections preserve canvas");
    bool threw = false;
    try { ScenePainter impossible{std::numeric_limits<std::size_t>::max(), 1}; }
    catch (const std::length_error&) { threw = true; }
    Require(threw, "unrepresentable startup counts rejected");
}

void RingsAndClipping(SoftwareCanvas& canvas) {
    const GridConfig grid{1, 1, 1};
    Simulation simulation{0, {grid, 4, {}, ResourceSettings{}}};
    ScenarioSnapshot frame{0, 4, 1};
    Camera2D camera{grid, View};
    ScenePainter painter{0, 1};
    Require(simulation.TryApplyFieldEdit({FieldEditKind::set, 0, {.5F, .5F}, .25F, 4}), "committed ring");
    Require(frame.TryCapture(simulation), "ring capture");
    Require(painter.TryDraw(*canvas.renderer, frame, camera, {}), "ring draw");
    const auto ring = canvas.Pixel(770, 356);
    Require(ring[1] > 180 && ring[0] < 140, "committed attract ring green");
    SceneUi ui{};
    ui.preview = FieldEdit{FieldEditKind::set, 1, {.5F, .5F}, .4F, -4};
    Require(painter.TryDraw(*canvas.renderer, frame, camera, ui), "preview draw");
    const auto preview = canvas.Pixel(848, 356);
    Require(preview[0] > 180 && preview[1] > 180 && preview[2] > 180, "preview white rather than committed color");
    // Preview never changes the captured field slots.
    Require(frame.GetFields()[1].kind == FieldEditKind::remove, "preview not committed");
    Require(camera.TryZoom({640, 356}, 16), "zoom for clipping");
    Require(painter.TryDraw(*canvas.renderer, frame, camera, ui), "zoomed field clipping");
    Require(canvas.Pixel(640, 617) == Background && canvas.Pixel(23, 356) == Background, "zoomed geometry stays inside viewport");
    Require(simulation.TryApplyFieldEdit({FieldEditKind::set, 2,
        {std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()}, std::numeric_limits<float>::max(), 1}), "finite external field accepted");
    Require(simulation.TryApplyFieldEdit({FieldEditKind::set, 3, {.5F, .5F}, 0, 0}), "legacy zero radius accepted");
    Require(frame.TryCapture(simulation) && painter.TryDraw(*canvas.renderer, frame, camera, {}), "finite extreme/zero fields draw safely");
    Camera2D tiny{{1, 1, std::numeric_limits<float>::denorm_min()}, View};
    Simulation tiny_world{0, {{1, 1, std::numeric_limits<float>::denorm_min()}, 4, {}, ResourceSettings{}}};
    Require(tiny_world.TryApplyFieldEdit({FieldEditKind::set, 0, {std::numeric_limits<float>::max(), 0}, std::numeric_limits<float>::max(), 4}), "huge relative field accepted");
    Require(frame.TryCapture(tiny_world) && painter.TryDraw(*canvas.renderer, frame, tiny, {}), "tiny physical world safe double clipping");
}

void DirectionCuesAndFlow(SoftwareCanvas& canvas) {
    const GridConfig grid{1, 1, 1};
    Simulation simulation{0, {grid, 4, {}, ResourceSettings{}}};
    ScenarioSnapshot frame{0, 4, 1};
    Camera2D camera{grid, View};
    ScenePainter painter{0, 1};
    // Fit is independently known: origin380,96; scale520; center640,356.
    // An east radial cue at60% of radius130 is centered718,356.
    Require(simulation.TryApplyFieldEdit({FieldEditKind::set, 0, {.5F, .5F}, .25F, 4}) &&
        frame.TryCapture(simulation) && painter.TryDraw(*canvas.renderer, frame, camera, {}), "inward cue fixture");
    Require(canvas.Pixel(711, 356) != Red && canvas.Pixel(725, 356) == Red, "attract chevron points inward by shape");
    Require(simulation.TryApplyFieldEdit({FieldEditKind::set, 0, {.5F, .5F}, .25F, -4}) &&
        frame.TryCapture(simulation) && painter.TryDraw(*canvas.renderer, frame, camera, {}), "outward cue fixture");
    Require(canvas.Pixel(711, 356) == Red && canvas.Pixel(725, 356) != Red, "repel chevron points outward by shape");
    Require(simulation.TryApplyFieldEdit({FieldEditKind::set, 0, {.5F, .5F}, .25F, 0}) &&
        frame.TryCapture(simulation) && painter.TryDraw(*canvas.renderer, frame, camera, {}), "zero-strength cue fixture");
    Require(canvas.Pixel(711, 356) == Red && canvas.Pixel(725, 356) == Red, "zero strength draws no direction");
    const FieldEdit forward{FieldEditKind::set_flow, 0, {.25F, .5F}, .1F, 4, {.75F, .5F}};
    Require(simulation.TryApplyFieldEdit(forward) && frame.TryCapture(simulation) &&
        painter.TryDraw(*canvas.renderer, frame, camera, {}), "forward flow fixture");
    // Centerline510..770 at356, influence edges304/408, cap extrema458/822.
    Require(canvas.Pixel(647, 356) != Red && canvas.Pixel(631, 356) == Red, "forward chevron points toward endpoint");
    Require(canvas.Pixel(534, 304) != Red && canvas.Pixel(458, 356) != Red, "corridor shows width and end cap");
    SceneUi ui{}; ui.selected_slot = 1;
    SDL_ClearError();
    Require(painter.TryDraw(*canvas.renderer, frame, camera, ui), "unselected flow fixture");
    // Count coverage away from chevrons/caps instead of assuming fractional edge tie-breaking.
    const auto coverage = [&canvas] {
        int covered = 0;
        for (int y = 300; y <= 308; ++y) if (canvas.Pixel(560, y) != Red) ++covered;
        return covered;
    };
    const auto thin = coverage();
    ui.selected_slot = 0;
    Require(painter.TryDraw(*canvas.renderer, frame, camera, ui), "selected flow fixture");
    Require(thin > 0 && coverage() > thin, "selected corridor covers more rows than unselected corridor");
    Require(simulation.TryApplyFieldEdit({FieldEditKind::remove, 0}) && frame.TryCapture(simulation), "empty preview fixture");
    ui.preview = forward;
    Require(painter.TryDraw(*canvas.renderer, frame, camera, ui), "flow preview fixture");
    const auto dashed = canvas.Pixel(514, 304);
    Require(dashed[0] > 180 && dashed[1] > 180 && canvas.Pixel(534, 304) == Red,
        "preview corridor is white dashed with independent gap");
    Require(frame.GetFields()[0].kind == FieldEditKind::remove, "flow preview never mutates owned state");
    ui.preview.reset();
    const FieldEdit reverse{FieldEditKind::set_flow, 0, {.75F, .5F}, .1F, 4, {.25F, .5F}};
    Require(simulation.TryApplyFieldEdit(reverse) && frame.TryCapture(simulation) &&
        painter.TryDraw(*canvas.renderer, frame, camera, ui), "reverse flow fixture");
    Require(canvas.Pixel(631, 356) != Red && canvas.Pixel(647, 356) == Red, "reversing endpoints reverses shape");
    Require(camera.TryZoom({640, 356}, 2) && painter.TryDraw(*canvas.renderer, frame, camera, ui), "zoom flow fixture");
    // Zoom2 keeps midpoint640,356; centerline380..900; radius104.
    Require(canvas.Pixel(647, 356) == Red && canvas.Pixel(631, 356) != Red && canvas.Pixel(560, 252) != Red,
        "zoom preserves direction and world influence width");
    Require(camera.TryZoom({640, 356}, 8) && painter.TryDraw(*canvas.renderer, frame, camera, ui), "clipped flow fixture");
    Require(canvas.Pixel(23, 356) == Background && canvas.Pixel(640, 617) == Background, "flow clipping stays inside viewport");
    const auto maximum = std::numeric_limits<float>::max();
    Require(simulation.TryApplyFieldEdit({FieldEditKind::set_flow, 1, {-maximum, maximum}, maximum, 4, {maximum, -maximum}}) &&
        frame.TryCapture(simulation) && painter.TryDraw(*canvas.renderer, frame, camera, ui), "extreme flow stays rasterizer safe");
}

std::uint64_t RegionDigest(SoftwareCanvas& canvas, SDL_Rect region) {
    Require(SDL_FlushRenderer(canvas.renderer.get()), "region flush");
    const auto* pixels = static_cast<const std::uint8_t*>(canvas.surface->pixels);
    std::uint64_t digest = 14695981039346656037ULL;
    for (int y = region.y; y < region.y + region.h; ++y)
        for (int x = region.x * 4; x < (region.x + region.w) * 4; ++x) {
            digest ^= pixels[y * canvas.surface->pitch + x];
            digest *= 1099511628211ULL;
        }
    return digest;
}
void TextMatches(SoftwareCanvas& canvas, SoftwareCanvas& oracle, const char* text,
                 SDL_Rect region, std::array<std::uint8_t, 4> color, float scale) {
    Require(SDL_SetRenderDrawColor(oracle.renderer.get(), 11, 19, 32, 255) &&
        SDL_RenderClear(oracle.renderer.get()) &&
        SDL_SetRenderDrawColor(oracle.renderer.get(), color[0], color[1], color[2], color[3]) &&
        SDL_SetRenderScale(oracle.renderer.get(), scale, scale) &&
        SDL_RenderDebugText(oracle.renderer.get(), region.x / scale, region.y / scale, text) &&
        SDL_SetRenderScale(oracle.renderer.get(), 1, 1), "independent expected text");
    Require(RegionDigest(canvas, region) == RegionDigest(oracle, region), "visible mission text matches expected label");
}
void MissionHud(SoftwareCanvas& canvas) {
    const GridConfig grid{4, 1, 1};
    Simulation simulation{4, {grid, 4, {}, ResourceSettings{}}};
    simulation.tick();
    ScenarioSnapshot frame{4, 4, 4};
    Camera2D camera{grid, View};
    ScenePainter painter{4, 4};
    Require(frame.TryCapture(simulation), "mission frame capture");
    Require(frame.GetInfo()->biomass->reserve == 3, "independent three-contact fixture");
    Require(painter.TryDraw(*canvas.renderer, frame, camera, {}), "legacy no mission draw");
    const auto world = RegionDigest(canvas, {24, 96, 1232, 520});
    const auto ledger = RegionDigest(canvas, {24, 70, 1232, 8});
    Require(canvas.Pixel(900, 20) == Background, "no mission has no progress overlay");
    SoftwareCanvas oracle;
    SceneUi ui{};
    ui.mission = ReclamationMissionProgress{{6, 2}, 3, 1, ReclamationMissionOutcome::active};
    Require(painter.TryDraw(*canvas.renderer, frame, camera, ui), "active mission draw");
    Require(canvas.Pixel(900, 20) == Cyan && canvas.Pixel(1200, 20) ==
        std::array<std::uint8_t, 4>{22, 38, 55, 255}, "active progress half-filled");
    TextMatches(canvas, oracle, "ACTIVE", {536, 16, 128, 16}, Cyan, 2);
    constexpr std::array<std::uint8_t, 4> White{219, 232, 249, 255};
    TextMatches(canvas, oracle, "Recovered 3 / 6 | Ticks left 1 (simulation ticks)", {24, 44, 1200, 8}, White, 1);
    TextMatches(canvas, oracle,
        "ACTIVE: Recover the quota before ticks run out. Drag FLOW or place ATTRACT/REPEL. R or RESTART: fresh challenge.",
        {24, 84, 1232, 8}, White, 1);
    Require(RegionDigest(canvas, {24, 96, 1232, 520}) == world &&
        RegionDigest(canvas, {24, 70, 1232, 8}) == ledger, "mission preserves world pass and conservation ledger");
    const auto active_top = RegionDigest(canvas, {24, 0, 1232, 96});
    // Quota overshoot must fill, not extend, the bar.
    ui.mission = ReclamationMissionProgress{{2, 2}, 3, 1, ReclamationMissionOutcome::won};
    Require(painter.TryDraw(*canvas.renderer, frame, camera, ui), "won mission draw");
    constexpr std::array<std::uint8_t, 4> Won{115, 227, 173, 255};
    Require(canvas.Pixel(1254, 20) == Won && canvas.Pixel(1257, 20) == Background,
        "won overshoot bounded to progress track");
    TextMatches(canvas, oracle, "WON", {536, 16, 128, 16}, Won, 2);
    TextMatches(canvas, oracle, "WON: Biomass quota recovered. Run stopped; inspect with pan/zoom. R or RESTART: play again.",
        {24, 84, 1232, 8}, White, 1);
    Require(RegionDigest(canvas, {24, 96, 1232, 520}) == world, "win does not alter world geometry");
    const auto won_top = RegionDigest(canvas, {24, 0, 1232, 96});
    ui.mission = ReclamationMissionProgress{{4, 1}, 3, 1, ReclamationMissionOutcome::lost};
    Require(painter.TryDraw(*canvas.renderer, frame, camera, ui), "lost mission draw");
    constexpr std::array<std::uint8_t, 4> Lost{255, 199, 107, 255};
    Require(canvas.Pixel(900, 20) == Lost, "loss has distinct graphical progress");
    TextMatches(canvas, oracle, "LOST", {536, 16, 128, 16}, Lost, 2);
    TextMatches(canvas, oracle, "Recovered 3 / 4 | Ticks left 0 (simulation ticks)", {24, 44, 1200, 8}, White, 1);
    TextMatches(canvas, oracle, "LOST: Deadline reached before quota. Run stopped; inspect with pan/zoom. R or RESTART: try again.",
        {24, 84, 1232, 8}, White, 1);
    Require(RegionDigest(canvas, {24, 96, 1232, 520}) == world && active_top != won_top &&
        won_top != RegionDigest(canvas, {24, 0, 1232, 96}), "all outcomes visible without world changes");
    const auto valid = RegionDigest(canvas, {0, 0, 1280, 720});
    for (const auto bad : std::array{
        ReclamationMissionProgress{{4, 1}, 3, 2, ReclamationMissionOutcome::lost},
        ReclamationMissionProgress{{0, 2}, 3, 1, ReclamationMissionOutcome::won},
        ReclamationMissionProgress{{6, 0}, 3, 1, ReclamationMissionOutcome::active},
        ReclamationMissionProgress{{4, 1}, 4, 1, ReclamationMissionOutcome::won},
        ReclamationMissionProgress{{2, 2}, 3, 1, ReclamationMissionOutcome::active},
        ReclamationMissionProgress{{4, 2}, 3, 1, ReclamationMissionOutcome::lost},
        ReclamationMissionProgress{{4, 2}, 3, 1, static_cast<ReclamationMissionOutcome>(99)}}) {
        ui.mission = bad;
        Require(!painter.TryDraw(*canvas.renderer, frame, camera, ui) &&
            RegionDigest(canvas, {0, 0, 1280, 720}) == valid, "inconsistent mission rejected before canvas mutation");
    }
    // A zero-progress initial boundary is still drawable, with an empty bounded bar.
    Simulation initial{0, {grid, 4, {}, ResourceSettings{}}};
    Require(frame.TryCapture(initial), "initial mission capture");
    ui.mission = ReclamationMissionProgress{{1, 2}, 0, 0, ReclamationMissionOutcome::active};
    Require(painter.TryDraw(*canvas.renderer, frame, camera, ui) && canvas.Pixel(900, 20) ==
        std::array<std::uint8_t, 4>{22, 38, 55, 255}, "zero progress bar empty");
}

void Export(SoftwareCanvas& canvas, const char* path) {
    const GridConfig grid{64, 32, 1};
    Simulation simulation{2048, {grid, 4, SteeringSettings{}, ResourceSettings{}}};
    Require(simulation.TryApplyFieldEdit({FieldEditKind::set, 0, {24, 16}, 8, 4}), "export attract");
    Require(simulation.TryApplyFieldEdit({FieldEditKind::set, 1, {48, 16}, 8, -4}), "export repel");
    for (int i = 0; i < 60; ++i) simulation.tick();
    ScenarioSnapshot frame{2048, 4, 2048};
    Camera2D camera{grid, View};
    ScenePainter painter{2048, 2048};
    SceneUi ui{};
    ui.message = "Owned frame / applied fields / conservation ledger";
    ui.preview = FieldEdit{FieldEditKind::set, 2, {32, 8}, 8, 4};
    Require(frame.TryCapture(simulation) && painter.TryDraw(*canvas.renderer, frame, camera, ui), "export frame");
    Require(SDL_FlushRenderer(canvas.renderer.get()) && SDL_SaveBMP(canvas.surface.get(), path), "optional export outside verified loop");
}
void ExportFlow(SoftwareCanvas& canvas, const char* path, bool zoom) {
    const GridConfig grid{64, 32, 1};
    Simulation simulation{2048, {grid, 4, SteeringSettings{}, ResourceSettings{}}};
    Require(simulation.TryApplyFieldEdit({FieldEditKind::set_flow, 0, {12, 12}, 8, 4, {44, 12}}), "export applied flow");
    Require(simulation.TryApplyFieldEdit({FieldEditKind::set, 1, {24, 24}, 8, 4}), "export inward field");
    Require(simulation.TryApplyFieldEdit({FieldEditKind::set, 2, {52, 24}, 8, -4}), "export outward field");
    for (int i = 0; i < 60; ++i) simulation.tick();
    ScenarioSnapshot frame{2048, 4, 2048};
    Camera2D camera{grid, View};
    ScenePainter painter{2048, 2048};
    if (zoom) Require(camera.TryZoom({640, 356}, 2), "export zoom");
    SceneUi ui{}; ui.tool = FieldTool::flow; ui.selected_slot = 0;
    ui.preview = FieldEdit{FieldEditKind::set_flow, 3, {18, 20}, 8, 4, {46, 28}};
    ui.message = "Actual owned tick60 / solid applied FLOW / white dashed uncommitted preview";
    Require(frame.TryCapture(simulation) && painter.TryDraw(*canvas.renderer, frame, camera, ui), "flow export frame");
    Require(SDL_FlushRenderer(canvas.renderer.get()) && SDL_SaveBMP(canvas.surface.get(), path), "flow export save");
}
}
int main(int argc, char** argv) {
    try {
        // Software rendering needs no window/display initialization. Host owns this scope.
        SoftwareCanvas canvas;
        PaletteAndRetained(canvas);
        StructuralPixels(canvas);
        RejectionBeforeDraw(canvas);
        RingsAndClipping(canvas);
        DirectionCuesAndFlow(canvas);
        MissionHud(canvas);
        if (argc == 2) Export(canvas, argv[1]);
        else if (argc == 3 && std::string_view{argv[1]} == "--export-flow-fit") ExportFlow(canvas, argv[2], false);
        else if (argc == 3 && std::string_view{argv[1]} == "--export-flow-zoom") ExportFlow(canvas, argv[2], true);
        else if (argc != 1) throw std::invalid_argument("Expected output.bmp or --export-flow-fit/--export-flow-zoom output.bmp");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s: %s\n", error.what(), SDL_GetError());
        return 1;
    }
}
