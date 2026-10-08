#pragma once

#include <cstddef>

namespace crucible {
/** Startup row execution policy shared by Runtime, Simulation and Scheduling.
 * One worker uses the coordinator. Multiple workers use an owned bounded pool.
 * Runtime alone may resolve partitions0; Scheduling receives positive resolved
 * counts. No asynchronous work or runtime worker-count mutation is implied.
 */
struct ExecutionSettings {
    std::size_t workers{1};
    std::size_t partitions{1};
};
}
