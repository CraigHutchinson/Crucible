#include <crucible/simulation.hpp>
#include <iostream>

int main() {
    crucible::Simulation simulation{150'000};
    for (int tick = 0; tick < 60; ++tick) simulation.tick();
    std::cout << "Crucible headless ECS foundation: 150000 entities, 60 ticks, checksum="
              << simulation.checksum() << '\n';
}
