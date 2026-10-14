#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <iostream>
#include <math/vec3.h>
#include <memory>
#include <optional>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdexcept>
#include <string>
#include <string_view>

#include "crucible/application/frontend.hpp"
#include "crucible/application/game_journey.hpp"
#include "crucible/presentation/FieldTool.hpp"
#include "crucible/presentation/ScenarioSnapshot.hpp"
#include "crucible/presentation/production/swarm_intro_adapter.hpp"
#include "crucible/presentation/production/production_renderer.hpp"
#include "crucible/runtime/ReferenceMissionRoute.hpp"

namespace
{
using namespace crucible;
using namespace crucible::application;
using namespace crucible::presentation::production;
using Clock = std::chrono::steady_clock;

struct LaunchOptions
{
    std::optional<std::filesystem::path> capture_, profile_;
    std::optional<std::chrono::milliseconds> captureTime_;
    std::uint64_t captureFrame_{4}, frameLimit_{};
    std::string_view screen_{"intro"};
    bool mission_{}, route_{};
};

std::uint64_t parseUnsigned(std::string_view text)
{
    std::uint64_t value{};
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size())
        throw std::invalid_argument("Expected an unsigned integer");
    return value;
}

LaunchOptions parseOptions(int argc, char** argv)
{
    LaunchOptions result;
    for (int i = 1; i < argc; ++i)
    {
        const std::string_view flag{argv[i]};
        auto argument = [&]() -> std::string_view {
            if (++i == argc) throw std::invalid_argument("Missing option argument");
            return argv[i];
        };
        if (flag == "--capture") result.capture_ = std::filesystem::path{argument()};
        else if (flag == "--profile") result.profile_ = std::filesystem::path{argument()};
        else if (flag == "--capture-ms")
        {
            const auto time = parseUnsigned(argument());
            if (time > 45000) throw std::invalid_argument("Capture presentation time must be 0..45000 ms");
            result.captureTime_ = std::chrono::milliseconds{static_cast<std::int64_t>(time)};
        }
        else if (flag == "--capture-frame") result.captureFrame_ = parseUnsigned(argument());
        else if (flag == "--frames") result.frameLimit_ = parseUnsigned(argument());
        else if (flag == "--screen") result.screen_ = argument();
        else if (flag == "--menu") result.screen_ = "menu";
        else if (flag == "--mission") result.mission_ = true;
        else if (flag == "--route") { result.route_ = true; result.mission_ = true; }
        else throw std::invalid_argument("Unknown crucible_game option");
    }
    if (result.captureFrame_ == 0 || result.captureFrame_ > 100000 || result.frameLimit_ > 100000)
        throw std::invalid_argument("Frame bounds must be 1..100000 (omitted frame limit is interactive)");
    if (result.screen_ != "intro" && result.screen_ != "menu" && result.screen_ != "options" && result.screen_ != "missions")
        throw std::invalid_argument("Screen must be intro, menu, options or missions");
    if (result.capture_ && result.frameLimit_ && result.frameLimit_ < result.captureFrame_)
        throw std::invalid_argument("Frame limit precedes capture frame");
    return result;
}

struct SdlSession
{
    SdlSession() { if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) throw std::runtime_error(SDL_GetError()); }
    ~SdlSession() { SDL_Quit(); }
};
struct PlatformBackend
{
    explicit PlatformBackend(SDL_Window& window)
    {
        if (!ImGui_ImplSDL3_InitForOther(&window)) throw std::runtime_error("ImGui SDL3 initialization failed");
    }
    ~PlatformBackend() { ImGui_ImplSDL3_Shutdown(); }
};

bool navigate(GameJourney& journey, NavigationAction action)
{
    const auto view = journey.getFrontendView();
    return journey.tryApplyIntent({view.screen_, view.transitionSerial_, action});
}

