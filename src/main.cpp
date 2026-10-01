#include <crucible/simulation.hpp>

#include <array>
#include <iostream>
#include <stdexcept>

#include <crucible/runtime/headless.hpp>
#include <crucible/runtime/HeadlessSession.hpp>

namespace {
void RunScenario() {
    using namespace crucible;
    using runtime::HeadlessSession;
    constexpr Simulation::ScenarioOptions options{{64, 32, 1.0F}, 2};
    Simulation scenario{2048, options};
    HeadlessSession session{scenario, {8, 8}};
    const std::array paint{
        FieldEdit{FieldEditKind::set, 0, {24, 16}, 20, 4},
        FieldEdit{FieldEditKind::set, 1, {48, 8}, 8, -2}};
    if (session.GetIngress().TryAdmit(paint).status != runtime::CommandIngress::AdmissionStatus::accepted)
        throw std::runtime_error("Scenario paint admission failed");
    session.Pause();
    if (session.TryStep().status != HeadlessSession::StepStatus::paused)
        throw std::runtime_error("Paused scenario advanced");
    session.Resume();
    for (int tick = 0; tick < 20; ++tick) {
        if (tick == 10) {
            const std::array remove{FieldEdit{FieldEditKind::remove, 0}};
            if (session.GetIngress().TryAdmit(remove).status != runtime::CommandIngress::AdmissionStatus::accepted)
                throw std::runtime_error("Scenario removal admission failed");
        }
        if (session.TryStep().status != HeadlessSession::StepStatus::advanced)
            throw std::runtime_error("Scenario tick did not complete");
    }

    Simulation replayed{2048, options};
    HeadlessSession replay{replayed, {8, 8}};
    if (replay.TryReplay(session.GetTrace(), 20).status != HeadlessSession::StepStatus::advanced ||
        replayed.checksum() != scenario.checksum() ||
        replayed.GetBlightInfectedCount() != scenario.GetBlightInfectedCount() ||
        replayed.GetOccupiedCellCount() != scenario.GetOccupiedCellCount())
        throw std::runtime_error("Scenario replay diverged");
    const auto neighbors = scenario.TryCountNeighbors({32, 16}, 3);
    if (!neighbors) throw std::runtime_error("Scenario neighbor query failed");
    session.GetIngress().Close();
    std::cout << "Crucible wave1 scenario: entities=2048 ticks=20 commands=" << session.GetTrace().size()
              << " occupied_cells=" << scenario.GetOccupiedCellCount()
              << " nearby=" << *neighbors << " blight=" << scenario.GetBlightInfectedCount()
              << " replay=exact\n";
}
}

int main() {
    try {
        crucible::Simulation simulation{150'000};
        crucible::runtime::run_ticks(simulation, 60);
        std::cout << "Crucible headless ECS foundation: 150000 entities, 60 ticks, checksum="
                  << simulation.checksum() << '\n';
        RunScenario();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
