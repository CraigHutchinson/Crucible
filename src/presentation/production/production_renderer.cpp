/*
 * UI layout/material/scissor portions adapted from Filament v1.77.3 ImGuiHelper.
 * Copyright (C) 2018 The Android Open Source Project.
 * Licensed under the Apache License, Version 2.0; see LICENSE.filament.
 * http://www.apache.org/licenses/LICENSE-2.0
 * Distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND.
 */
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstring>
#include <filament/Camera.h>
#include <filament/Engine.h>
#include <filament/IndexBuffer.h>
#include <filament/IndirectLight.h>
#include <filament/InstanceBuffer.h>
#include <filament/LightManager.h>
#include <filament/Material.h>
#include <filament/MaterialInstance.h>
#include <filament/Options.h>
#include <filament/RenderableManager.h>
#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/SwapChain.h>
#include <filament/Texture.h>
#include <filament/TextureSampler.h>
#include <filament/VertexBuffer.h>
#include <filament/View.h>
#include <filament/Viewport.h>
#include <imgui.h>
#include <limits>
#include <math/mat3.h>
#include <math/mat4.h>
#include <math/quat.h>
#include <numbers>
#include <SDL3/SDL.h>
#if defined(__APPLE__)
#include <SDL3/SDL_metal.h>
#endif
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <utils/EntityManager.h>

#include "crucible/presentation/production/detail/frame_validation.hpp"
#include "crucible/presentation/production/detail/mesh_validation.hpp"
#include "crucible/presentation/production/detail/native_transforms.hpp"
#include "crucible/presentation/production/nanite_model.hpp"
#include "crucible/presentation/production/production_renderer.hpp"
#include "crucible/presentation/production/resources/crucible_materials.h"

namespace crucible::presentation::production
{
namespace
{
using namespace filament;
using namespace filament::math;
constexpr std::size_t maxVertices = 32768;
constexpr std::size_t maxIndices = 98304;
constexpr std::size_t maxCommands = 256;
constexpr std::size_t textureBytes = 64 * 1024 * 1024;
constexpr std::size_t residentBytes = 128 * 1024 * 1024;
constexpr std::size_t slotBytes = 15 * 1024 * 1024;
constexpr std::size_t externalStagingLimit = 192 * 1024 * 1024;
static_assert(3 * slotBytes + 2 * textureBytes + 16 * 1024 * 1024 <= externalStagingLimit);
struct NativeMeshVertex
{
    float3 position_{};
    quatf tangent_{};
    float2 roleIntensity_{}; // raw UV0 data; nanite_proxy.mat disables Filament's default y flip.
};
static_assert(sizeof(NativeMeshVertex) == 36);
static_assert(sizeof(ImDrawVert) == 20 && sizeof(ImDrawIdx) == 2);
static_assert(sizeof(NanitePose) == 36 && sizeof(mat4f) == 64 && sizeof(float4) == 16);
static_assert((sizeof(NanitePose) + sizeof(mat4f) + sizeof(float4)) * CinematicFrame::maxInstances <= 512 * 1024);

struct UploadPayload
{
    std::array<ImDrawVert, maxVertices> vertices_{};
    std::array<std::uint16_t, maxIndices> indices_{};
    std::array<NanitePose, CinematicFrame::maxInstances> poses_{};
    std::array<mat4f, CinematicFrame::maxInstances> transforms_{};
    std::array<float4, CinematicFrame::maxInstances> cores_{};
    std::array<NativeMeshVertex, NaniteMeshFrame::maxVertices> mesh_{};
};
static_assert(sizeof(UploadPayload) <= slotBytes);

/// Stable callback state; the owning renderer retains it through Engine teardown.
struct UploadSlot
{
    UploadPayload payload_{};
    std::array<std::byte, slotBytes - sizeof(UploadPayload) - 64> padding_{};
    std::atomic<unsigned> pending_{};
    VertexBuffer* vertices_{}; // owning; destroyed through the owning Engine.
    IndexBuffer* indices_{}; // owning; destroyed through the owning Engine.
    VertexBuffer* meshVertices_{}; // owning; destroyed through the owning Engine.
    float3 meshMinimum_{}, meshMaximum_{}; // validated copied mesh bounds for shadow fitting.
};
static_assert(sizeof(UploadSlot) <= slotBytes);

struct MeshVertex
{
    float3 position_{};
    quatf tangent_{};
    float2 uv_{};
};

/// Startup-only nanite and floor geometry, retained for asynchronous upload.
struct Mesh
{
    std::vector<MeshVertex> vertices_;
    std::vector<std::uint16_t> indices_;
    VertexBuffer* vertexBuffer_{}; // owning; destroyed through the owning Engine.
    IndexBuffer* indexBuffer_{}; // owning; destroyed through the owning Engine.

    void triangle(float3 a, float3 b, float3 c, float2 uv = {})
    {
        const auto tangent = normalize(b - a);
        const auto normal = normalize(cross(b - a, c - a));
        const auto orientation = mat3f::packTangentFrame(mat3f{tangent, cross(normal, tangent), normal});
        const auto first = static_cast<std::uint16_t>(vertices_.size());
        vertices_.insert(vertices_.end(), {{a, orientation, uv}, {b, orientation, uv}, {c, orientation, uv}});
        indices_.insert(indices_.end(), {first, static_cast<std::uint16_t>(first + 1), static_cast<std::uint16_t>(first + 2)});
    }

    void upload(Engine& engine)
    {
        vertexBuffer_ = VertexBuffer::Builder().vertexCount(static_cast<std::uint32_t>(vertices_.size()))
            .bufferCount(1)
            .attribute(VertexAttribute::POSITION, 0, VertexBuffer::AttributeType::FLOAT3,
                offsetof(MeshVertex, position_), sizeof(MeshVertex))
            .attribute(VertexAttribute::TANGENTS, 0, VertexBuffer::AttributeType::FLOAT4,
                offsetof(MeshVertex, tangent_), sizeof(MeshVertex))
            .attribute(VertexAttribute::UV0, 0, VertexBuffer::AttributeType::FLOAT2,
                offsetof(MeshVertex, uv_), sizeof(MeshVertex)).build(engine);
        indexBuffer_ = IndexBuffer::Builder().indexCount(static_cast<std::uint32_t>(indices_.size()))
            .bufferType(IndexBuffer::IndexType::USHORT).build(engine);
        if (!vertexBuffer_ || !indexBuffer_)
        {
            throw std::runtime_error("Production geometry initialization failed");
        }
        vertexBuffer_->setBufferAt(engine, 0, VertexBuffer::BufferDescriptor(vertices_.data(), vertices_.size() * sizeof(MeshVertex)));
        indexBuffer_->setBuffer(engine, IndexBuffer::BufferDescriptor(indices_.data(), indices_.size() * sizeof(std::uint16_t)));
    }
};

/** The procedural nanite at the desktop hero detail; its material role rides in uv0.x.
 * Housing panes are left out: this lit opaque material would hide the heart, and the
 * transparent housing has not been received in the native renderer yet.
 */
Mesh makeNanite()
{
    const auto detail = naniteDevicePresets()[3].hero_; // desktop class
    const auto source = buildNanite(detail);
    if (source.vertices_.size() > std::numeric_limits<std::uint16_t>::max())
    {
        throw std::length_error("Nanite detail exceeds the 16-bit index range");
    }
    Mesh mesh;
    mesh.vertices_.reserve(source.vertices_.size());
    for (const auto& vertex : source.vertices_)
    {
        const float3 normal{vertex.normal_[0], vertex.normal_[1], vertex.normal_[2]};
        const float3 reference = std::abs(normal.y) < 0.9F ? float3{0, 1, 0} : float3{1, 0, 0};
        const float3 tangent = normalize(cross(reference, normal));
        mesh.vertices_.push_back({{vertex.position_[0], vertex.position_[1], vertex.position_[2]},
            mat3f::packTangentFrame(mat3f{tangent, cross(normal, tangent), normal}),
            {static_cast<float>(vertex.material_), 0}});
    }
    mesh.indices_.reserve(source.indices_.size());
    for (std::size_t index = 0; index < source.indices_.size(); index += 3)
    {
        if (source.vertices_[source.indices_[index]].material_ == NaniteMaterial::Housing) continue;
        for (std::size_t corner = 0; corner < 3; ++corner)
        {
            mesh.indices_.push_back(static_cast<std::uint16_t>(source.indices_[index + corner]));
        }
    }
    return mesh;
}

Mesh makeFloor()
{
    Mesh mesh;
    mesh.triangle({-96, -0.01F, -96}, {-96, -0.01F, 160}, {96, -0.01F, 160});
    mesh.triangle({-96, -0.01F, -96}, {96, -0.01F, 160}, {96, -0.01F, -96});
    for (auto& vertex : mesh.vertices_)
    {
        vertex.uv_ = {(vertex.position_.x + 96) / 192, (vertex.position_.z + 96) / 256};
    }
    return mesh;
}

void releaseUpload(void*, std::size_t, void* user)
{
    static_cast<std::atomic<unsigned>*>(user)->fetch_sub(1, std::memory_order_release);
}

Material* material(Engine& engine, const std::uint8_t* bytes, std::size_t size)
{
    auto* result = Material::Builder().package(bytes, size).build(engine);
    if (!result)
    {
        throw std::runtime_error("Exact pinned production material was refused");
    }
    return result;
}
}

struct ProductionRenderer::State
{
    /// Engine ownership encloses all resources and callback storage below.
    struct EngineDeleter
    {
        void operator()(Engine* engine) const
        {
            Engine::destroy(&engine);
        }
    };
    /// One fixed UI texture record, including resources awaiting a qualified retirement.
    struct UiTexture
    {
        ImTextureData* source_{}; // non-owning; root context owns until SetStatus(Destroyed).
        Texture* texture_{}; // owning; destroyed through Engine, not exposed as an ImTextureID.
        std::size_t bytes_{};
        bool retired_{};
    };
    /// Shared geometry with owned automatic transform storage for a bounded batch.
    struct Batch
    {
        utils::Entity entity_{};
        InstanceBuffer* instances_{}; // owning; outlives associated renderable.
        MaterialInstance* material_{}; // owning; destroyed through Engine.
        std::size_t activeCount_{};
    };

