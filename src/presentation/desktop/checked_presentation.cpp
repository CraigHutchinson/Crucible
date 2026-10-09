#include <SDL3/SDL.h>
#include <stdexcept>
#include <string_view>

#if defined(_WIN32)
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#endif

#include "crucible/presentation/desktop/checked_presentation.hpp"

namespace crucible::presentation::desktop {
struct CheckedPresentation::State {
    SDL_Renderer& renderer;
    bool supported{};
    std::optional<Receipt> failure{};
#if defined(_WIN32)
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> swapchain;

    [[nodiscard]] bool hasOriginalIdentity() const noexcept {
        const auto properties = SDL_GetRendererProperties(&renderer);
        return properties && SDL_GetPointerProperty(properties, SDL_PROP_RENDERER_D3D11_DEVICE_POINTER,
            nullptr) == device.Get() && SDL_GetPointerProperty(properties,
            SDL_PROP_RENDERER_D3D11_SWAPCHAIN_POINTER, nullptr) == swapchain.Get();
    }
    [[nodiscard]] std::optional<Receipt> checkDevice() noexcept {
        if (!SDL_IsMainThread()) return Receipt{Status::deviceError, {}};
        if (!hasOriginalIdentity()) return Receipt{Status::deviceChanged, {}};
        const auto reason = device->GetDeviceRemovedReason();
        if (FAILED(reason)) return Receipt{Status::deviceError, static_cast<std::int32_t>(reason)};
        return std::nullopt;
    }
#endif
    explicit State(SDL_Renderer& value) : renderer(value) {
#if defined(_WIN32)
        const auto* name = SDL_GetRendererName(&renderer);
        if (!name || std::string_view{name} != "direct3d11") return;
        const auto properties = SDL_GetRendererProperties(&renderer);
        if (!properties) return;
        auto* nativeDevice = static_cast<ID3D11Device*>(SDL_GetPointerProperty(properties,
            SDL_PROP_RENDERER_D3D11_DEVICE_POINTER, nullptr));
        auto* nativeSwapchain = static_cast<IDXGISwapChain1*>(SDL_GetPointerProperty(properties,
            SDL_PROP_RENDERER_D3D11_SWAPCHAIN_POINTER, nullptr));
        if (!nativeDevice || !nativeSwapchain) return;
        if (!SDL_IsMainThread()) throw std::invalid_argument("Checked presentation requires desktop main thread");
        device = nativeDevice;
        swapchain = nativeSwapchain;
        supported = true;
#endif
    }
};

CheckedPresentation::CheckedPresentation(SDL_Renderer& renderer)
    : state_(std::make_unique<State>(renderer)) {}
CheckedPresentation::~CheckedPresentation() = default;
CheckedPresentation::Receipt CheckedPresentation::present() noexcept {
    auto& state = *state_;
    if (!state.supported) return {Status::unsupported, {}};
    if (state.failure) return *state.failure;
#if defined(_WIN32)
    state.failure = state.checkDevice();
    if (state.failure) return *state.failure;
    if (!SDL_FlushRenderer(&state.renderer)) {
        state.failure = Receipt{Status::deviceError, {}};
        return *state.failure;
    }
    state.failure = state.checkDevice();
    if (state.failure) return *state.failure;
    const DXGI_PRESENT_PARAMETERS parameters{};
    const auto result = state.swapchain->Present1(1, 0, &parameters);
    // Native present may unbind the target; SDL's documented interop invalidation restores it next draw.
    const bool invalidated = SDL_FlushRenderer(&state.renderer);
    const auto nativeResult = static_cast<std::int32_t>(result);
    if (!invalidated) {
        state.failure = Receipt{Status::deviceError, nativeResult};
        return *state.failure;
    }
    if (result == DXGI_ERROR_WAS_STILL_DRAWING) return {Status::busy, nativeResult};
    if (result == DXGI_STATUS_OCCLUDED) return {Status::occluded, nativeResult};
    if (result != S_OK) {
        state.failure = Receipt{Status::deviceError, nativeResult};
        return *state.failure;
    }
    state.failure = state.checkDevice();
    if (state.failure) return *state.failure;
    return {Status::handedOff, nativeResult};
#else
    return {Status::unsupported, {}};
#endif
}
bool CheckedPresentation::isSupported() const noexcept { return state_->supported; }
}
