#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

struct SDL_Renderer;

namespace crucible::presentation::desktop {
/** Correlates GPU command completion with owned frame identities.
 * All operations require the desktop coordinator. The renderer must outlive this
 * observer. Completion never establishes successful native presentation or scanout.
 */
class FrameCompletionObserver {
public:
    /// The concrete completion mechanism received by this instance.
    enum class Capability { unsupported, direct3d11 };
    /// Outcome of recording a post-presentation GPU completion marker.
    enum class RecordStatus { recorded, full, invalid, unsupported, deviceChanged, deviceError };
    /// Outcome of polling the oldest marker; only complete retires its slot.
    enum class PollStatus { empty, pending, complete, unsupported, deviceChanged, deviceError };
    /// Copied identity; positive run/frame IDs increase across successful records.
    struct FrameIdentity {
        std::uint64_t runId{}, frameId{}, tick{};
        /// Conventional value equality for correlating an observer receipt.
        bool operator==(const FrameIdentity&) const noexcept = default;
    };
    /// Owned polling receipt, with identity populated for pending or completed work.
    struct PollResult {
        PollStatus status{PollStatus::empty};
        FrameIdentity frame{};
    };

    /** Creates bounded native event-query storage, or an explicit unsupported observer.
     * @param renderer Main-thread SDL renderer borrowed until observer destruction.
     * @param capacity Positive maximum pending markers, allocated once at startup.
     * @throws std::invalid_argument Zero capacity or invalid native setup.
     * @throws std::runtime_error Native event-query construction fails.
     * @throws std::bad_alloc Startup storage cannot be allocated.
     */
    FrameCompletionObserver(SDL_Renderer& renderer, std::size_t capacity);
    /** Drains recorded markers before releasing native query references.
     * @note A driver can block; the host process timeout is the external bound.
     * Native failure is logged; this observer owns no submitted frame buffers.
     */
    ~FrameCompletionObserver();
    FrameCompletionObserver(const FrameCompletionObserver&) = delete;
    FrameCompletionObserver& operator=(const FrameCompletionObserver&) = delete;
    FrameCompletionObserver(FrameCompletionObserver&&) = delete;
    FrameCompletionObserver& operator=(FrameCompletionObserver&&) = delete;

    /** Records commands after the host's complete world/HUD presentation call.
     * @param frame Owned run/frame/tick identity; run and frame are positive and
     * frame increases within a run, or run increases. Tick may repeat for redraws.
     * @return Recorded, refusal without mutation, unsupported, or latched native failure.
     * @note Flushes SDL before native interop and sends the marker once. No heap
     * allocation by this observer. Does not validate the host's presentation result.
     */
    [[nodiscard]] RecordStatus recordFrame(FrameIdentity frame) noexcept;
    /** Polls GPU completion without flushing the native command buffer again.
     * @return Oldest copied receipt; pending preserves its slot, complete retires it.
     * Unsupported and errors never report successful completion.
     */
    [[nodiscard]] PollResult pollOldest() noexcept;
    /** Joins every recorded native marker against its original device/context.
     * @return True after all markers complete or if unsupported with no native work;
     * false on native error. This return does not establish observer capability.
     * @note May block on the driver; failure never invents a completion receipt.
     */
    [[nodiscard]] bool tryDrain() noexcept;
    /** Reports the concrete mechanism available at construction.
     * @return Unsupported for other backends/platforms; never inferred from a device inventory.
     */
    [[nodiscard]] Capability getCapability() const noexcept;
    /** Reports marker storage still owned by unfinished work.
     * @return Number of pending query slots, including work retained after a device change.
     */
    [[nodiscard]] std::size_t getPendingCount() const noexcept;
private:
    struct State;
    std::unique_ptr<State> state_;
};
}
