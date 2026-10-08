#include <chrono>
#include <stdexcept>

#include "crucible/presentation/ScenarioSnapshot.hpp"
#include "crucible/runtime/InspectorSession.hpp"

namespace {
void require(bool result) {
    if (!result) throw std::runtime_error("scenario receiving failed");
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
    settings.population = 150000;
    settings.grid = {500, 300, 1.0F};
    runtime::InspectorSession large{settings};
    require(large.GetSnapshot().GetInfo()->samples == 150000);
    require(large.GetSnapshot().GetInfo()->cells == 150000);
    require(large.TryPump(std::chrono::nanoseconds{16'666'667}).advanced_ticks == 1);
    require(!large.GetMission() && large.GetSnapshot().GetInfo()->completed_tick == 1);
    settings.grid.columns = 0;
    bool rejected = false;
    try { runtime::InspectorSession invalid{settings}; }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected);
}
