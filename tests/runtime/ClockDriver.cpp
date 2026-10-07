#include <crucible/runtime/ClockDriver.hpp>

#include <array>
#include <chrono>
#include <iostream>
#include <limits>
#include <stdexcept>

#include <crucible/simulation.hpp>

namespace {
using crucible::Simulation;
using crucible::runtime::ClockDriver;
using crucible::runtime::HeadlessSession;
using Status = ClockDriver::Status;
using namespace std::chrono_literals;

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void CheckExactClock() {
    Simulation simulation{3};
    HeadlessSession session{simulation, {2, 2}};
    ClockDriver clock{session};
    Require(clock.TryPump(0ns).advanced_ticks == 0, "zero time");
    Require(clock.TryPump(16'666'666ns).advanced_ticks == 0, "fraction below tick");
    Require(clock.TryPump(1ns).advanced_ticks == 1, "fraction crosses exact rational tick");
    const auto exact = clock.TryPump(33'333'333ns);
    Require(exact.advanced_ticks == 2 && exact.summary.completed_tick == 3 &&
        exact.summary.discarded_scaled_nanoseconds == 0, "50ms is exactly three ticks");
    Require(clock.TryPump(16'666'666ns).advanced_ticks == 0, "exact schedule has no drift");
    Require(clock.TryPump(1ns).advanced_ticks == 1, "next exact rational threshold");

    Simulation capped_simulation{0};
    HeadlessSession capped_session{capped_simulation, {1, 0}};
    ClockDriver capped{capped_session};
    const auto excess = capped.TryPump(100ms + 1ns);
    Require(excess.advanced_ticks == 4 && excess.summary.completed_tick == 4 &&
        excess.summary.discarded_scaled_nanoseconds == 2'000'000'000,
        "four tick cap discards two whole ticks");
    Require(capped.TryPump(16'666'666ns).advanced_ticks == 1,
        "cap retains fractional remainder");
}

void CheckInvalidTime() {
    Simulation simulation{0};
    HeadlessSession session{simulation, {1, 0}};
    ClockDriver clock{session};
    Require(clock.TryPump(1ns).advanced_ticks == 0, "seed carry");
    constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
    const auto addition_overflow = std::chrono::nanoseconds{
        static_cast<std::chrono::nanoseconds::rep>(maximum / 60)};
    Require(clock.TryPump(-1ns).status == Status::invalid_elapsed, "negative time rejected");
    Require(clock.TryPump(std::chrono::nanoseconds::max()).status == Status::invalid_elapsed,
        "scaled multiplication overflow rejected");
    Require(clock.TryPump(addition_overflow).status == Status::invalid_elapsed,
        "carry addition overflow rejected");
    Require(clock.GetSummary().completed_tick == 0 &&
        clock.GetSummary().discarded_scaled_nanoseconds == 0, "invalid time changes no counters");
    Require(clock.TryPump(16'666'666ns).advanced_ticks == 1, "invalid time preserves carry");

    clock.Pause();
    const auto huge = std::chrono::nanoseconds{
        static_cast<std::chrono::nanoseconds::rep>(maximum / 60)};
    Require(clock.TryPump(huge).status == Status::paused, "supported huge paused duration");
    Require(clock.TryPump(huge).status == Status::paused &&
        clock.GetSummary().discarded_scaled_nanoseconds == maximum,
        "discard counter saturates safely");
}

void CheckLifecycleAndCounters() {
    Simulation simulation{3, {{4, 4, 1.0F}, 1}};
    HeadlessSession session{simulation, {1, 2}};
    ClockDriver clock{session};
    const std::array edits{crucible::FieldEdit{crucible::FieldEditKind::set,
        0, {0, 0}, 1.0F, 1.0F}};
    Require(session.GetIngress().TryAdmit(edits).status ==
        crucible::runtime::CommandIngress::AdmissionStatus::accepted, "input admitted");
    Require(session.GetIngress().TryAdmit(edits).status ==
        crucible::runtime::CommandIngress::AdmissionStatus::full, "full input rejected");
    Require(clock.TryPump(5ms).advanced_ticks == 0, "prepause fraction");
    clock.Pause();
    clock.Pause();
    const auto paused = clock.TryPump(1s);
    Require(paused.status == Status::paused && paused.summary.completed_tick == 0 &&
        paused.summary.ingress_pending == 1 && paused.summary.ingress_rejected == 1 &&
        paused.summary.discarded_scaled_nanoseconds == 60'300'000'000,
        "pause discards elapsed and fraction but keeps admitted edits");
    clock.Resume();
    clock.Resume();
    Require(clock.TryPump(0ns).advanced_ticks == 0, "resume has no paused backlog");
    const auto advanced = clock.TryPump(16'666'667ns);
    Require(advanced.advanced_ticks == 1 && advanced.summary.applied_commands == 1 &&
        advanced.summary.ingress_pending == 0 && advanced.summary.ingress_accepted == 1,
        "resumed boundary publishes completed command count");
    clock.Close();
    clock.Close();
    Require(clock.TryPump(1s).status == Status::closed &&
        clock.GetSummary().completed_tick == 1, "close stops ticks");
    Require(session.GetIngress().TryAdmit(edits).status ==
        crucible::runtime::CommandIngress::AdmissionStatus::closed, "close rejects admission");
    clock.Resume();
    Require(clock.TryPump(1s).status == Status::closed, "resume cannot reopen closed driver");
}

void CheckBlockedAndSchedules() {
    Simulation simulation{1, {{4, 4, 1.0F}, 1}};
    HeadlessSession session{simulation, {1, 0}};
    ClockDriver clock{session};
    const std::array edits{crucible::FieldEdit{crucible::FieldEditKind::set,
        0, {0, 0}, 1.0F, 1.0F}};
    Require(session.GetIngress().TryAdmit(edits).status ==
        crucible::runtime::CommandIngress::AdmissionStatus::accepted, "blocked input admitted");
    const auto before = simulation.checksum();
    const auto blocked = clock.TryPump(100ms);
    Require(blocked.status == Status::blocked && blocked.advanced_ticks == 0 &&
        blocked.boundary_status == HeadlessSession::StepStatus::trace_full &&
        blocked.summary.ingress_pending == 1 && blocked.summary.discarded_scaled_nanoseconds == 0 &&
        simulation.checksum() == before, "trace full preserves failed boundary");
    const auto retry = clock.TryPump(100ms);
    Require(retry.status == Status::blocked && !retry.boundary_status &&
        retry.summary.completed_tick == 0, "blocked driver does not retry or accrue time");
    clock.Pause();
    clock.Resume();
    Require(clock.TryPump(0ns).status == Status::blocked, "lifecycle cannot bypass blocked boundary");
    clock.Close();
    Require(clock.GetSummary().discarded_scaled_nanoseconds == 6'000'000'000,
        "close explicitly discards blocked backlog");

    Simulation closed_simulation{0};
    HeadlessSession closed_session{closed_simulation, {1, 0}};
    ClockDriver externally_closed{closed_session};
    closed_session.GetIngress().Close();
    Require(externally_closed.TryPump(50ms).status == Status::closed,
        "externally closed session stops driver at attempted boundary");

    Simulation first{3}, second{3};
    HeadlessSession first_session{first, {1, 0}}, second_session{second, {1, 0}};
    ClockDriver first_clock{first_session}, second_clock{second_session};
    Require(first_clock.TryPump(50ms).advanced_ticks == 3, "combined clock schedule");
    for (const auto elapsed : {10ms, 20ms, 20ms})
        static_cast<void>(second_clock.TryPump(elapsed));
    Require(first_clock.GetSummary().completed_tick == second_clock.GetSummary().completed_tick &&
        first.checksum() == second.checksum(), "clock schedule does not alter exact ticks");
}

void CheckCompletedBoundaryStop() {
    Simulation simulation{0};
    HeadlessSession session{simulation, {1, 0}};
    std::uint64_t observed{};
    ClockDriver clock{session, [&] {
        observed = session.GetCompletedTick();
        return observed == 2;
    }};
    clock.Pause();
    Require(clock.TryPump(1s).advanced_ticks == 0 && observed == 0,
        "paused pump never invokes completed-boundary callback");
    clock.Resume();
    const auto stopped = clock.TryPump(100ms + 1ns);
    Require(stopped.status == Status::closed && stopped.advanced_ticks == 2 &&
        observed == 2 && stopped.summary.completed_tick == 2 &&
        stopped.summary.discarded_scaled_nanoseconds == 64'000'000'060,
        "stop callback sees completed ticks and discards whole and fractional carry");
    clock.Resume();
    Require(clock.TryPump(100ms).advanced_ticks == 0 && observed == 2,
        "terminal stop callback cannot be repeated or resumed");

    Simulation failed_simulation{0};
    HeadlessSession failed_session{failed_simulation, {1, 0}};
    ClockDriver failed{failed_session, []() -> bool { throw std::logic_error("boundary observer failed"); }};
    bool threw{};
    try { static_cast<void>(failed.TryPump(100ms)); }
    catch (const std::logic_error&) { threw = true; }
    Require(threw && failed.GetStatus() == Status::blocked && failed.GetSummary().completed_tick == 1,
        "throwing callback latches blocked after its completed tick");
    Require(failed.TryPump(100ms).advanced_ticks == 0, "throwing callback never runs again");
}
}

int main() {
    try {
        CheckExactClock();
        CheckInvalidTime();
        CheckLifecycleAndCounters();
        CheckBlockedAndSchedules();
        CheckCompletedBoundaryStop();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
