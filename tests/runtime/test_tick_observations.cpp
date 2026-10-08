#include <chrono>
#include <stdexcept>

#include "crucible/presentation/ScenarioSnapshot.hpp"
#include "crucible/runtime/InspectorSession.hpp"

namespace {
void require(bool condition) {
    if (!condition) throw std::runtime_error("tick observation receiving failed");
}
}

int main() {
    using namespace crucible;
    using Session = runtime::InspectorSession;
    constexpr auto interval = std::chrono::nanoseconds{16'666'667};
    const ScenarioSettings settings{.population = 257};
    Session oracle{settings};
    Session observed{settings, {64, 4096}, std::nullopt, Session::ExecutionPath::integrated,
        Session::Diagnostics::disabled, {2, true}};
    require(observed.getTickObservations().empty() && observed.getDroppedTickObservations() == 0);
    const FieldEdit edit{FieldEditKind::set_flow, 0, {4, 4}, 16, 4, {52, 12}};
    require(oracle.TryAdmitFieldEdit(edit).status == runtime::CommandIngress::AdmissionStatus::accepted);
    require(observed.TryAdmitFieldEdit(edit).status == runtime::CommandIngress::AdmissionStatus::accepted);
    require(oracle.TryPump(interval).advanced_ticks == 1);
    require(observed.TryPump(interval).advanced_ticks == 1);
    require(observed.GetSnapshot().HasEqualState(oracle.GetSnapshot()));
    require(oracle.getTickObservations().empty());
    const auto prefix = observed.getTickObservations();
    const auto first = prefix.front();
    require(first.runId == 1 && first.completedTick == 1 && first.appliedCommands == 1);
    require(first.boundary.count() >= 0 && first.simulation && first.simulation->completedTick == 1);
    const auto phases = *first.simulation;
    require(phases.inputRows == 257 && phases.queryRows == 257 && phases.queryScratchCapacity == 257);
    require(phases.occupiedCells > 0 && phases.workers == 1 && phases.partitions == 1 && phases.taskCapacity == 0);
    require(phases.gather + phases.index + phases.propose + phases.commit + phases.resources + phases.rebuild <= first.boundary);
    observed.Pause();
    require(observed.TryPump(std::chrono::seconds{1}).advanced_ticks == 0);
    require(observed.getTickObservations().size() == 1 && observed.getDroppedTickObservations() == 0);
    observed.Resume();
    require(observed.TryPump(interval * 3).advanced_ticks == 3);
    const auto full = observed.getTickObservations();
    require(full.size() == 2 && full[1].completedTick == 2 && full[1].appliedCommands == 0);
    require(prefix.front().completedTick == first.completedTick && prefix.front().boundary == first.boundary);
    require(observed.getDroppedTickObservations() == 2 && observed.GetSummary().completed_tick == 4);
    observed.Restart();
    require(observed.getRunId() == 2 && observed.getTickObservations().empty() && observed.getDroppedTickObservations() == 0);
    require(observed.TryPump(interval).advanced_ticks == 1 && observed.getTickObservations().front().runId == 2);

    Session boundaryOnly{settings, {64, 4096}, std::nullopt, Session::ExecutionPath::integrated,
        Session::Diagnostics::disabled, {4, false}};
    require(boundaryOnly.TryPump(interval * 4).advanced_ticks == 4);
    require(boundaryOnly.getTickObservations().size() == 4 && !boundaryOnly.getTickObservations().front().simulation);
    boundaryOnly.Close();
    require(boundaryOnly.TryPump(interval).advanced_ticks == 0 && boundaryOnly.getTickObservations().size() == 4);

    for (const bool stages : {false, true}) {
        bool rejected{};
        try {
            Session invalid{settings, {64, 4096}, std::nullopt, Session::ExecutionPath::direct,
                Session::Diagnostics::disabled, {1, stages}};
        } catch (const std::invalid_argument&) { rejected = true; }
        require(rejected);
    }
    bool rejected{};
    try {
        Session invalid{settings, {64, 4096}, std::nullopt, Session::ExecutionPath::integrated,
            Session::Diagnostics::disabled, {0, true}};
    } catch (const std::invalid_argument&) { rejected = true; }
    require(rejected);
}
