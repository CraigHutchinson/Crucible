#include "FaultController.hpp"
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <unistd.h>
extern "C" bool SDLCALL __real_SDL_WaitForGPUIdle(SDL_GPUDevice*);
namespace gpu_fault {
namespace { thread_local Controller* active{}; }
Controller* Active() noexcept { return active; }
Controller::Controller(SDL_GPUDevice& device) : device_{&device} {
    if (active) throw std::logic_error("nested GPU fault controller");
    active = this;
}
Controller::~Controller() { active = nullptr; }
void Controller::Arm(Call call, std::size_t ordinal) noexcept {
    armed_ = call; ordinal_ = ordinal; seen_ = 0; triggered_ = false;
}
bool Controller::Hit(Call call) noexcept {
    ++calls_[static_cast<std::size_t>(call)];
    if (call != armed_ || ++seen_ != ordinal_) return false;
    triggered_ = true; return true;
}
std::size_t Controller::Calls(Call call) const noexcept { return calls_[static_cast<std::size_t>(call)]; }
void Controller::Created(void* pointer, Kind kind) noexcept {
    if (!pointer) return;
    if (Owns(pointer, kind)) { good_ = false; return; }
    for (auto& h : handles_) {
        if (!h.pointer) { h = {pointer, kind, false}; ++created_; return; }
    }
    good_ = false;
}
void Controller::Released(void* pointer, Kind kind) noexcept {
    if (forbid_release) { std::fputs("FAULT_UNSAFE_RELEASE\n", stderr); std::fflush(stderr); std::_Exit(87); }
    for (auto& h : handles_) {
        if (h.pointer == pointer && h.kind == kind) {
            if (h.mapped) good_ = false;
            h = {}; ++releases_; return;
        }
    }
    good_ = false;
}
void Controller::Mapped(void* pointer) noexcept {
    for (auto& h : handles_) if (h.pointer == pointer && h.kind == Kind::transfer) {
        if (h.mapped) good_ = false;
        h.mapped = true; return;
    }
    good_ = false;
}
void Controller::Unmapped(void* pointer) noexcept {
    for (auto& h : handles_) if (h.pointer == pointer && h.kind == Kind::transfer) {
        if (!h.mapped) good_ = false;
        h.mapped = false; return;
    }
    good_ = false;
}
bool Controller::Owns(void* pointer, Kind kind) const noexcept {
    if (!pointer) return false;
    for (const auto& h : handles_) if (h.pointer == pointer && h.kind == kind) return true;
    return false;
}
bool Controller::Balanced() const noexcept {
    if (!good_ || created_ != releases_) return false;
    for (const auto& h : handles_) if (h.pointer) return false;
    return true;
}
bool Controller::PhysicalWait() noexcept {
    const bool done = __real_SDL_WaitForGPUIdle(device_);
    if (done) ++successful_waits;
    return done;
}
}
namespace {
void Known(gpu_fault::Controller* c, void* p, gpu_fault::Kind kind) noexcept {
    if (c && !c->Owns(p, kind)) {
        std::fputs("FAULT_UNEXPECTED_HANDLE\n", stderr); std::fflush(stderr); std::abort();
    }
}
gpu_fault::Controller* Command(SDL_GPUCommandBuffer* p) noexcept {
    auto* c = gpu_fault::Active(); Known(c, p, gpu_fault::Kind::command); return c;
}
gpu_fault::Controller* For(SDL_GPUDevice* d) noexcept {
    auto* c = gpu_fault::Active();
    return c && c->Device() == d ? c : nullptr;
}
}
extern "C" SDL_GPUShader* SDLCALL __real_SDL_CreateGPUShader(SDL_GPUDevice*, const SDL_GPUShaderCreateInfo*);
extern "C" SDL_GPUShader* SDLCALL __wrap_SDL_CreateGPUShader(SDL_GPUDevice* d, const SDL_GPUShaderCreateInfo* i) {
    auto* c = For(d);
    if (c && c->Hit(gpu_fault::Call::create)) { SDL_SetError("injected SDL_CreateGPUShader"); return nullptr; }
    auto* p = __real_SDL_CreateGPUShader(d, i);
    if (c) c->Created(p, gpu_fault::Kind::shader);
    return p;
}
extern "C" SDL_GPUGraphicsPipeline* SDLCALL __real_SDL_CreateGPUGraphicsPipeline(SDL_GPUDevice*, const SDL_GPUGraphicsPipelineCreateInfo*);
extern "C" SDL_GPUGraphicsPipeline* SDLCALL __wrap_SDL_CreateGPUGraphicsPipeline(SDL_GPUDevice* d, const SDL_GPUGraphicsPipelineCreateInfo* i) {
    auto* c = For(d);
    if (c && c->Hit(gpu_fault::Call::create)) { SDL_SetError("injected SDL_CreateGPUGraphicsPipeline"); return nullptr; }
    auto* p = __real_SDL_CreateGPUGraphicsPipeline(d, i);
    if (c) c->Created(p, gpu_fault::Kind::pipeline);
    return p;
}
extern "C" SDL_GPUBuffer* SDLCALL __real_SDL_CreateGPUBuffer(SDL_GPUDevice*, const SDL_GPUBufferCreateInfo*);
extern "C" SDL_GPUBuffer* SDLCALL __wrap_SDL_CreateGPUBuffer(SDL_GPUDevice* d, const SDL_GPUBufferCreateInfo* i) {
    auto* c = For(d);
    if (c && c->Hit(gpu_fault::Call::create)) { SDL_SetError("injected SDL_CreateGPUBuffer"); return nullptr; }
    auto* p = __real_SDL_CreateGPUBuffer(d, i);
    if (c) c->Created(p, gpu_fault::Kind::buffer);
    return p;
}
extern "C" SDL_GPUTransferBuffer* SDLCALL __real_SDL_CreateGPUTransferBuffer(SDL_GPUDevice*, const SDL_GPUTransferBufferCreateInfo*);
extern "C" SDL_GPUTransferBuffer* SDLCALL __wrap_SDL_CreateGPUTransferBuffer(SDL_GPUDevice* d, const SDL_GPUTransferBufferCreateInfo* i) {
    auto* c = For(d);
    if (c && c->Hit(gpu_fault::Call::create)) { SDL_SetError("injected SDL_CreateGPUTransferBuffer"); return nullptr; }
    auto* p = __real_SDL_CreateGPUTransferBuffer(d, i);
    if (c) c->Created(p, gpu_fault::Kind::transfer);
    return p;
}
extern "C" SDL_GPUTexture* SDLCALL __real_SDL_CreateGPUTexture(SDL_GPUDevice*, const SDL_GPUTextureCreateInfo*);
extern "C" SDL_GPUTexture* SDLCALL __wrap_SDL_CreateGPUTexture(SDL_GPUDevice* d, const SDL_GPUTextureCreateInfo* i) {
    auto* c = For(d);
    if (c && c->Hit(gpu_fault::Call::create)) { SDL_SetError("injected SDL_CreateGPUTexture"); return nullptr; }
    auto* p = __real_SDL_CreateGPUTexture(d, i);
    if (c) c->Created(p, gpu_fault::Kind::texture);
    return p;
}
extern "C" void SDLCALL __real_SDL_ReleaseGPUShader(SDL_GPUDevice*, SDL_GPUShader*);
extern "C" void SDLCALL __wrap_SDL_ReleaseGPUShader(SDL_GPUDevice* d, SDL_GPUShader* p) {
    if (auto* c = For(d)) { Known(c, p, gpu_fault::Kind::shader); c->Released(p, gpu_fault::Kind::shader); }
    __real_SDL_ReleaseGPUShader(d, p);
}
extern "C" void SDLCALL __real_SDL_ReleaseGPUGraphicsPipeline(SDL_GPUDevice*, SDL_GPUGraphicsPipeline*);
extern "C" void SDLCALL __wrap_SDL_ReleaseGPUGraphicsPipeline(SDL_GPUDevice* d, SDL_GPUGraphicsPipeline* p) {
    if (auto* c = For(d)) { Known(c, p, gpu_fault::Kind::pipeline); c->Released(p, gpu_fault::Kind::pipeline); }
    __real_SDL_ReleaseGPUGraphicsPipeline(d, p);
}
extern "C" void SDLCALL __real_SDL_ReleaseGPUBuffer(SDL_GPUDevice*, SDL_GPUBuffer*);
extern "C" void SDLCALL __wrap_SDL_ReleaseGPUBuffer(SDL_GPUDevice* d, SDL_GPUBuffer* p) {
    if (auto* c = For(d)) { Known(c, p, gpu_fault::Kind::buffer); c->Released(p, gpu_fault::Kind::buffer); }
    __real_SDL_ReleaseGPUBuffer(d, p);
}
extern "C" void SDLCALL __real_SDL_ReleaseGPUTransferBuffer(SDL_GPUDevice*, SDL_GPUTransferBuffer*);
extern "C" void SDLCALL __wrap_SDL_ReleaseGPUTransferBuffer(SDL_GPUDevice* d, SDL_GPUTransferBuffer* p) {
    if (auto* c = For(d)) { Known(c, p, gpu_fault::Kind::transfer); c->Released(p, gpu_fault::Kind::transfer); }
    __real_SDL_ReleaseGPUTransferBuffer(d, p);
}
extern "C" void SDLCALL __real_SDL_ReleaseGPUTexture(SDL_GPUDevice*, SDL_GPUTexture*);
extern "C" void SDLCALL __wrap_SDL_ReleaseGPUTexture(SDL_GPUDevice* d, SDL_GPUTexture* p) {
    if (auto* c = For(d)) { Known(c, p, gpu_fault::Kind::texture); c->Released(p, gpu_fault::Kind::texture); }
    __real_SDL_ReleaseGPUTexture(d, p);
}
extern "C" void SDLCALL __real_SDL_ReleaseGPUFence(SDL_GPUDevice*, SDL_GPUFence*);
extern "C" void SDLCALL __wrap_SDL_ReleaseGPUFence(SDL_GPUDevice* d, SDL_GPUFence* p) {
    if (auto* c = For(d)) { Known(c, p, gpu_fault::Kind::fence); c->Released(p, gpu_fault::Kind::fence); }
    __real_SDL_ReleaseGPUFence(d, p);
}
extern "C" SDL_GPUCommandBuffer* SDLCALL __real_SDL_AcquireGPUCommandBuffer(SDL_GPUDevice*);
extern "C" SDL_GPUCommandBuffer* SDLCALL __wrap_SDL_AcquireGPUCommandBuffer(SDL_GPUDevice* d) {
    auto* c = For(d);
    if (c && c->Hit(gpu_fault::Call::acquire)) { SDL_SetError("injected acquire"); return nullptr; }
    auto* p = __real_SDL_AcquireGPUCommandBuffer(d);
    if (c) c->Created(p, gpu_fault::Kind::command);
    return p;
}
extern "C" bool SDLCALL __real_SDL_CancelGPUCommandBuffer(SDL_GPUCommandBuffer*);
extern "C" bool SDLCALL __wrap_SDL_CancelGPUCommandBuffer(SDL_GPUCommandBuffer* p) {
    auto* c = Command(p);
    const bool canceled = __real_SDL_CancelGPUCommandBuffer(p);
    if (c) {
        if (!canceled) { std::fputs("real cancellation failed\n", stderr); std::abort(); }
        c->Released(p, gpu_fault::Kind::command); ++c->canceled_count;
    }
    return canceled;
}
extern "C" void* SDLCALL __real_SDL_MapGPUTransferBuffer(SDL_GPUDevice*, SDL_GPUTransferBuffer*, bool);
extern "C" void* SDLCALL __wrap_SDL_MapGPUTransferBuffer(SDL_GPUDevice* d, SDL_GPUTransferBuffer* p, bool cycle) {
    auto* c = For(d); Known(c, p, gpu_fault::Kind::transfer);
    if (c && c->Hit(gpu_fault::Call::map)) { SDL_SetError("injected map"); return nullptr; }
    auto* mapped = __real_SDL_MapGPUTransferBuffer(d, p, cycle);
    if (mapped && c) c->Mapped(p);
    return mapped;
}
extern "C" void SDLCALL __real_SDL_UnmapGPUTransferBuffer(SDL_GPUDevice*, SDL_GPUTransferBuffer*);
extern "C" void SDLCALL __wrap_SDL_UnmapGPUTransferBuffer(SDL_GPUDevice* d, SDL_GPUTransferBuffer* p) {
    if (auto* c = For(d)) { Known(c, p, gpu_fault::Kind::transfer); c->Unmapped(p); }
    __real_SDL_UnmapGPUTransferBuffer(d, p);
}
extern "C" SDL_GPUCopyPass* SDLCALL __real_SDL_BeginGPUCopyPass(SDL_GPUCommandBuffer*);
extern "C" SDL_GPUCopyPass* SDLCALL __wrap_SDL_BeginGPUCopyPass(SDL_GPUCommandBuffer* p) {
    if (auto* c = Command(p); c && c->Hit(gpu_fault::Call::copy)) {
        SDL_SetError("injected copy pass"); return nullptr;
    }
    return __real_SDL_BeginGPUCopyPass(p);
}
extern "C" SDL_GPURenderPass* SDLCALL __real_SDL_BeginGPURenderPass(SDL_GPUCommandBuffer*, const SDL_GPUColorTargetInfo*, Uint32, const SDL_GPUDepthStencilTargetInfo*);
extern "C" SDL_GPURenderPass* SDLCALL __wrap_SDL_BeginGPURenderPass(SDL_GPUCommandBuffer* p, const SDL_GPUColorTargetInfo* t, Uint32 n, const SDL_GPUDepthStencilTargetInfo* depth) {
    if (auto* c = Command(p); c && c->Hit(gpu_fault::Call::render)) {
        SDL_SetError("injected render pass"); return nullptr;
    }
    return __real_SDL_BeginGPURenderPass(p, t, n, depth);
}
extern "C" SDL_GPUFence* SDLCALL __real_SDL_SubmitGPUCommandBufferAndAcquireFence(SDL_GPUCommandBuffer*);
extern "C" SDL_GPUFence* SDLCALL __wrap_SDL_SubmitGPUCommandBufferAndAcquireFence(SDL_GPUCommandBuffer* p) {
    auto* c = Command(p);
    if (c && c->Hit(gpu_fault::Call::submit)) {
        // Reject before the real submit: no acquired real fence may be discarded.
        __wrap_SDL_CancelGPUCommandBuffer(p);
        SDL_SetError("injected pre-submit rejection"); return nullptr;
    }
    auto* fence = __real_SDL_SubmitGPUCommandBufferAndAcquireFence(p);
    if (c) {
        c->Released(p, gpu_fault::Kind::command);
        c->Created(fence, gpu_fault::Kind::fence);
        if (c->submission_count < c->submitted_fences.size())
            c->submitted_fences[c->submission_count] = fence;
        ++c->submission_count;
    }
    return fence;
}
extern "C" bool SDLCALL __wrap_SDL_WaitForGPUIdle(SDL_GPUDevice* d) {
    auto* c = For(d);
    if (c && c->hang_wait) {
        std::fputs("FAULT_HANG_WAIT_ENTERED\n", stderr); std::fflush(stderr);
        for (;;) pause();
    }
    if (c && c->Hit(gpu_fault::Call::wait)) { SDL_SetError("injected drain failure"); return false; }
    // Failure is sticky for the failed-drain child so destruction receives it too.
    if (c && c->forbid_release) { SDL_SetError("injected sticky drain failure"); return false; }
    return c ? c->PhysicalWait() : __real_SDL_WaitForGPUIdle(d);
}
extern "C" bool SDLCALL __real_SDL_QueryGPUFence(SDL_GPUDevice*, SDL_GPUFence*);
extern "C" bool SDLCALL __wrap_SDL_QueryGPUFence(SDL_GPUDevice* d, SDL_GPUFence* p) {
    auto* c = For(d);
    if (c) {
        Known(c, p, gpu_fault::Kind::fence);
        static_cast<void>(c->Hit(gpu_fault::Call::query));
        if (c->hold_queries && c->visible_fence != p) return false;
    }
    return __real_SDL_QueryGPUFence(d, p);
}
