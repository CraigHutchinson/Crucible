#include <crucible/simulation.hpp>
#include <sub0pipeline/sub0pipeline.hpp>
#include <sub0pub/sub0pub.hpp>
#include <sub0log/log.hpp>
#include <cmath>
#include <iostream>

int main() {
    crucible::Simulation empty{0};
    empty.tick();
    if (empty.checksum() != 0.0) return 1;
    crucible::Simulation simulation{1000};
    simulation.tick();
    if (std::abs(simulation.checksum() - 25.0) > 0.001) {
        std::cerr << "ECS position integration failed\n";
        return 1;
    }
    for (int i = 1; i < 60; ++i) simulation.tick();
    return std::abs(simulation.checksum() - 1500.0) > 0.01 ? 1 : 0;
}
