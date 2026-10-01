#include <crucible/runtime/headless.hpp>
#include <crucible/simulation.hpp>
#include <cmath>

int main() {
    crucible::Simulation empty{0};
    crucible::runtime::run_ticks(empty, 60);
    if (empty.checksum() != 0.0) return 1;

    crucible::Simulation simulation{1000};
    crucible::runtime::run_ticks(simulation, 0);
    if (simulation.checksum() != 0.0) return 1;
    crucible::runtime::run_ticks(simulation, 1);
    if (std::abs(simulation.checksum() - 25.0) > 0.001) return 1;
    crucible::runtime::run_ticks(simulation, 59);
    return std::abs(simulation.checksum() - 1500.0) > 0.01 ? 1 : 0;
}
