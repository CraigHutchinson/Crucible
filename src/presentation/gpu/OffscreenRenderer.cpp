#include <crucible/presentation/gpu/OffscreenRenderer.hpp>
#include <crucible/presentation/gpu/InstancePacket.hpp>
#include "SlotSchedule.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <exception>
#include <limits>
#include <stdexcept>
#include <vector>
namespace crucible::presentation::gpu {
namespace {
std::uint32_t BufferBytes(std::size_t samples, std::size_t cells) {
    constexpr auto limit = std::numeric_limits<std::uint32_t>::max() / sizeof(Instance);
    if (cells > limit || samples > limit - cells) throw std::length_error("GPU buffer capacity");
    return static_cast<std::uint32_t>(std::max(std::size_t{1}, samples + cells) * sizeof(Instance));
}
std::vector<std::uint32_t> ShaderWords(std::span<const std::byte> bytes) {
    if (bytes.size() < 20 || bytes.size() % 4 != 0) throw std::invalid_argument("Invalid SPIR-V byte count");
    std::vector<std::uint32_t> words(bytes.size() / 4);
    std::memcpy(words.data(), bytes.data(), bytes.size());
    if (words[0] != 0x07230203U) throw std::invalid_argument("Invalid SPIR-V magic");
    return words;
}
void Check(bool valid) { if (!valid) throw std::runtime_error(SDL_GetError()); }
}
struct OffscreenRenderer::State {
    struct Slot {
        SDL_GPUTransferBuffer* upload{};
        SDL_GPUTransferBuffer* download{};
        SDL_GPUBuffer* instances{};
        SDL_GPUTexture* output{};
        SDL_GPUFence* fence{};
    };
    SDL_GPUDevice& device;
    const std::shared_ptr<const std::byte> identity{std::make_shared<const std::byte>(std::byte{})};
    std::size_t sample_capacity{}, cell_capacity{};
    std::uint32_t instance_bytes{};
    SDL_GPUShader* vertex{};
    SDL_GPUShader* fragment{};
    SDL_GPUGraphicsPipeline* pipeline{};
    SDL_GPUBuffer* quad{};
    SDL_GPUBuffer* indices{};
    SDL_GPUTransferBuffer* startup_transfer{};
    SDL_GPUFence* startup_fence{};
    std::array<Slot, detail::SlotSchedule::Count> slots{};
    detail::SlotSchedule schedule;
    bool failed{}, has_work{};
    State(SDL_GPUDevice& value, std::size_t samples, std::size_t cells)
        : device(value), sample_capacity(samples), cell_capacity(cells), instance_bytes(BufferBytes(samples, cells)) {}
    bool Drain() noexcept {
        if (has_work && !SDL_WaitForGPUIdle(&device)) { failed = true; return false; }
        has_work = false;
        for (std::size_t i = 0; i < slots.size(); ++i)
            if (schedule.Get(i).submitted) schedule.Complete(i);
        return true;
    }
    ~State() {
        if (!Drain()) {
            SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Crucible GPU drain failed; refusing live resource release: %s", SDL_GetError());
            std::terminate();
        }
        if (startup_fence) SDL_ReleaseGPUFence(&device, startup_fence);
        if (startup_transfer) SDL_ReleaseGPUTransferBuffer(&device, startup_transfer);
        for (auto& slot : slots) {
            if (slot.fence) SDL_ReleaseGPUFence(&device, slot.fence);
            if (slot.upload) SDL_ReleaseGPUTransferBuffer(&device, slot.upload);
            if (slot.download) SDL_ReleaseGPUTransferBuffer(&device, slot.download);
            if (slot.instances) SDL_ReleaseGPUBuffer(&device, slot.instances);
            if (slot.output) SDL_ReleaseGPUTexture(&device, slot.output);
        }
        if (pipeline) SDL_ReleaseGPUGraphicsPipeline(&device, pipeline);
        if (vertex) SDL_ReleaseGPUShader(&device, vertex);
        if (fragment) SDL_ReleaseGPUShader(&device, fragment);
        if (quad) SDL_ReleaseGPUBuffer(&device, quad);
        if (indices) SDL_ReleaseGPUBuffer(&device, indices);
    }
    void Initialize(std::span<const std::byte> vertex_bytes, std::span<const std::byte> fragment_bytes) {
        const auto vert = ShaderWords(vertex_bytes), frag = ShaderWords(fragment_bytes);
        SDL_GPUShaderCreateInfo shader{};
        shader.entrypoint = "main"; shader.format = SDL_GPU_SHADERFORMAT_SPIRV;
        shader.stage = SDL_GPU_SHADERSTAGE_VERTEX; shader.num_uniform_buffers = 1;
        shader.code = reinterpret_cast<const Uint8*>(vert.data()); shader.code_size = vertex_bytes.size();
        vertex = SDL_CreateGPUShader(&device, &shader); Check(vertex != nullptr);
        shader.stage = SDL_GPU_SHADERSTAGE_FRAGMENT; shader.num_uniform_buffers = 0;
        shader.code = reinterpret_cast<const Uint8*>(frag.data()); shader.code_size = fragment_bytes.size();
        fragment = SDL_CreateGPUShader(&device, &shader); Check(fragment != nullptr);
        const std::array descriptions{
            SDL_GPUVertexBufferDescription{0, 8, SDL_GPU_VERTEXINPUTRATE_VERTEX, 0},
            SDL_GPUVertexBufferDescription{1, sizeof(Instance), SDL_GPU_VERTEXINPUTRATE_INSTANCE, 0}};
        const std::array attributes{
            SDL_GPUVertexAttribute{0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 0},
            SDL_GPUVertexAttribute{1, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, 0},
            SDL_GPUVertexAttribute{2, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, 16}};
        SDL_GPUColorTargetDescription color_target{};
        color_target.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        SDL_GPUGraphicsPipelineCreateInfo create{};
        create.vertex_shader = vertex; create.fragment_shader = fragment;
        create.vertex_input_state = {descriptions.data(), static_cast<Uint32>(descriptions.size()),
                                    attributes.data(), static_cast<Uint32>(attributes.size())};
        create.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        create.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        create.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        create.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
        create.rasterizer_state.enable_depth_clip = true;
        create.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
        create.target_info.color_target_descriptions = &color_target;
        create.target_info.num_color_targets = 1;
        pipeline = SDL_CreateGPUGraphicsPipeline(&device, &create); Check(pipeline != nullptr);
        const SDL_GPUBufferCreateInfo quad_info{SDL_GPU_BUFFERUSAGE_VERTEX, 32, 0};
        const SDL_GPUBufferCreateInfo index_info{SDL_GPU_BUFFERUSAGE_INDEX, 12, 0};
        quad = SDL_CreateGPUBuffer(&device, &quad_info); Check(quad != nullptr);
        indices = SDL_CreateGPUBuffer(&device, &index_info); Check(indices != nullptr);
        const SDL_GPUTransferBufferCreateInfo startup_info{SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, 48, 0};
        startup_transfer = SDL_CreateGPUTransferBuffer(&device, &startup_info); Check(startup_transfer != nullptr);
        auto* mapped = static_cast<std::byte*>(SDL_MapGPUTransferBuffer(&device, startup_transfer, false));
        Check(mapped != nullptr);
        constexpr std::array<float, 8> corners{-1, -1, 1, -1, 1, 1, -1, 1};
        constexpr std::array<std::uint16_t, 6> triangles{0, 1, 2, 0, 2, 3};
        std::memcpy(mapped, corners.data(), sizeof corners);
        std::memcpy(mapped + 32, triangles.data(), sizeof triangles);
        SDL_UnmapGPUTransferBuffer(&device, startup_transfer);
        auto* command = SDL_AcquireGPUCommandBuffer(&device); Check(command != nullptr);
        auto* copy = SDL_BeginGPUCopyPass(command);
        if (!copy) { static_cast<void>(SDL_CancelGPUCommandBuffer(command)); Check(false); }
        const SDL_GPUTransferBufferLocation source_quad{startup_transfer, 0}, source_indices{startup_transfer, 32};
        const SDL_GPUBufferRegion quad_region{quad, 0, 32}, index_region{indices, 0, 12};
        SDL_UploadToGPUBuffer(copy, &source_quad, &quad_region, false);
        SDL_UploadToGPUBuffer(copy, &source_indices, &index_region, false);
        SDL_EndGPUCopyPass(copy);
        has_work = true;
        startup_fence = SDL_SubmitGPUCommandBufferAndAcquireFence(command); Check(startup_fence != nullptr);
        Check(Drain());
        SDL_ReleaseGPUFence(&device, startup_fence); startup_fence = nullptr;
        SDL_ReleaseGPUTransferBuffer(&device, startup_transfer); startup_transfer = nullptr;
        for (auto& slot : slots) {
            const SDL_GPUBufferCreateInfo instance_info{SDL_GPU_BUFFERUSAGE_VERTEX, instance_bytes, 0};
            const SDL_GPUTransferBufferCreateInfo upload_info{SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, instance_bytes, 0};
            const SDL_GPUTransferBufferCreateInfo download_info{SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD, ReadbackBytes, 0};
            SDL_GPUTextureCreateInfo output_info{};
            output_info.type = SDL_GPU_TEXTURETYPE_2D;
            output_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
            output_info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
            output_info.width = Width; output_info.height = Height;
            output_info.layer_count_or_depth = 1; output_info.num_levels = 1;
            output_info.sample_count = SDL_GPU_SAMPLECOUNT_1;
            slot.instances = SDL_CreateGPUBuffer(&device, &instance_info); Check(slot.instances != nullptr);
            slot.upload = SDL_CreateGPUTransferBuffer(&device, &upload_info); Check(slot.upload != nullptr);
            slot.download = SDL_CreateGPUTransferBuffer(&device, &download_info); Check(slot.download != nullptr);
            slot.output = SDL_CreateGPUTexture(&device, &output_info); Check(slot.output != nullptr);
        }
    }
    bool Complete(std::size_t index) noexcept {
        if (schedule.Get(index).complete) return true;
        if (slots[index].fence && SDL_QueryGPUFence(&device, slots[index].fence)) {
            schedule.Complete(index); return true;
        }
        return false;
    }
};
OffscreenRenderer::OffscreenRenderer(SDL_GPUDevice& device, std::size_t samples, std::size_t cells,
                                     std::span<const std::byte> vertex, std::span<const std::byte> fragment)
    : state_(std::make_unique<State>(device, samples, cells)) { state_->Initialize(vertex, fragment); }
OffscreenRenderer::~OffscreenRenderer() = default;
bool OffscreenRenderer::TryDrain() noexcept { return state_->Drain(); }
Submission OffscreenRenderer::TrySubmit(const InstancePacket& packet, const Camera2D& camera) noexcept {
    auto& state = *state_;
    if (state.failed) return {SubmitStatus::device_error};
    const auto projection = TryMakeProjection(packet, camera);
    const auto cells = packet.GetCells(), markers = packet.GetMarkers();
    if (!projection || cells.size() > state.cell_capacity || markers.size() > state.sample_capacity ||
        !state.schedule.CanSubmit()) return {SubmitStatus::invalid};
    for (std::size_t i = 0; i < state.slots.size(); ++i)
        if (state.schedule.Get(i).submitted) static_cast<void>(state.Complete(i));
    const auto available = state.schedule.FindAvailable();
    if (!available) return {SubmitStatus::busy};
    auto& slot = state.slots[*available];
    auto* command = SDL_AcquireGPUCommandBuffer(&state.device);
    if (!command) { state.failed = true; return {SubmitStatus::device_error}; }
    const auto fail = [&]() {
        static_cast<void>(SDL_CancelGPUCommandBuffer(command));
        state.failed = true;
        return Submission{SubmitStatus::device_error};
    };
    auto* mapped = static_cast<std::byte*>(SDL_MapGPUTransferBuffer(&state.device, slot.upload, false));
    if (!mapped) return fail();
    const auto cell_bytes = cells.size_bytes(), marker_bytes = markers.size_bytes();
    if (cell_bytes) std::memcpy(mapped, cells.data(), cell_bytes);
    if (marker_bytes) std::memcpy(mapped + cell_bytes, markers.data(), marker_bytes);
    SDL_UnmapGPUTransferBuffer(&state.device, slot.upload);
    auto* copy = SDL_BeginGPUCopyPass(command);
    if (!copy) return fail();
    const SDL_GPUTransferBufferLocation upload{slot.upload, 0};
    const SDL_GPUBufferRegion destination{slot.instances, 0, static_cast<Uint32>(cell_bytes + marker_bytes)};
    SDL_UploadToGPUBuffer(copy, &upload, &destination, false);
    SDL_EndGPUCopyPass(copy);
    SDL_GPUColorTargetInfo target{};
    target.texture = slot.output; target.clear_color = {11 / 255.F, 19 / 255.F, 32 / 255.F, 1};
    target.load_op = SDL_GPU_LOADOP_CLEAR; target.store_op = SDL_GPU_STOREOP_STORE;
    auto* pass = SDL_BeginGPURenderPass(command, &target, 1, nullptr);
    if (!pass) return fail();
    const SDL_GPUViewport viewport{0, 0, Width, Height, 0, 1};
    const SDL_Rect scissor{24, 96, 1232, 520};
    SDL_SetGPUViewport(pass, &viewport); SDL_SetGPUScissor(pass, &scissor);
    SDL_BindGPUGraphicsPipeline(pass, state.pipeline);
    const SDL_GPUBufferBinding quad{state.quad, 0}, index{state.indices, 0};
    SDL_BindGPUVertexBuffers(pass, 0, &quad, 1);
    SDL_BindGPUIndexBuffer(pass, &index, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    auto uniforms = *projection;
    if (!cells.empty()) {
        const SDL_GPUBufferBinding instances{slot.instances, 0};
        SDL_BindGPUVertexBuffers(pass, 1, &instances, 1);
        SDL_PushGPUVertexUniformData(command, 0, &uniforms, sizeof uniforms);
        SDL_DrawGPUIndexedPrimitives(pass, 6, static_cast<Uint32>(cells.size()), 0, 0, 0);
    }
    if (!markers.empty()) {
        const SDL_GPUBufferBinding instances{slot.instances, static_cast<Uint32>(cell_bytes)};
        SDL_BindGPUVertexBuffers(pass, 1, &instances, 1);
        uniforms.canvas_marker[3] = 1;
        SDL_PushGPUVertexUniformData(command, 0, &uniforms, sizeof uniforms);
        SDL_DrawGPUIndexedPrimitives(pass, 6, static_cast<Uint32>(markers.size()), 0, 0, 0);
    }
    SDL_EndGPURenderPass(pass);
    copy = SDL_BeginGPUCopyPass(command);
    if (!copy) return fail();
    const SDL_GPUTextureRegion image{slot.output, 0, 0, 0, 0, 0, Width, Height, 1};
    const SDL_GPUTextureTransferInfo download{slot.download, 0, Width, Height};
    SDL_DownloadFromGPUTexture(copy, &image, &download);
    SDL_EndGPUCopyPass(copy);
    state.has_work = true;
    auto* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(command);
    if (!fence) { state.failed = true; return {SubmitStatus::device_error}; }
    if (slot.fence) SDL_ReleaseGPUFence(&state.device, slot.fence);
    slot.fence = fence;
    const auto id = state.schedule.Submit(*available, packet.GetTick());
    return {SubmitStatus::submitted, id, packet.GetTick(), *available, state.identity};
}
ReadbackStatus OffscreenRenderer::PollReadback(const Submission& submission, std::span<std::byte> destination) noexcept {
    auto& state = *state_;
    if (state.failed) return ReadbackStatus::device_error;
    if (submission.status != SubmitStatus::submitted || destination.size() < ReadbackBytes) return ReadbackStatus::invalid;
    if (submission.issuer.expired()) return ReadbackStatus::expired;
    if (submission.issuer.owner_before(state.identity) || state.identity.owner_before(submission.issuer))
        return ReadbackStatus::invalid;
    if (!state.schedule.Matches(submission.slot, submission.id) ||
        state.schedule.Get(submission.slot).tick != submission.tick) return ReadbackStatus::expired;
    if (!state.Complete(submission.slot)) return ReadbackStatus::pending;
    auto* mapped = SDL_MapGPUTransferBuffer(&state.device, state.slots[submission.slot].download, false);
    if (!mapped) { state.failed = true; return ReadbackStatus::device_error; }
    std::memcpy(destination.data(), mapped, ReadbackBytes);
    SDL_UnmapGPUTransferBuffer(&state.device, state.slots[submission.slot].download);
    return ReadbackStatus::complete;
}
}
