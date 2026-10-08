#include <crucible/runtime/InspectorSession.hpp>
#include <crucible/runtime/IntentDelivery.hpp>
#include <crucible/runtime/BoundaryPipeline.hpp>
#if CRUCIBLE_ENABLE_DIAGNOSTICS
#include <crucible/runtime/RuntimeDiagnostics.hpp>
#endif
#include <limits>
#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <crucible/simulation.hpp>
#include <stdexcept>
#include <vector>
namespace crucible::runtime {
namespace {
GridExtent requireExtent(GridConfig grid) {
    const auto extent = grid.TryValidate();
    if (!extent) throw std::invalid_argument("Invalid inspector scenario geometry");
    return *extent;
}
}
struct InspectorSession::Run {
    const GridExtent extent;
    Simulation simulation;
    HeadlessSession session;
    std::optional<ReclamationMissionProgress> mission, pending_mission;
    std::uint64_t startup_reserve{};
    std::unique_ptr<IntentDelivery> delivery;
    std::unique_ptr<presentation::ScenarioSnapshot> direct_frame;
    std::unique_ptr<BoundaryPipeline> graph;
    const std::uint64_t runId;
    std::vector<TickObservation> tickObservations;
    std::size_t tickObservationCount{};
    std::uint64_t droppedTickObservations{};
    ClockDriver clock;
    Run(const ScenarioSettings& scenario, HeadlessSession::Limits limits,
            std::optional<ReclamationMissionSettings> settings,
            ExecutionPath execution, std::uint64_t run_id, ObservationSettings observations)
        : extent(requireExtent(scenario.grid)),
          simulation(scenario.population, {scenario.grid, scenario.fieldCapacity,
              scenario.steering, scenario.resources, scenario.structural, observations.simulationStages}),
          session(simulation, limits),
          delivery(execution == ExecutionPath::integrated
              ? std::make_unique<IntentDelivery>(session.GetIngress(), run_id) : nullptr),
          direct_frame(execution == ExecutionPath::direct
              ? std::make_unique<presentation::ScenarioSnapshot>(scenario.population, scenario.fieldCapacity, extent.cells) : nullptr),
          graph(execution == ExecutionPath::integrated
              ? std::make_unique<BoundaryPipeline>(session, simulation, scenario.population, scenario.fieldCapacity, extent.cells,
                    [this](const auto& frame) { return mission ? StageMission(frame) : false; }) : nullptr),
          runId(run_id), tickObservations(observations.capacity),
          clock(session, graph ? std::function<bool()>{[this] { return graph->IsTerminal(); }}
              : (settings ? std::function<bool()>{[this] {
                  return UpdateMission(simulation.GetBiomassLedger(), simulation.GetStructuralState(),
                      session.GetCompletedTick());
                }} : std::function<bool()>{}),
              graph ? std::function<HeadlessSession::StepResult()>{[this] { return StepGraph(); }}
                    : std::function<HeadlessSession::StepResult()>{}) {
        if (direct_frame && !direct_frame->TryCapture(simulation))
            throw std::logic_error("initial inspector frame capacity");
        if (settings) {
            const auto ledger = simulation.GetBiomassLedger();
            if (!ledger) throw std::logic_error("mission requires finite reclamation");
            if (settings->target_reclaimed == 0 || settings->deadline_ticks == 0 ||
                settings->target_reclaimed > ledger->remaining_stock)
                throw std::invalid_argument("Invalid reclamation mission quota/deadline");
            startup_reserve = ledger->reserve;
            mission = ReclamationMissionProgress{*settings};
        }
    }
    const presentation::ScenarioSnapshot& Frame() const noexcept {
        return graph ? graph->GetFrame() : *direct_frame;
    }
    CommandIngress::Admission Admit(std::span<const BoundaryCommand> commands) {
        return delivery ? delivery->TryAdmitCommands(commands).admission
                        : session.GetIngress().TryAdmitCommands(commands);
    }
    HeadlessSession::StepResult StepGraph() {
        const bool observed = !tickObservations.empty();
        const auto before = observed ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};
        const auto commandsBefore = observed ? session.GetTrace().size() : 0;
        pending_mission.reset();
        const auto result = graph->TryStep();
        if (result.status == HeadlessSession::StepStatus::advanced && pending_mission)
            mission = pending_mission; // noexcept value commit after successful frame publication
        if (observed && result.status == HeadlessSession::StepStatus::advanced) {
            const auto end = std::chrono::steady_clock::now();
            if (tickObservationCount < tickObservations.size())
                tickObservations[tickObservationCount++] = {runId, session.GetCompletedTick(),
                    std::chrono::duration_cast<std::chrono::nanoseconds>(end - before),
                    session.GetTrace().size() - commandsBefore, simulation.getTickStatistics()};
            else if (droppedTickObservations != std::numeric_limits<std::uint64_t>::max())
                ++droppedTickObservations;
        }
        return result;
    }
    bool StageMission(const presentation::ScenarioSnapshot& frame) {
        const auto info = frame.GetInfo();
        if (!info) throw std::logic_error("mission frame invariant");
        pending_mission = ComputeMission(info->biomass, info->structural, info->completed_tick);
        return pending_mission->outcome != ReclamationMissionOutcome::active;
    }
    bool UpdateMission(std::optional<BiomassLedger> ledger,
            std::optional<StructuralState> structure, std::uint64_t tick) {
        mission = ComputeMission(ledger, structure, tick);
        return mission->outcome != ReclamationMissionOutcome::active;
    }
    ReclamationMissionProgress ComputeMission(std::optional<BiomassLedger> ledger,
            std::optional<StructuralState> structure, std::uint64_t tick) const {
        if (!mission || !ledger || ledger->reserve < startup_reserve)
            throw std::logic_error("mission reserve invariant");
        auto next = *mission;
        next.reclaimed = structure ? ledger->harvested : ledger->reserve - startup_reserve;
        next.completed_tick = tick;
        if (next.reclaimed >= next.settings.target_reclaimed &&
            (!structure || (structure->occupied && structure->hold_ticks >= structure->settings.hold_ticks)))
            next.outcome = ReclamationMissionOutcome::won;
        else if (next.completed_tick >= next.settings.deadline_ticks ||
            (structure && ledger->mobile_mass + (structure->occupied ? StructuralSettings::refund : 0) < StructuralSettings::cost))
            next.outcome = ReclamationMissionOutcome::lost;
        return next;
    }
};
InspectorSession::InspectorSession(std::size_t samples, HeadlessSession::Limits limits,
        std::optional<ReclamationMissionSettings> mission, bool structural, ExecutionPath execution,
        Diagnostics diagnostics)
    : InspectorSession(ScenarioSettings{.population = samples,
          .structural = structural ? std::optional{StructuralSettings{}} : std::nullopt},
          limits, structural && !mission ? std::optional{ReclamationMissionSettings{}} : mission,
          execution, diagnostics) {}
