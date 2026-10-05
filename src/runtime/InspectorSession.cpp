#include <crucible/runtime/InspectorSession.hpp>
#include <crucible/runtime/IntentDelivery.hpp>
#include <crucible/runtime/BoundaryPipeline.hpp>
#include <limits>
#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <crucible/simulation.hpp>
#include <stdexcept>
namespace crucible::runtime {
struct InspectorSession::Run {
    Simulation simulation;
    HeadlessSession session;
    std::optional<ReclamationMissionProgress> mission, pending_mission;
    std::uint64_t startup_reserve{};
    std::unique_ptr<IntentDelivery> delivery;
    std::unique_ptr<presentation::ScenarioSnapshot> direct_frame;
    std::unique_ptr<BoundaryPipeline> graph;
    ClockDriver clock;
    Run(std::size_t samples, HeadlessSession::Limits limits,
            std::optional<ReclamationMissionSettings> settings, bool structural,
            ExecutionPath execution, std::uint64_t run_id)
        : simulation(samples, {{64, 32, 1.0F}, 4, SteeringSettings{}, ResourceSettings{},
              structural ? std::optional{StructuralSettings{}} : std::nullopt}),
          session(simulation, limits),
          delivery(execution == ExecutionPath::integrated
              ? std::make_unique<IntentDelivery>(session.GetIngress(), run_id) : nullptr),
          direct_frame(execution == ExecutionPath::direct
              ? std::make_unique<presentation::ScenarioSnapshot>(samples, 4, 2048) : nullptr),
          graph(execution == ExecutionPath::integrated
              ? std::make_unique<BoundaryPipeline>(session, simulation, samples, 4, 2048,
                    [this](const auto& frame) { return mission ? StageMission(frame) : false; }) : nullptr),
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
        pending_mission.reset();
        const auto result = graph->TryStep();
        if (result.status == HeadlessSession::StepStatus::advanced && pending_mission)
            mission = pending_mission; // noexcept value commit after successful frame publication
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
        std::optional<ReclamationMissionSettings> mission, bool structural, ExecutionPath execution)
    : samples_(samples), limits_(limits), mission_(structural && !mission ? std::optional{ReclamationMissionSettings{}} : mission),
      structural_(structural), execution_(execution),
      run_(std::make_unique<Run>(samples, limits, mission_, structural, execution, run_id_)) {}
InspectorSession::~InspectorSession() = default;
ClockDriver::PumpResult InspectorSession::TryPump(std::chrono::nanoseconds elapsed) {
    const auto result = run_->clock.TryPump(elapsed);
    if (result.advanced_ticks && run_->direct_frame && !run_->direct_frame->TryCapture(run_->simulation)) {
        run_->clock.Close();
        throw std::logic_error("inspector frame capacity invariant");
    }
    return result;
}
CommandIngress::Admission InspectorSession::TryAdmitFieldEdit(const FieldEdit& edit) {
    const auto command = BoundaryCommand{edit};
    return run_->Admit(std::span{&command, 1});
}
CommandIngress::Admission InspectorSession::TryFuseRelay() {
    const auto command = BoundaryCommand::Fuse();
    return run_->Admit(std::span{&command, 1});
}
CommandIngress::Admission InspectorSession::TryShatterRelay() {
    const auto structure = run_->simulation.GetStructuralState();
    const auto command = BoundaryCommand::Shatter(structure ? structure->generation : 0);
    return run_->Admit(std::span{&command, 1});
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
    auto replacement = std::make_unique<Run>(samples_, limits_, mission_, structural_, execution_, run_id_ + 1);
    run_->clock.Close();
    run_.swap(replacement);
    ++run_id_;
}
ClockDriver::Status InspectorSession::GetStatus() const noexcept { return run_->clock.GetStatus(); }
ClockDriver::Summary InspectorSession::GetSummary() const { return run_->clock.GetSummary(); }
std::optional<ReclamationMissionProgress> InspectorSession::GetMission() const noexcept { return run_->mission; }
const presentation::ScenarioSnapshot& InspectorSession::GetSnapshot() const noexcept { return run_->Frame(); }
std::span<const AppliedCommand> InspectorSession::GetTrace() const noexcept { return run_->session.GetTrace(); }
}
