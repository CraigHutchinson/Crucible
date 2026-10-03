#include <crucible/simulation.hpp>

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <vector>

#include <crucible/blight/Grid.hpp>
#include <crucible/contracts/timing.hpp>
#include <crucible/contracts/SampleId.hpp>
#include <crucible/fields/FieldSet.hpp>
#include <crucible/interactions/Reclamation.hpp>
#include <crucible/spatial/Grid.hpp>
#include <crucible/swarm/integration.hpp>
#include <crucible/swarm/Steering.hpp>

namespace crucible {
namespace {
std::size_t ValidatePopulation(std::size_t count) {
    // The pinned ECS allocator masks indices to 24 bits without an overflow result.
    if (count > (std::size_t{1} << 24)) throw std::length_error("Population exceeds ECS identity domain");
    return count;
}

GridExtent RequireExtent(GridConfig grid) {
    const auto extent = grid.TryValidate();
    if (!extent) throw std::invalid_argument("Invalid scenario geometry");
    return *extent;
}
}

struct Simulation::ScenarioState {
    using Queries = std::tuple<sub0ecs::Query<Position, Velocity, SampleId>, sub0ecs::Query<Position>>;
    ScenarioState(ScenarioOptions options, std::size_t count)
        : options(options), extent(RequireExtent(options.grid)), grid(options.grid, count),
          fields(options.field_capacity), blight(options.grid), samples(count),
          tick_input(options.steering || options.resources ? count : 0), next_state(options.steering ? count : 0) {
        if (options.steering) steering.emplace(options.grid, *options.steering, count);
        if (options.resources) reclamation.emplace(options.grid, count, *options.resources);
        if (!blight.TrySeed(options.grid.columns / 2, options.grid.rows / 2))
            throw std::logic_error("Validated scenario seed was out of bounds");
    }

    ScenarioOptions options;
    GridExtent extent;
    spatial::Grid grid;
    fields::FieldSet fields;
    blight::Grid blight;
    std::vector<spatial::SpatialSample> samples;
    std::vector<SampleState> tick_input, next_state;
    std::optional<swarm::Steering> steering;
    std::optional<interactions::Reclamation> reclamation;
    sub0ecs::store::World<Queries> world;
};

Simulation::Simulation(std::size_t count) { Populate(ValidatePopulation(count)); }

Simulation::Simulation(std::size_t count, ScenarioOptions options)
    : scenario_(std::make_unique<ScenarioState>(options, ValidatePopulation(count))) {
    Populate(count);
    RebuildSpatial();
}

Simulation::~Simulation() = default;

void Simulation::Populate(std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        if (scenario_) {
            const auto config = scenario_->options.grid;
            const Position position{
                static_cast<float>((static_cast<double>(i % config.columns) + 0.5) * config.cell_size),
                static_cast<float>((static_cast<double>((i / config.columns) % config.rows) + 0.5) * config.cell_size)};
            scenario_->world.create(position, Velocity{1.0F, 0.5F}, SampleId{static_cast<std::uint64_t>(i) + 1});
        } else {
            world_.create(Position{}, Velocity{1.0F, 0.5F});
        }
    }
}

void Simulation::RebuildSpatial() {
    auto& state = *scenario_;
    std::size_t index = 0;
    state.world.each<Position, Velocity, SampleId>([&](const Position& position, const Velocity&, const SampleId& id) {
        state.samples[index++] = {id, position};
    });
    // Fixed population and immutable identities make this a validation failure,
    // not an overflow policy; no successful tick is reported after an invariant failure.
    if (index != state.samples.size() || !state.grid.TryRebuild(state.samples))
        throw std::logic_error("Scenario spatial rebuild violated fixed population");
}

