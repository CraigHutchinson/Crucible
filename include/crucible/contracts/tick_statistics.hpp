#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

namespace crucible {
/** Copied CPU phase intervals for one successfully committed simulation tick.
 * Produced only in an explicit attribution arm. These are wall-clock intervals,
 * not GPU measurements, task sums or complete Runtime boundary durations.
 */
struct TickStatistics {
    std::uint64_t completedTick{};
    std::chrono::nanoseconds gather{}, index{}, propose{}, commit{}, resources{}, rebuild{};
    std::size_t inputRows{}, queryRows{}, occupiedCells{}, queryScratchCapacity{}; ///< Scratch is the total across all startup partitions.
    std::size_t workers{1}, partitions{1}, taskCapacity{}; ///< Effective path's selected worker bound, active ranges and fixed graph-node bound; not observed thread utilization.
};
}
