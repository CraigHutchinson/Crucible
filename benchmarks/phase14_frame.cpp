#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#if defined(_WIN32)
#include <d3d11.h>
#include <dxgi.h>
#include <psapi.h>
#include <wrl/client.h>
#endif

#include "crucible/contracts/SampleId.hpp"
#include "crucible/fields/FieldSet.hpp"
#include "crucible/presentation/ScenarioSnapshot.hpp"
#include "crucible/presentation/desktop/frame_completion_observer.hpp"
#include "desktop/DesktopApp.hpp"

namespace {
using App = crucible::desktop::DesktopApp;
using Observer = crucible::presentation::desktop::FrameCompletionObserver;
using Presenter = crucible::presentation::desktop::CheckedPresentation;
using Clock = std::chrono::steady_clock;
using Nanoseconds = std::chrono::nanoseconds;
using Session = crucible::runtime::InspectorSession;
using Status = crucible::runtime::ClockDriver::Status;
/// Different real field workloads, with no-command reserved as a native comparator.
enum class Route { none, flow, gather };
/// Startup bounds; acceptance classification additionally checks the actual outcome.
struct Settings {
    std::string source;
    std::string captureDirectory;
    std::size_t population{100000}, frames{1800}, maxFrames{10000}, mutations{300};
    std::size_t workers{1}, partitions{};
    std::uint64_t warmupTicks{120}, seconds{30}, timeoutSeconds{1200};
    Route route{Route::flow};
    bool profileStages{};
};
/// Same bounded outer-boundary observations in both comparison arms; stage clocks are separate.
[[nodiscard]] std::size_t observationCapacity(const Settings& settings) noexcept {
    return 4 * settings.maxFrames + static_cast<std::size_t>(settings.warmupTicks) + 4;
}
/// Copied production camera values; center is observed through its logical viewport midpoint.
struct Camera { double scale{}, x{}, y{}; };
/// Copied resolved startup policy and actual storage bounds; no resident-memory claim.
struct Execution {
    std::size_t workers{}, partitions{}, queryCapacity{}, taskCapacity{}, queueCapacity{};
};
/// Completed sequential FP recomputations, copied before warmup and after measured drain.
struct RowFallbacks { std::uint64_t start{}, end{}; };
/// Cold process counters; peak working set covers the process lifetime through each sample.
struct ProcessMemory {
    bool received{};
    std::uint32_t error{};
    std::size_t workingSet{}, peakWorkingSet{}, privateUsage{};
    std::int64_t callNs{};
};
[[nodiscard]] ProcessMemory readProcessMemory() noexcept {
#if defined(_WIN32)
    const auto before = Clock::now();
    PROCESS_MEMORY_COUNTERS_EX counters{};
    constexpr auto counterBytes = static_cast<DWORD>(sizeof(counters));
    counters.cb = counterBytes;
    const bool received = K32GetProcessMemoryInfo(GetCurrentProcess(),
        reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters), counterBytes) != 0;
    const auto error = received ? 0U : GetLastError();
    return {received, error, received ? counters.WorkingSetSize : 0,
        received ? counters.PeakWorkingSetSize : 0, received ? counters.PrivateUsage : 0,
        std::chrono::duration_cast<Nanoseconds>(Clock::now() - before).count()};
#else
    return {};
#endif
}
void writeProcessMemory(ProcessMemory memory) {
    std::cout << "{\"received\":" << memory.received << ",\"error\":" << memory.error
        << ",\"working_set_bytes\":" << memory.workingSet << ",\"peak_working_set_bytes\":" << memory.peakWorkingSet
        << ",\"private_committed_bytes\":" << memory.privateUsage << ",\"sample_call_ns\":" << memory.callNs << '}';
}
/// Owned production observations; completion timestamps are poll observation bounds.
struct Frame {
    App::FrameStatistics app{};
    crucible::presentation::desktop::ScenePainter::DrawStatistics draw{};
    crucible::runtime::ClockDriver::Summary clock{};
    Status status{};
    std::int64_t begin{}, end{}, completion{}, markerBegin{}, markerEnd{}, handoff{}, pollNs{}, pacingLateNs{};
    std::uint64_t pacingSkipped{};
    std::size_t polls{};
    std::optional<Observer::RecordStatus> marker{};
    int width{}, height{};
    bool visible{};
    bool focused{};
    std::size_t events{};
    Camera cameraBefore{}, cameraAfter{};
    int cameraPhase{-1};
    std::size_t cameraEvents{}, cameraTransitions{}, operatorCameraEvents{};
};
/// One scripted mutation's event, admission, application and completed-frame identity.
struct Input {
    std::uint64_t run{}, sequence{}, tick{}, frame{}, applicationFrame{}, eventFrame{};
    std::int64_t eventBegin{}, admitted{}, applied{}, completed{};
    Status status{};
    bool accepted{}, visible{};
};
/// Cold comparison at equal simulation age, outside all measured frame intervals.
struct Quality {
    std::size_t influenced{}, cohort{}, displaced{};
    double signedMean{}, signedMaximum{};
    std::int64_t cpuNs{};
    std::int64_t captureCpuNs{};
    bool compared{};
};
/// Copied actual native adapter identity; host inventory is not a renderer receipt.
struct Device {
    std::uint32_t vendor{}, model{}, luidLow{};
    std::int32_t luidHigh{};
    bool received{};
    std::optional<std::int64_t> driverVersion{};
    std::string name;
};
[[nodiscard]] Device readDevice(SDL_Renderer& renderer) {
#if defined(_WIN32)
    const auto properties = SDL_GetRendererProperties(&renderer);
    auto* device = static_cast<ID3D11Device*>(SDL_GetPointerProperty(properties,
        SDL_PROP_RENDERER_D3D11_DEVICE_POINTER, nullptr)); // non-owning; app outlives this cold call
    if (!device) return {};
    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
    DXGI_ADAPTER_DESC description{};
    if (FAILED(device->QueryInterface(IID_PPV_ARGS(dxgiDevice.GetAddressOf()))) ||
        FAILED(dxgiDevice->GetAdapter(adapter.GetAddressOf())) || FAILED(adapter->GetDesc(&description))) return {};
    Device identity{description.VendorId, description.DeviceId, description.AdapterLuid.LowPart,
        description.AdapterLuid.HighPart, true, std::nullopt, {}};
    std::array<char, 512> name{};
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, description.Description, -1,
        name.data(), static_cast<int>(name.size()), nullptr, nullptr) > 0) identity.name = name.data();
    LARGE_INTEGER driver{};
    if (SUCCEEDED(adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &driver))) identity.driverVersion = driver.QuadPart;
    return identity;