InspectorSession::InspectorSession(ScenarioSettings scenario, HeadlessSession::Limits limits,
        std::optional<ReclamationMissionSettings> mission, ExecutionPath execution,
        Diagnostics diagnostics, ObservationSettings observations)
    : scenarioSettings_(scenario), limits_(limits), mission_(mission),
      execution_(execution), observationSettings_(observations) {
    if ((observations.capacity && execution != ExecutionPath::integrated) ||
        (observations.simulationStages && !observations.capacity))
        throw std::invalid_argument("tick observations require integrated execution and positive attribution capacity");
    run_ = std::make_unique<Run>(scenarioSettings_, limits, mission_, execution, run_id_, observations);
#if CRUCIBLE_ENABLE_DIAGNOSTICS
    if (diagnostics == Diagnostics::bounded) {
        try { diagnostics_ = std::make_unique<RuntimeDiagnostics>(); }
        catch (const std::bad_alloc&) { diagnostics_.reset(); }
    }
#else
    if (diagnostics != Diagnostics::disabled)
        throw std::invalid_argument("bounded diagnostics require CRUCIBLE_ENABLE_DIAGNOSTICS=ON");
#endif
}
InspectorSession::~InspectorSession() = default;
ClockDriver::PumpResult InspectorSession::TryPump(std::chrono::nanoseconds elapsed) {
    const auto result = run_->clock.TryPump(elapsed);
    if (result.advanced_ticks && run_->direct_frame && !run_->direct_frame->TryCapture(run_->simulation)) {
        run_->clock.Close();
        throw std::logic_error("inspector frame capacity invariant");
    }
#if CRUCIBLE_ENABLE_DIAGNOSTICS
    if (diagnostics_) {
        const auto trace = run_->session.GetTrace();
        for (const auto& applied : trace.subspan(diagnostic_trace_size_))
            if (applied.command.action != BoundaryAction::field &&
                applied.result != StructuralCommandResult::applied)
                diagnostics_->RecordStructuralRefusal({run_id_, applied.tick}, applied.command, applied.result);
        diagnostic_trace_size_ = trace.size();
        if (run_->mission && run_->mission->outcome != ReclamationMissionOutcome::active &&
            !mission_summary_recorded_) {
            diagnostics_->RecordMissionSummary(run_id_, *run_->mission);
            mission_summary_recorded_ = true;
        }
    }
#endif
    return result;
}
CommandIngress::Admission InspectorSession::AdmitCommand(const BoundaryCommand& command) {
    const auto receipt = run_->Admit(std::span{&command, 1});
#if CRUCIBLE_ENABLE_DIAGNOSTICS
    if (diagnostics_ && receipt.status != CommandIngress::AdmissionStatus::accepted)
        diagnostics_->RecordAdmissionRefusal({run_id_, run_->session.GetCompletedTick()}, command, receipt.status);
#endif
    return receipt;
}
CommandIngress::Admission InspectorSession::TryAdmitFieldEdit(const FieldEdit& edit) {
    const auto command = BoundaryCommand{edit};
    return AdmitCommand(command);
}
CommandIngress::Admission InspectorSession::TryFuseRelay() {
    const auto command = BoundaryCommand::Fuse();
    return AdmitCommand(command);
}
CommandIngress::Admission InspectorSession::TryShatterRelay() {
    const auto structure = run_->simulation.GetStructuralState();
    const auto command = BoundaryCommand::Shatter(structure ? structure->generation : 0);
    return AdmitCommand(command);
}
std::optional<StructuralCommandResult> InspectorSession::GetLastStructuralResult() const noexcept {
    const auto trace = run_->session.GetTrace();
    for (auto index = trace.size(); index > 0; --index)
        if (trace[index - 1].command.action != BoundaryAction::field) return trace[index - 1].result;
    return std::nullopt;
}
void InspectorSession::Pause() noexcept { run_->clock.Pause(); }
void InspectorSession::Resume() noexcept { run_->clock.Resume(); }
void InspectorSession::Close() { run_->clock.Close(); }
void InspectorSession::Restart() {
    if (run_id_ == std::numeric_limits<std::uint64_t>::max())
        throw std::overflow_error("inspector run identifier exhausted");
    auto replacement = std::make_unique<Run>(scenarioSettings_, limits_, mission_, execution_, run_id_ + 1, observationSettings_);
    run_->clock.Close();
    run_.swap(replacement);
    ++run_id_;
    diagnostic_trace_size_ = 0;
    mission_summary_recorded_ = false;
}
ClockDriver::Status InspectorSession::GetStatus() const noexcept { return run_->clock.GetStatus(); }
ClockDriver::Summary InspectorSession::GetSummary() const { return run_->clock.GetSummary(); }
std::optional<ReclamationMissionProgress> InspectorSession::GetMission() const noexcept { return run_->mission; }
const presentation::ScenarioSnapshot& InspectorSession::GetSnapshot() const noexcept { return run_->Frame(); }
std::span<const AppliedCommand> InspectorSession::GetTrace() const noexcept { return run_->session.GetTrace(); }
std::span<const InspectorSession::TickObservation> InspectorSession::getTickObservations() const noexcept {
    return std::span{run_->tickObservations}.first(run_->tickObservationCount);
}
std::uint64_t InspectorSession::getDroppedTickObservations() const noexcept {
    return run_->droppedTickObservations;
}
const RuntimeDiagnostics* InspectorSession::GetDiagnostics() const noexcept {
#if CRUCIBLE_ENABLE_DIAGNOSTICS
    return diagnostics_.get();
#else
    return nullptr;
#endif
}
}