    SDL_Window& window_;
    ImGuiContext& context_;
    std::unique_ptr<Engine, EngineDeleter> engine_{};
    Renderer* renderer_{}; // owning; destroyed through Engine.
    SwapChain* swapChain_{}; // owning; destroyed through Engine.
    Scene* scene_{}; // owning; destroyed through Engine.
    Scene* uiScene_{}; // owning; destroyed through Engine.
    View* view_{}; // owning; destroyed through Engine.
    View* uiView_{}; // owning; destroyed through Engine.
    Camera* camera_{}; // owning component of cameraEntity_, destroyed through Engine.
    Camera* uiCamera_{}; // owning component of uiCameraEntity_, destroyed through Engine.
    Texture* studioReflections_{}; // owning; outlives ambient_ reflection binding.
    IndirectLight* ambient_{}; // owning; destroyed through Engine.
    Texture* cores_{}; // owning; destroyed through Engine.
    Material* naniteMaterial_{}; // owning; outlives its instances.
    Material* proxyMaterial_{}; // owning; outlives its sole instance.
    MaterialInstance* proxyInstance_{}; // owning; destroyed through Engine.
    IndexBuffer* proxyIndices_{}; // owning; immutable identity indices.
    utils::Entity proxyEntity_{};
    std::vector<std::uint32_t> identityIndices_{};
    Material* floorMaterial_{}; // owning; outlives its instance.
    Material* uiMaterial_{}; // owning; outlives its instances.
    MaterialInstance* floorInstance_{}; // owning; destroyed through Engine.
    std::array<MaterialInstance*, maxCommands> uiInstances_{}; // owning entries; destroyed through Engine.
    utils::Entity cameraEntity_{}, uiCameraEntity_{}, uiEntity_{}, floorEntity_{}, keyEntity_{};
    Mesh nanite_{makeNanite()}, floor_{makeFloor()};
    std::vector<Batch> batches_{};
    std::array<std::unique_ptr<UploadSlot>, 3> slots_{};
    std::array<UiTexture, 4> textures_{};
    std::vector<std::byte> textureStaging_{textureBytes};
    std::atomic<unsigned> texturePending_{};
    CaptureResult capture_{};
    std::atomic<bool> captureComplete_{};
    std::optional<CaptureRequest> captureRequest_{};
    bool captureSubmitted_{};
    DrawableExtent extent_{};
    Metadata metadata_{};
    std::uint64_t generation_{1}, lastFrame_{};
    bool failed_{};
    ImGuiBackendFlags previousRendererFlags_{};
    const char* previousRendererName_{}; // non-owning; root context/backend owns.
    int previousTextureWidth_{}, previousTextureHeight_{};
    bool contextBound_{};
    void* nativeWindow_{}; // non-owning HWND; Apple layer owned by metalView_.
#if defined(__APPLE__)
    SDL_MetalView metalView_{}; // owning; released after Engine destruction.
#endif

    State(SDL_Window& window, ImGuiContext& context) : window_(window), context_(context)
    {
        for (auto& slot : slots_)
        {
            slot = std::make_unique<UploadSlot>();
        }
        capture_.rgba_.resize(textureBytes);
    }

    ~State()
    {
        // Engine destruction joins the driver and purges descriptors while all
        // slots, meshes, texture/capture storage and this callback state still live.
        if (engine_)
        {
            try
            {
                engine_->flushAndWait();
                cleanupResources();
            }
            catch (...)
            {
                // Unrecoverable framework failures are never reported as success.
                // Engine owns any remaining resources through its shutdown path.
            }
            engine_.reset();
        }
        if (contextBound_)
        {
            auto* previous = ImGui::GetCurrentContext();
            ImGui::SetCurrentContext(&context_);
            for (auto& texture : textures_)
            {
                if (texture.source_)
                {
                    texture.source_->SetTexID(ImTextureID_Invalid);
                    texture.source_->SetStatus(ImTextureStatus_Destroyed);
                }
            }
            auto& io = ImGui::GetIO();
            io.BackendFlags = (io.BackendFlags & ~(ImGuiBackendFlags_RendererHasTextures | ImGuiBackendFlags_RendererHasVtxOffset))
                | previousRendererFlags_;
            io.BackendRendererName = previousRendererName_;
            io.BackendRendererUserData = nullptr;
            ImGui::GetPlatformIO().Renderer_TextureMaxWidth = previousTextureWidth_;
            ImGui::GetPlatformIO().Renderer_TextureMaxHeight = previousTextureHeight_;
            ImGui::SetCurrentContext(previous);
        }
#if defined(__APPLE__)
        if (metalView_)
        {
            SDL_Metal_DestroyView(metalView_);
        }
#endif
    }

