#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>

#include "crucible/presentation/production/nanite_mesh_frame.hpp"

namespace crucible::presentation::production
{
/** Receives the actual Scaffold prototype through a bounded production mesh.
 * @note Coordinator-only; decorative state never advances mission Runtime.
 * Returned mesh borrows expire on the next advance, seek, replay or destruction.
 */
class SwarmIntroAdapter
{
public:
    /// Animated foreground or hidden bounded settling behind an already usable UI.
    enum class PlaybackMode { animated, settling };
    /// Copies both clocks and explicit decorative catch-up/preparation receipts.
    struct AdvanceResult
    {
        std::uint32_t advancedTicks_{}, totalPresentedTicks_{};
        std::chrono::nanoseconds droppedPresentationTime_{};
        float choreographySeconds_{};
        bool sequenceFinished_{}, staticReady_{};
    };

    /** Allocates source Scaffold1536, its snapshot and reusable mesh storage.
     * @throws std::exception Source initialization or cold allocation fails.
     */
    SwarmIntroAdapter();
    /// Releases source and producer storage without touching mission Runtime.
    ~SwarmIntroAdapter();
    SwarmIntroAdapter(const SwarmIntroAdapter&) = delete;
    SwarmIntroAdapter& operator=(const SwarmIntroAdapter&) = delete;
    SwarmIntroAdapter(SwarmIntroAdapter&&) = delete;
    SwarmIntroAdapter& operator=(SwarmIntroAdapter&&) = delete;

    /** Installs actual drawable values before the next source tick/camera/LOD pass.
     * @param aspect Finite drawable width/height in[1/32,32].
     * @param pixelsWide Finite drawable width in[1,4096].
     * @return False without mutation when values are unsupported.
     */
    [[nodiscard]] bool trySetViewport(float aspect, float pixelsWide) noexcept;
    /** Advances at most4 actual source ticks; foreground debt is capped at250ms.
     * @param elapsed Active wall time, excluding suspension, in[0,24hours].
     * @param mode Animated playback or hidden preparation with no animated frame.
     * @return Actual integral ticks, source choreography time and discarded debt.
     * @throws std::invalid_argument Elapsed time is outside the supported range.
     * @throws std::runtime_error Mesh refusal or default settling exceeds4096ticks.
     * @note Settling ignores elapsed and advances at most4 ticks per call. Its
     * frame stays unavailable until sequence completion; caller shows static floor.
     */
    [[nodiscard]] AdvanceResult advance(std::chrono::nanoseconds elapsed,
        PlaybackMode mode = PlaybackMode::animated);
    /// Discards suspended presentation debt without advancing either clock.
    void suspend() noexcept;
    /** Reconstructs the source at the same viewport and clears both clocks/debt.
     * @throws std::exception Cold source construction fails, preserving old state.
     */
    void replay();
    /** Performs explicit cold receiving seek to an integral presentation tick.
     * @param targetTicks Forward target at most2700ticks (45 presented seconds).
     * @return False without advancing for backward/out-of-range targets.
     * @throws std::runtime_error A source/mesh receiving operation fails.
     * @note This synchronous receiving operation is not immediate interactive startup.
     */
    [[nodiscard]] bool trySeekPresentedTicks(std::uint32_t targetTicks);
    /** Borrows the last produced mesh or a completed static settled view.
     * @return No frame before first evaluation or during hidden preparation.
     * @note The caller synchronously consumes the borrow before the next mutation.
     */
    [[nodiscard]] std::optional<NaniteMeshFrame> getFrame() const noexcept;
    /** Copies the most recent advancement/preparation receipt.
     * @return Actual clocks and readiness, never an authored-time reinterpretation.
     */
    [[nodiscard]] AdvanceResult getProgress() const noexcept;

private:
    struct State;
    std::unique_ptr<State> state_;
};
}
