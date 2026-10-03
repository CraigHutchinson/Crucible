#include <crucible/presentation/gpu/OffscreenRenderer.hpp>
#include <crucible/presentation/gpu/InstancePacket.hpp>
#include <crucible/presentation/Camera2D.hpp>
#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <crucible/presentation/desktop/ScenePainter.hpp>
#include <crucible/presentation/desktop/SceneUi.hpp>
#include <crucible/simulation.hpp>
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>
namespace {
using namespace crucible;
using namespace crucible::presentation;
using namespace crucible::presentation::gpu;
constexpr ScreenRect View{24, 96, 1232, 520};
void Require(bool condition, const char* why) { if (!condition) throw std::runtime_error(why); }
std::vector<std::byte> Load(const char* path) {
    std::ifstream file{path, std::ios::binary | std::ios::ate};
    Require(file.good(), "compiled shader file");
    const auto size = file.tellg(); Require(size > 0, "shader file size");
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    file.seekg(0); file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    Require(file.good(), "compiled shader read"); return bytes;
}
struct DeviceDelete { void operator()(SDL_GPUDevice* p) const noexcept { SDL_DestroyGPUDevice(p); } };
struct SurfaceDelete { void operator()(SDL_Surface* p) const noexcept { SDL_DestroySurface(p); } };
struct RendererDelete { void operator()(SDL_Renderer* p) const noexcept { SDL_DestroyRenderer(p); } };
struct VideoLifetime {
    VideoLifetime() { Require(SDL_Init(SDL_INIT_VIDEO), "required video loader for offscreen Vulkan"); }
    ~VideoLifetime() { SDL_Quit(); }
    VideoLifetime(const VideoLifetime&) = delete;
    VideoLifetime& operator=(const VideoLifetime&) = delete;
};
std::vector<std::byte> Read(OffscreenRenderer& renderer, const InstancePacket& packet, const Camera2D& camera,
                          Submission* receipt = nullptr) {
    auto submitted = renderer.TrySubmit(packet, camera);
    if (submitted.status == SubmitStatus::busy) {
        Require(renderer.TryDrain(), "busy diagnostic drain"); submitted = renderer.TrySubmit(packet, camera);
    }
    Require(submitted.status == SubmitStatus::submitted, "actual instance submission");
    std::vector<std::byte> bytes(OffscreenRenderer::ReadbackBytes, std::byte{0x71});
    const auto first = renderer.PollReadback(submitted, bytes);
    Require(first == ReadbackStatus::pending || first == ReadbackStatus::complete, "poll submitted fence");
    if (first == ReadbackStatus::pending)
        Require(std::all_of(bytes.begin(), bytes.end(), [](std::byte b) { return b == std::byte{0x71}; }), "pending does not publish bytes");
    Require(renderer.TryDrain(), "completed readback fence");
    Require(renderer.PollReadback(submitted, bytes) == ReadbackStatus::complete, "completed GPU readback");
    if (receipt) *receipt = submitted;
    return bytes;
}
void PixelIs(std::span<const std::byte> bytes, int x, int y, std::array<int, 4> color) {
    const auto offset = (y * 1280 + x) * 4;
    for (int c = 0; c < 4; ++c)
        Require(std::abs(std::to_integer<int>(bytes[static_cast<std::size_t>(offset + c)]) - color[c]) <= 1,
                "frozen one-byte palette interior tolerance");
}
void PaletteAndReceipts(SDL_GPUDevice& device, std::span<const std::byte> vertex, std::span<const std::byte> fragment) {
    const GridConfig grid{3, 1, 1};
    Simulation simulation{3, {grid, 0}};
    ScenarioSnapshot frame{3, 0, 3};
    InstancePacket packet{3, 3};
    const std::array<OpaqueColor, 3> cells{{{1, 0, 0, 1}, {0, 1, 0, 1}, {0, 0, 1, 1}}};
    const std::array<OpaqueColor, 3> markers{{{1, 1, 0, 1}, {0, 1, 1, 1}, {1, 0, 1, 1}}};
    Require(frame.TryCapture(simulation) && packet.TryCapture(frame, cells, markers), "synthetic presentation palette");
    Camera2D camera{grid, View};
    Submission destroyed;
    {
        OffscreenRenderer renderer{device, 3, 3, vertex, fragment};
        Submission receipt;
        const auto bytes = Read(renderer, packet, camera, &receipt);
        PixelIs(bytes, 200, 300, {255, 0, 0, 255}); PixelIs(bytes, 600, 300, {0, 255, 0, 255});
        PixelIs(bytes, 1000, 300, {0, 0, 255, 255});
        PixelIs(bytes, 229, 356, {255, 255, 0, 255}); PixelIs(bytes, 640, 356, {0, 255, 255, 255});
        PixelIs(bytes, 1050, 356, {255, 0, 255, 255});
        std::vector<std::byte> sentinel(OffscreenRenderer::ReadbackBytes, std::byte{0x52});
        Require(renderer.PollReadback(receipt, std::span{sentinel}.first(1)) == ReadbackStatus::invalid &&
            sentinel[0] == std::byte{0x52}, "undersized readback unchanged");
        Camera2D wrong{{1, 1, 1}, View};
        Require(renderer.TrySubmit(packet, wrong).status == SubmitStatus::invalid &&
            renderer.PollReadback(receipt, sentinel) == ReadbackStatus::complete && sentinel == bytes,
            "invalid camera cannot overwrite a retained target");
        simulation.tick(); Require(frame.TryCapture(simulation), "recapture later mutable frame");
        Submission replacement;
        Require(Read(renderer, packet, camera, &replacement) == bytes && replacement.tick == 0,
            "device input retains old packet after snapshot mutation");
        Require(renderer.PollReadback(receipt, sentinel) == ReadbackStatus::expired, "reused slot invalidates old receipt");
        OffscreenRenderer second{device, 3, 3, vertex, fragment};
        Require(second.PollReadback(replacement, sentinel) == ReadbackStatus::invalid, "foreign renderer receipt denied");
        destroyed = replacement;
    }
    OffscreenRenderer replacement{device, 3, 3, vertex, fragment};
    std::vector<std::byte> bytes(OffscreenRenderer::ReadbackBytes);
    Require(replacement.PollReadback(destroyed, bytes) == ReadbackStatus::expired, "destroyed issuer identity expires");
    bool rejected{};
    try { OffscreenRenderer invalid{device, 3, 3, {}, fragment}; }
    catch (const std::invalid_argument&) { rejected = true; }
    Require(rejected, "constructor malformed shader rejected with safe partial cleanup");
}
void CompareOracle(SDL_GPUDevice& device, std::span<const std::byte> vertex, std::span<const std::byte> fragment,
                   GridConfig grid, std::size_t samples, bool evolved) {
    const auto extent = *grid.TryValidate();
    Simulation simulation{samples, {grid, 0, {}, ResourceSettings{}}};
    if (evolved) for (int i = 0; i < 20; ++i) simulation.tick();
    ScenarioSnapshot frame{samples, 0, extent.cells};
    InstancePacket packet{samples, extent.cells};
    Require(frame.TryCapture(simulation) && packet.TryCapture(frame), "oracle frame extraction");
    Camera2D camera{grid, View};
    OffscreenRenderer renderer{device, samples, extent.cells, vertex, fragment};
    std::unique_ptr<SDL_Surface, SurfaceDelete> surface{SDL_CreateSurface(1280, 720, SDL_PIXELFORMAT_RGBA32)};
    Require(surface != nullptr, "software oracle surface");
    std::unique_ptr<SDL_Renderer, RendererDelete> software{SDL_CreateSoftwareRenderer(surface.get())};
    Require(software != nullptr, "software oracle renderer");
    desktop::ScenePainter direct{samples, extent.cells};
    for (const bool zoom : {false, true}) {
        if (zoom) Require(camera.TryZoom({640, 356}, 4) && camera.TryPan({17, -9}), "oracle zoom/pan");
        const auto bytes = Read(renderer, packet, camera);
        Require(direct.TryDraw(*software, frame, camera, {}) && SDL_FlushRenderer(software.get()), "actual production software oracle");
        std::vector<bool> edges(1280 * 720);
        const auto vertical = [&](double x, double y0, double y1) {
            for (int px = std::max(24, static_cast<int>(std::floor(x - 1))); px <= std::min(1255, static_cast<int>(std::ceil(x + 1))); ++px)
                for (int py = std::max(96, static_cast<int>(std::floor(y0 - 1))); py <= std::min(615, static_cast<int>(std::ceil(y1 + 1))); ++py)
                    if (std::abs(px + .5 - x) <= 1) edges[py * 1280 + px] = true;
        };
        const auto horizontal = [&](double y, double x0, double x1) {
            for (int py = std::max(96, static_cast<int>(std::floor(y - 1))); py <= std::min(615, static_cast<int>(std::ceil(y + 1))); ++py)
                for (int px = std::max(24, static_cast<int>(std::floor(x0 - 1))); px <= std::min(1255, static_cast<int>(std::ceil(x1 + 1))); ++px)
                    if (std::abs(py + .5 - y) <= 1) edges[py * 1280 + px] = true;
        };
        const auto origin = *camera.TryToScreen({0, 0});
        const auto scale = camera.GetScale();
        const auto left = std::clamp(origin.x, 24., 1256.), top = std::clamp(origin.y, 96., 616.);
        const auto right = std::clamp(origin.x + extent.width * scale, 24., 1256.);
        const auto bottom = std::clamp(origin.y + extent.height * scale, 96., 616.);
        for (std::size_t col = 0; col <= grid.columns; ++col)
            vertical(std::clamp(origin.x + col * static_cast<double>(grid.cell_size) * scale, 24., 1256.), top, bottom);
        for (std::size_t row = 0; row <= grid.rows; ++row)
            horizontal(std::clamp(origin.y + row * static_cast<double>(grid.cell_size) * scale, 96., 616.), left, right);
        const double half = std::clamp(scale * .08, 2., 5.);
        for (const auto& sample : frame.GetSamples()) {
            const auto p = *camera.TryToScreen(sample.position);
            const auto x0 = std::clamp(p.x - half, 24., 1256.), x1 = std::clamp(p.x + half, 24., 1256.);
            const auto y0 = std::clamp(p.y - half, 96., 616.), y1 = std::clamp(p.y + half, 96., 616.);
            vertical(x0, y0, y1); vertical(x1, y0, y1); horizontal(y0, x0, x1); horizontal(y1, x0, x1);
        }
        const auto* expected = static_cast<const std::uint8_t*>(surface->pixels);
        std::size_t checked{};
        for (int y = 96; y < 616; ++y) for (int x = 24; x < 1256; ++x) {
            if (edges[y * 1280 + x]) continue;
            ++checked;
            for (int c = 0; c < 4; ++c)
                Require(std::abs(std::to_integer<int>(bytes[(y * 1280 + x) * 4 + c]) -
                    expected[y * surface->pitch + x * 4 + c]) <= 1, "GPU/software interior exceeds frozen byte tolerance");
        }
        Require(checked > 1232 * 520 / 4, "analytic edge mask cannot hide most of the image");
    }
}
}
int main(int argc, char** argv) {
    try {
        Require(argc == 3, "vertex/fragment SPIR-V paths required");
        const auto vertex = Load(argv[1]), fragment = Load(argv[2]);
        VideoLifetime video;
        std::unique_ptr<SDL_GPUDevice, DeviceDelete> device{SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, "vulkan")};
        Require(device != nullptr, "required executing Vulkan device (no skip)");
        std::cout << "SDL_GPU backend " << SDL_GetGPUDeviceDriver(device.get()) << '\n';
        PaletteAndReceipts(*device, vertex, fragment);
        CompareOracle(*device, vertex, fragment, {64, 32, 1}, 2048, false);
        CompareOracle(*device, vertex, fragment, {64, 32, 1}, 2048, true);
        CompareOracle(*device, vertex, fragment, {7, 5, .1F}, 3, false);
        CompareOracle(*device, vertex, fragment, {1, 1, std::numeric_limits<float>::denorm_min()}, 0, false);
        CompareOracle(*device, vertex, fragment, {4, 1, std::numeric_limits<float>::max() / 4}, 0, false);
        std::cout << "instancing/readback, palette variants, numeric oracle and receipt lifetime passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << ": " << SDL_GetError() << '\n'; return 1; }
}