void Simulation::tick() {
    if (completed_ticks_ == std::numeric_limits<std::uint64_t>::max())
        throw std::overflow_error("Simulation tick identity exhausted");
    if (scenario_) {
        auto& state = *scenario_;
        if (state.steering) {
            std::size_t index = 0;
            state.world.each<Position, Velocity, SampleId>([&](const Position& position, const Velocity& velocity, const SampleId& id) {
                state.tick_input[index++] = {id, position, velocity};
            });
            std::ranges::sort(state.tick_input, {}, &SampleState::id);
            for (std::size_t i = 0; i < state.tick_input.size(); ++i)
                state.samples[i] = {state.tick_input[i].id, state.tick_input[i].position};
            if (index != state.tick_input.size() || !state.grid.TryRebuild(state.samples) ||
                !state.steering->TryCompute(state.tick_input, state.fields, state.grid, state.next_state))
                throw std::logic_error("Scenario steering rejected tick-start state");
            state.world.each<Position, Velocity, SampleId>([&](Position& position, Velocity& velocity, const SampleId& id) {
                const auto sample = std::ranges::lower_bound(state.next_state, id, {}, &SampleState::id);
                position = sample->position;
                velocity = sample->velocity;
            });
        } else {
            state.world.each<Position, Velocity, SampleId>([&](Position& position, Velocity& velocity, const SampleId&) {
                const auto acceleration = state.fields.Sample(position);
                constexpr double limit = std::numeric_limits<float>::max();
                velocity.x = static_cast<float>(std::clamp(static_cast<double>(velocity.x) +
                    static_cast<double>(acceleration.x) * tick_seconds, -limit, limit));
                velocity.y = static_cast<float>(std::clamp(static_cast<double>(velocity.y) +
                    static_cast<double>(acceleration.y) * tick_seconds, -limit, limit));
                // Double intermediates avoid overflow before clipping to finite world bounds.
                position.x = static_cast<float>(std::clamp(static_cast<double>(position.x) +
                    static_cast<double>(velocity.x) * tick_seconds, 0.0, static_cast<double>(state.extent.width)));
                position.y = static_cast<float>(std::clamp(static_cast<double>(position.y) +
                    static_cast<double>(velocity.y) * tick_seconds, 0.0, static_cast<double>(state.extent.height)));
            });
        }
        if (state.reclamation) {
            std::size_t index = 0;
            state.world.each<Position, Velocity, SampleId>([&](const Position& position, const Velocity& velocity, const SampleId& id) {
                state.tick_input[index++] = {id, position, velocity};
            });
            if (index != state.tick_input.size() || !state.reclamation->TryStep(state.blight, state.tick_input))
                throw std::logic_error("Scenario reclamation rejected post-move state");
        } else {
            state.blight.Step();
        }
        RebuildSpatial();
        ++completed_ticks_;
        return;
    }
    world_.each<Position, Velocity>([](Position& position, const Velocity& velocity) {
        swarm::integrate_position(position, velocity);
    });
    ++completed_ticks_;
}
double Simulation::checksum() {
    double sum = 0.0;
    if (scenario_) {
        scenario_->world.each<Position>([&sum](const Position& position) {
            sum += static_cast<double>(position.x) + static_cast<double>(position.y);
        });
    } else {
        world_.each<Position>([&sum](const Position& position) { sum += position.x + position.y; });
    }
    return sum;
}

bool Simulation::TryApplyFieldEdit(const FieldEdit& edit) noexcept {
    return scenario_ && scenario_->fields.TryApplyEdit(edit) == fields::EditResult::applied;
}

std::size_t Simulation::GetFieldCapacity() const noexcept {
    return scenario_ ? scenario_->options.field_capacity : 0;
}

std::size_t Simulation::GetBlightInfectedCount() const noexcept {
    return scenario_ ? scenario_->blight.GetInfectedCount() : 0;
}

std::size_t Simulation::GetOccupiedCellCount() const noexcept {
    return scenario_ ? scenario_->grid.GetOccupiedCellCount() : 0;
}

std::optional<BiomassLedger> Simulation::GetBiomassLedger() const noexcept {
    if (!scenario_ || !scenario_->reclamation) return std::nullopt;
    return scenario_->reclamation->GetLedger();
}

std::optional<std::size_t> Simulation::TryCountNeighbors(Position center, float radius) noexcept {
    if (!scenario_) return std::nullopt;
    const auto neighbors = scenario_->grid.TryQuery(center, radius);
    if (!neighbors) return std::nullopt;
    return neighbors->size();
}
}

namespace crucible {
std::optional<ScenarioStateInfo> Simulation::TryCopyState(StateCopyDestination destination) noexcept {
    if (!scenario_) return std::nullopt;
    auto& state = *scenario_;
    const auto count = state.samples.size();
    if (destination.samples.size() < count || destination.fields.size() < state.options.field_capacity ||
        destination.blight.size() < state.extent.cells ||
        (state.reclamation && destination.stocks.size() < state.extent.cells)) return std::nullopt;
    std::size_t index = 0;
    state.world.each<Position, Velocity, SampleId>([&](const Position& position, const Velocity& velocity, const SampleId& id) {
        destination.samples[index++] = {id, position, velocity};
    });
    // Population and identities are private and fixed after construction.
    std::ranges::sort(destination.samples.first(count), {}, &SampleState::id);
    static_cast<void>(state.fields.TryCopyEdits(destination.fields));
    for (std::size_t row = 0; row < state.options.grid.rows; ++row)
        for (std::size_t column = 0; column < state.options.grid.columns; ++column)
            destination.blight[row * state.options.grid.columns + column] = state.blight.IsInfected(column, row) ? 1 : 0;
    std::optional<BiomassLedger> ledger;
    if (state.reclamation) {
        std::ranges::copy(state.reclamation->GetStocks(), destination.stocks.begin());
        ledger = state.reclamation->GetLedger();
    }
    return ScenarioStateInfo{state.options.grid, completed_ticks_, count, state.options.field_capacity, state.extent.cells, ledger};
}
}
