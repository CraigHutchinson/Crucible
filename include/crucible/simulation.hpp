#pragma once
#include <crucible/contracts/Position.hpp>
#include <crucible/contracts/Velocity.hpp>
#include <crucible/contracts/FieldEdit.hpp>
#include <crucible/contracts/GridConfig.hpp>
#include <crucible/contracts/StateCopy.hpp>
#include <crucible/contracts/SteeringSettings.hpp>
#include <crucible/contracts/ResourceSettings.hpp>
#include "crucible/contracts/tick_statistics.hpp"
#include "crucible/contracts/execution_settings.hpp"
#include "crucible/contracts/row_execution_storage.hpp"
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
        std::optional<ResourceSettings> resources{}; ///< Enables finite reclamation; absent preserves legacy spread.
        std::optional<StructuralSettings> structural{}; ///< Unit-mass fixed-identity relay mode; requires resources.
        bool observeTimings{}; ///< Explicit CPU attribution arm; disabled ordinary/timing comparison path.
        ExecutionSettings rowExecution{}; ///< Positive resolved startup counts; requires steering when changed from 1/1.
    };

    /// Legacy ECS-only workload; no fields, grid or Blight state is constructed.
    explicit Simulation(std::size_t count);
    /// Allocate the bounded scenario at startup. Invalid geometry/count/settings,
    /// unrepresentable biomass or allocation failure throws before a usable Simulation
    /// exists. Population never grows.
    Simulation(std::size_t count, ScenarioOptions options);
    ~Simulation();
    Simulation(const Simulation&) = delete;
    Simulation& operator=(const Simulation&) = delete;
    Simulation(Simulation&&) = delete;
    Simulation& operator=(Simulation&&) = delete;

    void tick();
    /** Copies phase timing for the last successfully committed observed tick.
     * @return Absent when attribution is disabled, before the first tick or after a failed tick.
     * @note Exclusive coordinator access; no storage borrow escapes.
     */
    [[nodiscard]] std::optional<TickStatistics> getTickStatistics() const noexcept;
    /** Copies actual startup row scratch, graph and pending-queue bounds.
     * @return Zero bounds for scenarios without the row adapter.
     * @note Coordinator-only; includes retained capacity during FP fallback.
     */
    [[nodiscard]] RowExecutionStorage getRowExecutionStorage() const noexcept;
    /** Counts completed ticks recomputed sequentially after joined unsupported FP.
     * @return Cumulative fallback count; rejected/failed ticks are excluded.
     * @note Coordinator-only; zero without the row adapter. No failure is masked.
     */
    [[nodiscard]] std::uint64_t getRowFallbackCount() const noexcept;
    [[nodiscard]] double checksum();
    /// Boundary-only field application; false leaves field slots unchanged. The
    /// legacy workload has zero slots and rejects every edit.
    [[nodiscard]] bool TryApplyFieldEdit(const FieldEdit& edit) noexcept;
    [[nodiscard]] std::size_t GetFieldCapacity() const noexcept;
    /// Boundary-only atomic transitions; normal refusal leaves simulation state unchanged.
    [[nodiscard]] StructuralCommandResult TryFuseRelay() noexcept;
    [[nodiscard]] StructuralCommandResult TryShatterRelay(std::uint64_t generation) noexcept;
    /// Owned observation with current mobile eligibility; no retained ECS/query borrow.
    [[nodiscard]] std::optional<StructuralState> GetStructuralState() const noexcept;
    /// Completed scenario observations; return zero for the legacy workload.
    [[nodiscard]] std::size_t GetBlightInfectedCount() const noexcept;
    [[nodiscard]] std::size_t GetOccupiedCellCount() const noexcept;
    /** Returns an owned completed-boundary ledger without copying world geometry.
     * @return Finite-resource quantities, or nullopt for scenarios without resources.
     * @note Requires exclusive coordinator access; no retained ECS/storage borrow.
     */
    [[nodiscard]] std::optional<BiomassLedger> GetBiomassLedger() const noexcept;
    /// Consume an immediate radius query without exposing the grid's borrowed buffer.
    /// Invalid input or the legacy workload returns nullopt.
    [[nodiscard]] std::optional<std::size_t> TryCountNeighbors(Position center, float radius) noexcept;
    /** Copies sorted samples, all field slots, row-major infection and resource stock.
     * @param[out] destination Mutually disjoint caller-owned spans; stocks required only when resources are enabled.
     * @return Geometry, used lengths, completed tick and optional biomass ledger; nullopt on capacity failure or ECS-only mode.
     * @note Requires exclusive coordinator access. Rejection writes nothing. Boundary edits
     * already applied are visible; capture after a completed boundary for replay.
     */
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
