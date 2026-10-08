#pragma once

#include <cstddef>

namespace crucible {
/** Copies actual startup storage bounds of the consumed row execution adapter.
 * Query capacity counts SampleId slots across partition scratch, task capacity
 * counts graph nodes, and queue capacity counts pending pool submissions.
 * Zero means that storage is absent. This is not resident or peak process memory.
 */
struct RowExecutionStorage {
    std::size_t queryScratchCapacity{}, taskCapacity{}, queueCapacity{};
};
}
