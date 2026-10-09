#include <chrono>
#include <stdexcept>

#include "crucible/presentation/ScenarioSnapshot.hpp"
#include "crucible/runtime/InspectorSession.hpp"

namespace {
void require(bool result) {
    if (!result) throw std::runtime_error("scenario receiving failed");
}
void receiveScale(crucible::ScenarioSettings settings) {
    using namespace crucible;
    using Session = runtime::InspectorSession;
    Session integrated{settings};
    Session direct{settings, {64, 4096}, std::nullopt, Session::ExecutionPath::direct};
    const auto receiveBoundary = [&] {
        require(integrated.TryPump(std::chrono::nanoseconds{16'666'667}).advanced_ticks == 1);
        require(direct.TryPump(std::chrono::nanoseconds{16'666'667}).advanced_ticks == 1);
        require(integrated.GetSnapshot().HasEqualState(direct.GetSnapshot()));
        const auto mass = *integrated.GetSnapshot().GetInfo()->biomass;
        require(mass.initial_total == mass.remaining_stock + mass.mobile_mass + mass.reserve);
    };
    require(integrated.GetSnapshot().HasEqualState(direct.GetSnapshot()));
    const FieldEdit flow{FieldEditKind::set_flow, 0, {20, 20}, 16, 4, {300, 120}};
    require(integrated.TryAdmitFieldEdit(flow).status == runtime::CommandIngress::AdmissionStatus::accepted);
    require(direct.TryAdmitFieldEdit(flow).status == runtime::CommandIngress::AdmissionStatus::accepted);
    receiveBoundary();
    integrated.Pause(); direct.Pause();
    const FieldEdit gather{FieldEditKind::set, 1, {120, 120}, 32, 4};
    require(integrated.TryAdmitFieldEdit(gather).status == runtime::CommandIngress::AdmissionStatus::accepted);
    require(direct.TryAdmitFieldEdit(gather).status == runtime::CommandIngress::AdmissionStatus::accepted);
    require(integrated.TryPump(std::chrono::seconds{1}).advanced_ticks == 0);
    require(direct.TryPump(std::chrono::seconds{1}).advanced_ticks == 0);
    require(integrated.GetTrace().size() == 1 && direct.GetTrace().size() == 1);
    integrated.Resume(); direct.Resume();
    receiveBoundary();
    receiveBoundary();
    require(integrated.GetTrace().size() == 2 && direct.GetTrace().size() == 2);
    integrated.Restart(); direct.Restart();
    require(integrated.getRunId() == 2 && direct.getRunId() == 2);
    require(integrated.GetTrace().empty() && direct.GetTrace().empty());
    require(integrated.GetSnapshot().HasEqualState(direct.GetSnapshot()));
    receiveBoundary();
}
}

int main() {
    using namespace crucible;
    ScenarioSettings settings{.population = 100000, .grid = {400, 250, 1.0F}};
    runtime::InspectorSession session{settings};
    const auto initial = session.GetSnapshot().GetInfo();
    require(initial && initial->samples == settings.population && initial->cells == 100000);
    require(initial->grid.columns == 400 && initial->grid.rows == 250 && !session.GetMission());
    require(session.getRunId() == 1 && session.GetSnapshot().GetSamples().size() == 100000);
    require(session.TryPump(std::chrono::nanoseconds{16'666'667}).advanced_ticks == 1);
    require(session.GetSnapshot().GetInfo()->completed_tick == 1);
    session.Restart();
    require(session.getRunId() == 2 && session.GetSnapshot().GetInfo()->completed_tick == 0);
    require(session.GetSnapshot().GetInfo()->cells == 100000 && !session.GetMission());
    receiveScale(settings);
    settings.population = 150000;
    settings.grid = {500, 300, 1.0F};
    runtime::InspectorSession large{settings};
    require(large.GetSnapshot().GetInfo()->samples == 150000);
    require(large.GetSnapshot().GetInfo()->cells == 150000);
    require(large.TryPump(std::chrono::nanoseconds{16'666'667}).advanced_ticks == 1);
    require(!large.GetMission() && large.GetSnapshot().GetInfo()->completed_tick == 1);
    receiveScale(settings);
    settings.grid.columns = 0;
    bool rejected = false;
    try { runtime::InspectorSession invalid{settings}; }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected);
}
