#include <SDL3/SDL.h>
#include <chrono>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <thread>

#include "crucible/presentation/Camera2D.hpp"
#include "crucible/presentation/desktop/checked_presentation.hpp"
#include "crucible/presentation/desktop/ScenePainter.hpp"
#include "crucible/presentation/desktop/SceneUi.hpp"
#include "crucible/presentation/desktop/frame_completion_observer.hpp"
#include "crucible/presentation/ScenarioSnapshot.hpp"
#include "crucible/simulation.hpp"

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
struct SurfaceDelete { void operator()(SDL_Surface* value) const noexcept { SDL_DestroySurface(value); } };
struct RendererDelete { void operator()(SDL_Renderer* value) const noexcept { SDL_DestroyRenderer(value); } };
struct Canvas {
    std::unique_ptr<SDL_Surface, SurfaceDelete> surface;
    std::unique_ptr<SDL_Renderer, RendererDelete> renderer;
    Canvas() : surface(SDL_CreateSurface(1280, 864, SDL_PIXELFORMAT_RGBA32)),
        renderer(surface ? SDL_CreateSoftwareRenderer(surface.get()) : nullptr) {
        require(static_cast<bool>(renderer), "software receiver creation");
    }
};
constexpr crucible::presentation::ScreenRect viewport{24, 96, 1232, 520};
void closedCounts(crucible::presentation::desktop::ScenePainter::DrawStatistics counts) {
    require(counts.authoritativeMobile == counts.individualSamples + counts.aggregatedSamples + counts.hiddenSamples,
        "all mobile identities accounted for exactly once");
    require(counts.aggregateMarks <= counts.aggregatedSamples, "aggregate marks cannot invent population");
}
void exactView(Canvas& canvas) {
    using namespace crucible;
    using namespace crucible::presentation;
    using namespace crucible::presentation::desktop;
    const GridConfig grid{16, 8, 1};
    Simulation simulation{128, {grid, 4, {}, {}}};
    ScenarioSnapshot frame{128, 4, 128};
    Camera2D camera{grid, viewport};
    ScenePainter painter{128, 128};
    require(frame.TryCapture(simulation) && painter.TryDraw(*canvas.renderer, frame, camera, {}), "exact fitted view");
    auto counts = painter.getDrawStatistics();
    require(counts.authoritativeMobile == 128 && counts.individualSamples == 128 &&
        counts.hiddenSamples == 0 && counts.visibleCells == 128 && counts.aggregateMarks == 0,
        "fit preserves individual population");
    require(camera.TryZoom({640, 356}, 4) && painter.TryDraw(*canvas.renderer, frame, camera, {}), "zoomed exact view");
    counts = painter.getDrawStatistics();
    // Independent fit: zoomed world interval x[5.6308,10.3692], y[3,5].
    require(counts.individualSamples == 8 && counts.hiddenSamples == 120, "four columns by two rows visible");
    closedCounts(counts);
    Camera2D wrong{{1, 1, 1}, viewport};
    require(!painter.TryDraw(*canvas.renderer, frame, wrong, {}) &&
        painter.getDrawStatistics().individualSamples == 8, "rejection retains previous statistics");
}
void densityView(Canvas& canvas) {
    using namespace crucible;
    using namespace crucible::presentation;
    using namespace crucible::presentation::desktop;
    const GridConfig grid{256, 256, 1};
    Simulation simulation{65536, {grid, 4, {}, {}}};
    ScenarioSnapshot frame{65536, 4, 65536}, reference{65536, 4, 65536};
    Camera2D camera{grid, viewport};
    ScenePainter painter{65536, 65536, ScenePainter::ViewPolicy::densityOverview};
    require(frame.TryCapture(simulation) && reference.TryCapture(simulation) &&
        painter.TryDraw(*canvas.renderer, frame, camera, {}), "density overview draw");
    const auto overview = painter.getDrawStatistics();
    closedCounts(overview);
    require(overview.aggregatedSamples == 65536 && overview.individualSamples == 0 &&
        overview.hiddenSamples == 0 && overview.aggregateMarks > 0 && overview.aggregateMarks < 65536,
        "overview aggregates representations while preserving live count");
    require(camera.TryZoom({640, 356}, 4) && painter.TryDraw(*canvas.renderer, frame, camera, {}), "density detail draw");
    const auto detail = painter.getDrawStatistics();
    closedCounts(detail);
    require(detail.individualSamples > 0 && detail.aggregatedSamples == 0 && detail.hiddenSamples > 0,
        "detail returns to visible individuals");
    require(frame.HasEqualState(reference), "view policy never changes copied simulation state");
}
void unsupportedObserver(Canvas& canvas) {
    using Observer = crucible::presentation::desktop::FrameCompletionObserver;
    Observer observer{*canvas.renderer, 3};
    require(observer.getCapability() == Observer::Capability::unsupported &&
        observer.recordFrame({1, 1, 0}) == Observer::RecordStatus::unsupported &&
        observer.pollOldest().status == Observer::PollStatus::unsupported &&
        observer.getPendingCount() == 0 && observer.tryDrain(), "software observer cannot claim GPU completion");
    bool rejected = false;
    try { Observer invalid{*canvas.renderer, 0}; }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "zero observation capacity refused at startup");
    crucible::presentation::desktop::CheckedPresentation presentation{*canvas.renderer};
    require(!presentation.isSupported() && presentation.present().status ==
        crucible::presentation::desktop::CheckedPresentation::Status::unsupported,
        "software renderer cannot claim native checked handoff");
}
void nativeCompletion() {
    using Observer = crucible::presentation::desktop::FrameCompletionObserver;
    require(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS), "native receiver video initialization");
    struct Quit { ~Quit() { SDL_Quit(); } } quit;
    struct WindowDelete { void operator()(SDL_Window* value) const noexcept { SDL_DestroyWindow(value); } };
    std::unique_ptr<SDL_Window, WindowDelete> window{SDL_CreateWindow("Crucible completion receiving", 320, 240, 0)};
    require(static_cast<bool>(window), "native receiver window creation");
    std::unique_ptr<SDL_Renderer, RendererDelete> renderer{SDL_CreateRenderer(window.get(), "direct3d11")};
    require(static_cast<bool>(renderer), "declared D3D11 renderer required, no software substitution");
    Observer observer{*renderer, 2};
    crucible::presentation::desktop::CheckedPresentation checked{*renderer};
    require(checked.isSupported(), "actual checked native presentation mechanism required");
    require(observer.getCapability() == Observer::Capability::direct3d11, "actual native completion mechanism required");
    const auto present = [&] {
        require(SDL_SetRenderDrawColor(renderer.get(), 11, 19, 32, 255) &&
            SDL_RenderClear(renderer.get()), "native frame drawing");
        const auto receipt = checked.present();
        require(receipt.status == crucible::presentation::desktop::CheckedPresentation::Status::handedOff &&
            receipt.nativeResult && *receipt.nativeResult == 0, "checked S_OK native handoff, never double present");
    };
    present();
    require(observer.recordFrame({1, 1, 7}) == Observer::RecordStatus::recorded, "first native marker");
    present();
    require(observer.recordFrame({1, 2, 8}) == Observer::RecordStatus::recorded &&
        observer.getPendingCount() == 2, "startup marker capacity");
    require(observer.recordFrame({1, 3, 9}) == Observer::RecordStatus::full &&
        observer.recordFrame({1, 2, 8}) == Observer::RecordStatus::invalid &&
        observer.getPendingCount() == 2, "full and duplicate preserve pending markers");
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    Observer::PollResult receipt;
    do {
        receipt = observer.pollOldest();
        require(receipt.status == Observer::PollStatus::pending || receipt.status == Observer::PollStatus::complete,
            "native polling cannot silently replace error with completion");
        require(std::chrono::steady_clock::now() < deadline, "native completion observation deadline");
        if (receipt.status == Observer::PollStatus::pending) std::this_thread::yield();
    } while (receipt.status == Observer::PollStatus::pending);
    require(receipt.frame == Observer::FrameIdentity{1, 1, 7} && observer.getPendingCount() == 1,
        "oldest native receipt retains run/frame/tick correlation");
    require(observer.tryDrain() && observer.getPendingCount() == 0 &&
        observer.pollOldest().status == Observer::PollStatus::empty, "native drain retires all remaining slots");
    present();
    require(observer.recordFrame({2, 1, 0}) == Observer::RecordStatus::recorded && observer.tryDrain(),
        "new run may restart frame and tick identities after old work joins");
    present();
    require(observer.recordFrame({2, 2, 1}) == Observer::RecordStatus::recorded, "marker before synthetic identity replacement");
    const auto properties = SDL_GetRendererProperties(renderer.get());
    auto* device = SDL_GetPointerProperty(properties, SDL_PROP_RENDERER_D3D11_DEVICE_POINTER, nullptr);
    require(device && SDL_SetPointerProperty(properties, SDL_PROP_RENDERER_D3D11_DEVICE_POINTER, nullptr),
        "synthetic public device-property replacement");
    const auto changed = observer.pollOldest();
    const auto changedPresentation = checked.present();
    require(SDL_SetPointerProperty(properties, SDL_PROP_RENDERER_D3D11_DEVICE_POINTER, device), "restore native device property");
    require(changed.status == Observer::PollStatus::deviceChanged && observer.getPendingCount() == 1 &&
        changedPresentation.status == crucible::presentation::desktop::CheckedPresentation::Status::deviceChanged &&
        observer.recordFrame({2, 3, 2}) == Observer::RecordStatus::deviceChanged && observer.tryDrain() &&
        observer.getPendingCount() == 0, "identity change latches refusal and original context drains retained work");
    SDL_Log("Native handoff and completed-command receiving passed; physical scanout was not received");
}
}
int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string_view{argv[1]} == "--native-completion") {
            nativeCompletion();
            return 0;
        }
        require(argc == 1, "expected no arguments or --native-completion");
        Canvas canvas;
        exactView(canvas);
        densityView(canvas);
        unsupportedObserver(canvas);
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s: %s\n", error.what(), SDL_GetError());
        return 1;
    }
}
