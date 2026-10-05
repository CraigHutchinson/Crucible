#include <crucible/simulation.hpp>

#include <algorithm>
#include <cmath>
#include <exception>
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
          tick_input(options.steering || options.resources ? count : 0), next_state(options.steering ? count : 0),
          activities(options.structural ? count : 0, SampleActivity::mobile) {
        if (options.structural) {
            const auto& settings = *options.structural;
            if (!options.resources || options.resources->mass_per_sample != 1 ||
                !std::isfinite(settings.relay_center.x) || !std::isfinite(settings.relay_center.y) ||
                settings.relay_center.x < 0 || settings.relay_center.x > extent.width ||
                settings.relay_center.y < 0 || settings.relay_center.y > extent.height ||
                !std::isfinite(settings.eligibility_radius) || settings.eligibility_radius < 0 ||
                !std::isfinite(settings.protection_radius) || settings.protection_radius < 0 || settings.hold_ticks == 0)
                throw std::invalid_argument("Invalid structural scenario settings");
            structural.emplace();
            structural->settings = settings;
        }
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
    std::vector<SampleActivity> activities;
    std::optional<StructuralState> structural;

    SampleActivity Activity(SampleId id) const noexcept {
        return activities.empty() ? SampleActivity::mobile : activities[static_cast<std::size_t>(id.value - 1)];
    }
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
    std::size_t index = 0, total = 0;
    state.world.each<Position, Velocity, SampleId>([&](const Position& position, const Velocity&, const SampleId& id) {
        ++total;
        if (state.Activity(id) == SampleActivity::mobile) state.samples[index++] = {id, position};
    });
    if (total != state.samples.size() || !state.grid.TryRebuild(std::span{state.samples}.first(index)))
        throw std::logic_error("Scenario spatial rebuild violated fixed identity population");
}

