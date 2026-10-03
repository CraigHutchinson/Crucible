#include <crucible/runtime/InspectorSession.hpp>
#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <crucible/simulation.hpp>
#include <stdexcept>
namespace crucible::runtime {
struct InspectorSession::Run {
    Simulation simulation;
    HeadlessSession session;
    ClockDriver clock;
    presentation::ScenarioSnapshot frame;
    Run(std::size_t samples, HeadlessSession::Limits limits)
        : simulation(samples, {{64, 32, 1.0F}, 4, SteeringSettings{}, ResourceSettings{}}),
          session(simulation, limits), clock(session), frame(samples, 4, 2048) {
        if (!frame.TryCapture(simulation)) throw std::logic_error("initial inspector frame capacity");
    }
};
InspectorSession::InspectorSession(std::size_t samples, HeadlessSession::Limits limits)
    : samples_(samples), limits_(limits), run_(std::make_unique<Run>(samples, limits)) {}
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
    auto replacement = std::make_unique<Run>(samples_, limits_);
    run_->clock.Close();
    run_.swap(replacement);
}
ClockDriver::Status InspectorSession::GetStatus() const noexcept { return run_->clock.GetStatus(); }
ClockDriver::Summary InspectorSession::GetSummary() const { return run_->clock.GetSummary(); }
const presentation::ScenarioSnapshot& InspectorSession::GetSnapshot() const noexcept { return run_->frame; }
std::span<const AppliedCommand> InspectorSession::GetTrace() const noexcept { return run_->session.GetTrace(); }
}
