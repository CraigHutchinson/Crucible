#pragma once
#include <cstddef>

namespace crucible { class Simulation; }

namespace crucible::runtime {
/// Run an exact number of sequential ticks without a wall clock. Simulation is
/// borrowed only for this call and must be exclusively accessed by the caller.
/// Zero ticks leave it unchanged; failures propagate without scheduling more ticks.
void run_ticks(Simulation& simulation, std::size_t count);
}