#else
    static_cast<void>(renderer);
    return {};
#endif
}
/// SDL closes only after app and native observers have released their borrows.
struct Video {
    Video() { if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) throw std::runtime_error(SDL_GetError()); }
    ~Video() { SDL_Quit(); }
    Video(const Video&) = delete;
    Video& operator=(const Video&) = delete;
};

[[nodiscard]] std::uint64_t parseInteger(std::string_view value) {
    std::uint64_t result{};
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size())
        throw std::invalid_argument("Expected an unsigned integer");
    return result;
}
[[nodiscard]] Settings parseSettings(int argc, char** argv) {
    Settings settings;
    bool hasWorkers{}, hasPartitions{};
    for (int i = 1; i < argc; ++i) {
        const std::string_view option{argv[i]};
        if (++i == argc) throw std::invalid_argument("Option requires a value");
        const std::string_view value{argv[i]};
        if (option == "--source") settings.source = value;
        else if (option == "--capture-dir") settings.captureDirectory = value;
        else if (option == "--population") settings.population = static_cast<std::size_t>(parseInteger(value));
        else if (option == "--frames") settings.frames = static_cast<std::size_t>(parseInteger(value));
        else if (option == "--max-frames") settings.maxFrames = static_cast<std::size_t>(parseInteger(value));
        else if (option == "--mutations") settings.mutations = static_cast<std::size_t>(parseInteger(value));
        else if (option == "--workers" || option == "--partitions") {
            auto& seen = option == "--workers" ? hasWorkers : hasPartitions;
            const auto count = parseInteger(value);
            if (seen || count > (option == "--workers" ? 32U : 128U) || (option == "--workers" && count == 0))
                throw std::invalid_argument("Worker/partition options require one bounded unsigned integer each");
            seen = true;
            auto& selected = option == "--workers" ? settings.workers : settings.partitions;
            selected = static_cast<std::size_t>(count);
        }
        else if (option == "--warmup-ticks") settings.warmupTicks = parseInteger(value);
        else if (option == "--seconds") settings.seconds = parseInteger(value);
        else if (option == "--timeout-seconds") settings.timeoutSeconds = parseInteger(value);
        else if (option == "--profile-stages") {
            if (value != "true" && value != "false") throw std::invalid_argument("Profile stages requires true or false");
            settings.profileStages = value == "true";
        }
        else if (option == "--route") {
            if (value == "none") settings.route = Route::none;
            else if (value == "flow") settings.route = Route::flow;
            else if (value == "gather") settings.route = Route::gather;
            else throw std::invalid_argument("Route must be none, flow, or gather");
        } else throw std::invalid_argument("Unknown option");
    }
    if (settings.source.size() != 40 || !std::ranges::all_of(settings.source,
        [](char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); }))
        throw std::invalid_argument("--source requires the exact lowercase 40-digit source commit");
    if (settings.source != CRUCIBLE_PHASE14_BUILD_SOURCE)
        throw std::invalid_argument("Source argument differs from configured capture binary; configure/build the exact checkpoint");
    if ((settings.population != 100000 && settings.population != 150000) || !settings.frames ||
        settings.maxFrames < settings.frames || settings.maxFrames > 100000 ||
        settings.mutations > 1000 || settings.warmupTicks > 10000 ||
        !settings.workers || settings.workers > 32 || settings.partitions > 128 ||
        !settings.timeoutSeconds || settings.timeoutSeconds > 3600 || settings.seconds >= settings.timeoutSeconds)
        throw std::invalid_argument("Invalid bounded scale capture settings");
    return settings;
}
[[nodiscard]] std::int64_t elapsedNs(Clock::time_point origin) noexcept {
    return std::chrono::duration_cast<Nanoseconds>(Clock::now() - origin).count();
}
/// Cold JSON string encoding, including control characters in a native error receipt.
[[nodiscard]] std::string jsonString(std::string_view value) {
    std::string encoded{"\""};
    for (const unsigned char character : value) {
        if (character == '"' || character == '\\') { encoded += '\\'; encoded += static_cast<char>(character); }
        else if (character < 32) {
            constexpr std::string_view digits{"0123456789abcdef"};
            encoded += "\\u00";
            encoded += digits[character / 16];
            encoded += digits[character % 16];
        } else encoded += static_cast<char>(character);
    }
    encoded += '"';
    return encoded;
}
void checkEvent(App& app, const SDL_Event& event) {
    if (app.HandleEvent(event) != SDL_APP_CONTINUE) throw std::runtime_error("Desktop capture was closed");
}
void pressKey(App& app, SDL_Keycode key) {
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = key;
    checkEvent(app, event);
}
[[nodiscard]] Camera readCamera(App& app) {
    const auto viewport = app.GetCamera().GetViewport();
    const auto center = app.GetCamera().TryToWorld({viewport.x + viewport.width / 2,
        viewport.y + viewport.height / 2});
    if (!center) throw std::runtime_error("Camera viewport midpoint is outside the world");
    return {app.GetCamera().GetScale(), center->x, center->y};
}
[[nodiscard]] bool sameCamera(Camera first, Camera second) noexcept {
    return std::abs(first.scale - second.scale) <= 1e-8 &&
        std::abs(first.x - second.x) <= 1e-4 && std::abs(first.y - second.y) <= 1e-4;
}
[[nodiscard]] SDL_FPoint cameraWindowPoint(App& app,
    crucible::presentation::ScreenPoint logical) {
    float x{}, y{};
    if (!SDL_RenderCoordinatesToWindow(&app.getRenderer(), static_cast<float>(logical.x),
        static_cast<float>(logical.y), &x, &y)) throw std::runtime_error(SDL_GetError());
    return {x, y};
}
/// The frozen six-frame camera workload uses the same production event adapter as field gestures.
void dispatchCamera(App& app, Route route, std::size_t index, Frame& frame) {
    frame.cameraBefore = readCamera(app);
    if (route == Route::none) { frame.cameraAfter = frame.cameraBefore; return; }
    frame.cameraPhase = static_cast<int>(index % 6);
    const auto viewport = app.GetCamera().GetViewport();
    const crucible::presentation::ScreenPoint midpoint{viewport.x + viewport.width / 2,
        viewport.y + viewport.height / 2};
    if (frame.cameraPhase == 0 || frame.cameraPhase == 5) {
        pressKey(app, SDLK_F);
        frame.cameraEvents = 1;
    } else if (frame.cameraPhase == 1 || frame.cameraPhase == 4) {
        const auto anchor = cameraWindowPoint(app, midpoint);
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_WHEEL;
        event.wheel.y = frame.cameraPhase == 1 ? 5.0F : -5.0F;
        event.wheel.mouse_x = static_cast<float>(anchor.x);
        event.wheel.mouse_y = static_cast<float>(anchor.y);
        checkEvent(app, event);
        frame.cameraEvents = 1;
    } else {
        const double direction = frame.cameraPhase == 2 ? 1 : -1;
        const auto first = cameraWindowPoint(app, midpoint);
        const auto last = cameraWindowPoint(app, {midpoint.x + direction * 24, midpoint.y + direction * 12});
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        event.button.button = SDL_BUTTON_MIDDLE;
        event.button.x = static_cast<float>(first.x);
        event.button.y = static_cast<float>(first.y);
        checkEvent(app, event);
        event = {};
        event.type = SDL_EVENT_MOUSE_MOTION;
        event.motion.x = static_cast<float>(last.x);
        event.motion.y = static_cast<float>(last.y);
        checkEvent(app, event);
        event = {};
        event.type = SDL_EVENT_MOUSE_BUTTON_UP;
        event.button.button = SDL_BUTTON_MIDDLE;
        event.button.x = static_cast<float>(last.x);
        event.button.y = static_cast<float>(last.y);
        checkEvent(app, event);
        frame.cameraEvents = 3;
    }
    frame.cameraAfter = readCamera(app);
    frame.cameraTransitions = sameCamera(frame.cameraBefore, frame.cameraAfter) ? 0 : 1;
}
[[nodiscard]] bool isOperatorCameraEvent(const SDL_Event& event) noexcept {
    return event.type == SDL_EVENT_MOUSE_WHEEL ||
        ((event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP) &&
            event.button.button == SDL_BUTTON_MIDDLE) ||
        (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F);
}
void clickWorld(App& app, crucible::Position world, Uint32 type) {
    const auto logical = app.GetCamera().TryToScreen(world);
    if (!logical) throw std::runtime_error("Script world projection failed");
    float x{}, y{};
    if (!SDL_RenderCoordinatesToWindow(&app.getRenderer(), static_cast<float>(logical->x),
        static_cast<float>(logical->y), &x, &y)) throw std::runtime_error(SDL_GetError());
    SDL_Event event{};
    event.type = type;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = x;
    event.button.y = y;
    checkEvent(app, event);
}
/// Mutates through the production keyboard/coordinate/gesture adapter, never ingress directly.
void dispatchMutation(App& app, Route route, std::size_t index, std::size_t& currentSlot) {
    const auto grid = app.GetCamera().GetConfig();
    const float width = static_cast<float>(grid.columns) * grid.cell_size;
    const float height = static_cast<float>(grid.rows) * grid.cell_size;
    const auto kind = route == Route::gather ? index % 3 : 0;
    while (currentSlot != kind) { pressKey(app, SDLK_TAB); currentSlot = (currentSlot + 1) % 4; }
    if (kind == 0) {
        pressKey(app, SDLK_4);
        const float y = (index % 2 == 0 ? 0.49F : 0.51F) * height;
        clickWorld(app, {0.02F * width, y}, SDL_EVENT_MOUSE_BUTTON_DOWN);
        clickWorld(app, {0.98F * width, y}, SDL_EVENT_MOUSE_BUTTON_UP);
    } else {
        pressKey(app, kind == 1 ? SDLK_2 : SDLK_1);
        clickWorld(app, {(kind == 1 ? 0.25F : 0.85F) * width, 0.5F * height}, SDL_EVENT_MOUSE_BUTTON_DOWN);
        clickWorld(app, {(kind == 1 ? 0.25F : 0.85F) * width, 0.5F * height}, SDL_EVENT_MOUSE_BUTTON_UP);
    }
}
[[nodiscard]] bool isVisible(App& app) noexcept {
    const auto flags = SDL_GetWindowFlags(&app.GetWindow());
    return !(flags & (SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED));
}
/// Uses the real FieldSet on initial authoritative samples; reference shares the exact scenario/tick.
[[nodiscard]] Quality compareQuality(const App::StartupSettings& settings,
    const std::vector<crucible::SampleState>& finalSamples, std::uint64_t finalTick,
    std::span<const crucible::runtime::AppliedCommand> trace) {
    Quality quality;
    const auto begin = Clock::now();
    Session reference{settings.scenario};
    std::vector<std::uint8_t> cohort(finalSamples.size());
    crucible::fields::FieldSet fields{settings.scenario.fieldCapacity};
    std::uint64_t referenceTick{};
    const auto advanceReference = [&] {
        const auto pump = reference.TryPump(Nanoseconds{16'666'667});
        if (pump.status != Status::running || pump.advanced_ticks != 1 || pump.summary.completed_tick != ++referenceTick)
            throw std::runtime_error("No-command reference failed to advance exactly one tick");
    };
    if (!trace.empty()) {
        while (referenceTick + 1 < trace.front().tick) advanceReference();
        if (fields.TryApplyEdit(trace.front().command.field) != crucible::fields::EditResult::applied)
            throw std::runtime_error("Quality route field could not be received");
        const auto initial = reference.GetSnapshot().GetSamples();
        for (std::size_t i = 0; i < initial.size(); ++i) {
            const auto force = fields.Sample(initial[i].position);
            if (force.x != 0 || force.y != 0) { cohort[i] = 1; ++quality.influenced; }
        }
    }
    while (referenceTick < finalTick) advanceReference();
    const auto samples = reference.GetSnapshot().GetSamples();
    if (samples.size() != finalSamples.size()) throw std::runtime_error("Reference population differs");
    const double threshold = 0.05 * settings.scenario.grid.columns * settings.scenario.grid.cell_size;
    for (std::size_t i = 0; i < samples.size(); ++i) {
        if (samples[i].id != finalSamples[i].id) throw std::runtime_error("Reference sample identity differs");
        if (!cohort[i]) continue;
        const double displacement = static_cast<double>(finalSamples[i].position.x) - samples[i].position.x;
        quality.signedMean += displacement;
        quality.signedMaximum = std::max(quality.signedMaximum, displacement);
        ++quality.cohort;
        if (displacement >= threshold) ++quality.displaced;
    }
    if (quality.cohort) quality.signedMean /= static_cast<double>(quality.cohort);
    quality.compared = true;
    quality.cpuNs = std::chrono::duration_cast<Nanoseconds>(Clock::now() - begin).count();
    return quality;
}