bool trySetFullscreen(SDL_Window& window, bool requested)
{
    const bool previous = (SDL_GetWindowFlags(&window) & SDL_WINDOW_FULLSCREEN) != 0;
    int pixelWidth{}, pixelHeight{};
    if (SDL_SetWindowFullscreen(&window, requested) && SDL_SyncWindow(&window) &&
        ((SDL_GetWindowFlags(&window) & SDL_WINDOW_FULLSCREEN) != 0) == requested &&
        SDL_GetWindowSizeInPixels(&window, &pixelWidth, &pixelHeight) &&
        pixelWidth > 0 && pixelHeight > 0 && pixelWidth <= 4096 && pixelHeight <= 4096 &&
        pixelWidth <= pixelHeight * 32 && pixelHeight <= pixelWidth * 32)
        return true;
    // A denied or timed-out asynchronous request must not become a saved preference.
    if (!SDL_SetWindowFullscreen(&window, previous) || !SDL_SyncWindow(&window) ||
        ((SDL_GetWindowFlags(&window) & SDL_WINDOW_FULLSCREEN) != 0) != previous)
        throw std::runtime_error("Fullscreen change failed and the previous native mode could not be confirmed; preference was not saved");
    return false;
}

CinematicFrame buildWorld(const presentation::ScenarioSnapshot& snapshot,
    float aspect, std::array<NanitePose, CinematicFrame::maxInstances>& poses)
{
    const auto info = snapshot.GetInfo();
    if (!info || !info->grid.TryValidate()) throw std::runtime_error("Missing world geometry");
    const auto extent = *info->grid.TryValidate();
    const auto samples = snapshot.GetSamples();
    if (samples.size() + extent.cells > poses.size()) throw std::runtime_error("Production world capacity exceeded");
    std::size_t count{};
    for (const auto& sample : samples)
    {
        const float angle = std::atan2(sample.velocity.y, sample.velocity.x);
        auto& pose = poses[count++];
        pose = {};
        pose.position_ = {sample.position.x, 0.32F, sample.position.y};
        pose.orientation_ = {0, std::sin(angle * 0.5F), 0, std::cos(angle * 0.5F)};
        pose.scale_ = sample.activity == SampleActivity::anchored ? 0.43F : 0.26F;
        pose.coreIntensity_ = sample.activity == SampleActivity::lost ? 0.0F : 1.0F;
    }
    const auto stocks = snapshot.GetStocks();
    const auto blight = snapshot.GetBlight();
    for (std::size_t cell = 0; cell < extent.cells; ++cell)
    {
        auto& pose = poses[count++];
        pose = {};
        pose.position_ = {(static_cast<float>(cell % info->grid.columns) + 0.5F) * info->grid.cell_size,
            0.03F, (static_cast<float>(cell / info->grid.columns) + 0.5F) * info->grid.cell_size};
        pose.scale_ = 0.12F;
        pose.coreIntensity_ = cell < blight.size() && blight[cell] ? 2.0F :
            (cell < stocks.size() && stocks[cell] ? 0.4F : 0.0F);
    }
    CinematicFrame frame;
    frame.poses_ = std::span<const NanitePose>{poses.data(), count};
    const float halfHeight = std::max(extent.height * 0.5F, extent.width * 0.5F / aspect) * 1.16F;
    const float cameraHeight = std::min(440.0F, std::max(50.0F, halfHeight / std::tan(0.45F)));
    frame.camera_.target_ = {extent.width * 0.5F, 0, extent.height * 0.5F};
    frame.camera_.eye_ = {extent.width * 0.5F, cameraHeight, extent.height * 0.5F + cameraHeight * 0.12F};
    frame.camera_.verticalFovRadians_ = std::max(0.9F, 2.0F * std::atan(halfHeight / cameraHeight));
    return frame;
}

std::optional<Position> pickGround(float x, float y, float width, float height,
    float aspect, const CinematicCamera& camera)
{
    using filament::math::float3;
    const float3 eye{camera.eye_[0], camera.eye_[1], camera.eye_[2]};
    const float3 target{camera.target_[0], camera.target_[1], camera.target_[2]};
    const auto forward = normalize(target - eye);
    const auto right = normalize(cross(forward, float3{0, 1, 0}));
    const auto up = cross(right, forward);
    const float tangent = std::tan(camera.verticalFovRadians_ * 0.5F);
    const auto ray = forward + right * ((2 * x / width - 1) * tangent * aspect)
        + up * ((1 - 2 * y / height) * tangent);
    if (ray.y >= -0.00001F) return std::nullopt;
    const auto point = eye + ray * (-eye.y / ray.y);
    if (!std::isfinite(point.x) || !std::isfinite(point.z) || point.x < 0 || point.x > 64 || point.z < 0 || point.z > 32)
        return std::nullopt;
    return Position{point.x, point.z};
}

