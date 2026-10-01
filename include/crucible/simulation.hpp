#pragma once
#include <crucible/contracts/Position.hpp>
#include <crucible/contracts/Velocity.hpp>
#include <crucible/contracts/FieldEdit.hpp>
#include <crucible/contracts/GridConfig.hpp>
#include <crucible/contracts/StateCopy.hpp>
#include <crucible/contracts/SteeringSettings.hpp>
#include <sub0ecs/sub0ecs.hpp>
#include <cstddef>
#include <memory>
#include <optional>
#include <tuple>

namespace crucible {
/// Owns the ECS world and optional bounded headless scenario. All calls require
/// exclusive coordinator access; no component or scratch borrow escapes a tick.
class Simulation {
public:
    /// Concrete scenario storage: fixed rectangular geometry and radial field slots.
    struct ScenarioOptions {
        GridConfig grid;
        std::size_t field_capacity{};
        std::optional<SteeringSettings> steering{};
    };

    /// Legacy ECS-only workload; no fields, grid or Blight state is constructed.
    explicit Simulation(std::size_t count);
    /// Allocate the bounded scenario at startup. Invalid geometry/count or allocation
    /// failure throws before a usable Simulation exists. Population never grows.
    Simulation(std::size_t count, ScenarioOptions options);
    ~Simulation();
    Simulation(const Simulation&) = delete;
    Simulation& operator=(const Simulation&) = delete;
    Simulation(Simulation&&) = delete;
    Simulation& operator=(Simulation&&) = delete;

    void tick();
    [[nodiscard]] double checksum();
    /// Boundary-only field application; false leaves field slots unchanged. The
    /// legacy workload has zero slots and rejects every edit.
    [[nodiscard]] bool TryApplyFieldEdit(const FieldEdit& edit) noexcept;
    [[nodiscard]] std::size_t GetFieldCapacity() const noexcept;
    /// Completed scenario observations; return zero for the legacy workload.
    [[nodiscard]] std::size_t GetBlightInfectedCount() const noexcept;
    [[nodiscard]] std::size_t GetOccupiedCellCount() const noexcept;
    /// Consume an immediate radius query without exposing the grid's borrowed buffer.
    /// Invalid input or the legacy workload returns nullopt.
    [[nodiscard]] std::optional<std::size_t> TryCountNeighbors(Position center, float radius) noexcept;
    /// Copies sorted samples, all field slots and row-major Blight into owned caller storage.
    /// Requires exclusive coordinator access. Capacity failure or legacy mode writes nothing.
    /// Boundary edits already applied are visible; capture after a completed boundary for replay.
    [[nodiscard]] std::optional<ScenarioStateInfo> TryCopyState(StateCopyDestination destination) noexcept;
private:
    struct ScenarioState;
    void Populate(std::size_t count);
    void RebuildSpatial();
    using Queries = std::tuple<sub0ecs::Query<Position, Velocity>, sub0ecs::Query<Position>>;
    sub0ecs::store::World<Queries> world_;
    std::unique_ptr<ScenarioState> scenario_;
    std::uint64_t completed_ticks_{};
};
}