    void cleanupResources()
    {
        auto& entities = utils::EntityManager::get();
        for (auto& batch : batches_)
        {
            if (batch.entity_) engine_->destroy(batch.entity_);
            engine_->destroy(batch.instances_);
            engine_->destroy(batch.material_);
            if (batch.entity_) entities.destroy(batch.entity_);
        }
        for (auto entity : {uiEntity_, floorEntity_, keyEntity_, proxyEntity_})
        {
            if (entity)
            {
                engine_->destroy(entity);
                entities.destroy(entity);
            }
        }
        for (auto entity : {cameraEntity_, uiCameraEntity_})
        {
            if (entity)
            {
                engine_->destroyCameraComponent(entity);
                entities.destroy(entity);
            }
        }
        for (auto* instance : uiInstances_) engine_->destroy(instance);
        engine_->destroy(proxyInstance_);
        engine_->destroy(proxyMaterial_);
        engine_->destroy(proxyIndices_);
        engine_->destroy(floorInstance_);
        engine_->destroy(naniteMaterial_);
        engine_->destroy(floorMaterial_);
        engine_->destroy(uiMaterial_);
        engine_->destroy(cores_);
        for (auto& texture : textures_) engine_->destroy(texture.texture_);
        for (auto& slot : slots_)
        {
            engine_->destroy(slot->vertices_);
            engine_->destroy(slot->indices_);
            engine_->destroy(slot->meshVertices_);
        }
        for (auto* mesh : {&nanite_, &floor_})
        {
            engine_->destroy(mesh->vertexBuffer_);
            engine_->destroy(mesh->indexBuffer_);
        }
        engine_->destroy(ambient_);
        engine_->destroy(studioReflections_);
        engine_->destroy(view_);
        engine_->destroy(uiView_);
        engine_->destroy(scene_);
        engine_->destroy(uiScene_);
        engine_->destroy(renderer_);
        engine_->destroy(swapChain_);
    }

