#include <SDL3/SDL.h>
#include <stdexcept>
#include <thread>
#include <vector>

#if defined(_WIN32)
#include <d3d11.h>
#include <wrl/client.h>
#endif

#include "crucible/presentation/desktop/frame_completion_observer.hpp"

namespace crucible::presentation::desktop {
struct FrameCompletionObserver::State {
    SDL_Renderer& renderer;
    Capability capability{Capability::unsupported};
    RecordStatus failure{RecordStatus::recorded};
    FrameIdentity last{};
    std::size_t head{}, pending{};
#if defined(_WIN32)
    struct Slot {
        Microsoft::WRL::ComPtr<ID3D11Query> query;
        FrameIdentity frame{};
    };
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    std::vector<Slot> slots;

    [[nodiscard]] ID3D11Device* currentDevice() const noexcept {
        const auto properties = SDL_GetRendererProperties(&renderer);
        return properties ? static_cast<ID3D11Device*>(SDL_GetPointerProperty(properties,
            SDL_PROP_RENDERER_D3D11_DEVICE_POINTER, nullptr)) : nullptr;
    }
    [[nodiscard]] bool checkDevice() noexcept {
        if (!SDL_IsMainThread()) { failure = RecordStatus::deviceError; return false; }
        if (currentDevice() != device.Get()) { failure = RecordStatus::deviceChanged; return false; }
        if (FAILED(device->GetDeviceRemovedReason())) { failure = RecordStatus::deviceError; return false; }
        return true;
    }
    [[nodiscard]] PollResult pollNative() noexcept {
        const auto identity = slots[head].frame;
        BOOL complete = FALSE;
        const auto result = context->GetData(slots[head].query.Get(), &complete,
            sizeof complete, D3D11_ASYNC_GETDATA_DONOTFLUSH);
        if (FAILED(result)) { failure = RecordStatus::deviceError; return {PollStatus::deviceError, identity}; }
        if (result != S_OK || !complete) return {PollStatus::pending, identity};
        head = (head + 1) % slots.size();
        --pending;
        return {PollStatus::complete, identity};
    }
#endif
    State(SDL_Renderer& value, std::size_t capacity) : renderer(value) {
        if (capacity == 0) throw std::invalid_argument("Completion observer needs positive capacity");
#if defined(_WIN32)
        auto* native = currentDevice();
        if (!native) return;
        if (!SDL_IsMainThread()) throw std::invalid_argument("Native observer requires desktop main thread");
        device = native;
        device->GetImmediateContext(context.GetAddressOf());
        if (!context) throw std::runtime_error("Native renderer has no immediate context");
        slots.resize(capacity);
        const D3D11_QUERY_DESC description{D3D11_QUERY_EVENT, 0};
        for (auto& slot : slots)
            if (FAILED(device->CreateQuery(&description, slot.query.GetAddressOf())))
                throw std::runtime_error("Native completion event-query creation failed");
        capability = Capability::direct3d11;
#endif
    }
};

FrameCompletionObserver::FrameCompletionObserver(SDL_Renderer& renderer, std::size_t capacity)
    : state_(std::make_unique<State>(renderer, capacity)) {}
FrameCompletionObserver::~FrameCompletionObserver() {
    if (!tryDrain()) SDL_Log("Frame completion observer drain failed; no successful completion claimed");
}
FrameCompletionObserver::RecordStatus FrameCompletionObserver::recordFrame(FrameIdentity frame) noexcept {
    auto& state = *state_;
    if (state.capability == Capability::unsupported) return RecordStatus::unsupported;
    if (state.failure != RecordStatus::recorded) return state.failure;
    if (frame.runId == 0 || frame.frameId == 0 || frame.runId < state.last.runId ||
        (frame.runId == state.last.runId && frame.frameId <= state.last.frameId)) return RecordStatus::invalid;
#if defined(_WIN32)
    if (!state.checkDevice()) return state.failure;
    if (state.pending == state.slots.size()) return RecordStatus::full;
    if (!SDL_FlushRenderer(&state.renderer)) {
        state.failure = RecordStatus::deviceError;
        return state.failure;
    }
    auto& slot = state.slots[(state.head + state.pending) % state.slots.size()];
    slot.frame = frame;
    state.context->End(slot.query.Get());
    // Send the marker once; DONOTFLUSH polling must not leave it buffered indefinitely.
    state.context->Flush();
    state.last = frame;
    ++state.pending;
    return RecordStatus::recorded;
#else
    return RecordStatus::unsupported;
#endif
}
FrameCompletionObserver::PollResult FrameCompletionObserver::pollOldest() noexcept {
    auto& state = *state_;
    if (state.capability == Capability::unsupported) return {PollStatus::unsupported, {}};
#if defined(_WIN32)
    const auto identity = state.pending ? state.slots[state.head].frame : FrameIdentity{};
    if (state.failure == RecordStatus::deviceChanged) return {PollStatus::deviceChanged, identity};
    if (state.failure == RecordStatus::deviceError) return {PollStatus::deviceError, identity};
    if (!state.checkDevice()) return {state.failure == RecordStatus::deviceChanged
        ? PollStatus::deviceChanged : PollStatus::deviceError, identity};
    if (state.pending == 0) return {PollStatus::empty, {}};
    return state.pollNative();
#else
    return {PollStatus::unsupported, {}};
#endif
}
bool FrameCompletionObserver::tryDrain() noexcept {
    auto& state = *state_;
    if (state.capability == Capability::unsupported) return true;
#if defined(_WIN32)
    if (!SDL_IsMainThread()) return false;
    while (state.pending != 0) {
        const auto result = state.pollNative();
        if (result.status == PollStatus::deviceError) return false;
        if (result.status == PollStatus::pending) std::this_thread::yield();
    }
    return state.failure != RecordStatus::deviceError;
#else
    return true;
#endif
}
FrameCompletionObserver::Capability FrameCompletionObserver::getCapability() const noexcept {
    return state_->capability;
}
std::size_t FrameCompletionObserver::getPendingCount() const noexcept { return state_->pending; }
}
