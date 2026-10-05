#include <crucible/runtime/HeadlessSession.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

#include <crucible/simulation.hpp>
#include <crucible/presentation/ScenarioSnapshot.hpp>

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
        left.command.field.kind == right.command.field.kind && left.command.field.slot == right.command.field.slot &&
        left.command.field.center.x == right.command.field.center.x && left.command.field.center.y == right.command.field.center.y &&
        left.command.field.radius == right.command.field.radius && left.command.field.strength == right.command.field.strength &&
        left.command.field.end.x == right.command.field.end.x && left.command.field.end.y == right.command.field.end.y;
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
    Require(session.GetTrace()[0].command.field.strength == 1.0F && session.GetTrace()[0].tick == 1,
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

void CheckFlowFullReplay(bool steering) {
    const Simulation::ScenarioOptions options{{8, 4, 1}, 2,
        steering ? std::optional{crucible::SteeringSettings{}} : std::nullopt, crucible::ResourceSettings{}};
    Simulation live{3, options}, replayed{3, options};
    HeadlessSession session{live, {8, 8}}, replay{replayed, {8, 8}};
    crucible::presentation::ScenarioSnapshot first{3, 2, 32}, final{3, 2, 32}, expected{3, 2, 32};
    session.Pause();
    auto batch = std::array{FieldEdit{FieldEditKind::set_flow, 0, {.5F, .5F}, 2, 8, {3.5F, .5F}},
                            FieldEdit{FieldEditKind::set, 1, {2, 2}, 1, -2}};
    Require(session.GetIngress().TryAdmit(batch).status == CommandIngress::AdmissionStatus::accepted,
        "flow accepted while paused");
    batch[0].end = {};
    Require(session.TryStep().status == StepStatus::paused && session.GetTrace().empty() &&
        first.TryCapture(live) && first.GetFields()[0].kind == FieldEditKind::remove,
        "queued flow does not mutate paused snapshot");
    session.Resume();
    Require(session.TryStep().status == StepStatus::advanced && first.TryCapture(live) &&
        first.GetFields()[0].end.x == 3.5F && first.GetFields()[0].end.y == .5F &&
        session.GetTrace()[0].command.field.end.x == 3.5F, "completed flow owns admitted endpoint");
    if (!steering) {
        const auto sample = first.GetSamples()[0];
        constexpr double dt = 1.0 / 60.0;
        Require(std::abs(sample.velocity.x - (1.0 + 8.0 * dt)) < 0.000001 &&
            std::abs(sample.position.x - (.5 + (1.0 + 8.0 * dt) * dt)) < 0.000001 &&
            sample.velocity.y == .5F, "flow independently accelerates the production simulation");
    }
    const std::array replacement{FieldEdit{FieldEditKind::remove, 0},
        FieldEdit{FieldEditKind::set_flow, 1, {6, .5F}, 2, 4, {1, .5F}}};
    Require(session.GetIngress().TryAdmit(replacement).status == CommandIngress::AdmissionStatus::accepted &&
        session.TryStep().status == StepStatus::advanced && session.TryStep().status == StepStatus::advanced &&
        final.TryCapture(live), "flow replace and erase consumed at boundaries");
    Require(first.GetInfo()->completed_tick == 1 && first.GetFields()[0].kind == FieldEditKind::set_flow &&
        first.GetFields()[0].end.x == 3.5F && first.GetFields()[1].kind == FieldEditKind::set,
        "retained completed flow snapshot unchanged by later edits");
    Require(replay.TryReplay(session.GetTrace(), 3).status == StepStatus::advanced && expected.TryCapture(replayed),
        "flow trace independently replays");
    const auto a = *final.GetInfo(), b = *expected.GetInfo();
    Require(a.completed_tick == b.completed_tick && a.grid.columns == b.grid.columns &&
        a.grid.rows == b.grid.rows && a.grid.cell_size == b.grid.cell_size && a.samples == b.samples &&
        a.fields == b.fields && a.cells == b.cells && a.biomass == b.biomass, "flow full replay metadata/ledger");
    Require(std::ranges::equal(final.GetSamples(), expected.GetSamples(), [](const auto& p, const auto& q) {
        return p.id == q.id && p.position.x == q.position.x && p.position.y == q.position.y &&
            p.velocity.x == q.velocity.x && p.velocity.y == q.velocity.y;
    }) && std::ranges::equal(final.GetFields(), expected.GetFields(), [](const auto& p, const auto& q) {
        return p.kind == q.kind && p.slot == q.slot && p.center.x == q.center.x && p.center.y == q.center.y &&
            p.radius == q.radius && p.strength == q.strength && p.end.x == q.end.x && p.end.y == q.end.y;
    }) && std::ranges::equal(final.GetBlight(), expected.GetBlight()) &&
        std::ranges::equal(final.GetStocks(), expected.GetStocks()), "flow full replay samples/fields/infection/stock");
    Require(std::ranges::equal(session.GetTrace(), replay.GetTrace(), SameRecord), "flow full replay applied commands");
    auto invalid_trace = std::array{session.GetTrace()[0], session.GetTrace()[1]};
    invalid_trace[0].command.field.end = invalid_trace[0].command.field.center;
    Simulation untouched{3, options};
    HeadlessSession invalid_replay{untouched, {8, 8}};
    const auto before = untouched.checksum();
    Require(invalid_replay.TryReplay(invalid_trace, 1).status == StepStatus::replay_invalid &&
        untouched.checksum() == before && invalid_replay.GetCompletedTick() == 0 &&
        invalid_replay.GetTrace().empty() && !invalid_replay.GetIngress().CaptureCutoff().closed,
        "invalid endpoint replay rejected before all mutation");
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
    trace[1].command.field.slot = 2;
    Require(session.TryReplay(trace, 2).status == StepStatus::replay_invalid, "invalid slot rejected");
    trace[1].command.field.slot = 1;
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
        CheckFlowFullReplay(false);
        CheckFlowFullReplay(true);
        CheckTraceCapacity();
        CheckReplayValidation();
        CheckStartupFailure();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