    void initialize()
    {
        if (ImGui::GetCurrentContext() != &context_ || ImGui::GetIO().BackendRendererUserData || ImGui::GetIO().BackendRendererName)
        {
            throw std::invalid_argument("Production renderer requires the sole current root context");
        }
        int width{}, height{};
        if (!SDL_GetWindowSizeInPixels(&window_, &width, &height) || width < 0 || height < 0 || width > 4096 || height > 4096)
        {
            throw std::invalid_argument("Production drawable exceeds the frozen bounds");
        }
        extent_ = {static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height)};
#if defined(_WIN32)
        metadata_.backend_ = Metadata::Backend::vulkan;
        nativeWindow_ = SDL_GetPointerProperty(SDL_GetWindowProperties(&window_), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        if (nativeWindow_) engine_.reset(Engine::create(Engine::Backend::VULKAN));
#elif defined(__APPLE__)
        metadata_.backend_ = Metadata::Backend::metal;
        metalView_ = SDL_Metal_CreateView(&window_);
        nativeWindow_ = metalView_ ? SDL_Metal_GetLayer(metalView_) : nullptr;
        if (nativeWindow_) engine_.reset(Engine::create(Engine::Backend::METAL));
#else
        throw std::runtime_error("Production platform surface is not received on this target");
#endif
        if (!nativeWindow_ || !engine_)
        {
            throw std::runtime_error("Filament production device/surface initialization failed");
        }
        const auto expectedBackend = metadata_.backend_ == Metadata::Backend::vulkan ? Engine::Backend::VULKAN : Engine::Backend::METAL;
        if (engine_->getBackend() != expectedBackend)
        {
            throw std::runtime_error("Production backend differs from the explicitly selected capability");
        }
        metadata_.automaticInstanceLimit_ = engine_->getMaxAutomaticInstances();
        metadata_.coreFloatTextureSupported_ = Texture::isTextureFormatSupported(*engine_, Texture::InternalFormat::RGBA32F);
        if (!metadata_.automaticInstanceLimit_ || metadata_.automaticInstanceLimit_ > CinematicFrame::maxInstances
            || !metadata_.coreFloatTextureSupported_)
        {
            throw std::runtime_error("Production core atlas or instancing capability is unsupported");
        }
        constexpr char pin[] = "d852e34cd5629a6f3851d613dcd17aa0ffd025a2";
        std::copy(std::begin(pin), std::end(pin), metadata_.sourcePin_.begin());
        if (extent_.width_ && extent_.height_)
            swapChain_ = engine_->createSwapChain(nativeWindow_, SwapChain::CONFIG_READABLE);
        renderer_ = engine_->createRenderer();
        scene_ = engine_->createScene();
        uiScene_ = engine_->createScene();
        view_ = engine_->createView();
        uiView_ = engine_->createView();
        if ((extent_.width_ && extent_.height_ && !swapChain_) || !renderer_ || !scene_ || !uiScene_ || !view_ || !uiView_)
        {
            throw std::runtime_error("Production frame resources could not be created");
        }
        auto& entities = utils::EntityManager::get();
        cameraEntity_ = entities.create();
        uiCameraEntity_ = entities.create();
        camera_ = engine_->createCamera(cameraEntity_);
        uiCamera_ = engine_->createCamera(uiCameraEntity_);
        if (!camera_ || !uiCamera_)
        {
            throw std::runtime_error("Production scene/UI cameras could not be created");
        }
        view_->setScene(scene_);
        view_->setCamera(camera_);
        view_->setAntiAliasing(View::AntiAliasing::FXAA);
        BloomOptions bloom;
        bloom.enabled = true;
        bloom.strength = 0.075F;
        bloom.resolution = 384;
        view_->setBloomOptions(bloom);
        AmbientOcclusionOptions ao;
        ao.enabled = true;
        ao.radius = 0.6F;
        view_->setAmbientOcclusionOptions(ao);
        uiView_->setScene(uiScene_);
        uiView_->setCamera(uiCamera_);
        uiView_->setPostProcessingEnabled(false);
        uiView_->setShadowingEnabled(false);
        uiView_->setBlendMode(View::BlendMode::TRANSLUCENT);
        Renderer::ClearOptions clear;
        clear.clear = true;
        clear.clearColor = {0.015F, 0.025F, 0.045F, 1};
        renderer_->setClearOptions(clear);
        // Immutable build-owned studio radiance: cmgen's real GGX prefilter plus
        // its pre-scaled irradiance SH. This is procedural light, not captured HDR.
        constexpr std::size_t shBytes = 9 * 3 * sizeof(float);
        constexpr std::size_t reflectionBytes = 6 * 4 * (32 * 32 + 16 * 16 + 8 * 8 + 4 * 4 + 2 * 2 + 1);
        static_assert(sizeof(float3) == 3 * sizeof(float));
        static_assert(std::endian::native == std::endian::little);
        if (CRUCIBLE_MATERIALS_STUDIO_ENVIRONMENT_SIZE != shBytes + reflectionBytes ||
            !Texture::isTextureFormatSupported(*engine_, Texture::InternalFormat::R11F_G11F_B10F))
            throw std::runtime_error("Bounded convolved studio environment is unsupported");
        studioReflections_ = Texture::Builder().width(32).height(32).levels(6)
            .sampler(Texture::Sampler::SAMPLER_CUBEMAP).format(Texture::InternalFormat::R11F_G11F_B10F).build(*engine_);
        if (!studioReflections_) throw std::runtime_error("Studio reflection cubemap initialization failed");
        std::size_t offset = shBytes;
        for (std::size_t level = 0; level < 6; ++level)
        {
            const auto dimension = static_cast<std::uint32_t>(32 >> level);
            const std::size_t bytes = 6 * 4 * dimension * dimension;
            // Packaged arrays remain immutable and alive through Engine teardown.
            Texture::PixelBufferDescriptor pixels(const_cast<std::uint8_t*>(CRUCIBLE_MATERIALS_STUDIO_ENVIRONMENT_DATA + offset),
                bytes, Texture::Format::RGB, Texture::Type::UINT_10F_11F_11F_REV, 1);
            studioReflections_->setImage(*engine_, level, 0, 0, 0, dimension, dimension, 6, std::move(pixels));
            offset += bytes;
        }
        std::array<float3, 9> irradiance{};
        std::memcpy(irradiance.data(), CRUCIBLE_MATERIALS_STUDIO_ENVIRONMENT_DATA, shBytes);
        for (const auto& coefficient : irradiance)
            for (std::size_t axis = 0; axis < 3; ++axis)
                if (!std::isfinite(coefficient[axis]) || std::abs(coefficient[axis]) > 64)
                    throw std::runtime_error("Studio irradiance coefficient is invalid");
        ambient_ = IndirectLight::Builder().irradiance(3, irradiance.data()).reflections(studioReflections_)
            .intensity(18000).build(*engine_);
        if (!ambient_)
        {
            throw std::runtime_error("Production ambient lighting could not be created");
        }
        scene_->setIndirectLight(ambient_);
        keyEntity_ = entities.create();
        if (LightManager::Builder(LightManager::Type::DIRECTIONAL).direction({-0.35F, -1, -0.45F})
                .color({0.8F, 0.91F, 1}).intensity(60000).castShadows(true).build(*engine_, keyEntity_) != LightManager::Builder::Success)
        {
            throw std::runtime_error("Production lighting could not be created");
        }
        scene_->addEntity(keyEntity_);
        naniteMaterial_ = material(*engine_, CRUCIBLE_MATERIALS_NANITE_DATA, CRUCIBLE_MATERIALS_NANITE_SIZE);
        floorMaterial_ = material(*engine_, CRUCIBLE_MATERIALS_FLOOR_DATA, CRUCIBLE_MATERIALS_FLOOR_SIZE);
        uiMaterial_ = material(*engine_, CRUCIBLE_MATERIALS_UI_BLIT_DATA, CRUCIBLE_MATERIALS_UI_BLIT_SIZE);
        proxyMaterial_ = material(*engine_, CRUCIBLE_MATERIALS_NANITE_PROXY_DATA, CRUCIBLE_MATERIALS_NANITE_PROXY_SIZE);
        proxyInstance_ = proxyMaterial_->createInstance();
        proxyEntity_ = entities.create();
        proxyIndices_ = IndexBuffer::Builder().indexCount(NaniteMeshFrame::maxVertices)
            .bufferType(IndexBuffer::IndexType::UINT).build(*engine_);
        if (!proxyInstance_ || !proxyIndices_) throw std::runtime_error("Production proxy mesh initialization failed");
        identityIndices_.resize(NaniteMeshFrame::maxVertices);
        for (std::size_t index = 0; index < identityIndices_.size(); ++index)
            identityIndices_[index] = static_cast<std::uint32_t>(index);
        proxyIndices_->setBuffer(*engine_, IndexBuffer::BufferDescriptor(identityIndices_.data(), identityIndices_.size() * sizeof(std::uint32_t)));
        cores_ = Texture::Builder().width(64).height(64).levels(1).sampler(Texture::Sampler::SAMPLER_2D)
            .format(Texture::InternalFormat::RGBA32F).build(*engine_);
        if (!cores_)
        {
            throw std::runtime_error("Production per-instance core atlas could not be created");
        }
        nanite_.upload(*engine_);
        floor_.upload(*engine_);
        floorInstance_ = floorMaterial_->createInstance();
        floorEntity_ = entities.create();
        if (!floorInstance_ || RenderableManager::Builder(1).boundingBox({{0, 0, 32}, {96, 1, 128}})
                .geometry(0, RenderableManager::PrimitiveType::TRIANGLES, floor_.vertexBuffer_, floor_.indexBuffer_)
                .material(0, floorInstance_).receiveShadows(true).build(*engine_, floorEntity_) != RenderableManager::Builder::Success)
        {
            throw std::runtime_error("Production floor could not be created");
        }
        scene_->addEntity(floorEntity_);
        const auto limit = metadata_.automaticInstanceLimit_;
        batches_.resize((CinematicFrame::maxInstances + limit - 1) / limit);
        for (std::size_t index = 0; index < batches_.size(); ++index)
        {
            auto& batch = batches_[index];
            batch.entity_ = entities.create();
            batch.instances_ = InstanceBuffer::Builder(limit).build(*engine_);
            batch.material_ = naniteMaterial_->createInstance();
            if (!batch.instances_ || !batch.material_)
            {
                throw std::runtime_error("Production instance batch could not be created");
            }
            batch.material_->setParameter("cores", cores_, TextureSampler(TextureSampler::MinFilter::NEAREST, TextureSampler::MagFilter::NEAREST));
            batch.material_->setParameter("batchBase", static_cast<std::int32_t>(index * limit));
        }
        for (auto& instance : uiInstances_)
        {
            instance = uiMaterial_->createInstance();
            if (!instance) throw std::runtime_error("Production UI material instance failed");
        }
        uiEntity_ = entities.create();
        uiScene_->addEntity(uiEntity_);
        for (auto& slot : slots_)
        {
            slot->vertices_ = VertexBuffer::Builder().vertexCount(maxVertices).bufferCount(1)
                .attribute(VertexAttribute::POSITION, 0, VertexBuffer::AttributeType::FLOAT2, offsetof(ImDrawVert, pos), sizeof(ImDrawVert))
                .attribute(VertexAttribute::UV0, 0, VertexBuffer::AttributeType::FLOAT2, offsetof(ImDrawVert, uv), sizeof(ImDrawVert))
                .attribute(VertexAttribute::COLOR, 0, VertexBuffer::AttributeType::UBYTE4, offsetof(ImDrawVert, col), sizeof(ImDrawVert))
                .normalized(VertexAttribute::COLOR).build(*engine_);
            slot->indices_ = IndexBuffer::Builder().indexCount(maxIndices).bufferType(IndexBuffer::IndexType::USHORT).build(*engine_);
            slot->meshVertices_ = VertexBuffer::Builder().vertexCount(NaniteMeshFrame::maxVertices).bufferCount(1)
                .attribute(VertexAttribute::POSITION, 0, VertexBuffer::AttributeType::FLOAT3, offsetof(NativeMeshVertex, position_), sizeof(NativeMeshVertex))
                .attribute(VertexAttribute::TANGENTS, 0, VertexBuffer::AttributeType::FLOAT4, offsetof(NativeMeshVertex, tangent_), sizeof(NativeMeshVertex))
                .attribute(VertexAttribute::UV0, 0, VertexBuffer::AttributeType::FLOAT2, offsetof(NativeMeshVertex, roleIntensity_), sizeof(NativeMeshVertex)).build(*engine_);
            if (!slot->vertices_ || !slot->indices_ || !slot->meshVertices_) throw std::runtime_error("Production UI buffers failed");
        }
        auto& io = ImGui::GetIO();
        previousRendererFlags_ = io.BackendFlags & (ImGuiBackendFlags_RendererHasTextures | ImGuiBackendFlags_RendererHasVtxOffset);
        previousRendererName_ = io.BackendRendererName;
        previousTextureWidth_ = ImGui::GetPlatformIO().Renderer_TextureMaxWidth;
        previousTextureHeight_ = ImGui::GetPlatformIO().Renderer_TextureMaxHeight;
        io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures | ImGuiBackendFlags_RendererHasVtxOffset;
        io.BackendRendererName = "Crucible_Filament_1.77.3_bounded";
        io.BackendRendererUserData = this;
        ImGui::GetPlatformIO().Renderer_TextureMaxWidth = 4096;
        ImGui::GetPlatformIO().Renderer_TextureMaxHeight = 4096;
        contextBound_ = true;
        setExtent();
        if (engine_->hasUnrecoverableFailure())
        {
            throw std::runtime_error("Production engine failed during initialization");
        }
    }

    void setExtent()
    {
        view_->setViewport({0, 0, extent_.width_, extent_.height_});
        uiView_->setViewport({0, 0, extent_.width_, extent_.height_});
        if (extent_.width_ && extent_.height_)
        {
            DynamicResolutionOptions quality;
            quality.enabled = true;
            const auto scale = std::min({1.0F, 1600.0F / extent_.width_, 900.0F / extent_.height_});
            quality.minScale = {scale, scale};
            quality.maxScale = {scale, scale};
            view_->setDynamicResolutionOptions(quality);
        }
    }

    FrameReceipt receipt(Status status, Reason reason = Reason::none, std::uint64_t frame = 0) const noexcept
    {
        return {status, reason, frame, generation_};
    }

