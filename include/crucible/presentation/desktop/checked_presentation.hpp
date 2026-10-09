#pragma once

#include <cstdint>
#include <memory>
#include <optional>

struct SDL_Renderer;

namespace crucible::presentation::desktop {
/** Presents one SDL-drawn frame with an explicit native D3D11 handoff result.
 * All operations require the desktop coordinator. The renderer outlives this
 * receiver. Fixed sync interval one is local policy, not a physical scanout promise.
 */
class CheckedPresentation {
public:
    /// Native acceptance, non-presenting backpressure, or an explicit capability/error result.
    enum class Status { handedOff, busy, occluded, unsupported, deviceChanged, deviceError };
    /// Copied outcome; nativeResult is the signed HRESULT when a native operation supplied it.
    struct Receipt {
        Status status{Status::unsupported};
        std::optional<std::int32_t> nativeResult{};
    };

    /** Owns original device/swapchain COM references, or explicit unsupported state.
     * @param renderer Main-thread SDL renderer borrowed until receiver destruction.
     * @throws std::invalid_argument Native construction is off the desktop coordinator.
     * @throws std::bad_alloc Startup receiver storage cannot be allocated.
     */
    explicit CheckedPresentation(SDL_Renderer& renderer);
    /** Releases original device/swapchain references on the desktop coordinator.
     * @note Owns no submitted GPU buffers and performs no completion wait. The
     * host drains its completion observer before destroying this receiver/renderer.
     */
    ~CheckedPresentation();
    CheckedPresentation(const CheckedPresentation&) = delete;
    CheckedPresentation& operator=(const CheckedPresentation&) = delete;
    CheckedPresentation(CheckedPresentation&&) = delete;
    CheckedPresentation& operator=(CheckedPresentation&&) = delete;

    /** Flushes SDL drawing, presents once, and invalidates SDL's native state cache.
     * @return S_OK handoff, separately classified busy/occlusion, unsupported, or
     * latched device/identity error. SDL flush failure has no invented HRESULT.
     * @note Replaces SDL_RenderPresent; the host must never present the same frame
     * twice. Handoff proves neither GPU completion nor physical display/scanout.
     */
    [[nodiscard]] Receipt present() noexcept;
    /** Reports whether the construction received actual D3D11 device/swapchain properties.
     * @return False on other platforms/backends; inventory cannot establish this value.
     */
    [[nodiscard]] bool isSupported() const noexcept;
private:
    struct State;
    std::unique_ptr<State> state_;
};
}
