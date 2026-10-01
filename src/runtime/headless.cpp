#include <crucible/runtime/headless.hpp>
#include <crucible/simulation.hpp>

namespace crucible::runtime {
void run_ticks(Simulation& simulation, std::size_t count) {
    for (std::size_t tick = 0; tick < count; ++tick) simulation.tick();
}
}
