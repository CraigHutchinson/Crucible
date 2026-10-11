#include <algorithm>
#include <array>
#include <bit>
#include <cfenv>
#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "crucible/fields/FieldSet.hpp"
#include "crucible/presentation/ScenarioSnapshot.hpp"
#include "crucible/runtime/InspectorSession.hpp"
#include "crucible/simulation.hpp"
#include "crucible/spatial/Grid.hpp"
#include "crucible/swarm/Steering.hpp"

namespace {
using namespace crucible;
using Session = runtime::InspectorSession;
constexpr auto interval = std::chrono::nanoseconds{16'666'667};

void require(bool value) {
    if (!value) throw std::runtime_error("parallel simulation receiving failed");
}

void requireSame(const presentation::ScenarioSnapshot& first, const presentation::ScenarioSnapshot& second) {
    require(first.HasEqualState(second));
    const auto left = first.GetSamples(), right = second.GetSamples();
    for (std::size_t index = 0; index < left.size(); ++index) {
        const auto& a = left[index];
        const auto& b = right[index];
        const std::array x{a.position.x, a.position.y, a.velocity.x, a.velocity.y};
        const std::array y{b.position.x, b.position.y, b.velocity.x, b.velocity.y};
        for (std::size_t component = 0; component < x.size(); ++component)
            require(std::bit_cast<std::uint32_t>(x[component]) == std::bit_cast<std::uint32_t>(y[component]));
    }
}

void receiveCommitOracle(std::size_t population, ExecutionSettings execution, bool structural)
{
    const GridConfig config{32, 16, 1};
    Simulation::ScenarioOptions options{config, 2, SteeringSettings{}, ResourceSettings{}};
    options.rowExecution = execution;
    if (structural) {
        options.structural = StructuralSettings{};
        options.structural->relay_center = population == 64 ? Position{16, 1} : Position{16, 4};
        options.structural->eligibility_radius = population == 64 ? 32 : 6;
    }
    Simulation simulation{population, options};
    presentation::ScenarioSnapshot before{population, 2, 512}, after{population, 2, 512};
    fields::FieldSet fields{2};
    spatial::Grid grid{config, population};
    swarm::Steering steering{config, *options.steering, population};
    const FieldEdit flow{FieldEditKind::set_flow, 0, {1, 4}, 12, 4, {30, 10}};
    require(simulation.TryApplyFieldEdit(flow));
    require(fields.TryApplyEdit(flow) == fields::EditResult::applied);
    const auto boundary = [&] {
        require(before.TryCapture(simulation));
        std::vector<SampleState> input;
        std::vector<spatial::SpatialSample> positions;
        for (const auto& sample : before.GetSamples()) {
            if (sample.activity != SampleActivity::mobile) continue;
            input.push_back(sample);
            positions.push_back({sample.id, sample.position});
        }
        require(std::ranges::is_sorted(input, {}, &SampleState::id));
        require(grid.TryRebuild(positions));
        std::vector<SampleState> expected(input.size());
        // The public all-row reference bypasses Simulation's gather, row mapping and commit.
        require(steering.TryCompute(input, fields, grid, expected));
        simulation.tick();
        require(after.TryCapture(simulation));
        require(after.GetInfo()->completed_tick == before.GetInfo()->completed_tick + 1);
        for (const auto& actual : after.GetSamples()) {
            const auto previous = std::ranges::find(before.GetSamples(), actual.id, &SampleState::id);
            require(previous != before.GetSamples().end());
            const auto oracle = actual.activity == SampleActivity::mobile
                ? std::ranges::find(expected, actual.id, &SampleState::id) : expected.end();
            require(actual.activity == previous->activity);
            if (actual.activity == SampleActivity::mobile) require(oracle != expected.end());
            const auto& wanted = oracle == expected.end() ? *previous : *oracle;
            const std::array observed{actual.position.x, actual.position.y, actual.velocity.x, actual.velocity.y};
            const std::array reference{wanted.position.x, wanted.position.y, wanted.velocity.x, wanted.velocity.y};
            for (std::size_t component = 0; component < observed.size(); ++component)
                require(std::bit_cast<std::uint32_t>(observed[component]) ==
                    std::bit_cast<std::uint32_t>(reference[component]));
        }
    };
    boundary();
    boundary();
    if (!structural) return;
    for (int cycle = 0; cycle < (population == 64 ? 1 : 2); ++cycle) {
        require(simulation.TryFuseRelay() == StructuralCommandResult::applied);
        const auto structure = simulation.GetStructuralState().value();
        if (population != 64) require(structure.members.front().value > 1);
        boundary();
        boundary();
        require(simulation.TryShatterRelay(structure.generation) == StructuralCommandResult::applied);
        boundary();
        require(std::ranges::count(after.GetSamples(), SampleActivity::lost, &SampleState::activity) ==
            (cycle + 1) * static_cast<int>(StructuralSettings::cost - StructuralSettings::refund));
        boundary();
    }
}

void receiveScenario(ScenarioSettings settings, bool structural) {
    Session sequential{settings};
    Session pair{settings, {64, 4096}, std::nullopt, Session::ExecutionPath::integrated,
        Session::Diagnostics::disabled, {16, true}, {2, 0}};
    Session parallel{settings, {64, 4096}, std::nullopt, Session::ExecutionPath::integrated,
        Session::Diagnostics::disabled, {16, true}, {4, 7}};
    const std::array sessions{&sequential, &pair, &parallel};
    require(pair.getRowExecution().workers == 2 && pair.getRowExecution().partitions == 4);
    require(sequential.getRowExecutionStorage().queryScratchCapacity == settings.population);
    require(sequential.getRowExecutionStorage().taskCapacity == 1 && sequential.getRowExecutionStorage().queueCapacity == 0);
    require(pair.getRowExecutionStorage().queryScratchCapacity == settings.population * 4);
    require(pair.getRowExecutionStorage().taskCapacity == 4 && pair.getRowExecutionStorage().queueCapacity == 4);
    const auto boundary = [&] {
        for (auto* session : sessions) require(session->TryPump(interval).advanced_ticks == 1);
        requireSame(sequential.GetSnapshot(), pair.GetSnapshot());
        requireSame(sequential.GetSnapshot(), parallel.GetSnapshot());
        for (auto* session : sessions) require(session->getRowFallbackCount() == 0);
        require(pair.getTickObservations().back().simulation->workers == 2);
        require(pair.getTickObservations().back().simulation->partitions == 4);
        const auto statistics = *parallel.getTickObservations().back().simulation;
        require(statistics.workers == 4 && statistics.partitions == 7 && statistics.taskCapacity == 7);
        require(statistics.queryScratchCapacity == settings.population * 7);
    };
    const FieldEdit flow{FieldEditKind::set_flow, 0, {4, 4}, 16, 4, {52, 12}};
    for (auto* session : sessions)
        require(session->TryAdmitFieldEdit(flow).status == runtime::CommandIngress::AdmissionStatus::accepted);
    boundary();
    for (auto* session : sessions) session->Pause();
    const FieldEdit repel{FieldEditKind::set, 1, {20, 16}, 12, -4};
    for (auto* session : sessions) {
        require(session->TryAdmitFieldEdit(repel).status == runtime::CommandIngress::AdmissionStatus::accepted);
        require(session->TryPump(std::chrono::seconds{1}).advanced_ticks == 0);
        require(session->GetTrace().size() == 1);
        session->Resume();
    }
    boundary();
    if (structural) {
        for (auto* session : sessions)
            require(session->TryFuseRelay().status == runtime::CommandIngress::AdmissionStatus::accepted);
        boundary();
        require(parallel.GetSnapshot().GetInfo()->structural->occupied);
        for (auto* session : sessions)
            require(session->TryShatterRelay().status == runtime::CommandIngress::AdmissionStatus::accepted);
        boundary();
        require(parallel.GetSnapshot().GetInfo()->biomass->lost_mass == 16);
    }
    const auto trace = parallel.GetTrace();
    Simulation replay{settings.population, {settings.grid, settings.fieldCapacity, settings.steering,
        settings.resources, settings.structural, false, {2, 5}}};
    runtime::HeadlessSession replaySession{replay, {64, 4096}};
    require(replaySession.TryReplay(trace, parallel.GetSummary().completed_tick).status ==
        runtime::HeadlessSession::StepStatus::advanced);
    const auto extent = *settings.grid.TryValidate();
    presentation::ScenarioSnapshot received{settings.population, settings.fieldCapacity, extent.cells};
    require(received.TryCapture(replay));
    requireSame(parallel.GetSnapshot(), received);
    for (auto* session : sessions) session->Restart();
    require(pair.getRunId() == 2 && parallel.getRunId() == 2);
    for (auto* session : sessions) require(session->getRowFallbackCount() == 0);
    require(pair.getRowExecution().workers == 2 && pair.getRowExecution().partitions == 4);
    require(pair.getTickObservations().empty() && parallel.getTickObservations().empty());
    boundary();
}

void receiveBounds() {
    Simulation legacy{0};
    legacy.tick();
    require(legacy.getRowFallbackCount() == 0);
    const ScenarioSettings settings{.population = 23};
    for (const auto execution : {ExecutionSettings{0, 1}, {33, 1}, {2, 24}, {1, 129}}) {
        bool rejected{};
        try {
            Session invalid{settings, {64, 4096}, std::nullopt, Session::ExecutionPath::integrated,
                Session::Diagnostics::disabled, {}, execution};
        } catch (const std::invalid_argument&) { rejected = true; }
        require(rejected);
    }
    Session empty{ScenarioSettings{.population = 0}, {64, 4096}, std::nullopt,
        Session::ExecutionPath::integrated, Session::Diagnostics::disabled, {1, true}, {1, 0}};
    require(empty.TryPump(interval).advanced_ticks == 1);
    require(empty.getTickObservations().front().simulation->inputRows == 0);
    require(empty.getRowFallbackCount() == 0);
    bool rejected{};
    try { Simulation noSteering{23, {{64, 32, 1}, 4, std::nullopt, std::nullopt, std::nullopt, false, {2, 4}}}; }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected);
}
}

int main() {
    std::fenv_t original{};
    require(std::fegetenv(&original) == 0);
    struct Restore {
        std::fenv_t& environment;
        ~Restore() { static_cast<void>(std::fesetenv(&environment)); }
    } restore{original};
    receiveBounds();
    for (const int rounding : {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO}) {
        require(std::fesetround(rounding) == 0);
        receiveScenario(ScenarioSettings{.population = 257}, false);
    }
    require(std::fesetround(FE_TONEAREST) == 0);
    for (const auto population : {0U, 1U, 257U})
        receiveCommitOracle(population, {1, 1}, false);
    for (const auto execution : {ExecutionSettings{1, 1}, ExecutionSettings{2, 4}, ExecutionSettings{4, 7}}) {
        receiveCommitOracle(257, execution, true);
        receiveCommitOracle(64, execution, true);
    }
    ScenarioSettings structural;
    structural.structural = StructuralSettings{};
    structural.structural->eligibility_radius = 80;
    receiveScenario(structural, true);
    receiveScenario(ScenarioSettings{.population = 100000, .grid = {400, 250, 1}}, false);
    receiveScenario(ScenarioSettings{.population = 150000, .grid = {500, 300, 1}}, false);
}
