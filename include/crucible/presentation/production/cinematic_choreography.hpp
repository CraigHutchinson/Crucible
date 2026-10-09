#pragma once

#include <chrono>
#include <cstdint>
#include <vector>

#include "crucible/presentation/production/cinematic_frame.hpp"

namespace crucible::presentation::production
{
/** Choreographs authored silver-wedge lettering without advancing gameplay.
 * @note One coordinator owns this producer. Returned scene borrows must be
 * consumed before its next evaluation or destruction.
 */
class CinematicChoreography
{
public:
    /// Authored sequence duration; the final composition is also the static title.
    static constexpr std::chrono::milliseconds duration{12000};

    /** Allocates the bounded authored wordmark and reusable pose storage.
     * @throws std::bad_alloc Startup storage cannot be allocated.
     * @throws std::length_error Authored strokes exceed the frozen scene capacity.
     */
    CinematicChoreography();
    CinematicChoreography(const CinematicChoreography&) = delete;
    CinematicChoreography& operator=(const CinematicChoreography&) = delete;
    CinematicChoreography(CinematicChoreography&&) = delete;
    CinematicChoreography& operator=(CinematicChoreography&&) = delete;

    /** Evaluates macro wake, ordered assembly, hero hold and menu composition.
     * @param elapsed Authored time, clamped to zero through duration.
     * @param drawableAspect Actual drawable width/height, finite in [1/32,32].
     * @return Complete scene borrowing reused storage until the next call.
     * @throws std::invalid_argument Unsupported or nonfinite aspect.
     * @note Coordinator-only. No allocation after construction; no wall-clock
     * sampling. Decorative interpolation does not claim simulation replay parity.
     */
    [[nodiscard]] CinematicFrame evaluate(std::chrono::milliseconds elapsed,
        float drawableAspect);
private:
    std::vector<NanitePose> settled_;
    std::vector<NanitePose> poses_;
    std::vector<std::uint8_t> letters_;
};
}
