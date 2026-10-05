#include <crucible/runtime/InspectorSession.hpp>
#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <crucible/simulation.hpp>
#include <stdexcept>
namespace crucible::runtime {
struct InspectorSession::Run {
    Simulation simulation;
    HeadlessSession session;
    std::optional<ReclamationMissionProgress> mission;
    std::uint64_t startup_reserve{};
    ClockDriver clock;
    presentation::ScenarioSnapshot frame;
    Run(std::size_t samples, HeadlessSession::Limits limits,
            std::optional<ReclamationMissionSettings> settings, bool structural)
        : simulation(samples, {{64, 32, 1.0F}, 4, SteeringSettings{}, ResourceSettings{},
              structural ? std::optional{StructuralSettings{}} : std::nullopt}),
          session(simulation, limits),
          clock(session, settings ? std::function<bool()>{[this] { return EvaluateMission(); }}
                                  : std::function<bool()>{}), frame(samples, 4, 2048) {
        if (!frame.TryCapture(simulation)) throw std::logic_error("initial inspector frame capacity");
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
    bool EvaluateMission() {
        const auto ledger = simulation.GetBiomassLedger();
        if (!mission || !ledger || ledger->reserve < startup_reserve)
            throw std::logic_error("mission reserve invariant");
        const auto structure = simulation.GetStructuralState();
        mission->reclaimed = structure ? ledger->harvested : ledger->reserve - startup_reserve;
        mission->completed_tick = session.GetCompletedTick();
        if (mission->reclaimed >= mission->settings.target_reclaimed &&
            (!structure || (structure->occupied && structure->hold_ticks >= structure->settings.hold_ticks)))
            mission->outcome = ReclamationMissionOutcome::won;
        else if (mission->completed_tick >= mission->settings.deadline_ticks ||
            (structure && ledger->mobile_mass + (structure->occupied ? StructuralSettings::refund : 0) < StructuralSettings::cost))
            mission->outcome = ReclamationMissionOutcome::lost;
        return mission->outcome != ReclamationMissionOutcome::active;
    }
};
InspectorSession::InspectorSession(std::size_t samples, HeadlessSession::Limits limits,
        std::optional<ReclamationMissionSettings> mission, bool structural)
    : samples_(samples), limits_(limits), mission_(structural && !mission ? std::optional{ReclamationMissionSettings{}} : mission),
      structural_(structural), run_(std::make_unique<Run>(samples, limits, mission_, structural)) {}
InspectorSession::~InspectorSession() = default;
ClockDriver::PumpResult InspectorSession::TryPump(std::chrono::nanoseconds elapsed) {
    const auto result = run_->clock.TryPump(elapsed);
    if (result.advanced_ticks && !run_->frame.TryCapture(run_->simulation)) {
        run_->clock.Close();
        throw std::logic_error("inspector frame capacity invariant");
    }
    return result;
}
CommandIngress::Admission InspectorSession::TryAdmitFieldEdit(const FieldEdit& edit) {
    return run_->session.GetIngress().TryAdmit(std::span{&edit, 1});
}
CommandIngress::Admission InspectorSession::TryFuseRelay() {
    const auto command = BoundaryCommand::Fuse();
    return run_->session.GetIngress().TryAdmitCommands(std::span{&command, 1});
}
CommandIngress::Admission InspectorSession::TryShatterRelay() {
    const auto structure = run_->simulation.GetStructuralState();
    const auto command = BoundaryCommand::Shatter(structure ? structure->generation : 0);
    return run_->session.GetIngress().TryAdmitCommands(std::span{&command, 1});
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
    auto replacement = std::make_unique<Run>(samples_, limits_, mission_, structural_);
    run_->clock.Close();
    run_.swap(replacement);
}
ClockDriver::Status InspectorSession::GetStatus() const noexcept { return run_->clock.GetStatus(); }
ClockDriver::Summary InspectorSession::GetSummary() const { return run_->clock.GetSummary(); }
std::optional<ReclamationMissionProgress> InspectorSession::GetMission() const noexcept { return run_->mission; }
const presentation::ScenarioSnapshot& InspectorSession::GetSnapshot() const noexcept { return run_->frame; }
std::span<const AppliedCommand> InspectorSession::GetTrace() const noexcept { return run_->session.GetTrace(); }
}
