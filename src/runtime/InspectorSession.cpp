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
            std::optional<ReclamationMissionSettings> settings)
        : simulation(samples, {{64, 32, 1.0F}, 4, SteeringSettings{}, ResourceSettings{}}),
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
        mission->reclaimed = ledger->reserve - startup_reserve;
        mission->completed_tick = session.GetCompletedTick();
        if (mission->reclaimed >= mission->settings.target_reclaimed)
            mission->outcome = ReclamationMissionOutcome::won;
        else if (mission->completed_tick >= mission->settings.deadline_ticks)
            mission->outcome = ReclamationMissionOutcome::lost;
        return mission->outcome != ReclamationMissionOutcome::active;
    }
};
InspectorSession::InspectorSession(std::size_t samples, HeadlessSession::Limits limits,
        std::optional<ReclamationMissionSettings> mission)
    : samples_(samples), limits_(limits), mission_(mission),
      run_(std::make_unique<Run>(samples, limits, mission)) {}
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
void InspectorSession::Pause() noexcept { run_->clock.Pause(); }
void InspectorSession::Resume() noexcept { run_->clock.Resume(); }
void InspectorSession::Close() { run_->clock.Close(); }
void InspectorSession::Restart() {
    auto replacement = std::make_unique<Run>(samples_, limits_, mission_);
    run_->clock.Close();
    run_.swap(replacement);
}
ClockDriver::Status InspectorSession::GetStatus() const noexcept { return run_->clock.GetStatus(); }
ClockDriver::Summary InspectorSession::GetSummary() const { return run_->clock.GetSummary(); }
std::optional<ReclamationMissionProgress> InspectorSession::GetMission() const noexcept { return run_->mission; }
const presentation::ScenarioSnapshot& InspectorSession::GetSnapshot() const noexcept { return run_->frame; }
std::span<const AppliedCommand> InspectorSession::GetTrace() const noexcept { return run_->session.GetTrace(); }
}
