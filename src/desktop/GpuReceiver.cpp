// Executes the concrete offscreen world pass from completed production mission frames.
// Three captures distinguish initial/evolved state from a synthetic multi-color
// presentation fixture; this diagnostic does not replace the interactive desktop.
#include <crucible/presentation/gpu/OffscreenRenderer.hpp>
#include <crucible/presentation/gpu/InstancePacket.hpp>
#include <crucible/presentation/Camera2D.hpp>
#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <crucible/presentation/desktop/ScenePainter.hpp>
#include <crucible/presentation/desktop/SceneUi.hpp>
#include <crucible/runtime/InspectorSession.hpp>
#include <crucible/runtime/ReferenceMissionRoute.hpp>
#include <SDL3/SDL.h>
#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
using namespace crucible;
namespace gpu = presentation::gpu;
void Require(bool ok, const char* reason) { if (!ok) throw std::runtime_error(reason); }
std::vector<std::byte> ReadShader(const char* path) {
    std::ifstream file{path, std::ios::binary | std::ios::ate};
    Require(file.good(), "shader input open");
    const auto size = file.tellg();
    Require(size > 0 && size <= 1024 * 1024, "bounded shader input");
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    file.seekg(0); file.read(reinterpret_cast<char*>(bytes.data()), size);
    Require(file.good(), "shader input read");
    return bytes;
}
void ReadFrame(gpu::OffscreenRenderer& renderer, const gpu::Submission& submission, std::span<std::byte> bytes) {
    Require(submission.status == gpu::SubmitStatus::submitted, "diagnostic frame submitted");
    const auto end = std::chrono::steady_clock::now() + std::chrono::seconds{30};
    while (true) {
        const auto status = renderer.PollReadback(submission, bytes);
        if (status == gpu::ReadbackStatus::complete) return;
        Require(status == gpu::ReadbackStatus::pending, "diagnostic readback failure");
        Require(std::chrono::steady_clock::now() < end, "diagnostic readback timeout");
        SDL_Delay(1);
    }
}
void Save(std::span<std::byte> bytes, const std::filesystem::path& path) {
    std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface{
        SDL_CreateSurfaceFrom(1280, 720, SDL_PIXELFORMAT_RGBA32, bytes.data(), 1280 * 4), SDL_DestroySurface};
    Require(surface && SDL_SaveBMP(surface.get(), path.string().c_str()), "GPU capture save");
}
}
int main(int argc, char** argv) {
    try {
        Require(argc == 4 || argc == 5, "usage: crucible_gpu_receiver VERTEX_SPIRV FRAGMENT_SPIRV NEW_OUTPUT_DIRECTORY [--palette]");
        Require(argc == 4 || std::string_view{argv[4]} == "--palette", "unknown diagnostic argument");
        const auto vertex = ReadShader(argv[1]), fragment = ReadShader(argv[2]);
        const std::filesystem::path directory{argv[3]};
        Require(std::filesystem::create_directory(directory), "GPU output directory must be new");
        Require(SDL_Init(SDL_INIT_VIDEO), "GPU diagnostic SDL startup");
        struct Quit { ~Quit() { SDL_Quit(); } } quit;
        std::unique_ptr<SDL_GPUDevice, decltype(&SDL_DestroyGPUDevice)> device{
            SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, "vulkan"), SDL_DestroyGPUDevice};
        Require(device != nullptr, "Vulkan GPU device required");
        gpu::OffscreenRenderer renderer{*device, 2048, 2048, vertex, fragment};
        gpu::InstancePacket packet{2048, 2048};
        runtime::InspectorSession run{2048, {64, 4096}, ReclamationMissionSettings{}};
        presentation::Camera2D camera{{64, 32, 1}, {24, 96, 1232, 520}};
        std::vector<std::byte> rgba(gpu::OffscreenRenderer::ReadbackBytes);
        std::ofstream metadata{directory / "frames.json"};
        Require(metadata.good(), "GPU metadata open");
        metadata << "{\"backend\":\"" << SDL_GetGPUDeviceDriver(device.get())
            << "\",\"size\":[1280,720],\"scope\":\"offscreen world only\",\"frames\":[";
        for (const auto tick : {0, 60}) {
            while (run.GetMission()->completed_tick < static_cast<std::uint64_t>(tick)) {
                if (const auto edit = runtime::GetReferenceMissionRouteEdit(run.GetMission()->completed_tick))
                    Require(run.TryAdmitFieldEdit(*edit).status == runtime::CommandIngress::AdmissionStatus::accepted,
                        "GPU live route admitted");
                Require(run.TryPump(std::chrono::nanoseconds{16'666'667}).advanced_ticks == 1, "GPU live completed tick");
            }
            if (tick == 0) {
                std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface{
                    SDL_CreateSurface(1280, 720, SDL_PIXELFORMAT_RGBA32), SDL_DestroySurface};
                Require(surface != nullptr, "matched software surface");
                std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> software{
                    SDL_CreateSoftwareRenderer(surface.get()), SDL_DestroyRenderer};
                Require(software != nullptr, "matched software renderer");
                presentation::desktop::ScenePainter painter{2048, 2048};
                presentation::desktop::SceneUi ui{};
                ui.mission = run.GetMission();
                Require(painter.TryDraw(*software, run.GetSnapshot(), camera, ui) &&
                    SDL_FlushRenderer(software.get()) &&
                    SDL_SaveBMP(surface.get(), (directory / "initial-software.bmp").string().c_str()),
                    "same-input software capture");
            }
            Require(packet.TryCapture(run.GetSnapshot()), "GPU owned live packet");
            const auto receipt = renderer.TrySubmit(packet, camera);
            ReadFrame(renderer, receipt, rgba);
            Save(rgba, directory / (tick == 0 ? "initial.bmp" : "evolved.bmp"));
            if (tick) metadata << ',';
            const auto mission = *run.GetMission();
            const auto ledger = *run.GetSnapshot().GetInfo()->biomass;
            metadata << "{\"tick\":" << receipt.tick << ",\"reclaimed\":" << mission.reclaimed
                << ",\"initial_total\":" << ledger.initial_total << ",\"stock\":" << ledger.remaining_stock
                << ",\"mobile\":" << ledger.mobile_mass << ",\"reserve\":" << ledger.reserve << ",\"commands\":" << run.GetTrace().size()
                << ",\"receipt\":" << receipt.id << ",\"readback\":\"complete\"}";
        }
        metadata << "],\"applied_trace\":[";
        bool first_command = true;
        for (const auto& command : run.GetTrace()) {
            if (!first_command) metadata << ',';
            first_command = false;
            const auto& edit = command.command.field;
            metadata << "{\"sequence\":" << command.sequence << ",\"applied_tick\":" << command.tick
                << ",\"kind\":\"" << (edit.kind == FieldEditKind::set ? "set" :
                    edit.kind == FieldEditKind::set_flow ? "set_flow" : "remove")
                << "\",\"slot\":" << edit.slot << ",\"x\":" << edit.center.x << ",\"y\":" << edit.center.y
                << ",\"radius\":" << edit.radius << ",\"strength\":" << edit.strength
                << ",\"end_x\":" << edit.end.x << ",\"end_y\":" << edit.end.y << '}';
        }
        metadata << ']';
        if (argc == 5) {
            constexpr std::array<gpu::OpaqueColor, 3> palette{{{.2F,.7F,1,1},{.8F,.4F,.2F,1},{.5F,.3F,.8F,1}}};
            std::vector<gpu::OpaqueColor> cells(2048), markers(2048);
            for (std::size_t i = 0; i < cells.size(); ++i) { cells[i] = palette[i % 3]; markers[i] = palette[(i + 1) % 3]; }
            Require(packet.TryCapture(run.GetSnapshot(), cells, markers), "synthetic palette packet");
            const auto palette_receipt = renderer.TrySubmit(packet, camera);
            ReadFrame(renderer, palette_receipt, rgba);
            Save(rgba, directory / "synthetic-palette.bmp");
            metadata << ",\"palette\":\"synthetic presentation colors; no faction gameplay\",\"palette_tick\":"
                << palette_receipt.tick << ",\"palette_receipt\":" << palette_receipt.id
                << ",\"palette_readback\":\"complete\"";
        }
        metadata << "}\n";
        metadata.flush();
        Require(metadata.good() && renderer.TryDrain(), "GPU diagnostic metadata and retirement");
        std::cout << "Vulkan instancing receiver: completed initial/evolved readbacks; tick=60 samples=2048\n";
    } catch (const std::exception& error) { std::cerr << error.what() << ": " << SDL_GetError() << '\n'; return 1; }
}