    UiTexture* findTexture(ImTextureData* source)
    {
        for (auto& texture : textures_)
        {
            if (texture.source_ == source && !texture.retired_) return &texture;
        }
        return nullptr;
    }

    bool isKnownTexture(const ImDrawCmd& command, const ImDrawData& data)
    {
        if (command.TexRef._TexData)
        {
            auto* source = command.TexRef._TexData;
            if (auto* texture = findTexture(source))
                return source->Status != ImTextureStatus_Destroyed && source->Status != ImTextureStatus_WantDestroy
                    && source->TexID == static_cast<ImTextureID>(texture - textures_.data() + 1);
            if (source->Status != ImTextureStatus_WantCreate || !data.Textures) return false;
            return std::find(data.Textures->begin(), data.Textures->end(), source) != data.Textures->end();
        }
        const auto id = command.TexRef._TexID;
        return id >= 1 && id <= textures_.size() && textures_[id - 1].texture_ && !textures_[id - 1].retired_;
    }

    Reason validateUi(const ImDrawData& data)
    {
        if (!data.Valid || data.CmdListsCount < 0 || data.CmdListsCount > 16
            || data.CmdLists.Size != data.CmdListsCount || data.TotalVtxCount < 0 || data.TotalIdxCount < 0
            || data.TotalVtxCount > static_cast<int>(maxVertices) || data.TotalIdxCount > static_cast<int>(maxIndices)
            || !std::isfinite(data.DisplaySize.x) || !std::isfinite(data.DisplaySize.y)
            || data.DisplaySize.x <= 0 || data.DisplaySize.y <= 0
            || data.DisplayPos.x != 0 || data.DisplayPos.y != 0
            || !std::isfinite(data.FramebufferScale.x) || !std::isfinite(data.FramebufferScale.y)
            || data.FramebufferScale.x < 0.5F || data.FramebufferScale.x > 4
            || data.FramebufferScale.y < 0.5F || data.FramebufferScale.y > 4
            || std::abs(static_cast<double>(data.DisplaySize.x) * data.FramebufferScale.x - extent_.width_) > 0.5
            || std::abs(static_cast<double>(data.DisplaySize.y) * data.FramebufferScale.y - extent_.height_) > 0.5)
        {
            return Reason::invalidUi;
        }
        std::size_t totalVertices{}, totalIndices{}, totalCommands{};
        for (auto* list : data.CmdLists)
        {
            if (!list || list->VtxBuffer.Size < 0 || list->IdxBuffer.Size < 0 || list->CmdBuffer.Size < 0) return Reason::invalidUi;
            totalVertices += list->VtxBuffer.Size;
            totalIndices += list->IdxBuffer.Size;
            totalCommands += list->CmdBuffer.Size;
            if (totalVertices > maxVertices || totalIndices > maxIndices || totalCommands > maxCommands) return Reason::capacity;
            for (const auto& vertex : list->VtxBuffer)
            {
                if (!std::isfinite(vertex.pos.x) || !std::isfinite(vertex.pos.y)
                    || !std::isfinite(vertex.uv.x) || !std::isfinite(vertex.uv.y)) return Reason::invalidUi;
            }
            std::size_t cursor{};
            for (const auto& command : list->CmdBuffer)
            {
                if (command.UserCallback || command.IdxOffset != cursor || command.ElemCount % 3
                    || command.ElemCount > static_cast<std::size_t>(list->IdxBuffer.Size) - cursor
                    || !std::isfinite(command.ClipRect.x) || !std::isfinite(command.ClipRect.y)
                    || !std::isfinite(command.ClipRect.z) || !std::isfinite(command.ClipRect.w)
                    || command.ClipRect.z < command.ClipRect.x || command.ClipRect.w < command.ClipRect.y
                    || !isKnownTexture(command, data)) return Reason::invalidUi;
                for (std::size_t index = cursor; index < cursor + command.ElemCount; ++index)
                {
                    if (static_cast<std::uint64_t>(list->IdxBuffer[static_cast<int>(index)]) + command.VtxOffset
                        >= static_cast<std::uint64_t>(list->VtxBuffer.Size)) return Reason::invalidUi;
                }
                cursor += command.ElemCount;
            }
            if (cursor != static_cast<std::size_t>(list->IdxBuffer.Size)) return Reason::invalidUi;
        }
        if (totalVertices != static_cast<std::size_t>(data.TotalVtxCount) || totalIndices != static_cast<std::size_t>(data.TotalIdxCount)) return Reason::invalidUi;
        std::size_t actions{}, upload{}, resident{}, available{};
        for (const auto& texture : textures_)
        {
            resident += texture.bytes_;
            available += texture.bytes_ == 0;
        }
        if (data.Textures)
        {
            if (data.Textures->Size < 0 || data.Textures->Size > 8) return Reason::capacity;
            for (int index = 0; index < data.Textures->Size; ++index)
            {
                auto* source = (*data.Textures)[index];
                if (!source) return Reason::invalidUi;
                for (int prior = 0; prior < index; ++prior)
                {
                    if ((*data.Textures)[prior] == source) return Reason::invalidUi;
                }
                if (source->Status == ImTextureStatus_OK || source->Status == ImTextureStatus_Destroyed) continue;
                if (source->Status == ImTextureStatus_WantDestroy)
                {
                    if (!findTexture(source)) return Reason::invalidUi;
                    if (source->UnusedFrames > 0) ++actions;
                    continue;
                }
                if (source->Status != ImTextureStatus_WantCreate && source->Status != ImTextureStatus_WantUpdates) return Reason::invalidUi;
                if (source->Format != ImTextureFormat_RGBA32 || source->Width <= 0 || source->Height <= 0
                    || source->Width > 4096 || source->Height > 4096 || !source->Pixels) return Reason::capacity;
                const auto bytes = static_cast<std::size_t>(source->Width) * source->Height * 4;
                ++actions;
                upload += bytes;
                if (source->Status == ImTextureStatus_WantCreate)
                {
                    if (findTexture(source) || source->TexID != ImTextureID_Invalid || !available) return Reason::capacity;
                    --available;
                    resident += bytes;
                }
                else
                {
                    const auto* texture = findTexture(source);
                    if (!texture || texture->bytes_ != bytes || source->TexID != static_cast<ImTextureID>(texture - textures_.data() + 1)) return Reason::invalidUi;
                }
                if (upload > textureBytes || resident > residentBytes) return Reason::capacity;
            }
        }
        if (actions > 4) return Reason::capacity;
        if (upload && texturePending_.load(std::memory_order_acquire)) return Reason::busy;
        return Reason::none;
    }

    void updateTextures(const ImDrawData& data)
    {
        if (!data.Textures) return;
        std::size_t offset{};
        for (auto* source : *data.Textures)
        {
            if (source->Status == ImTextureStatus_WantDestroy && source->UnusedFrames > 0)
            {
                auto* texture = findTexture(source);
                engine_->destroy(texture->texture_);
                texture->texture_ = nullptr;
                texture->source_ = nullptr;
                texture->retired_ = true; // still charged until qualified drain.
                source->SetTexID(ImTextureID_Invalid);
                source->SetStatus(ImTextureStatus_Destroyed);
                continue;
            }
            if (source->Status != ImTextureStatus_WantCreate && source->Status != ImTextureStatus_WantUpdates) continue;
            auto* texture = findTexture(source);
            const auto bytes = static_cast<std::size_t>(source->Width) * source->Height * 4;
            if (!texture)
            {
                texture = &*std::find_if(textures_.begin(), textures_.end(), [](const auto& value) { return value.bytes_ == 0; });
                texture->texture_ = Texture::Builder().width(source->Width).height(source->Height).levels(1)
                    .sampler(Texture::Sampler::SAMPLER_2D).format(Texture::InternalFormat::RGBA8).build(*engine_);
                if (!texture->texture_) throw std::runtime_error("Production UI texture creation failed");
                texture->source_ = source;
                texture->bytes_ = bytes;
            }
            std::memcpy(textureStaging_.data() + offset, source->Pixels, bytes);
            texturePending_.fetch_add(1, std::memory_order_relaxed);
            Texture::PixelBufferDescriptor descriptor(textureStaging_.data() + offset, bytes,
                Texture::Format::RGBA, Texture::Type::UBYTE, 1, 0, 0, 0, releaseUpload, &texturePending_);
            texture->texture_->setImage(*engine_, 0, std::move(descriptor));
            offset += bytes;
            source->SetTexID(static_cast<ImTextureID>(texture - textures_.data() + 1));
            source->SetStatus(ImTextureStatus_OK);
        }
    }

