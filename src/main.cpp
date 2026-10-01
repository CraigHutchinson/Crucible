#include <crucible/simulation.hpp>
#include <crucible/runtime/headless.hpp>
#include <iostream>

int main() {
    crucible::Simulation simulation{150'000};
    crucible::runtime::run_ticks(simulation, 60);
    std::cout << "Crucible headless ECS foundation: 150000 entities, 60 ticks, checksum="
              << simulation.checksum() << '\n';
}
