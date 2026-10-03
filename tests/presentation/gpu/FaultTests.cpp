#include "FaultController.hpp"
#include <crucible/presentation/gpu/OffscreenRenderer.hpp>
#include <crucible/presentation/gpu/InstancePacket.hpp>
#include <crucible/presentation/Camera2D.hpp>
#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <crucible/simulation.hpp>
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
namespace {
using namespace crucible;
using namespace crucible::presentation;
using namespace crucible::presentation::gpu;
using gpu_fault::Call;
using gpu_fault::Controller;
constexpr ScreenRect View{24, 96, 1232, 520};
void Require(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
std::vector<std::byte> Load(const char* path) {
    std::ifstream file{path, std::ios::binary | std::ios::ate};
    Require(file.good(), "shader file open");
    const auto size = file.tellg(); Require(size > 0 && size <= 1024 * 1024, "shader file size in (0,1MiB]");
    std::vector<std::byte> result(static_cast<std::size_t>(size));
    file.seekg(0); file.read(reinterpret_cast<char*>(result.data()), static_cast<std::streamsize>(result.size()));
    Require(file.good(), "shader file read"); return result;
}
struct DeviceDelete { void operator()(SDL_GPUDevice* p) const noexcept { SDL_DestroyGPUDevice(p); } };
struct SurfaceDelete { void operator()(SDL_Surface* p) const noexcept { SDL_DestroySurface(p); } };
struct VideoLifetime {
    VideoLifetime() { Require(SDL_Init(SDL_INIT_VIDEO), "video loader for offscreen Vulkan"); }
    ~VideoLifetime() { SDL_Quit(); }
    VideoLifetime(const VideoLifetime&) = delete;
    VideoLifetime& operator=(const VideoLifetime&) = delete;
};
struct Scene {
    GridConfig grid{1, 1, 1};
    Simulation simulation{0, {grid, 0}};
    ScenarioSnapshot frame{0, 0, 1};
    InstancePacket packet{0, 1};
    Camera2D camera{grid, View};
    void Capture(OpaqueColor color) {
        Require(frame.TryCapture(simulation) && packet.TryCapture(frame, std::span{&color, 1}), "owned colored packet");
    }
};
bool Sentinel(const std::vector<std::byte>& bytes) {
    return std::all_of(bytes.begin(), bytes.end(), [](std::byte b) { return b == std::byte{0x52}; });
}
void Pixel(std::span<const std::byte> bytes, OpaqueColor color) {
    constexpr std::size_t offset = (356 * OffscreenRenderer::Width + 640) * 4;
    for (std::size_t c = 0; c < color.size(); ++c)
        Require(std::abs(std::to_integer<int>(bytes[offset + c]) - static_cast<int>(color[c] * 255)) <= 1,
                "independent central interior pixel");
}
void StartupFailures(SDL_GPUDevice& device, std::span<const std::byte> vertex, std::span<const std::byte> fragment) {
    // The unchanged constructor owns eighteen allocations, including all three slots.
    for (std::size_t ordinal = 1; ordinal <= 18; ++ordinal) {
        Controller control{device}; control.Arm(Call::create, ordinal);
        bool threw = false;
        try { OffscreenRenderer renderer{device, 0, 1, vertex, fragment}; }
        catch (const std::runtime_error&) { threw = true; }
        Require(threw && control.Triggered(), "specific partial creation failure received");
        Require(control.Calls(Call::create) == ordinal, "no creation beyond failure");
        Require(control.Balanced(), "partial creation releases every real owned handle");
        Require(control.submission_count == (ordinal > 6 ? 1U : 0U), "partial creation startup submission scope");
        if (ordinal > 6) Require(control.successful_waits == 1, "submitted startup drained before cleanup");
    }
    for (const auto call : {Call::map, Call::acquire, Call::copy, Call::submit}) {
        Controller control{device}; control.Arm(call);
        bool threw = false;
        try { OffscreenRenderer renderer{device, 0, 1, vertex, fragment}; }
        catch (const std::runtime_error&) { threw = true; }
        Require(threw && control.Triggered() && control.Balanced(), "startup hook and balanced cleanup");
        Require(control.submission_count == 0, "startup rejection never submits a real command");
        Require(control.canceled_count == (call == Call::copy || call == Call::submit ? 1U : 0U), "startup real command cancellation");
    }
}
void FrameFailures(SDL_GPUDevice& device, std::span<const std::byte> vertex, std::span<const std::byte> fragment) {
    struct Case { Call call; std::size_t ordinal; };
    constexpr std::array cases{Case{Call::acquire, 1}, Case{Call::map, 1}, Case{Call::copy, 1},
                              Case{Call::render, 1}, Case{Call::copy, 2}, Case{Call::submit, 1}};
    Scene scene; scene.Capture({1, 0, 0, 1});
    for (const auto item : cases) {
        Controller control{device};
        {
            OffscreenRenderer renderer{device, 0, 1, vertex, fragment};
            const auto prior = renderer.TrySubmit(scene.packet, scene.camera);
            Require(prior.status == SubmitStatus::submitted && renderer.TryDrain(), "real prior frame completed");
            std::vector<std::byte> old(OffscreenRenderer::ReadbackBytes);
            Require(renderer.PollReadback(prior, old) == ReadbackStatus::complete, "prior frame readable"); Pixel(old, {1, 0, 0, 1});
            control.Arm(item.call, item.ordinal);
            const auto submitted_before = control.submission_count;
            const auto canceled_before = control.canceled_count;
            const auto failed = renderer.TrySubmit(scene.packet, scene.camera);
            Require(failed.status == SubmitStatus::device_error && failed.id == 0 && failed.issuer.expired() &&
                    control.Triggered(), "frame failure has no published receipt");
            Require(control.submission_count == submitted_before, "failure did not hide a real submitted fence");
            Require(control.canceled_count == canceled_before + (item.call == Call::acquire ? 0U : 1U), "failed real command canceled exactly once");
            const auto acquire_after = control.Calls(Call::acquire);
            Require(renderer.TrySubmit(scene.packet, scene.camera).status == SubmitStatus::device_error &&
                    control.Calls(Call::acquire) == acquire_after, "failure latches submit before mutation");
            std::vector<std::byte> destination(OffscreenRenderer::ReadbackBytes, std::byte{0x52});
            Require(renderer.PollReadback(prior, destination) == ReadbackStatus::device_error && Sentinel(destination),
                    "latched failure leaves readback destination unchanged");
            Require(control.Good(), "frame wrapper handle/map accounting");
        }
        Require(control.Balanced(), "frame failure drained/released every real owned handle");
    }
    Controller control{device};
    {
        OffscreenRenderer renderer{device, 0, 1, vertex, fragment};
        const auto receipt = renderer.TrySubmit(scene.packet, scene.camera);
        Require(receipt.status == SubmitStatus::submitted && renderer.TryDrain(), "readback failure real submitted frame");
        control.Arm(Call::map);
        std::vector<std::byte> destination(OffscreenRenderer::ReadbackBytes, std::byte{0x52});
        Require(renderer.PollReadback(receipt, destination) == ReadbackStatus::device_error &&
                control.Triggered() && Sentinel(destination), "failed readback map cannot publish bytes");
        const auto maps = control.Calls(Call::map);
        Require(renderer.PollReadback(receipt, destination) == ReadbackStatus::device_error && Sentinel(destination) &&
                control.Calls(Call::map) == maps && renderer.TrySubmit(scene.packet, scene.camera).status == SubmitStatus::device_error,
                "readback map failure latches both APIs");
    }
    Require(control.Balanced(), "readback failure cleanup");
}
void Export(const std::filesystem::path& path, std::span<const std::byte> bytes) {
    std::unique_ptr<SDL_Surface, SurfaceDelete> surface{SDL_CreateSurfaceFrom(1280, 720, SDL_PIXELFORMAT_RGBA32,
        const_cast<std::byte*>(bytes.data()), 1280 * 4)};
    Require(surface && SDL_SaveBMP(surface.get(), path.string().c_str()), "retained actual GPU BMP export");
}
void Retirement(SDL_GPUDevice& device, std::span<const std::byte> vertex, std::span<const std::byte> fragment,
                const char* export_path) {
    Scene scene;
    constexpr std::array<OpaqueColor, 4> colors{{{1, 0, 0, 1}, {0, 1, 0, 1}, {0, 0, 1, 1}, {1, 1, 0, 1}}};
    Controller control{device};
    std::ofstream events;
    if (export_path) {
        Require(std::filesystem::create_directory(export_path), "export directory must be new");
        events.open(std::filesystem::path{export_path} / "events.csv"); Require(events.good(), "receipt event export open");
        events << "event,id,tick,slot,status\n";
    }
    auto event = [&](const char* name, const Submission& s, const char* status) {
        if (export_path) { events << name << ',' << s.id << ',' << s.tick << ',' << s.slot << ',' << status << '\n';
            Require(events.good(), "receipt event export write"); }
    };
    {
        OffscreenRenderer renderer{device, 0, 1, vertex, fragment};
        control.hold_queries = true;
        std::array<Submission, 3> receipts;
        for (std::size_t i = 0; i < receipts.size(); ++i) {
            scene.Capture(colors[i]); receipts[i] = renderer.TrySubmit(scene.packet, scene.camera);
            Require(receipts[i].status == SubmitStatus::submitted && receipts[i].slot == i &&
                    receipts[i].tick == i && receipts[i].id == i + 1, "three real distinct submissions retain slots/ticks");
            event("submit", receipts[i], "submitted"); scene.simulation.tick();
        }
        const auto calls_before = std::array{control.Calls(Call::acquire), control.Calls(Call::map),
            control.Calls(Call::copy), control.Calls(Call::render), control.Calls(Call::submit)};
        scene.Capture(colors[3]);
        const auto busy = renderer.TrySubmit(scene.packet, scene.camera);
        Require(busy.status == SubmitStatus::busy && busy.id == 0, "fourth real receiver attempt busy");
        event("fourth", busy, "busy");
        Require(calls_before == std::array{control.Calls(Call::acquire), control.Calls(Call::map),
            control.Calls(Call::copy), control.Calls(Call::render), control.Calls(Call::submit)}, "busy performs no command/map/pass/submit mutation");
        std::vector<std::byte> destination(OffscreenRenderer::ReadbackBytes, std::byte{0x52});
        for (const auto& receipt : receipts) {
            Require(renderer.PollReadback(receipt, destination) == ReadbackStatus::pending && Sentinel(destination), "withheld query preserves destination");
            event("poll", receipt, "pending");
        }
        // Real completion is obtained independently; receiver bookkeeping stays pending.
        Require(control.PhysicalWait(), "real device completed all three while observations withheld");
        Require(renderer.TrySubmit(scene.packet, scene.camera).status == SubmitStatus::busy, "physical idle alone cannot retire hidden receipts");
        control.visible_fence = control.submitted_fences[2]; // Startup is fence zero; frame one is index two.
        Require(renderer.PollReadback(receipts[1], destination) == ReadbackStatus::complete, "one observed real fence retires exactly its slot");
        Pixel(destination, colors[1]); event("selective_poll", receipts[1], "complete");
        Require(renderer.PollReadback(receipts[0], destination) == ReadbackStatus::pending, "other real fence remains hidden");
        Pixel(destination, colors[1]);
        std::array<std::vector<std::byte>, 3> retained;
        retained[1] = destination;
        const auto replacement = renderer.TrySubmit(scene.packet, scene.camera);
        Require(replacement.status == SubmitStatus::submitted && replacement.slot == 1 && replacement.id == 4 && replacement.tick == 3,
                "only observed slot reused with next unique receipt");
        event("reuse", replacement, "submitted");
        Require(renderer.PollReadback(receipts[1], destination) == ReadbackStatus::expired && destination == retained[1], "stale reused receipt leaves destination unchanged");
        event("old_poll", receipts[1], "expired");
        Require(renderer.TryDrain(), "real blocking drain retires remaining slots"); event("drain", replacement, "complete_barrier");
        for (const auto i : {0U, 2U}) {
            retained[i].resize(OffscreenRenderer::ReadbackBytes);
            Require(renderer.PollReadback(receipts[i], retained[i]) == ReadbackStatus::complete, "drained original slot independent readback");
            Pixel(retained[i], colors[i]); event("readback", receipts[i], "complete");
        }
        Require(retained[0] != retained[1] && retained[1] != retained[2] && retained[0] != retained[2], "three retained outputs are distinct");
        Require(renderer.PollReadback(replacement, destination) == ReadbackStatus::complete, "replacement completes independently"); Pixel(destination, colors[3]);
        if (export_path) {
            for (std::size_t i = 0; i < retained.size(); ++i)
                Export(std::filesystem::path{export_path} / ("retained-" + std::to_string(i) + ".bmp"), retained[i]);
            events.flush(); Require(events.good(), "receipt event export flush");
        }
    }
    Require(control.Balanced(), "real three-slot resources/fences all drained and released");
}
std::size_t releases_before_terminate{};
[[noreturn]] void ExpectedTerminate() noexcept {
    const auto* control = gpu_fault::Active();
    if (control && control->Triggered() && control->Good() && control->Releases() == releases_before_terminate) {
        std::fputs("FAULT_TERMINATE_NO_RELEASE\n", stderr); std::fflush(stderr); std::_Exit(86);
    }
    std::fputs("FAULT_UNEXPECTED_TERMINATE\n", stderr); std::fflush(stderr); std::_Exit(88);
}
void Child(SDL_GPUDevice& device, std::span<const std::byte> vertex, std::span<const std::byte> fragment, bool hang) {
    Scene scene; scene.Capture({1, 0, 0, 1});
    Controller control{device};
    {
        OffscreenRenderer renderer{device, 0, 1, vertex, fragment};
        Require(renderer.TrySubmit(scene.packet, scene.camera).status == SubmitStatus::submitted, "child owns real queued work");
        releases_before_terminate = control.Releases();
        control.forbid_release = true;
        if (hang) control.hang_wait = true;
        else { control.Arm(Call::wait); std::set_terminate(ExpectedTerminate); }
        const bool drained = renderer.TryDrain();
        Require(!hang && !drained && control.Triggered() && control.Releases() == releases_before_terminate, "failed wait cannot release owned work");
        Require(renderer.TrySubmit(scene.packet, scene.camera).status == SubmitStatus::device_error, "failed drain latches receiver");
        std::fputs("FAULT_DRAIN_FALSE_NO_RELEASE\n", stderr); std::fflush(stderr);
        // Destruction repeats the sticky failed barrier and invokes the expected handler.
    }
    throw std::runtime_error("failed drain destruction returned");
}
}
int main(int argc, char** argv) {
    try {
        Require(argc == 4 || argc == 5, "usage: fault-test vertex.spv fragment.spv normal|failed-drain|hang|--export [export-directory]");
        const std::string_view mode{argv[3]};
        Require((argc == 4 && (mode == "normal" || mode == "failed-drain" || mode == "hang")) ||
                (argc == 5 && mode == "--export" && std::string_view{argv[4]}.size() != 0), "invalid fault-test mode/arguments");
        const auto vertex = Load(argv[1]), fragment = Load(argv[2]);
        VideoLifetime video;
        std::unique_ptr<SDL_GPUDevice, DeviceDelete> device{SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, "vulkan")};
        Require(device != nullptr, SDL_GetError());
        if (mode == "normal" || mode == "--export") {
            StartupFailures(*device, vertex, fragment); FrameFailures(*device, vertex, fragment);
            Retirement(*device, vertex, fragment, argc == 5 ? argv[4] : nullptr);
            std::cout << "GPU_FAULT_RECEIVING_PASS creation=18 startup=4 frame=6 readback=1 real_slots=3\n";
        } else if (mode == "failed-drain" || mode == "hang") Child(*device, vertex, fragment, mode == "hang");
        else throw std::invalid_argument("unknown fault-test mode");
        return 0;
    } catch (const std::exception& e) { std::cerr << "GPU fault fixture: " << e.what() << '\n'; return 1; }
}
