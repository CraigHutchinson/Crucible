#include <crucible/runtime/HeadlessSession.hpp>

#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>

#include <crucible/simulation.hpp>

namespace {
using crucible::FieldEdit;
using crucible::FieldEditKind;
using crucible::Simulation;
using crucible::runtime::AppliedCommand;
using crucible::runtime::CommandIngress;
using crucible::runtime::HeadlessSession;
using StepStatus = HeadlessSession::StepStatus;

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

Simulation::ScenarioOptions Options() { return {{4, 4, 1.0F}, 2}; }
FieldEdit Edit(std::size_t slot) { return {FieldEditKind::set, slot, {0, 0}, 10.0F, 1.0F}; }

bool SameRecord(const AppliedCommand& left, const AppliedCommand& right) {
    return left.sequence == right.sequence && left.tick == right.tick &&
        left.edit.kind == right.edit.kind && left.edit.slot == right.edit.slot &&
        left.edit.center.x == right.edit.center.x && left.edit.center.y == right.edit.center.y &&
        left.edit.radius == right.edit.radius && left.edit.strength == right.edit.strength;
}

void CheckPauseResumeAndReplay() {
    Simulation simulation{3, Options()};
    HeadlessSession session{simulation, {4, 4}};
    session.Pause();
    auto paint = std::array{Edit(0)};
    Require(session.GetIngress().TryAdmit(paint).status == CommandIngress::AdmissionStatus::accepted,
        "paused input admitted");
    paint[0].strength = 999.0F;
    const auto before = simulation.checksum();
    const auto paused = session.TryStep();
    Require(paused.status == StepStatus::paused && paused.tick == 0 &&
        simulation.checksum() == before && session.GetTrace().empty(), "pause suppresses boundaries");
    Require(session.GetIngress().GetStatistics().pending == 1, "pause retains input");
    session.Resume();
    const auto first = session.TryStep();
    Require(first.status == StepStatus::advanced && first.tick == 1 && first.applied_commands == 1,
        "resume applies queued command");
    Require(session.GetTrace()[0].edit.strength == 1.0F && session.GetTrace()[0].tick == 1,
        "trace owns admitted value and applied tick");
    const std::array changes{FieldEdit{FieldEditKind::remove, 0, {}, 0, 0}, Edit(1)};
    Require(session.GetIngress().TryAdmit(changes).status == CommandIngress::AdmissionStatus::accepted,
        "second boundary batch admitted");
    const auto second = session.TryStep();
    Require(second.status == StepStatus::advanced && second.tick == 2 && second.applied_commands == 2,
        "atomic batch applies at one boundary");
    Require(session.TryStep().status == StepStatus::advanced, "empty boundary advances");

    Simulation replayed{3, Options()};
    HeadlessSession replay{replayed, {4, 4}};
    const auto result = replay.TryReplay(session.GetTrace(), 3);
    Require(result.status == StepStatus::advanced && result.tick == 3 && result.applied_commands == 3,
        "replay exact boundary count");
    Require(replayed.checksum() == simulation.checksum(), "exact replay simulation observation");
    Require(replayed.GetBlightInfectedCount() == simulation.GetBlightInfectedCount(),
        "exact replay cellular observation");
    const auto original_trace = session.GetTrace();
    const auto replay_trace = replay.GetTrace();
    Require(original_trace.size() == replay_trace.size(), "replay trace count");
    for (std::size_t index{}; index < original_trace.size(); ++index)
        Require(SameRecord(original_trace[index], replay_trace[index]), "exact replay owned records");
    Require(replay.TryStep().status == StepStatus::closed, "replay closes live admission");
    session.GetIngress().Close();
    Require(session.TryStep().status == StepStatus::closed, "close stops new ticks");
}

void CheckTraceCapacity() {
    Simulation simulation{3, Options()};
    HeadlessSession session{simulation, {2, 1}};
    const std::array batch{Edit(0), Edit(1)};
    Require(session.GetIngress().TryAdmit(batch).status == CommandIngress::AdmissionStatus::accepted,
        "trace exhaustion input accepted");
    const auto before = simulation.checksum();
    const auto full = session.TryStep();
    Require(full.status == StepStatus::trace_full && full.tick == 0 && full.applied_commands == 0,
        "trace exhaustion explicit result");
    Require(simulation.checksum() == before && session.GetTrace().empty() &&
        session.GetIngress().GetStatistics().pending == 2, "trace full changes no boundary state");
    session.GetIngress().Close();
    Require(session.TryStep().status == StepStatus::closed, "close with exhausted trace and full queue");

    Simulation empty{0};
    HeadlessSession no_trace{empty, {1, 0}};
    Require(no_trace.TryStep().status == StepStatus::advanced, "zero trace permits empty ticks");
}

void CheckReplayValidation() {
    Simulation simulation{3, Options()};
    HeadlessSession session{simulation, {2, 2}};
    const auto before = simulation.checksum();
    std::array trace{AppliedCommand{Edit(0), 1, 1}, AppliedCommand{Edit(1), 2, 2}};
    trace[1].sequence = 1;
    Require(session.TryReplay(trace, 2).status == StepStatus::replay_invalid,
        "duplicate sequence rejected before mutation");
    trace[1].sequence = 2;
    trace[1].tick = 0;
    Require(session.TryReplay(trace, 2).status == StepStatus::replay_invalid, "zero tick rejected");
    trace[1].tick = 2;
    trace[1].edit.slot = 2;
    Require(session.TryReplay(trace, 2).status == StepStatus::replay_invalid, "invalid slot rejected");
    trace[1].edit.slot = 1;
    Require(session.TryReplay(trace, 1).status == StepStatus::replay_invalid, "short replay rejected");
    Require(simulation.checksum() == before && session.GetTrace().empty() &&
        !session.GetIngress().CaptureCutoff().closed, "invalid replay leaves pristine state open");
    session.Pause();
    Require(session.TryReplay(trace, 2).status == StepStatus::paused, "paused replay does not start");
    session.Resume();
    const auto result = session.TryReplay(trace, 2);
    Require(result.status == StepStatus::advanced && result.tick == 2, "valid replay after rejection");
    Require(session.TryReplay(trace, 2).status == StepStatus::replay_invalid, "nonpristine replay rejected");

    Simulation bounded_simulation{3, Options()};
    HeadlessSession bounded{bounded_simulation, {2, 1}};
    Require(bounded.TryReplay(trace, 2).status == StepStatus::trace_full, "bounded replay capacity");
    Require(bounded_simulation.checksum() == before && bounded.GetTrace().empty(),
        "oversized replay has no partial application");
}

void CheckStartupFailure() {
    Simulation simulation{0};
    bool failed{};
    try { HeadlessSession invalid{simulation, {1, std::numeric_limits<std::size_t>::max()}}; }
    catch (const std::length_error&) { failed = true; }
    Require(failed, "trace arithmetic overflow fails construction");
}
}

int main() {
    try {
        CheckPauseResumeAndReplay();
        CheckTraceCapacity();
        CheckReplayValidation();
        CheckStartupFailure();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