void saveCapture(ProductionRenderer::CaptureResult& capture, const std::filesystem::path& path,
    const ProductionRenderer::Metadata& metadata, std::chrono::milliseconds elapsed,
    std::optional<std::uint64_t> worldTick, SwarmIntroAdapter::AdvanceResult progress, std::size_t meshVertices, FrontendScreen screen)
{
    const auto width = capture.extent_.width_, height = capture.extent_.height_;
    const std::size_t stride = static_cast<std::size_t>(width) * 4;
    if (!width || !height || capture.rgba_.size() != stride * height) throw std::runtime_error("Invalid captured pixels");
    std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface{
        SDL_CreateSurfaceFrom(static_cast<int>(width), static_cast<int>(height), SDL_PIXELFORMAT_RGBA32,
            capture.rgba_.data(), static_cast<int>(stride)), SDL_DestroySurface};
    if (!surface || !SDL_SavePNG(surface.get(), path.string().c_str())) throw std::runtime_error(SDL_GetError());
    auto receipt = path;
    receipt += ".json";
    std::ofstream output{receipt};
    output << "{\"source_pin\":\"" << metadata.sourcePin_.data() << "\",\"frame\":" << capture.frameId_
        << ",\"surface_generation\":" << capture.surfaceGeneration_ << ",\"width\":" << width
        << ",\"height\":" << height << ",\"cinematic_ms\":" << elapsed.count()
        << ",\"sequence\":\"" << (worldTick ? "committed-mission-world" : (meshVertices ? "PR47-scaffold-nanite-body" : "decorative-static-floor-preparing"))
        << "\",\"completed_tick\":";
    if (worldTick) output << *worldTick; else output << "null";
    output << ",\"presented_ticks\":" << progress.totalPresentedTicks_
        << ",\"choreography_seconds\":" << progress.choreographySeconds_
        << ",\"static_ready\":" << (progress.staticReady_ ? "true" : "false")
        << ",\"mesh_vertices\":" << meshVertices
        << ",\"screen_id\":" << static_cast<unsigned>(screen)
        << ",\"scanout_verified\":false}\n";
    output.flush();
    if (!output) throw std::runtime_error("Capture receipt write failed");
}

