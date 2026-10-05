// Receive the startup-built production graph against direct committed state.
// Exercise retained publication, repeat epochs, normal suppression and fail-stop;
// no callback, ECS query or frame borrow is dispatched to another thread.
#include <crucible/runtime/BoundaryPipeline.hpp>
#include <crucible/simulation.hpp>
#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace crucible;
using namespace crucible::runtime;
using Status = HeadlessSession::StepStatus;
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
Simulation::ScenarioOptions Options() {
    return {{64, 32, 1}, 4, SteeringSettings{}, ResourceSettings{}, StructuralSettings{}};
}

void CheckRepeatAndStructuralParity() {
    Simulation integrated{2048, Options()}, direct{2048, Options()};
    HeadlessSession session{integrated, {8, 16}}, oracle{direct, {8, 16}};
    presentation::ScenarioSnapshot expected{2048, 4, 2048};
    std::size_t observations{};
    BoundaryPipeline* active{};
    BoundaryPipeline graph{session, integrated, 2048, 4, 2048,
        [&](const auto& frame) {
            Require(frame.GetInfo()->completed_tick == session.GetCompletedTick(),
                "completion consumes newly committed frame");
            Require(active->GetFrame().GetInfo()->completed_tick + 1 ==
                frame.GetInfo()->completed_tick, "publication follows successful completion");
            ++observations;
            return false;
        }};
    active = &graph;
    const auto admit = [&](std::span<const BoundaryCommand> commands) {
        Require(session.GetIngress().TryAdmitCommands(commands).status ==
            CommandIngress::AdmissionStatus::accepted, "graph batch admitted");
        Require(oracle.GetIngress().TryAdmitCommands(commands).status ==
            CommandIngress::AdmissionStatus::accepted, "oracle batch admitted");
    };
    const auto step = [&] {
        const auto received = graph.TryStep(), baseline = oracle.TryStep();
        Require(received.status == Status::advanced && received.tick == baseline.tick &&
            received.applied_commands == baseline.applied_commands, "concrete step parity");
        Require(expected.TryCapture(direct) && graph.GetFrame().HasEqualState(expected),
            "every owned state field matches direct boundary");
    };
    const auto fuse = BoundaryCommand::Fuse();
    admit(std::span{&fuse, 1}); step();
    Require(session.GetTrace()[0].result == StructuralCommandResult::insufficient_mass,
        "normal domain refusal still completes graph");
    const BoundaryCommand gather{FieldEdit{FieldEditKind::set, 0, {48.5F,16.5F}, 8, 4}};
    admit(std::span{&gather, 1});
    for (int index = 0; index < 60; ++index) step();
    admit(std::span{&fuse, 1}); step();
    Require(graph.GetFrame().GetInfo()->structural->occupied, "actual relay is fused");
    const std::array edits{BoundaryCommand::Shatter(0), BoundaryCommand::Shatter(1), fuse};
    admit(edits); step();
    Require(graph.GetFrame().GetInfo()->biomass->lost_mass == 16 && observations == 63,
        "actual structural loss and repeated graph epochs observed");
}

void CheckSuppressionAndTerminal() {
    Simulation resumed_world{64, Options()}; HeadlessSession resumed_session{resumed_world, {2, 0}};
    unsigned resumed_callbacks{};
    BoundaryPipeline resumed{resumed_session, resumed_world, 64, 4, 2048,
        [&](const auto&) { ++resumed_callbacks; return false; }};
    resumed_session.Pause();
    Require(resumed.TryStep().status == Status::paused && resumed_callbacks == 0,
        "cancelled paused epoch has no completion");
    resumed_session.Resume();
    Require(resumed.TryStep().status == Status::advanced && resumed_callbacks == 1 &&
        resumed.GetFrame().GetInfo()->completed_tick == 1,
        "successful graph epoch follows normal cancellation without reset");

    Simulation simulation{64, Options()}; HeadlessSession session{simulation, {2, 0}};
    unsigned callbacks{};
    BoundaryPipeline graph{session, simulation, 64, 4, 2048,
        [&](const auto&) { ++callbacks; return true; }};
    const auto* initial = &graph.GetFrame();
    session.Pause();
    Require(graph.TryStep().status == Status::paused && callbacks == 0 &&
        &graph.GetFrame() == initial, "paused graph skips capture and completion");
    session.Resume();
    const auto fuse = BoundaryCommand::Fuse();
    Require(session.GetIngress().TryAdmitCommands(std::span{&fuse, 1}).status ==
        CommandIngress::AdmissionStatus::accepted, "trace exhaustion command admitted");
    Require(graph.TryStep().status == Status::trace_full && callbacks == 0 &&
        session.GetIngress().GetStatistics().pending == 1 &&
        &graph.GetFrame() == initial, "trace full retains frame and pending input");
    session.GetIngress().Close();
    Require(graph.TryStep().status == Status::closed && callbacks == 0,
        "closed graph skips capture and completion");

    Simulation terminal_world{64, Options()}; HeadlessSession terminal_session{terminal_world, {2, 0}};
    BoundaryPipeline terminal{terminal_session, terminal_world, 64, 4, 2048,
        [&](const auto& frame) { ++callbacks; return frame.GetInfo()->completed_tick == 1; }};
    Require(terminal.TryStep().status == Status::advanced && terminal.IsTerminal() &&
        terminal_session.GetIngress().CaptureCutoff().closed, "terminal closes admission in same boundary");
    Require(terminal.TryStep().status == Status::closed &&
        terminal_session.GetCompletedTick() == 1 && callbacks == 1,
        "terminal prevents catch-up second boundary");
}

void CheckFailureRetentionAndStartup() {
    Simulation simulation{64, Options()}; HeadlessSession session{simulation, {2, 0}};
    bool fail{};
    unsigned callbacks{};
    BoundaryPipeline graph{session, simulation, 64, 4, 2048,
        [&](const auto&) { ++callbacks; if (fail) throw std::runtime_error("mission fixture"); return false; }};
    Require(graph.TryStep().status == Status::advanced, "seed good published frame");
    presentation::ScenarioSnapshot retained{64, 4, 2048};
    Require(retained.TryCapture(simulation), "copy retained good frame");
    const auto* published = &graph.GetFrame();
    fail = true;
    bool threw{};
    try { (void)graph.TryStep(); }
    catch (const std::runtime_error& error) { threw = std::string_view{error.what()} == "mission fixture"; }
    Require(threw && session.GetCompletedTick() == 2 &&
        session.GetIngress().CaptureCutoff().closed, "original callback failure propagates and closes input");
    Require(&graph.GetFrame() == published && graph.GetFrame().HasEqualState(retained),
        "failed completion cannot partially publish staged state");
    Require(graph.TryStep().status == Status::application_failed && callbacks == 2 &&
        session.GetCompletedTick() == 2, "failed graph never retries mutation or completion");
    bool rejected{};
    try { BoundaryPipeline invalid{session, simulation, 63, 4, 2048}; }
    catch (const std::invalid_argument&) { rejected = true; }
    Require(rejected, "undersized startup frame rejected before usable graph");
}
}
int main() {
    try { CheckRepeatAndStructuralParity(); CheckSuppressionAndTerminal(); CheckFailureRetentionAndStartup(); }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