    void copyUi(UploadSlot& slot, const ImDrawData& data)
    {
        std::size_t vertices{}, indices{};
        for (auto* list : data.CmdLists)
        {
            if (list->VtxBuffer.Size)
                std::memcpy(slot.payload_.vertices_.data() + vertices, list->VtxBuffer.Data, list->VtxBuffer.Size * sizeof(ImDrawVert));
            for (const auto& command : list->CmdBuffer)
            {
                for (std::size_t index = 0; index < command.ElemCount; ++index)
                {
                    slot.payload_.indices_[indices++] = static_cast<std::uint16_t>(
                        vertices + command.VtxOffset + list->IdxBuffer[static_cast<int>(command.IdxOffset + index)]);
                }
            }
            vertices += list->VtxBuffer.Size;
        }
    }

    void prepareScene(UploadSlot& slot, const CinematicFrame& frame)
    {
        auto& manager = engine_->getRenderableManager();
        scene_->remove(proxyEntity_);
        manager.destroy(proxyEntity_);
        AmbientOcclusionOptions ao;
        ao.enabled = true;
        ao.radius = 0.6F;
        view_->setAmbientOcclusionOptions(ao);
        auto& lights = engine_->getLightManager();
        const auto key = lights.getInstance(keyEntity_);
        lights.setDirection(key, {-0.35F, -1, -0.45F});
        lights.setColor(key, {0.8F, 0.91F, 1});
        lights.setShadowCaster(key, true);
        const auto limit = metadata_.automaticInstanceLimit_;
        for (std::size_t index = 0; index < batches_.size(); ++index)
        {
            auto& batch = batches_[index];
            const auto base = index * limit;
            const auto count = base < frame.poses_.size() ? std::min(limit, frame.poses_.size() - base) : 0;
            if (count != batch.activeCount_)
            {
                scene_->remove(batch.entity_);
                manager.destroy(batch.entity_);
                if (count)
                {
                    if (RenderableManager::Builder(1).boundingBox({{0, 0, 0}, {540, 540, 540}}).culling(false)
                        .geometry(0, RenderableManager::PrimitiveType::TRIANGLES, nanite_.vertexBuffer_, nanite_.indexBuffer_)
                        .material(0, batch.material_).instances(count, batch.instances_).castShadows(true).receiveShadows(true)
                        .build(*engine_, batch.entity_) != RenderableManager::Builder::Success)
                        throw std::runtime_error("Production instance renderable failed");
                    scene_->addEntity(batch.entity_);
                }
                batch.activeCount_ = count;
            }
            if (count)
            {
                scene_->addEntity(batch.entity_);
                batch.instances_->setLocalTransforms(slot.payload_.transforms_.data() + base, count);
                batch.material_->setParameter("glow", frame.glowIntensity_);
            }
        }
        slot.pending_.fetch_add(1, std::memory_order_relaxed);
        Texture::PixelBufferDescriptor descriptor(slot.payload_.cores_.data(), sizeof(slot.payload_.cores_),
            Texture::Format::RGBA, Texture::Type::FLOAT, 1, 0, 0, 0, releaseUpload, &slot.pending_);
        cores_->setImage(*engine_, 0, std::move(descriptor));
        prepareCamera(frame.camera_, frame.keyIntensity_);
    }

    void prepareCamera(const CinematicCamera& camera, float keyIntensity)
    {
        const auto& eye = camera.eye_;
        const auto& target = camera.target_;
        if (camera.orthographic_)
        {
            const auto extent = *camera.orthographic_;
            camera_->setProjection(Camera::Projection::ORTHO, -extent.halfWidth_, extent.halfWidth_,
                -extent.halfHeight_, extent.halfHeight_, 0.1, 2048);
        }
        else camera_->setProjection(camera.verticalFovRadians_ * 180.0 / std::numbers::pi,
            static_cast<double>(extent_.width_) / extent_.height_, 0.1, 2048, Camera::Fov::VERTICAL);
        camera_->lookAt({eye[0], eye[1], eye[2]}, {target[0], target[1], target[2]});
        auto& lights = engine_->getLightManager();
        lights.setIntensity(lights.getInstance(keyEntity_), keyIntensity * 15000);
    }

    void prepareScene(UploadSlot& slot, const NaniteMeshFrame& frame)
    {
        auto& manager = engine_->getRenderableManager();
        for (auto& batch : batches_) scene_->remove(batch.entity_);
        // Mission-scale AO has no qualified radius for the sub-unit core windows.
        // Keep it off in this mesh receiver until a matching radius is received.
        AmbientOcclusionOptions ao;
        ao.enabled = false;
        view_->setAmbientOcclusionOptions(ao);
        // The default pinned engine evaluates only the dominant directional
        // light. Aim that startup key slightly above/left of the camera; a second
        // directional fill would be ignored. Dense sub-unit self-shadow bias
        // remains unqualified, so this receiving arm retains lit PBR without it.
        auto& lights = engine_->getLightManager();
        const auto key = lights.getInstance(keyEntity_);
        const auto& eye = frame.camera_.eye_;
        const auto& target = frame.camera_.target_;
        const auto forward = normalize(float3{target[0] - eye[0], target[1] - eye[1], target[2] - eye[2]});
        const auto right = normalize(cross(forward, float3{0, 1, 0}));
        const auto up = cross(right, forward);
        lights.setDirection(key, normalize(forward + right * 0.25F - up * 0.18F));
        lights.setColor(key, {0.9F, 0.95F, 1});
        lights.setShadowCaster(key, false);
        scene_->remove(proxyEntity_);
        manager.destroy(proxyEntity_);
        if (!frame.vertices_.empty())
        {
            slot.pending_.fetch_add(1, std::memory_order_relaxed);
            slot.meshVertices_->setBufferAt(*engine_, 0, VertexBuffer::BufferDescriptor(slot.payload_.mesh_.data(),
                frame.vertices_.size() * sizeof(NativeMeshVertex), releaseUpload, &slot.pending_));
            proxyInstance_->setParameter("glow", frame.glowIntensity_);
            if (RenderableManager::Builder(1).boundingBox({(slot.meshMinimum_ + slot.meshMaximum_) * 0.5F,
                    (slot.meshMaximum_ - slot.meshMinimum_) * 0.5F + float3{0.001F}}).culling(false)
                .geometry(0, RenderableManager::PrimitiveType::TRIANGLES, slot.meshVertices_, proxyIndices_, 0, static_cast<std::uint32_t>(frame.vertices_.size()))
                .material(0, proxyInstance_).castShadows(true).receiveShadows(true)
                .build(*engine_, proxyEntity_) != RenderableManager::Builder::Success)
                throw std::runtime_error("Production proxy mesh renderable failed");
            scene_->addEntity(proxyEntity_);
        }
        prepareCamera(frame.camera_, frame.keyIntensity_);
    }

    template<class Frame>
    FrameReceipt submit(const Frame& frame, const ImDrawData& drawData, std::uint64_t frameId);

