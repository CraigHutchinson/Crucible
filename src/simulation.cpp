#include <crucible/simulation.hpp>

namespace crucible {
Simulation::Simulation(std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        world_.create(Position{}, Velocity{1.0F, 0.5F});
    }
}
void Simulation::tick() {
    world_.each<Position, Velocity>([](Position& position, Velocity& velocity) {
        constexpr float step = 1.0F / 60.0F;
        position.x += velocity.x * step;
        position.y += velocity.y * step;
    });
}
double Simulation::checksum() {
    double sum = 0.0;
    world_.each<Position>([&sum](Position& position) { sum += position.x + position.y; });
    return sum;
}
}
