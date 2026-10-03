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
}
int main(int argc, char** argv) {
    try {
        // Software rendering needs no window/display initialization. Host owns this scope.
        SoftwareCanvas canvas;
        PaletteAndRetained(canvas);
        RejectionBeforeDraw(canvas);
        RingsAndClipping(canvas);
        if (argc == 2) Export(canvas, argv[1]);
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s: %s\n", error.what(), SDL_GetError());
        return 1;
    }
}