    void prepareUi(UploadSlot& slot, const ImDrawData& data)
    {
        auto& manager = engine_->getRenderableManager();
        manager.destroy(uiEntity_);
        if (!data.TotalIdxCount) return;
        slot.pending_.fetch_add(1, std::memory_order_relaxed);
        slot.vertices_->setBufferAt(*engine_, 0, VertexBuffer::BufferDescriptor(slot.payload_.vertices_.data(),
            data.TotalVtxCount * sizeof(ImDrawVert), releaseUpload, &slot.pending_));
        slot.pending_.fetch_add(1, std::memory_order_relaxed);
        slot.indices_->setBuffer(*engine_, IndexBuffer::BufferDescriptor(slot.payload_.indices_.data(),
            data.TotalIdxCount * sizeof(ImDrawIdx), releaseUpload, &slot.pending_));
        std::size_t primitives{};
        for (auto* list : data.CmdLists)
            for (const auto& command : list->CmdBuffer) primitives += command.ElemCount != 0;
        if (!primitives) return;
        RenderableManager::Builder builder(primitives);
        builder.boundingBox({{0, 0, 0}, {10000, 10000, 10000}}).culling(false).castShadows(false).receiveShadows(false);
        std::size_t primitive{}, offset{};
        for (auto* list : data.CmdLists)
        {
            for (const auto& command : list->CmdBuffer)
            {
                if (!command.ElemCount) continue;
                const auto left = std::clamp((static_cast<double>(command.ClipRect.x) - data.DisplayPos.x) * data.FramebufferScale.x, 0.0, static_cast<double>(extent_.width_));
                const auto right = std::clamp((static_cast<double>(command.ClipRect.z) - data.DisplayPos.x) * data.FramebufferScale.x, 0.0, static_cast<double>(extent_.width_));
                const auto top = std::clamp((static_cast<double>(command.ClipRect.y) - data.DisplayPos.y) * data.FramebufferScale.y, 0.0, static_cast<double>(extent_.height_));
                const auto bottom = std::clamp((static_cast<double>(command.ClipRect.w) - data.DisplayPos.y) * data.FramebufferScale.y, 0.0, static_cast<double>(extent_.height_));
                auto* instance = uiInstances_[primitive];
                instance->setScissor(static_cast<std::uint32_t>(left), static_cast<std::uint32_t>(extent_.height_ - bottom),
                    static_cast<std::uint32_t>(right - left), static_cast<std::uint32_t>(bottom - top));
                const auto id = command.GetTexID();
                instance->setParameter("albedo", textures_[id - 1].texture_,
                    TextureSampler(TextureSampler::MinFilter::LINEAR, TextureSampler::MagFilter::LINEAR));
                builder.geometry(primitive, RenderableManager::PrimitiveType::TRIANGLES, slot.vertices_, slot.indices_, offset, command.ElemCount)
                    .blendOrder(primitive, static_cast<std::uint16_t>(primitive)).material(primitive, instance);
                offset += command.ElemCount;
                ++primitive;
            }
        }
        if (builder.build(*engine_, uiEntity_) != RenderableManager::Builder::Success)
            throw std::runtime_error("Production UI renderable failed");
        uiCamera_->setProjection(Camera::Projection::ORTHO, data.DisplayPos.x, data.DisplayPos.x + data.DisplaySize.x,
            data.DisplayPos.y + data.DisplaySize.y, data.DisplayPos.y, 0, 1);
    }
};

ProductionRenderer::ProductionRenderer(SDL_Window& window, ImGuiContext& context)
    : state_(std::make_unique<State>(window, context))
{
    state_->initialize();
}
ProductionRenderer::~ProductionRenderer() = default;

ProductionRenderer::Status ProductionRenderer::beginUiFrame()
{
    auto& state = *state_;
    if (state.failed_ || state.engine_->hasUnrecoverableFailure())
    {
        state.failed_ = true;
        return Status::failed;
    }
    if (ImGui::GetCurrentContext() != &state.context_) return Status::rejected;
    try
    {
        state.engine_->pumpMessageQueues();
        if (std::any_of(state.textures_.begin(), state.textures_.end(), [](const auto& texture) { return texture.retired_; }))
        {
            const auto drain = tryDrain(std::chrono::milliseconds{1});
            if (drain != DrainStatus::drained) return drain == DrainStatus::failed ? Status::failed : Status::skipped;
        }
        if (state.engine_->hasUnrecoverableFailure())
        {
            state.failed_ = true;
            return Status::failed;
        }
        return Status::scheduled;
    }
    catch (...)
    {
        state.failed_ = true;
        return Status::failed;
    }
}

template<class Frame>
ProductionRenderer::FrameReceipt ProductionRenderer::State::submit(const Frame& frame,
    const ImDrawData& drawData, std::uint64_t frameId)
{
    auto& state = *this;
    if (state.failed_ || state.engine_->hasUnrecoverableFailure())
    {
        state.failed_ = true;
        return state.receipt(Status::failed, Reason::backend, frameId);
    }
    if (ImGui::GetCurrentContext() != &state.context_) return state.receipt(Status::rejected, Reason::context, frameId);
    if (!frameId || frameId <= state.lastFrame_ || (state.captureRequest_ && !state.captureSubmitted_ && frameId > state.captureRequest_->frameId_))
        return state.receipt(Status::rejected, Reason::frameIdentity, frameId);
    if (!state.extent_.width_ || !state.extent_.height_) return state.receipt(Status::suspended, Reason::surface, frameId);
    if constexpr (std::is_same_v<Frame, NaniteMeshFrame>)
        if (frame.vertices_.size() > NaniteMeshFrame::maxVertices)
            return state.receipt(Status::rejected, Reason::capacity, frameId);
    if (!detail::isValidFrame(frame)) return state.receipt(Status::rejected, Reason::invalidScene, frameId);
    const auto validation = state.validateUi(drawData);
    if (validation != Reason::none) return state.receipt(validation == Reason::busy ? Status::skipped : Status::rejected, validation, frameId);
    auto found = std::find_if(state.slots_.begin(), state.slots_.end(), [](const auto& slot)
        { return slot->pending_.load(std::memory_order_acquire) == 0; });
    if (found == state.slots_.end()) return state.receipt(Status::skipped, Reason::busy, frameId);
    auto& slot = **found;
    if constexpr (std::is_same_v<Frame, CinematicFrame>)
    {
        for (std::size_t index = 0; index < frame.poses_.size(); ++index)
        {
            slot.payload_.poses_[index] = frame.poses_[index];
            const auto model = detail::tryBuildTransform(frame.poses_[index]);
            if (!model) return state.receipt(Status::rejected, Reason::invalidScene, frameId);
            slot.payload_.transforms_[index] = *model;
            slot.payload_.cores_[index] = {frame.poses_[index].coreIntensity_, 0, 0, 0};
        }
        std::fill(slot.payload_.cores_.begin() + frame.poses_.size(), slot.payload_.cores_.end(), float4{});
    }
    else
    {
        for (std::size_t index = 0; index < frame.vertices_.size(); ++index)
        {
            const auto& vertex = frame.vertices_[index];
            slot.payload_.mesh_[index] = {{vertex.position_[0], vertex.position_[1], vertex.position_[2]},
                detail::buildTangent(vertex.normal_),
                {static_cast<float>(vertex.role_), vertex.intensity_}};
            const auto& position = slot.payload_.mesh_[index].position_;
            if (index == 0) slot.meshMinimum_ = slot.meshMaximum_ = position;
            else for (std::size_t axis = 0; axis < 3; ++axis)
            {
                slot.meshMinimum_[axis] = std::min(slot.meshMinimum_[axis], position[axis]);
                slot.meshMaximum_[axis] = std::max(slot.meshMaximum_[axis], position[axis]);
            }
        }
    }
    state.copyUi(slot, drawData);
    try
    {
        state.updateTextures(drawData);
        // The pinned beginFrame commits material descriptors through Engine::prepare.
        // Populate every used sampler and resource before that commit, including the
        // first UI frame; changing bindings afterward misses the current frame commit.
        state.prepareScene(slot, frame);
        state.prepareUi(slot, drawData);
        if (!state.renderer_->beginFrame(state.swapChain_))
        {
            state.failed_ = state.engine_->hasUnrecoverableFailure();
            return state.receipt(state.failed_ ? Status::failed : Status::skipped,
                state.failed_ ? Reason::backend : Reason::busy, frameId);
        }
        // An accepted beginFrame always receives endFrame even if subsequent work
        // throws. The guard settles before the outer catch reports failure.
        struct EndFrame
        {
            State& state_;
            ~EndFrame()
            {
                try { state_.renderer_->endFrame(); }
                catch (...) { state_.failed_ = true; }
            }
        };
        {
            EndFrame finish{state};
            state.renderer_->render(state.view_);
            state.renderer_->render(state.uiView_);
            if (state.captureRequest_ && !state.captureSubmitted_ && state.captureRequest_->frameId_ == frameId)
            {
                state.capture_.frameId_ = frameId;
                state.capture_.surfaceGeneration_ = state.generation_;
                state.capture_.extent_ = state.extent_;
                state.captureComplete_.store(false, std::memory_order_relaxed);
                Texture::PixelBufferDescriptor descriptor(state.capture_.rgba_.data(),
                    static_cast<std::size_t>(state.extent_.width_) * state.extent_.height_ * 4,
                    Texture::Format::RGBA, Texture::Type::UBYTE, 1, 0, 0, 0,
                    [](void*, std::size_t, void* user) { static_cast<std::atomic<bool>*>(user)->store(true, std::memory_order_release); },
                    &state.captureComplete_);
                state.captureSubmitted_ = true;
                state.renderer_->readPixels(0, 0, state.extent_.width_, state.extent_.height_, std::move(descriptor));
            }
        }
        state.failed_ = state.failed_ || state.engine_->hasUnrecoverableFailure();
        if (state.failed_) return state.receipt(Status::failed, Reason::backend, frameId);
        state.lastFrame_ = frameId;
        return state.receipt(Status::scheduled, Reason::none, frameId);
    }
    catch (...)
    {
        state.failed_ = true;
        return state.receipt(Status::failed, Reason::backend, frameId);
    }
}

ProductionRenderer::FrameReceipt ProductionRenderer::tryRender(const CinematicFrame& frame,
    const ImDrawData& drawData, std::uint64_t frameId)
{
    return state_->submit(frame, drawData, frameId);
}
ProductionRenderer::FrameReceipt ProductionRenderer::tryRender(const NaniteMeshFrame& frame,
    const ImDrawData& drawData, std::uint64_t frameId)
{
    return state_->submit(frame, drawData, frameId);
}

ProductionRenderer::FrameReceipt ProductionRenderer::tryResize(DrawableExtent extent)
{
    auto& state = *state_;
    if (state.failed_ || state.engine_->hasUnrecoverableFailure())
    {
        state.failed_ = true;
        return state.receipt(Status::failed, Reason::backend);
    }
    if (extent.width_ > 4096 || extent.height_ > 4096) return state.receipt(Status::rejected, Reason::capacity);
    if (ImGui::GetCurrentContext() != &state.context_) return state.receipt(Status::rejected, Reason::context);
    if (state.captureRequest_) return state.receipt(Status::skipped, Reason::busy);
    if (state.generation_ == std::numeric_limits<std::uint64_t>::max()) return state.receipt(Status::failed, Reason::surface);
    const auto drained = tryDrain(std::chrono::seconds{2});
    if (drained != DrainStatus::drained)
        return state.receipt(drained == DrainStatus::failed ? Status::failed : Status::skipped,
            drained == DrainStatus::failed ? Reason::backend : Reason::busy);
    try
    {
        if (extent.width_ && extent.height_)
        {
            state.engine_->destroy(state.swapChain_);
            state.swapChain_ = nullptr;
            if (tryDrain(std::chrono::seconds{2}) != DrainStatus::drained)
            {
                state.failed_ = true;
                return state.receipt(Status::failed, Reason::surface);
            }
            state.swapChain_ = state.engine_->createSwapChain(state.nativeWindow_, SwapChain::CONFIG_READABLE);
            if (!state.swapChain_ || state.engine_->hasUnrecoverableFailure())
            {
                state.failed_ = true;
                return state.receipt(Status::failed, Reason::surface);
            }
        }
        state.extent_ = extent;
        ++state.generation_;
        state.setExtent();
        return state.receipt(extent.width_ && extent.height_ ? Status::scheduled : Status::suspended);
    }
    catch (...)
    {
        state.failed_ = true;
        return state.receipt(Status::failed, Reason::backend);
    }
}

ProductionRenderer::FrameReceipt ProductionRenderer::tryCapture(CaptureRequest request)
{
    auto& state = *state_;
    if (state.failed_ || state.engine_->hasUnrecoverableFailure())
    {
        state.failed_ = true;
        return state.receipt(Status::failed, Reason::backend, request.frameId_);
    }
    if (ImGui::GetCurrentContext() != &state.context_) return state.receipt(Status::rejected, Reason::context, request.frameId_);
    if (state.captureRequest_) return state.receipt(Status::skipped, Reason::busy, request.frameId_);
    if (!request.frameId_ || request.frameId_ <= state.lastFrame_ || request.surfaceGeneration_ != state.generation_)
        return state.receipt(Status::rejected, Reason::frameIdentity, request.frameId_);
    if (!state.extent_.width_ || !state.extent_.height_) return state.receipt(Status::suspended, Reason::surface, request.frameId_);
    state.capture_.rgba_.resize(textureBytes);
    state.captureRequest_ = request;
    state.captureSubmitted_ = false;
    state.captureComplete_.store(false, std::memory_order_relaxed);
    return state.receipt(Status::scheduled, Reason::none, request.frameId_);
}

std::optional<ProductionRenderer::CaptureResult> ProductionRenderer::takeCapture()
{
    auto& state = *state_;
    if (state.failed_ || state.engine_->hasUnrecoverableFailure())
    {
        state.failed_ = true;
        return std::nullopt;
    }
    if (!state.captureSubmitted_ || !state.captureComplete_.load(std::memory_order_acquire)) return std::nullopt;
    state.capture_.rgba_.resize(static_cast<std::size_t>(state.capture_.extent_.width_) * state.capture_.extent_.height_ * 4);
    auto result = std::move(state.capture_);
    state.capture_ = {};
    state.captureRequest_.reset();
    state.captureSubmitted_ = false;
    return result;
}

ProductionRenderer::DrainStatus ProductionRenderer::tryDrain(std::chrono::nanoseconds timeout)
{
    auto& state = *state_;
    if (state.failed_ || state.engine_->hasUnrecoverableFailure())
    {
        state.failed_ = true;
        return DrainStatus::failed;
    }
    if (timeout.count() < 0) return DrainStatus::failed;
    try
    {
        if (!state.engine_->flushAndWait(static_cast<std::uint64_t>(timeout.count())))
        {
            state.failed_ = state.engine_->hasUnrecoverableFailure();
            return state.failed_ ? DrainStatus::failed : DrainStatus::timeout;
        }
        state.engine_->pumpMessageQueues();
        if (state.engine_->hasUnrecoverableFailure())
        {
            state.failed_ = true;
            return DrainStatus::failed;
        }
        for (auto& texture : state.textures_)
        {
            if (texture.retired_) texture = {};
        }
        return DrainStatus::drained;
    }
    catch (...)
    {
        state.failed_ = true;
        return DrainStatus::failed;
    }
}

ProductionRenderer::Metadata ProductionRenderer::getMetadata() const noexcept
{
    auto result = state_->metadata_;
    result.extent_ = state_->extent_;
    result.surfaceGeneration_ = state_->generation_;
    return result;
}
}