int run(const LaunchOptions& options)
{
    SdlSession sdl;
    auto windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#if defined(__APPLE__)
    windowFlags |= SDL_WINDOW_METAL;
#endif
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window{
        SDL_CreateWindow("Crucible — Production Receiver", 1280, 864,
            windowFlags), SDL_DestroyWindow};
    if (!window) throw std::runtime_error(SDL_GetError());
    if (!SDL_SetWindowMinimumSize(window.get(), 480, 320)) throw std::runtime_error(SDL_GetError());
    std::unique_ptr<ImGuiContext, decltype(&ImGui::DestroyContext)> context{ImGui::CreateContext(), ImGui::DestroyContext};
    if (!context) throw std::runtime_error("ImGui context allocation failed");
    ImGui::SetCurrentContext(context.get());
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    const char* base = SDL_GetBasePath();
    if (!base) throw std::runtime_error(SDL_GetError());
    Frontend frontend{*context, std::filesystem::path{base} / "assets/production/fonts"};
    PlatformBackend platform{*window};
    ProductionRenderer renderer{*window, *context};
    SwarmIntroAdapter swarm;
    GameJourney journey{options.profile_};
    if (journey.getFrontendView().options_.fullscreen_ && !trySetFullscreen(*window, true))
        throw std::runtime_error("Saved fullscreen preference could not be applied");
    if (options.screen_ != "intro" || options.mission_) (void)navigate(journey, NavigationAction::skipIntro);
    if (options.mission_)
    {
        if (!navigate(journey, NavigationAction::newRun) || !navigate(journey, NavigationAction::beginMission))
            throw std::runtime_error("Receiving mission navigation failed");
    }
    else if (options.screen_ == "options") (void)navigate(journey, NavigationAction::showOptions);
    else if (options.screen_ == "missions") (void)navigate(journey, NavigationAction::showMissions);
    std::array<NanitePose, CinematicFrame::maxInstances> worldPoses{};
    presentation::FieldToolSettings tool;
    std::optional<Position> flowStart;
    auto lastPump = Clock::now();
    auto lastDecorativePump = lastPump;
    bool coldSeekDone{};
    std::optional<Clock::time_point> captureStarted;
    if (options.capture_) captureStarted = lastPump;
    bool captureReserved{}, captureSubmitted{}, captureDone{}, focused{true};
    std::uint64_t frameId{1}, lastRouteTick = UINT64_MAX;
    std::uint64_t lastAttempt{};
    std::optional<StructuralCommandResult> lastStructural;
    FrontendScreen previousScreen{journey.getFrontendView().screen_};
    std::chrono::milliseconds sceneTime{};
    std::chrono::milliseconds capturedSceneTime{};
    std::optional<std::uint64_t> capturedWorldTick;
    ProductionRenderer::Metadata captureMetadata{};
    SwarmIntroAdapter::AdvanceResult capturedSwarmProgress{};
    std::size_t capturedMeshVertices{};
    FrontendScreen capturedScreen{FrontendScreen::intro};
    std::string_view nativeError;
    auto completeCapture = [&] {
        if (captureSubmitted && !captureDone &&
            renderer.tryDrain(std::chrono::milliseconds{1}) == ProductionRenderer::DrainStatus::failed)
            throw std::runtime_error("Pending capture retirement failed");
        if (auto captured = renderer.takeCapture())
        {
            if (!options.capture_ || captured->frameId_ != options.captureFrame_ ||
                captured->surfaceGeneration_ != captureMetadata.surfaceGeneration_)
                throw std::runtime_error("Capture identity mismatch");
            saveCapture(*captured, *options.capture_, captureMetadata, capturedSceneTime, capturedWorldTick, capturedSwarmProgress, capturedMeshVertices, capturedScreen);
            captureDone = true;
        }
    };
    while (!journey.isQuitRequested())
    {
        const auto now = Clock::now();
        completeCapture();
        if (captureDone && !options.frameLimit_) break;
        if (captureStarted && !captureDone && now - *captureStarted > std::chrono::seconds{15}) throw std::runtime_error("Capture timed out");
        if (journey.getFrontendView().screen_ != FrontendScreen::playing) lastPump = now;
        bool skipIntro{}, mouseDown{}, mouseUp{}, fuse{}, shatter{};
        bool nextSlot{};
        std::optional<presentation::FieldTool> selectedTool;
        float mouseX{}, mouseY{};
        const auto eventView = journey.getFrontendView();
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT) (void)navigate(journey, NavigationAction::requestQuit);
            if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST || event.type == SDL_EVENT_WINDOW_MINIMIZED)
            {
                focused = false;
                swarm.suspend();
                lastDecorativePump = now;
                (void)navigate(journey, NavigationAction::pause);
                lastPump = now;
                flowStart.reset();
            }
            if (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED || event.type == SDL_EVENT_WINDOW_RESTORED) focused = true;
            if (eventView.screen_ == FrontendScreen::intro &&
                (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)))
            { skipIntro = true; continue; }
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
            {
                if (event.key.key == SDLK_ESCAPE && eventView.screen_ == FrontendScreen::playing)
                {
                    (void)navigate(journey, NavigationAction::pause);
                    lastPump = now;
                }
                if (event.key.key >= SDLK_1 && event.key.key <= SDLK_4)
                    selectedTool = static_cast<presentation::FieldTool>(event.key.key - SDLK_1);
                if (event.key.key == SDLK_TAB) nextSlot = true;
                fuse = fuse || event.key.key == SDLK_F;
                shatter = shatter || event.key.key == SDLK_X;
            }
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT)
            { mouseDown = true; mouseX = event.button.x; mouseY = event.button.y; }
            if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT)
            { mouseUp = true; mouseX = event.button.x; mouseY = event.button.y; }
        }
        int logicalWidth{}, logicalHeight{}, pixelWidth{}, pixelHeight{};
        if (!SDL_GetWindowSize(window.get(), &logicalWidth, &logicalHeight) ||
            !SDL_GetWindowSizeInPixels(window.get(), &pixelWidth, &pixelHeight)) throw std::runtime_error(SDL_GetError());
        auto metadata = renderer.getMetadata();
        if (captureReserved && !captureSubmitted &&
            (pixelWidth != static_cast<int>(captureMetadata.extent_.width_) ||
                pixelHeight != static_cast<int>(captureMetadata.extent_.height_)))
            throw std::runtime_error("Capture surface changed before the targeted frame was submitted");
        if (pixelWidth <= 0 || pixelHeight <= 0 || !focused)
        {
            swarm.suspend();
            lastDecorativePump = now;
            (void)navigate(journey, NavigationAction::pause);
            lastPump = now;
            flowStart.reset();
            SDL_Delay(10);
            continue;
        }
        const float aspect = static_cast<float>(pixelWidth) / static_cast<float>(pixelHeight);
        if (aspect < 1.0F / 32 || aspect > 32 || std::min(logicalWidth, logicalHeight) < 320 ||
            std::max(logicalWidth, logicalHeight) < 480)
            throw std::runtime_error("Unsupported production window dimensions");
        if (metadata.extent_.width_ != static_cast<std::uint32_t>(pixelWidth) || metadata.extent_.height_ != static_cast<std::uint32_t>(pixelHeight))
        {
            const auto resized = renderer.tryResize({static_cast<std::uint32_t>(pixelWidth), static_cast<std::uint32_t>(pixelHeight)});
            if (resized.status_ == ProductionRenderer::Status::failed) throw std::runtime_error("Production surface resize failed");
            if (resized.status_ == ProductionRenderer::Status::rejected)
                throw std::runtime_error("Unsupported production drawable extent (maximum 4096 pixels per edge)");
            if (resized.status_ != ProductionRenderer::Status::scheduled) { SDL_Delay(1); continue; }
            metadata = renderer.getMetadata();
        }
        const auto ready = renderer.beginUiFrame();
        if (ready == ProductionRenderer::Status::failed || ready == ProductionRenderer::Status::rejected)
            throw std::runtime_error("Production UI frame refused");
        if (ready != ProductionRenderer::Status::scheduled) { SDL_Delay(1); continue; }
        if (options.route_ && journey.getFrontendView().screen_ == FrontendScreen::playing)
        {
            const auto view = journey.getFrontendView();
            if (lastAttempt != journey.getAttemptId()) { lastAttempt = journey.getAttemptId(); lastRouteTick = UINT64_MAX; }
            const auto tick = view.progress_->completed_tick;
            if (tick != lastRouteTick)
            {
                if (const auto edit = runtime::GetReferenceMissionRouteEdit(tick))
                {
                    const auto receipt = journey.tryAdmitFieldEdit(*edit);
                    if (!receipt || receipt->status != runtime::CommandIngress::AdmissionStatus::accepted)
                        throw std::runtime_error("Reference route admission refused");
                }
                lastRouteTick = tick;
            }
        }
        (void)journey.tryPump(options.route_ ? std::chrono::nanoseconds{16'666'667} :
            std::chrono::duration_cast<std::chrono::nanoseconds>(now - lastPump));
        lastPump = now;
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        SDL_Rect safe;
        if (!SDL_GetWindowSafeArea(window.get(), &safe)) throw std::runtime_error(SDL_GetError());
        const auto beforeView = journey.getFrontendView();
        if (beforeView.screen_ == FrontendScreen::intro && beforeView.options_.reducedMotion_)
            (void)navigate(journey, NavigationAction::skipIntro);
        auto view = journey.getFrontendView({static_cast<float>(safe.x), static_cast<float>(safe.y),
            static_cast<float>(safe.w), static_cast<float>(safe.h)});
        if (!nativeError.empty()) view.error_ = nativeError;
        if (view.screen_ == FrontendScreen::intro && previousScreen != FrontendScreen::intro)
        {
            swarm.replay();
            lastDecorativePump = now;
        }
        previousScreen = view.screen_;
        if (!swarm.trySetViewport(aspect, static_cast<float>(pixelWidth)))
            throw std::runtime_error("Unsupported swarm viewport");
        if (options.captureTime_ && !coldSeekDone)
        {
            const auto ticks = static_cast<std::uint32_t>(options.captureTime_->count() * 60 / 1000);
            if (!swarm.trySeekPresentedTicks(ticks)) throw std::runtime_error("Swarm receiving seek refused");
            coldSeekDone = true;
        }
        const bool showWorld = journey.getSnapshot() && (view.screen_ == FrontendScreen::playing ||
            view.screen_ == FrontendScreen::paused || view.screen_ == FrontendScreen::results);
        const bool animatedIntro = view.screen_ == FrontendScreen::intro && !view.options_.reducedMotion_;
        if (showWorld) swarm.suspend();
        else if (!options.captureTime_)
        {
            (void)swarm.advance(std::chrono::duration_cast<std::chrono::nanoseconds>(now - lastDecorativePump),
                animatedIntro ? SwarmIntroAdapter::PlaybackMode::animated : SwarmIntroAdapter::PlaybackMode::settling);
        }
        else
        {
            // Freeze explicit receiving time while refreshing viewport/geometry without a tick.
            (void)swarm.advance(std::chrono::nanoseconds::zero());
        }
        lastDecorativePump = now;
        const auto swarmProgress = swarm.getProgress();
        sceneTime = std::chrono::milliseconds{static_cast<std::int64_t>(swarmProgress.totalPresentedTicks_) * 1000 / 60};
        const auto intent = frontend.draw(view);
        auto mesh = showWorld ? std::optional<NaniteMeshFrame>{} : swarm.getFrame();
        CinematicFrame frame = showWorld ? buildWorld(*journey.getSnapshot(), aspect, worldPoses) : CinematicFrame{};
        const auto& io = ImGui::GetIO();
        const bool activeInput = view.screen_ == FrontendScreen::playing && view.transitionSerial_ == eventView.transitionSerial_;
        if (activeInput && !io.WantCaptureKeyboard)
        {
            if ((selectedTool && *selectedTool != tool.tool) || nextSlot) flowStart.reset();
            if (selectedTool) tool.tool = *selectedTool;
            if (nextSlot) tool.slot = (tool.slot + 1) % 4;
        }
        if (activeInput && !io.WantCaptureMouse)
        {
            const auto point = pickGround(mouseX, mouseY, static_cast<float>(logicalWidth), static_cast<float>(logicalHeight), aspect, frame.camera_);
            if (mouseDown && point)
            {
                if (tool.tool == presentation::FieldTool::flow) flowStart = point;
                else if (const auto edit = presentation::TryBuildFieldEdit(tool, *point, 4))
                    if (const auto receipt = journey.tryAdmitFieldEdit(*edit)) SDL_Log("Field admission=%d", static_cast<int>(receipt->status));
            }
            if (mouseUp && flowStart && point)
                if (const auto edit = presentation::TryBuildFieldEdit(tool, *flowStart, 4, *point))
                    if (const auto receipt = journey.tryAdmitFieldEdit(*edit)) SDL_Log("Flow admission=%d", static_cast<int>(receipt->status));
        }
        if (mouseUp || view.screen_ != FrontendScreen::playing) flowStart.reset();
        if (activeInput && !io.WantCaptureKeyboard)
        {
            if (fuse) if (const auto receipt = journey.tryFuseRelay()) SDL_Log("Fuse admission=%d", static_cast<int>(receipt->status));
            if (shatter) if (const auto receipt = journey.tryShatterRelay()) SDL_Log("Shatter admission=%d", static_cast<int>(receipt->status));
        }
        ImGui::Render();
        if (options.capture_ && !captureReserved && frameId == options.captureFrame_)
        {
            const auto reserved = renderer.tryCapture({frameId, metadata.surfaceGeneration_});
            if (reserved.status_ == ProductionRenderer::Status::failed || reserved.status_ == ProductionRenderer::Status::rejected)
                throw std::runtime_error("Capture reservation refused");
            if (reserved.status_ != ProductionRenderer::Status::scheduled) { SDL_Delay(1); continue; }
            captureReserved = true;
            captureMetadata = metadata;
        }
        const auto receipt = mesh ? renderer.tryRender(*mesh, *ImGui::GetDrawData(), frameId) :
            renderer.tryRender(frame, *ImGui::GetDrawData(), frameId);
        if (receipt.status_ == ProductionRenderer::Status::failed || receipt.status_ == ProductionRenderer::Status::rejected)
            throw std::runtime_error("Production frame refused: " + std::to_string(static_cast<int>(receipt.reason_)));
        if (receipt.status_ == ProductionRenderer::Status::scheduled && options.capture_ && frameId == options.captureFrame_)
        {
            captureSubmitted = true;
            capturedSceneTime = sceneTime;
            capturedSwarmProgress = swarmProgress;
            capturedScreen = view.screen_;
            capturedMeshVertices = mesh ? mesh->vertices_.size() : 0;
            if (mesh)
            {
                std::array<std::size_t, 4> roles{};
                float minimum = 4.0F, maximum = 0.0F;
                for (const auto& vertex : mesh->vertices_)
                {
                    ++roles[static_cast<std::size_t>(vertex.role_)];
                    minimum = std::min(minimum, vertex.intensity_);
                    maximum = std::max(maximum, vertex.intensity_);
                }
                std::cout << "Captured mesh: shields=" << roles[0] << ", rails=" << roles[1]
                    << ", hubs=" << roles[2] << ", cores=" << roles[3]
                    << ", intensity_min=" << minimum << ", intensity_max=" << maximum << '\n';
            }
            capturedWorldTick = showWorld ? std::optional{journey.getSnapshot()->GetInfo()->completed_tick} : std::nullopt;
        }
        if (intent)
        {
            if (const auto* fullscreen = std::get_if<SetFullscreen>(&intent->action_))
            {
                const auto current = journey.getFrontendView();
                if (current.screen_ == intent->originScreen_ && current.transitionSerial_ == intent->transitionSerial_ &&
                    trySetFullscreen(*window, fullscreen->value_))
                {
                    (void)journey.tryApplyIntent(*intent);
                    nativeError = {};
                }
                else nativeError = "Fullscreen change was not confirmed by the native window; the preference was not saved.";
            }
            else (void)journey.tryApplyIntent(*intent);
        }
        if (skipIntro) (void)navigate(journey, NavigationAction::skipIntro);
        else if (view.screen_ == FrontendScreen::intro && swarmProgress.sequenceFinished_ && !options.captureTime_)
            (void)navigate(journey, NavigationAction::skipIntro);
        if (view.screen_ != FrontendScreen::playing || journey.getFrontendView().screen_ != FrontendScreen::playing)
            lastPump = Clock::now();
        if (const auto structural = journey.getLastStructuralResult(); structural != lastStructural)
        {
            if (structural) SDL_Log("Completed structural result=%d", static_cast<int>(*structural));
            lastStructural = structural;
        }
        completeCapture();
        if (receipt.status_ == ProductionRenderer::Status::scheduled)
        {
            if (options.frameLimit_ && frameId >= options.frameLimit_ && (!options.capture_ || captureDone)) break;
            ++frameId;
        }
        if (captureDone && !options.frameLimit_) break;
        SDL_Delay(1);
    }
    if (options.capture_ && !captureDone) throw std::runtime_error("Exited before capture completion");
    if (renderer.tryDrain(std::chrono::seconds{5}) != ProductionRenderer::DrainStatus::drained)
        throw std::runtime_error("Production qualified drain did not complete");
    std::cout << "Crucible production receiver: PR47-scaffold-nanite-body, SDL window/events, native composed frames="
        << frameId << ", capture=" << captureDone << ", final intro parity unqualified\n";
    return 0;
}
}

int main(int argc, char** argv)
{
    try { return run(parseOptions(argc, argv)); }
    catch (const std::exception& error) { std::cerr << "Crucible production receiver: " << error.what() << '\n'; return 1; }
}
