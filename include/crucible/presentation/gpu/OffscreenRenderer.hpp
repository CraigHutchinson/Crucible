#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
struct SDL_GPUDevice;
namespace crucible::presentation { class Camera2D; }
namespace crucible::presentation::gpu {
class InstancePacket;
enum class SubmitStatus { submitted, busy, invalid, device_error };
/// Owned receipt, valid only for this renderer until its slot is reused/destruction.
struct Submission {
    SubmitStatus status{SubmitStatus::invalid};
    std::uint64_t id{}, tick{};
    std::size_t slot{};
    std::weak_ptr<const void> issuer{}; ///< Identity only; does not prolong renderer/resources.
};
enum class ReadbackStatus { pending, complete, expired, invalid, device_error };
/** Concrete offscreen SDL_GPU instance consumer; fixed 1280x720 RGBA8 UNORM.
 * One device/coordinator thread. Caller owns the Vulkan/SPIR-V device and keeps it
 * alive until this object is destroyed. No packet/camera/shader-code borrow survives calls.
 */
class OffscreenRenderer {
public:
    static constexpr std::size_t Width = 1280, Height = 720, ReadbackBytes = Width * Height * 4;
    /** Creates a static quad, compiled pipeline and exactly three bounded upload/readback slots.
     * Shader bytes are compiled SPIR-V for entry main; copied during shader creation.
     * @throws std::length_error Counts/byte sizes cannot fit SDL buffers.
     * @throws std::invalid_argument Missing or malformed shader words.
     * @throws std::runtime_error SDL resource creation or startup upload fails.
     * Partial creation releases only after any submitted work has drained.
     */
    OffscreenRenderer(SDL_GPUDevice& device, std::size_t sample_capacity, std::size_t cell_capacity,
                      std::span<const std::byte> vertex_shader, std::span<const std::byte> fragment_shader);
    /** Drains before release. A failed drain terminates rather than freeing live work.
     * A hung driver can block; host/test process timeout is the external bound.
     */
    ~OffscreenRenderer();
    OffscreenRenderer(const OffscreenRenderer&) = delete;
    OffscreenRenderer& operator=(const OffscreenRenderer&) = delete;
    OffscreenRenderer(OffscreenRenderer&&) = delete;
    OffscreenRenderer& operator=(OffscreenRenderer&&) = delete;
    /** Copies/uploads both ranges and queues two instance draws plus readback.
     * Capacity/camera rejection precedes mutation. All-busy leaves slots/data unchanged.
     * Submitted means queued, never completed readback or presentation. Device failure
     * latches this renderer unusable; no failed receipt is published as a frame.
     */
    [[nodiscard]] Submission TrySubmit(const InstancePacket& packet, const Camera2D& camera) noexcept;
    /** Copies RGBA bytes only after this receipt's fence completes; no mapped pointer escapes.
     * Destination needs ReadbackBytes; rejection/pending/expired leaves it unchanged.
     */
    [[nodiscard]] ReadbackStatus PollReadback(const Submission& submission, std::span<std::byte> destination) noexcept;
    /// Blocking teardown barrier; false latches failure. Completed receipts remain readable.
    [[nodiscard]] bool TryDrain() noexcept;
private:
    struct State;
    std::unique_ptr<State> state_;
};
}