/// Writes only after capture/drain; all durations are integer nanoseconds from one steady origin.
void writeCapture(const Settings& settings, std::span<const Frame> frames, std::span<const Input> inputs,
    std::span<const crucible::runtime::AppliedCommand> trace, const Quality& quality,
    std::string_view renderer, std::string_view failure, bool drained, bool boundedOut,
    std::int64_t duration, std::int64_t warmupDuration, std::uint64_t warmupTicks,
    float refreshRate, float displayScale, float pixelDensity, std::uint64_t initialDiscarded, const Device& device,
    std::span<const Session::TickObservation> observations, std::size_t initialObservationCursor,
    std::uint64_t droppedObservations, crucible::presentation::ScreenRect cameraViewport, Execution execution,
    ProcessMemory startupMemory, ProcessMemory endMemory, RowFallbacks rowFallbacks) {
    const auto accepted = std::ranges::count_if(inputs, [](const Input& input) {
        return input.accepted && input.visible && input.status == Status::running && input.completed > 0;
    });
    const auto complete = std::ranges::count_if(frames, [](const Frame& frame) {
        return frame.completion > 0 && frame.visible && frame.status == Status::running &&
            frame.app.nativeReceipt && frame.app.nativeReceipt->status == Presenter::Status::handedOff;
    });
    const auto advancing = std::ranges::count_if(frames, [](const Frame& frame) { return frame.app.advancedTicks > 0; });
    const bool sampleMinimum = advancing >= 1800 && duration >= 30'000'000'000LL && warmupTicks >= 120;
    const bool fallbackReceipt = rowFallbacks.end >= rowFallbacks.start &&
        (execution.workers > 1 ? rowFallbacks.end == rowFallbacks.start : !rowFallbacks.start && !rowFallbacks.end);
    const bool qualifying = fallbackReceipt && startupMemory.received && endMemory.received && !settings.profileStages && !droppedObservations && failure.empty() && !boundedOut && drained && device.received && device.driverVersion && sampleMinimum &&
        complete == static_cast<std::ptrdiff_t>(frames.size()) &&
        (settings.route == Route::none || accepted >= 300);
    std::cout << std::setprecision(17);
    std::cout << "{\"type\":\"capture\",\"schema\":3,\"source\":" << jsonString(settings.source)
        << ",\"source_tree\":" << jsonString(CRUCIBLE_PHASE14_BUILD_TREE)
        << ",\"population\":" << settings.population << ",\"route\":" << static_cast<int>(settings.route)
        << ",\"renderer\":" << jsonString(renderer)
        << ",\"requested_frames\":" << settings.frames << ",\"frame_capacity\":" << settings.maxFrames
        << ",\"requested_mutations\":" << settings.mutations << ",\"observer_capacity\":8"
        << ",\"completion_mode\":\"joined-yield-zero\",\"dropped_frame_rows\":0,\"dropped_input_rows\":0"
        << ",\"pacing_mode\":\"fixed-60hz-skip-whole-overdue-periods\",\"display_refresh_hz\":" << refreshRate
        << ",\"window_display_scale\":" << displayScale << ",\"window_pixel_density\":" << pixelDensity
        << ",\"device_identity_received\":" << device.received << ",\"adapter_vendor_id\":" << device.vendor
        << ",\"adapter_device_id\":" << device.model << ",\"adapter_luid_low\":" << device.luidLow
        << ",\"adapter_luid_high\":" << device.luidHigh
        << ",\"adapter_name\":" << jsonString(device.name)
        << ",\"initial_tick\":" << warmupTicks
        << ",\"tick_observation_capacity\":" << observationCapacity(settings)
        << ",\"tick_observation_count\":" << observations.size()
        << ",\"initial_observation_cursor\":" << initialObservationCursor
        << ",\"dropped_tick_observations\":" << droppedObservations
        << ",\"profile_stages\":" << settings.profileStages
        << ",\"execution_schema\":1,\"requested_workers\":" << settings.workers
        << ",\"requested_partitions\":" << settings.partitions
        << ",\"resolved_workers\":" << execution.workers << ",\"resolved_partitions\":" << execution.partitions
        << ",\"row_execution_backend\":" << jsonString(execution.taskCapacity ? "row-partitions" : "pr29-sequential-control")
        << ",\"partition_query_scratch_capacity\":" << execution.queryCapacity
        << ",\"query_scratch_element_bytes\":" << sizeof(crucible::SampleId)
        << ",\"partition_query_scratch_bytes\":" << execution.queryCapacity * sizeof(crucible::SampleId)
        << ",\"row_task_capacity\":" << execution.taskCapacity << ",\"row_queue_capacity\":" << execution.queueCapacity
        << ",\"row_fallbacks_start\":" << rowFallbacks.start << ",\"row_fallbacks_end\":" << rowFallbacks.end
        << ",\"columns\":" << (settings.population == 100000 ? 400 : 500)
        << ",\"rows\":" << (settings.population == 100000 ? 250 : 300)
        << ",\"cell_size\":1,\"tool_radius\":16,\"tool_magnitude\":4,\"sync_interval\":1,\"present_flags\":0"
        << ",\"view_policy\":\"density-overview\",\"mission\":false"
        << ",\"camera_script\":\"overview-detail-pan-v1\",\"camera_mode\":"
        << jsonString(settings.route == Route::none ? "fixed-overview" : "six-frame-cycle")
        << ",\"camera_viewport_width\":" << cameraViewport.width
        << ",\"camera_viewport_height\":" << cameraViewport.height
        << ",\"duration_ns\":" << duration << ",\"warmup_duration_ns\":" << warmupDuration
        << ",\"internal_timeout_seconds\":" << settings.timeoutSeconds
        << ",\"initial_discarded_scaled_ns\":" << initialDiscarded
        << ",\"warmup_ticks\":" << warmupTicks << ",\"drained\":" << drained
        << ",\"bounded_out\":" << boundedOut << ",\"sample_minimum\":" << sampleMinimum
        << ",\"qualifying_capture\":" << qualifying << ",\"accepted_running_visible_completed_inputs\":" << accepted
        << ",\"completed_handoff_frames\":" << complete << ",\"failure\":" << jsonString(failure)
        << ",\"driver_version\":";
    if (device.driverVersion) std::cout << *device.driverVersion;
    else std::cout << "null";
    std::cout << ",\"gpu_semantics\":\"event-query completion observed after checked native handoff; no scanout\""
        << ",\"memory_schema\":1,\"memory_api\":\"K32GetProcessMemoryInfo\""
        << ",\"memory_peak_scope\":\"process-lifetime-through-sample\""
        << ",\"memory_sample_scope\":\"after-native-setup-and-after-measured-drain-before-cold-quality\",\"memory_startup\":";
    writeProcessMemory(startupMemory);
    std::cout << ",\"memory_end\":";
    writeProcessMemory(endMemory);
    std::cout << "}\n";
    for (const auto& frame : frames) {
        std::cout << "{\"type\":\"frame\",\"run\":" << frame.app.runId << ",\"frame\":" << frame.app.frameId
            << ",\"tick\":" << frame.clock.completed_tick << ",\"draw_tick\":" << frame.app.completedTick
            << ",\"advanced\":" << frame.app.advancedTicks
            << ",\"begin_ns\":" << frame.begin << ",\"end_ns\":" << frame.end
            << ",\"app_service_ns\":" << frame.app.service.count() << ",\"pump_ns\":" << frame.app.pump.count()
            << ",\"draw_ns\":" << frame.app.draw.count() << ",\"present_ns\":" << frame.app.present.count()
            << ",\"completion_observed_ns\":" << frame.completion
            << ",\"handoff_observed_ns\":" << frame.handoff << ",\"poll_call_ns\":" << frame.pollNs
            << ",\"poll_calls\":" << frame.polls
            << ",\"pacing_late_ns\":" << frame.pacingLateNs << ",\"pacing_skipped\":" << frame.pacingSkipped
            << ",\"marker_begin_ns\":" << frame.markerBegin << ",\"marker_end_ns\":" << frame.markerEnd
            << ",\"marker_status\":" << (frame.marker ? static_cast<int>(*frame.marker) : -1)
            << ",\"native_status\":" << (frame.app.nativeReceipt ? static_cast<int>(frame.app.nativeReceipt->status) : -1)
            << ",\"native_result\":";
        if (frame.app.nativeReceipt && frame.app.nativeReceipt->nativeResult) std::cout << *frame.app.nativeReceipt->nativeResult;
        else std::cout << "null";
        std::cout << ",\"presented\":" << frame.app.presented << ",\"status\":" << static_cast<int>(frame.status)
            << ",\"visible\":" << frame.visible << ",\"width\":" << frame.width << ",\"height\":" << frame.height
            << ",\"focused\":" << frame.focused << ",\"native_events\":" << frame.events
            << ",\"camera_phase\":" << frame.cameraPhase << ",\"camera_events\":" << frame.cameraEvents
            << ",\"camera_transitions\":" << frame.cameraTransitions
            << ",\"operator_camera_events\":" << frame.operatorCameraEvents
            << ",\"camera_scale_before\":" << frame.cameraBefore.scale
            << ",\"camera_center_x_before\":" << frame.cameraBefore.x
            << ",\"camera_center_y_before\":" << frame.cameraBefore.y
            << ",\"camera_scale\":" << frame.cameraAfter.scale
            << ",\"camera_center_x\":" << frame.cameraAfter.x << ",\"camera_center_y\":" << frame.cameraAfter.y
            << ",\"authoritative_mobile\":" << frame.draw.authoritativeMobile
            << ",\"individual\":" << frame.draw.individualSamples << ",\"aggregated\":" << frame.draw.aggregatedSamples
            << ",\"hidden\":" << frame.draw.hiddenSamples << ",\"marks\":" << frame.draw.aggregateMarks
            << ",\"cells\":" << frame.draw.visibleCells << ",\"applied_commands\":" << frame.clock.applied_commands
            << ",\"accepted\":" << frame.clock.ingress_accepted << ",\"rejected\":" << frame.clock.ingress_rejected
            << ",\"pending\":" << frame.clock.ingress_pending
            << ",\"discarded_scaled_ns\":" << frame.clock.discarded_scaled_nanoseconds << "}\n";
    }
    std::size_t observationFrame{};
    for (const auto& observation : observations.subspan(std::min(initialObservationCursor, observations.size()))) {
        while (observationFrame < frames.size() && frames[observationFrame].clock.completed_tick < observation.completedTick)
            ++observationFrame;
        const auto frameId = observationFrame < frames.size() ? frames[observationFrame].app.frameId : 0;
        std::cout << "{\"type\":\"tick\",\"run\":" << observation.runId << ",\"frame\":" << frameId
            << ",\"tick\":" << observation.completedTick << ",\"boundary_ns\":" << observation.boundary.count()
            << ",\"applied_commands\":" << observation.appliedCommands << ",\"simulation\":";
        if (observation.simulation) {
            const auto& stage = *observation.simulation;
            std::cout << "{\"tick\":" << stage.completedTick << ",\"gather_ns\":" << stage.gather.count()
                << ",\"index_ns\":" << stage.index.count() << ",\"propose_ns\":" << stage.propose.count()
                << ",\"commit_ns\":" << stage.commit.count() << ",\"resources_ns\":" << stage.resources.count()
                << ",\"rebuild_ns\":" << stage.rebuild.count() << ",\"input_rows\":" << stage.inputRows
                << ",\"query_rows\":" << stage.queryRows << ",\"occupied_cells\":" << stage.occupiedCells
                << ",\"query_scratch_capacity\":" << stage.queryScratchCapacity << ",\"workers\":" << stage.workers
                << ",\"partitions\":" << stage.partitions << ",\"task_capacity\":" << stage.taskCapacity << '}';
        } else std::cout << "null";
        std::cout << "}\n";
    }
    for (const auto& input : inputs)
        std::cout << "{\"type\":\"input\",\"run\":" << input.run << ",\"sequence\":" << input.sequence
            << ",\"tick\":" << input.tick << ",\"frame\":" << input.frame
            << ",\"application_frame\":" << input.applicationFrame
            << ",\"event_frame\":" << input.eventFrame
            << ",\"event_begin_ns\":" << input.eventBegin << ",\"admitted_ns\":" << input.admitted
            << ",\"applied_observed_ns\":" << input.applied << ",\"completed_observed_ns\":" << input.completed
            << ",\"accepted\":" << input.accepted << ",\"visible\":" << input.visible
            << ",\"status\":" << static_cast<int>(input.status) << "}\n";
    for (const auto& command : trace) {
        const auto& edit = command.command.field;
        std::cout << "{\"type\":\"trace\",\"sequence\":" << command.sequence << ",\"tick\":" << command.tick
            << ",\"action\":" << static_cast<int>(command.command.action) << ",\"result\":" << static_cast<int>(command.result)
            << ",\"kind\":" << static_cast<int>(edit.kind) << ",\"slot\":" << edit.slot
            << ",\"x\":" << edit.center.x << ",\"y\":" << edit.center.y << ",\"radius\":" << edit.radius
            << ",\"strength\":" << edit.strength << ",\"end_x\":" << edit.end.x << ",\"end_y\":" << edit.end.y << "}\n";
    }
    std::cout << "{\"type\":\"quality\",\"compared\":" << quality.compared << ",\"cpu_ns\":" << quality.cpuNs
        << ",\"capture_cpu_ns\":" << quality.captureCpuNs << ",\"capture_retained_tick\":" << (frames.empty() ? 0 : frames.back().clock.completed_tick)
        << ",\"initial_nonzero_field_samples\":" << quality.influenced << ",\"initial_corridor_cohort\":" << quality.cohort
        << ",\"cohort_displaced_at_least_5pct_width\":" << quality.displaced << ",\"signed_mean_dx\":" << quality.signedMean
        << ",\"signed_max_dx\":" << quality.signedMaximum << "}\n";
}

