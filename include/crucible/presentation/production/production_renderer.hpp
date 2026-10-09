#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "crucible/presentation/production/cinematic_frame.hpp"

struct SDL_Window;
struct ImGuiContext;
struct ImDrawData;

namespace crucible::presentation::production
{
/** Owns the bounded Filament scene/UI consumer on the application coordinator.
 * @note The supplied SDL window and sole ImGui context outlive this object.
 * No Runtime/ECS state enters the renderer. Pinned MSVC Panic paths can terminate
 * the process; status returns do not promise recovery from those failures.
 */
class ProductionRenderer
{
public:
    /// Submission disposition; scheduled is neither GPU success nor scanout.
    enum class Status { scheduled, skipped, suspended, rejected, failed };
    /// Bounded failure explanation consumed by the root application.
    enum class Reason { none, invalidScene, invalidUi, capacity, context, surface, backend, busy, frameIdentity };
    /// Actual drawable pixels, independent of logical UI coordinates.
    struct DrawableExtent { std::uint32_t width_{}, height_{}; };
    /// Correlates a submission with its application frame and surface epoch.
    struct FrameReceipt
    {
        Status status_{Status::failed};
        Reason reason_{Reason::none};
        std::uint64_t frameId_{}, surfaceGeneration_{};
    };
    /// Copied capability values; this does not expose framework/device objects.
    struct Metadata
    {
        enum class Backend { vulkan, metal };
        Backend backend_{Backend::vulkan};
        std::size_t automaticInstanceLimit_{};
        bool coreFloatTextureSupported_{};
        std::array<char, 41> sourcePin_{};
        DrawableExtent extent_{};
        std::uint64_t surfaceGeneration_{};
    };
    /// Queues cold readback of one future composed scene-plus-UI frame.
    struct CaptureRequest { std::uint64_t frameId_{}, surfaceGeneration_{}; };
    /// Owns bottom-up RGBA8 pixels after the framework's readback callback.
    struct CaptureResult
    {
        std::uint64_t frameId_{}, surfaceGeneration_{};
        DrawableExtent extent_{};
        std::vector<std::byte> rgba_{};
    };
    /// Qualified framework drain; no checked device-loss or handoff assertion.
    enum class DrainStatus { drained, timeout, failed };

    /** Constructs startup-bounded scene, materials and UI resources.
     * @param window Root-owned production platform window, never SDL_Renderer.
     * @param context Root-owned current ImGui context with themed fonts installed.
     * @throws std::invalid_argument Context/window or drawable is unsupported.
     * @throws std::runtime_error Framework/material/capability initialization fails.
     * @throws std::bad_alloc Startup adapter storage cannot be allocated.
     * @note Windows Vulkan is the initial receiver. Apple Metal remains a separate
     * build/native gate. Framework-fatal failures cannot be translated to throws.
     */
    ProductionRenderer(SDL_Window& window, ImGuiContext& context);
    /// Joins framework teardown before releasing callback-owned staging; preserves context.
    ~ProductionRenderer();
    ProductionRenderer(const ProductionRenderer&) = delete;
    ProductionRenderer& operator=(const ProductionRenderer&) = delete;
    ProductionRenderer(ProductionRenderer&&) = delete;
    ProductionRenderer& operator=(ProductionRenderer&&) = delete;

    /** Copies a fully validated scene/UI packet into a free bounded upload slot.
     * @return Correlated scheduled/skipped/suspended/rejected/failed disposition.
     * @note Borrowed inputs expire on return. Rejects before accepting beginFrame;
     * every accepted framework frame is ended, including a later recoverable error.
     */
    [[nodiscard]] FrameReceipt tryRender(const CinematicFrame& frame,
        const ImDrawData& drawData, std::uint64_t frameId);
    /** Drains outstanding work before replacing the surface extent.
     * @return A new surface generation on success; zero extent suspends submission.
     * @note Rejected extents retain the previous surface. Pending captures refuse resize.
     */
    [[nodiscard]] FrameReceipt tryResize(DrawableExtent extent);
    /** Pumps renderer callback/texture retirement work before root's ImGui::NewFrame.
     * @return Ready (scheduled), skipped while retiring textures, or a refusal.
     * @note Does not call NewFrame or change root's theme, font choices or ini policy.
     */
    [[nodiscard]] Status beginUiFrame();
    /** Reserves the single cold readback slot for a future frame on this surface.
     * @return scheduled for a reserved request; skipped/rejected/failed otherwise.
     * @throws std::bad_alloc Cold capture storage cannot be replenished after transfer.
     * @note Retry the targeted frame ID after a skipped submission; later IDs are
     * rejected until that capture is submitted. Pending storage is never replaced.
     */
    [[nodiscard]] FrameReceipt tryCapture(CaptureRequest request);
    /** Transfers completed owned pixels; nullopt while pending or with no request.
     * @return Completed result correlated to its actual composed frame/surface.
     */
    [[nodiscard]] std::optional<CaptureResult> takeCapture();
    /** Joins/purges with a bounded framework timeout without freeing pending storage.
     * @return Qualified drained, timeout (storage retained), or failed.
     */
    [[nodiscard]] DrainStatus tryDrain(std::chrono::nanoseconds timeout);
    /// Returns copied backend/pin/instancing/texture capabilities without allocation.
    [[nodiscard]] Metadata getMetadata() const noexcept;

private:
    struct State;
    std::unique_ptr<State> state_;
};
}
