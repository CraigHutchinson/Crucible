#pragma once
#include <sub0ecs/sub0ecs.hpp>
#include <cstddef>
#include <tuple>

namespace crucible {
struct Position { float x{}, y{}; };
struct Velocity { float x{}, y{}; };

// Initial ECS integration workload; flocking, fields and Blight are future systems.
class Simulation {
public:
    explicit Simulation(std::size_t count);
    void tick();
    [[nodiscard]] double checksum();
private:
    using Queries = std::tuple<sub0ecs::Query<Position, Velocity>, sub0ecs::Query<Position>>;
    sub0ecs::store::World<Queries> world_;
};
}