int capture(const Settings& settings) {
    std::vector<Frame> frames(settings.maxFrames);
    std::vector<Input> inputs(settings.mutations);
    std::vector<crucible::SampleState> finalSamples(settings.population);
    std::vector<crucible::runtime::AppliedCommand> finalTrace(4096);
    std::vector<std::size_t> sequenceToInput(4097, settings.mutations);
    std::vector<Session::TickObservation> observations(observationCapacity(settings));
    std::size_t frameCount{}, inputCount{}, traceCount{}, currentSlot{}, advancingFrames{};
    std::size_t observationCount{}, initialObservationCursor{};
    crucible::presentation::ScreenRect cameraViewport{};
    Execution execution{};
    RowFallbacks rowFallbacks{};
    ProcessMemory startupMemory{}, endMemory{};
    std::uint64_t droppedObservations{};
    std::uint64_t firstFrame{}, captureRun{}, completedTick{}, actualWarmupTicks{};
    std::uint64_t initialDiscarded{};
    std::int64_t duration{}, warmupDuration{};
    bool drained{}, boundedOut{};
    float refreshRate{}, displayScale{}, pixelDensity{};
    Device device;
    std::string failure, renderer;
    Quality quality;
    App::StartupSettings startup;
    startup.scenario.population = settings.population;
    startup.scenario.grid = settings.population == 100000 ? crucible::GridConfig{400,250,1} : crucible::GridConfig{500,300,1};
    startup.mission.reset();
    startup.toolRadius = 16;
    startup.toolMagnitude = 4;
    startup.viewPolicy = crucible::presentation::desktop::ScenePainter::ViewPolicy::densityOverview;
    startup.presentationMode = App::PresentationMode::checkedD3D11;
    startup.observations = {observationCapacity(settings), settings.profileStages};
    startup.rowExecution = {settings.workers, settings.partitions};
    const Video video;
    {
        App app{startup};
        cameraViewport = app.GetCamera().GetViewport();
        const auto resolved = app.GetSession().getRowExecution();
        const auto storage = app.GetSession().getRowExecutionStorage();
        execution = {resolved.workers, resolved.partitions, storage.queryScratchCapacity, storage.taskCapacity, storage.queueCapacity};
        if (execution.workers != settings.workers || !execution.partitions ||
            (settings.partitions && execution.partitions != settings.partitions))
            throw std::runtime_error("Runtime did not receive requested row execution settings");
        if (!execution.taskCapacity && (execution.workers != 1 || execution.partitions != 1 ||
            execution.queryCapacity || execution.queueCapacity))
            throw std::runtime_error("Sequential control cannot consume parallel workers or partitions");
        Observer observer{app.getRenderer(), 8};
        const auto* name = SDL_GetRendererName(&app.getRenderer());
        renderer = name ? name : "unknown";
        device = readDevice(app.getRenderer());
        if (const auto* mode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(&app.GetWindow()))) refreshRate = mode->refresh_rate;
        displayScale = SDL_GetWindowDisplayScale(&app.GetWindow());
        pixelDensity = SDL_GetWindowPixelDensity(&app.GetWindow());
        startupMemory = readProcessMemory();
        rowFallbacks.start = app.GetSession().getRowFallbackCount();
        if (observer.getCapability() != Observer::Capability::direct3d11)
            throw std::runtime_error("Concrete GPU completion observer is unsupported");
        const auto warmupBegin = Clock::now();
        std::optional<Clock::time_point> measuredOrigin;
        auto processEvents = [&] {
            std::size_t count{};
            SDL_Event event{};
            while (SDL_PollEvent(&event)) {
                ++count;
                if (isOperatorCameraEvent(event)) {
                    if (measuredOrigin && frameCount < frames.size()) ++frames[frameCount].operatorCameraEvents;
                    else throw std::runtime_error("Unexpected operator camera event during warmup");
                }
                checkEvent(app, event);
            }
            return count;
        };
        try {
            do {
                if (elapsedNs(warmupBegin) >= static_cast<std::int64_t>(settings.timeoutSeconds) * 1'000'000'000)
                    throw std::runtime_error("Warmup timed out");
                static_cast<void>(processEvents());
                if (app.GetSession().GetSummary().ingress_accepted || !app.GetSession().GetTrace().empty())
                    throw std::runtime_error("Unexpected operator field mutation during warmup");
                if (app.Iterate() != SDL_APP_CONTINUE) throw std::runtime_error("Warmup stopped");
            } while (app.GetSession().GetSummary().completed_tick < settings.warmupTicks);
            actualWarmupTicks = app.GetSession().GetSummary().completed_tick;
            initialObservationCursor = app.GetSession().getTickObservations().size();
            const auto warmupFrame = app.getFrameStatistics();
            if (!warmupFrame.presented || observer.recordFrame({warmupFrame.runId, warmupFrame.frameId,
                warmupFrame.completedTick}) != Observer::RecordStatus::recorded || !observer.tryDrain())
                throw std::runtime_error("Warmup handoff/completion did not drain");
            warmupDuration = elapsedNs(warmupBegin);
            captureRun = app.GetSession().getRunId();
            initialDiscarded = app.GetSession().GetSummary().discarded_scaled_nanoseconds;
            std::size_t consumedTrace{};
            const auto origin = Clock::now();
            measuredOrigin = origin;
            auto expectedCamera = readCamera(app);
            auto deadline = origin;
            constexpr Nanoseconds period{16'666'667};
            std::uint64_t scriptAccepted{};
            auto poll = [&] {
                for (;;) {
                    const auto pollBegin = Clock::now();
                    const auto result = observer.pollOldest();
                    const auto pollEnd = Clock::now();
                    if (result.frame.frameId >= firstFrame && result.frame.frameId - firstFrame < frameCount) {
                        auto& observed = frames[static_cast<std::size_t>(result.frame.frameId - firstFrame)];
                        observed.pollNs += std::chrono::duration_cast<Nanoseconds>(pollEnd - pollBegin).count();
                        ++observed.polls;
                    }
                    if (result.status == Observer::PollStatus::empty || result.status == Observer::PollStatus::pending) return;
                    if (result.status != Observer::PollStatus::complete) throw std::runtime_error("Native completion observation failed");
                    if (result.frame.runId != captureRun || result.frame.frameId < firstFrame ||
                        result.frame.frameId - firstFrame >= frameCount) throw std::runtime_error("Completion identity is stale");
                    auto& frame = frames[static_cast<std::size_t>(result.frame.frameId - firstFrame)];
                    if (frame.app.completedTick != result.frame.tick || frame.completion)
                        throw std::runtime_error("Completion frame/tick correlation failed");
                    frame.completion = elapsedNs(origin);
                    for (std::size_t i = 0; i < inputCount; ++i)
                        if (inputs[i].frame == result.frame.frameId && inputs[i].run == result.frame.runId)
                            inputs[i].completed = frame.completion;
                }
            };
            for (;;) {
                duration = elapsedNs(origin);
                if (advancingFrames >= settings.frames && duration >= static_cast<std::int64_t>(settings.seconds) * 1'000'000'000) break;
                if (frameCount == settings.maxFrames || duration >= static_cast<std::int64_t>(settings.timeoutSeconds) * 1'000'000'000) {
                    boundedOut = true; break;
                }
                auto& row = frames[frameCount];
                const auto wait = std::chrono::duration_cast<Nanoseconds>(deadline - Clock::now());
                if (wait.count() > 0) SDL_DelayPrecise(static_cast<Uint64>(wait.count()));
                row.begin = elapsedNs(origin);
                row.pacingLateNs = std::max<std::int64_t>(0, row.begin - std::chrono::duration_cast<Nanoseconds>(deadline - origin).count());
                if (app.GetSession().GetSummary().ingress_accepted != scriptAccepted)
                    throw std::runtime_error("Unexpected operator field mutation before event service");
                row.events = processEvents();
                if (row.operatorCameraEvents || !sameCamera(readCamera(app), expectedCamera))
                    throw std::runtime_error("Unexpected operator camera event or state change in measured workload");
                if (app.GetSession().GetSummary().ingress_accepted != scriptAccepted)
                    throw std::runtime_error("Unexpected operator field mutation during event service");
                poll();
                if (app.GetSession().getRunId() != captureRun) throw std::runtime_error("Restart is a separate capture cohort");
                dispatchCamera(app, settings.route, frameCount, row);
                expectedCamera = row.cameraAfter;
                if (settings.route != Route::none && inputCount < settings.mutations && frameCount % 6 == 0) {
                    auto& input = inputs[inputCount];
                    const auto before = app.GetSession().GetSummary();
                    input.run = captureRun;
                    input.eventFrame = app.getFrameStatistics().frameId + 1;
                    input.status = app.GetSession().GetStatus();
                    input.visible = isVisible(app);
                    input.eventBegin = elapsedNs(origin);
                    dispatchMutation(app, settings.route, inputCount, currentSlot);
                    input.admitted = elapsedNs(origin);
                    const auto after = app.GetSession().GetSummary();
                    input.accepted = after.ingress_accepted == before.ingress_accepted + 1;
                    if (input.accepted) {
                        ++scriptAccepted;
                        input.sequence = after.ingress_accepted;
                        if (input.sequence >= sequenceToInput.size()) throw std::runtime_error("Input mapping capacity exceeded");
                        sequenceToInput[static_cast<std::size_t>(input.sequence)] = inputCount;
                    }
                    ++inputCount;
                }
                if (app.Iterate() != SDL_APP_CONTINUE) throw std::runtime_error("Capture iteration stopped");
                row.handoff = elapsedNs(origin);
                row.app = app.getFrameStatistics();
                if (row.app.advancedTicks > 0) ++advancingFrames;
                row.draw = app.getDrawStatistics();
                row.clock = app.GetSession().GetSummary();
                row.status = app.GetSession().GetStatus();
                row.visible = isVisible(app);
                row.focused = (SDL_GetWindowFlags(&app.GetWindow()) & SDL_WINDOW_INPUT_FOCUS) != 0;
                if (!SDL_GetRenderOutputSize(&app.getRenderer(), &row.width, &row.height)) throw std::runtime_error(SDL_GetError());
                if (!frameCount) firstFrame = row.app.frameId;
                ++frameCount;
                const auto trace = app.GetSession().GetTrace();
                for (; consumedTrace < trace.size(); ++consumedTrace) {
                    const auto& command = trace[consumedTrace];
                    if (command.sequence >= sequenceToInput.size()) throw std::runtime_error("Trace mapping capacity exceeded");
                    const auto index = sequenceToInput[static_cast<std::size_t>(command.sequence)];
                    if (index >= inputCount) throw std::runtime_error("Applied command is outside the frozen script");
                    if (index < inputCount) {
                        inputs[index].tick = command.tick;
                        inputs[index].applicationFrame = row.app.frameId;
                        inputs[index].applied = elapsedNs(origin);
                    }
                }
                if (row.app.presented) {
                    row.markerBegin = elapsedNs(origin);
                    row.marker = observer.recordFrame({row.app.runId, row.app.frameId, row.app.completedTick});
                    row.markerEnd = elapsedNs(origin);
                    if (*row.marker != Observer::RecordStatus::recorded && *row.marker != Observer::RecordStatus::full)
                        throw std::runtime_error("GPU marker failed");
                    if (*row.marker == Observer::RecordStatus::recorded)
                        for (std::size_t i = 0; i < inputCount; ++i)
                            if (inputs[i].accepted && inputs[i].applied && !inputs[i].frame && inputs[i].tick <= row.app.completedTick)
                                inputs[i].frame = row.app.frameId;
                    // This received latency arm joins the marker before another frame begins.
                    while (observer.getPendingCount()) {
                        if (elapsedNs(origin) >= static_cast<std::int64_t>(settings.timeoutSeconds) * 1'000'000'000)
                            throw std::runtime_error("Frame completion polling timed out; original-context drain still required");
                        poll();
                        if (observer.getPendingCount()) SDL_Delay(0);
                    }
                }
                row.end = elapsedNs(origin);
                deadline += period;
                const auto overdue = std::chrono::duration_cast<Nanoseconds>(Clock::now() - deadline);
                if (overdue >= period) {
                    row.pacingSkipped = static_cast<std::uint64_t>(overdue / period);
                    deadline += period * static_cast<std::int64_t>(row.pacingSkipped);
                }
            }
            // Poll every retained receipt before joining, so drain cannot erase timing identities.
            while (observer.getPendingCount()) {
                if (elapsedNs(origin) >= static_cast<std::int64_t>(settings.timeoutSeconds) * 1'000'000'000)
                    throw std::runtime_error("Completion polling timed out; drain still required");
                poll();
                SDL_Delay(1);
            }
        } catch (const std::exception& error) {
            failure = error.what();
            if (measuredOrigin) {
                duration = elapsedNs(*measuredOrigin);
                const auto failed = app.getFrameStatistics();
                if (frameCount < frames.size() && (!frameCount || failed.frameId > frames[frameCount - 1].app.frameId)) {
                    auto& row = frames[frameCount++];
                    row.app = failed;
                    row.draw = app.getDrawStatistics();
                    row.clock = app.GetSession().GetSummary();
                    row.status = app.GetSession().GetStatus();
                    row.visible = isVisible(app);
                    row.end = duration;
                } else if (frameCount && !frames[frameCount - 1].end) frames[frameCount - 1].end = duration;
            }
        }
        drained = observer.tryDrain();
        rowFallbacks.end = app.GetSession().getRowFallbackCount();
        endMemory = readProcessMemory();
        if (!drained && failure.empty()) failure = "Completion drain failed";
        const auto samples = app.GetSession().GetSnapshot().GetSamples();
        std::ranges::copy(samples, finalSamples.begin());
        completedTick = app.GetSession().GetSummary().completed_tick;
        const auto trace = app.GetSession().GetTrace();
        traceCount = trace.size();
        std::ranges::copy(trace, finalTrace.begin());
        // Copy the immutable prefix cold. No per-frame observation scan/allocation changes service timing.
        const auto receivedObservations = app.GetSession().getTickObservations();
        observationCount = receivedObservations.size();
        std::ranges::copy(receivedObservations, observations.begin());
        droppedObservations = app.GetSession().getDroppedTickObservations();
        if (failure.empty() && !settings.captureDirectory.empty()) {
            const auto imageBegin = Clock::now();
            try {
                // Readback and saving are an explicitly cold, paused retained-frame quality arm.
                if (app.GetSession().GetStatus() == Status::running) pressKey(app, SDLK_SPACE);
                pressKey(app, SDLK_F);
                if (!app.requestFrameCapture(settings.captureDirectory + "/overview.bmp") || app.Iterate() != SDL_APP_CONTINUE)
                    throw std::runtime_error("Overview quality capture failed");
                const auto viewport = app.GetCamera().GetViewport();
                float x{}, y{};
                if (!SDL_RenderCoordinatesToWindow(&app.getRenderer(), static_cast<float>(viewport.x + viewport.width * 0.75),
                    static_cast<float>(viewport.y + viewport.height * 0.5), &x, &y)) throw std::runtime_error(SDL_GetError());
                SDL_Event zoom{};
                zoom.type = SDL_EVENT_MOUSE_WHEEL;
                zoom.wheel.y = 8;
                zoom.wheel.mouse_x = x;
                zoom.wheel.mouse_y = y;
                checkEvent(app, zoom);
                if (!app.requestFrameCapture(settings.captureDirectory + "/detail.bmp") || app.Iterate() != SDL_APP_CONTINUE)
                    throw std::runtime_error("Detail quality capture failed");
                const auto imageFrame = app.getFrameStatistics();
                if (!imageFrame.presented || observer.recordFrame({imageFrame.runId, imageFrame.frameId,
                    imageFrame.completedTick}) != Observer::RecordStatus::recorded || !observer.tryDrain())
                    throw std::runtime_error("Quality image frames did not hand off and drain");
            } catch (const std::exception& error) { failure = error.what(); }
            quality.captureCpuNs = std::chrono::duration_cast<Nanoseconds>(Clock::now() - imageBegin).count();
        }
    }
    if (failure.empty()) {
        try {
            const auto imageCpuNs = quality.captureCpuNs;
            quality = compareQuality(startup, finalSamples, completedTick, {finalTrace.data(), traceCount});
            quality.captureCpuNs = imageCpuNs;
        }
        catch (const std::exception& error) { failure = error.what(); }
    }
    writeCapture(settings, {frames.data(), frameCount}, {inputs.data(), inputCount},
        {finalTrace.data(), traceCount}, quality, renderer, failure, drained, boundedOut, duration, warmupDuration,
        actualWarmupTicks, refreshRate, displayScale, pixelDensity, initialDiscarded, device,
        {observations.data(), observationCount}, initialObservationCursor, droppedObservations, cameraViewport, execution,
        startupMemory, endMemory, rowFallbacks);
    return failure.empty() && !boundedOut ? 0 : 1;
}
}

int main(int argc, char** argv) {
    try { return capture(parseSettings(argc, argv)); }
    catch (const std::exception& error) { std::cerr << "Phase14 capture: " << error.what() << '\n'; return 1; }
}
