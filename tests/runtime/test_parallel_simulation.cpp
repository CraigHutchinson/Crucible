#include <array>
#include <bit>
#include <cfenv>
#include <chrono>
#include <cstdint>
#include <stdexcept>

#include "crucible/presentation/ScenarioSnapshot.hpp"
#include "crucible/runtime/InspectorSession.hpp"
#include "crucible/simulation.hpp"

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
    require(pair.getRowExecution().workers == 2 && pair.getRowExecution().partitions == 4);
    require(pair.getTickObservations().empty() && parallel.getTickObservations().empty());
    boundary();
}

void receiveBounds() {
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
    ScenarioSettings structural;
    structural.structural = StructuralSettings{};
    structural.structural->eligibility_radius = 80;
    receiveScenario(structural, true);
    receiveScenario(ScenarioSettings{.population = 100000, .grid = {400, 250, 1}}, false);
    receiveScenario(ScenarioSettings{.population = 150000, .grid = {500, 300, 1}}, false);
}