void Simulation::tick() {
    if (completed_ticks_ == std::numeric_limits<std::uint64_t>::max())
        throw std::overflow_error("Simulation tick identity exhausted");
    if (scenario_) {
        auto& state = *scenario_;
        if (state.steering) {
            std::size_t index = 0;
            state.world.each<Position, Velocity, SampleId>([&](const Position& position, const Velocity& velocity, const SampleId& id) {
                if (state.Activity(id) == SampleActivity::mobile) state.tick_input[index++] = {id, position, velocity};
            });
            auto input = std::span{state.tick_input}.first(index);
            auto output = std::span{state.next_state}.first(index);
            std::ranges::sort(input, {}, &SampleState::id);
            for (std::size_t i = 0; i < index; ++i)
                state.samples[i] = {state.tick_input[i].id, state.tick_input[i].position};
            if (!state.grid.TryRebuild(std::span{state.samples}.first(index)) ||
                !state.steering->TryCompute(input, state.fields, state.grid, output))
                throw std::logic_error("Scenario steering rejected tick-start state");
            state.world.each<Position, Velocity, SampleId>([&](Position& position, Velocity& velocity, const SampleId& id) {
                if (state.Activity(id) != SampleActivity::mobile) return;
                const auto sample = std::ranges::lower_bound(output, id, {}, &SampleState::id);
                position = sample->position;
                velocity = sample->velocity;
            });
        } else {
            state.world.each<Position, Velocity, SampleId>([&](Position& position, Velocity& velocity, const SampleId& id) {
                if (state.Activity(id) != SampleActivity::mobile) return;
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
                if (state.Activity(id) == SampleActivity::mobile) state.tick_input[index++] = {id, position, velocity};
            });
            std::optional<interactions::Reclamation::ProtectedArea> protection;
            if (state.structural && state.structural->occupied)
                protection = interactions::Reclamation::ProtectedArea{state.structural->settings.relay_center, state.structural->settings.protection_radius};
            if (!state.reclamation->TryStep(state.blight, std::span{state.tick_input}.first(index), protection))
                throw std::logic_error("Scenario reclamation rejected post-move state");
        } else {
            state.blight.Step();
        }
        RebuildSpatial();
        ++completed_ticks_;
        if (state.structural && state.structural->occupied &&
            state.structural->hold_ticks != std::numeric_limits<std::uint64_t>::max())
            ++state.structural->hold_ticks;
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
std::optional<StructuralState> Simulation::GetStructuralState() const noexcept {
    if (!scenario_ || !scenario_->structural) return std::nullopt;
    auto result = *scenario_->structural;
    result.eligible_mobile = 0;
    const double radius = result.settings.eligibility_radius;
    scenario_->world.each<Position, Velocity, SampleId>([&](const Position& position, const Velocity&, const SampleId& id) {
        if (scenario_->Activity(id) != SampleActivity::mobile) return;
        const double x = static_cast<double>(position.x) - result.settings.relay_center.x;
        const double y = static_cast<double>(position.y) - result.settings.relay_center.y;
        result.eligible_mobile += x * x + y * y <= radius * radius;
    });
    return result;
}

StructuralCommandResult Simulation::TryFuseRelay() noexcept {
    if (!scenario_ || !scenario_->structural) return StructuralCommandResult::disabled;
    auto& state = *scenario_;
    auto& structure = *state.structural;
    if (structure.occupied) return StructuralCommandResult::occupied;
    if (structure.generation == std::numeric_limits<std::uint64_t>::max())
        return StructuralCommandResult::generation_exhausted;
    const auto neighbors = state.grid.TryQuery(structure.settings.relay_center, structure.settings.eligibility_radius);
    if (!neighbors || neighbors->size() < StructuralSettings::cost) return StructuralCommandResult::insufficient_mass;
    std::array<SampleId, StructuralSettings::cost> members;
    std::ranges::copy(neighbors->first(StructuralSettings::cost), members.begin());
    if (!state.reclamation->TryAnchor(StructuralSettings::cost)) std::terminate();
    structure.members = members;
    structure.occupied = true;
    ++structure.generation;
    structure.hold_ticks = 0;
    state.world.each<Position, Velocity, SampleId>([&](Position& position, Velocity& velocity, const SampleId& id) {
        if (!std::ranges::binary_search(members, id)) return;
        state.activities[static_cast<std::size_t>(id.value - 1)] = SampleActivity::anchored;
        position = structure.settings.relay_center;
        velocity = {};
    });
    RebuildSpatial();
    return StructuralCommandResult::applied;
}

StructuralCommandResult Simulation::TryShatterRelay(std::uint64_t generation) noexcept {
    if (!scenario_ || !scenario_->structural) return StructuralCommandResult::disabled;
    auto& state = *scenario_;
    auto& structure = *state.structural;
    if (!structure.occupied) return StructuralCommandResult::empty;
    if (generation != structure.generation) return StructuralCommandResult::stale_generation;
    // Private membership and ledger are coordinated exclusively; this transfer
    // cannot fail after successful fusion with the immutable cost/refund rules.
    if (!state.reclamation->TryRelease(StructuralSettings::cost, StructuralSettings::refund))
        std::terminate();
    state.world.each<Position, Velocity, SampleId>([&](Position& position, Velocity& velocity, const SampleId& id) {
        const auto member = std::ranges::lower_bound(structure.members, id);
        if (member == structure.members.end() || *member != id) return;
        state.activities[static_cast<std::size_t>(id.value - 1)] =
            static_cast<std::size_t>(member - structure.members.begin()) < StructuralSettings::refund
                ? SampleActivity::mobile : SampleActivity::lost;
        position = structure.settings.relay_center;
        velocity = {};
    });
    structure.occupied = false;
    structure.hold_ticks = 0;
    structure.members = {};
    RebuildSpatial();
    return StructuralCommandResult::applied;
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
        destination.samples[index++] = {id, position, velocity, state.Activity(id)};
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
    return ScenarioStateInfo{state.options.grid, completed_ticks_, count, state.options.field_capacity, state.extent.cells, ledger, GetStructuralState()};
}
}
