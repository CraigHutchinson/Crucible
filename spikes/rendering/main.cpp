#include "WorldPacket.hpp"
#include "PacketPainter.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <crucible/presentation/Camera2D.hpp>
#include <crucible/presentation/desktop/ScenePainter.hpp>
#include <crucible/presentation/desktop/SceneUi.hpp>
#include <crucible/simulation.hpp>
#include <SDL3/SDL.h>
namespace {
using namespace crucible;
using namespace crucible::presentation;
using namespace crucible::spike;
using Clock = std::chrono::steady_clock;
constexpr ScreenRect View{24, 96, 1232, 520};
void Require(bool ok, const char* reason) { if (!ok) throw std::runtime_error(reason); }
struct SurfaceDelete { void operator()(SDL_Surface* p) const noexcept { SDL_DestroySurface(p); } };
struct RendererDelete { void operator()(SDL_Renderer* p) const noexcept { SDL_DestroyRenderer(p); } };
struct Canvas {
    std::unique_ptr<SDL_Surface, SurfaceDelete> surface{SDL_CreateSurface(1280, 720, SDL_PIXELFORMAT_RGBA32)};
    std::unique_ptr<SDL_Renderer, RendererDelete> renderer;
    Canvas() {
        Require(surface != nullptr, "surface");
        renderer.reset(SDL_CreateSoftwareRenderer(surface.get()));
        Require(renderer != nullptr, "software renderer");
    }
};
std::uint64_t Digest(std::span<const WorldInstance> instances) {
    std::uint64_t hash = 14695981039346656037ULL;
    for (const auto b : std::as_bytes(instances)) { hash ^= std::to_integer<std::uint8_t>(b); hash *= 1099511628211ULL; }
    return hash;
}
std::uint64_t WorldPixels(Canvas& canvas) {
    Require(SDL_FlushRenderer(canvas.renderer.get()), "flush");
    std::uint64_t hash = 14695981039346656037ULL;
    const auto* pixels = static_cast<const std::uint8_t*>(canvas.surface->pixels);
    for (int y = 96; y < 616; ++y)
        for (int x = 24 * 4; x < 1256 * 4; ++x) { hash ^= pixels[y * canvas.surface->pitch + x]; hash *= 1099511628211ULL; }
    return hash;
}
void Compare(Canvas& reference, Canvas& candidate, const ScenarioSnapshot& frame,
             const WorldPacket& packet, const Camera2D& camera) {
    desktop::ScenePainter direct{frame.GetSamples().size(), frame.GetBlight().size()};
    PacketPainter instances{std::max(packet.GetCells().size(), packet.GetMarkers().size())};
    Require(direct.TryDraw(*reference.renderer, frame, camera, {}), "direct draw");
    Require(instances.TryDraw(*candidate.renderer, packet, camera), "instance draw");
    Require(SDL_FlushRenderer(reference.renderer.get()) && SDL_FlushRenderer(candidate.renderer.get()), "comparison flush");
    for (int y = 96; y < 616; ++y) {
        const auto offset = y * reference.surface->pitch + 24 * 4;
        const auto other = y * candidate.surface->pitch + 24 * 4;
        Require(std::memcmp(static_cast<const std::byte*>(reference.surface->pixels) + offset,
            static_cast<const std::byte*>(candidate.surface->pixels) + other, 1232 * 4) == 0, "world raster differs");
    }
}
void Verify() {
    Canvas reference, candidate;
    Simulation simulation{2048, {{64, 32, 1}, 0, SteeringSettings{}, ResourceSettings{}}};
    ScenarioSnapshot frame{2048, 0, 2048};
    WorldPacket retained{2048, 2048}, fresh{2048, 2048};
    Camera2D camera{{64, 32, 1}, View};
    Require(frame.TryCapture(simulation) && retained.TryPack(frame), "initial pack");
    Compare(reference, candidate, frame, retained, camera);
    const auto initial_pixels = WorldPixels(candidate);
    const auto cells = Digest(retained.GetCells()), markers = Digest(retained.GetMarkers());
    for (int i = 0; i < 20; ++i) simulation.tick();
    Require(frame.TryCapture(simulation) && fresh.TryPack(frame) && fresh.GetTick() == 20, "next packet");
    Require(retained.GetTick() == 0 && Digest(retained.GetCells()) == cells && Digest(retained.GetMarkers()) == markers,
        "recapture/live mutation cannot modify retained packet");
    PacketPainter painter{2048};
    Require(painter.TryDraw(*candidate.renderer, retained, camera) && WorldPixels(candidate) == initial_pixels,
        "retained packet draws original frame after recapture");
    Compare(reference, candidate, frame, fresh, camera);
    Require(camera.TryZoom({640, 356}, 2) && camera.TryPan({37, -19}), "camera change");
    const auto digest = Digest(fresh.GetMarkers());
    Compare(reference, candidate, frame, fresh, camera);
    Require(Digest(fresh.GetMarkers()) == digest && fresh.GetTick() == 20, "camera redraw does not repack world");
    ScenarioSnapshot uncaptured{2048, 0, 2048};
    Require(!fresh.TryPack(uncaptured) && fresh.GetTick() == 20, "rejection retains metadata");
    WorldPacket too_small{0, 2048};
    Require(!too_small.TryPack(frame) && !too_small.GetGrid(), "sample capacity failure");
    WorldPacket too_few_cells{2048, 1};
    Require(!too_few_cells.TryPack(frame), "cell capacity failure");
    Simulation tiny{0, {{1, 1, std::numeric_limits<float>::denorm_min()}, 0}};
    ScenarioSnapshot tiny_frame{0, 0, 1};
    Require(tiny_frame.TryCapture(tiny) && !fresh.TryPack(tiny_frame) && fresh.GetTick() == 20 &&
        Digest(fresh.GetMarkers()) == digest, "fp32 half-size underflow rejects without mutation");
    Camera2D wrong{{1, 1, 1}, View};
    const auto before = WorldPixels(candidate);
    Require(!painter.TryDraw(*candidate.renderer, fresh, wrong) && WorldPixels(candidate) == before,
        "consumer preflight preserves canvas");
    bool threw = false;
    try { PacketPainter huge{std::numeric_limits<std::size_t>::max()}; }
    catch (const std::length_error&) { threw = true; }
    Require(threw, "geometry overflow rejection");
    std::cout << "verified: pixel-identical world passes; retained packet; camera-only redraw; bounded rejection\n";
}
struct Timing { double median{}, p95{}; };
Timing Summarize(std::vector<double> values) {
    std::ranges::sort(values);
    return {values[values.size() / 2], values[(values.size() * 95) / 100]};
}
void Measure(std::size_t count) {
    Simulation simulation{count, {{64, 32, 1}, 0, SteeringSettings{}, ResourceSettings{}}};
    ScenarioSnapshot frame{count, 0, 2048};
    WorldPacket packet{count, 2048};
    std::vector<double> capture, packing;
    capture.reserve(200); packing.reserve(200);
    for (int i = 0; i < 220; ++i) {
        const auto begin = Clock::now();
        Require(frame.TryCapture(simulation), "measured capture");
        const auto copied = Clock::now();
        Require(packet.TryPack(frame), "measured pack");
        const auto end = Clock::now();
        if (i >= 20) {
            capture.push_back(std::chrono::duration<double, std::micro>(copied - begin).count());
            packing.push_back(std::chrono::duration<double, std::micro>(end - copied).count());
        }
    }
    const auto a = Summarize(capture), b = Summarize(packing);
    const auto records = packet.GetCells().size() + packet.GetMarkers().size();
    Canvas reference, candidate;
    Camera2D camera{{64, 32, 1}, View};
    Compare(reference, candidate, frame, packet, camera); // outside measured intervals
    std::cout << "{\"samples\":" << count << ",\"records\":" << records
        << ",\"instance_bytes\":" << records * sizeof(WorldInstance)
        << ",\"expanded_bytes\":" << records * (4 * sizeof(SDL_Vertex) + 6 * sizeof(int))
        << ",\"capture_median_us\":" << a.median << ",\"capture_p95_us\":" << a.p95
        << ",\"pack_median_us\":" << b.median << ",\"pack_p95_us\":" << b.p95
        << ",\"pixel_equal\":true,\"digest\":" << Digest(packet.GetMarkers()) << "}\n";
}
void Export(const char* path) {
    Simulation simulation{2048, {{64, 32, 1}, 0, SteeringSettings{}, ResourceSettings{}}};
    for (int i = 0; i < 20; ++i) simulation.tick();
    ScenarioSnapshot frame{2048, 0, 2048};
    WorldPacket packet{2048, 2048};
    Canvas canvas;
    PacketPainter painter{2048};
    Camera2D camera{{64, 32, 1}, View};
    Require(frame.TryCapture(simulation) && packet.TryPack(frame) && painter.TryDraw(*canvas.renderer, packet, camera), "export draw");
    Require(SDL_FlushRenderer(canvas.renderer.get()) && SDL_SaveBMP(canvas.surface.get(), path), "export");
}
}
int main(int argc, char** argv) {
    try {
        const std::string_view mode = argc > 1 ? argv[1] : "--verify";
        if (mode == "--verify" && argc <= 2) Verify();
        else if (mode == "--measure" && argc == 2) { Measure(2048); Measure(150000); }
        else if (mode == "--export" && argc == 3) Export(argv[2]);
        else throw std::invalid_argument("Usage: crucible_render_spike [--verify | --measure | --export output.bmp]");
    } catch (const std::exception& e) { std::cerr << e.what() << ": " << SDL_GetError() << '\n'; return 1; }
}
