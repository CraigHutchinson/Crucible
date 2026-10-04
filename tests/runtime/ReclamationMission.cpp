#include <crucible/runtime/InspectorSession.hpp>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <crucible/simulation.hpp>

namespace {
using namespace std::chrono_literals;
using namespace crucible;
using namespace crucible::runtime;
using Outcome = ReclamationMissionOutcome;
using Status = ClockDriver::Status;

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void SameFrame(const presentation::ScenarioSnapshot& first, const presentation::ScenarioSnapshot& second) {
    Require(first.GetInfo()->completed_tick == second.GetInfo()->completed_tick &&
        first.GetInfo()->biomass == second.GetInfo()->biomass &&
        std::ranges::equal(first.GetBlight(), second.GetBlight()) &&
        std::ranges::equal(first.GetStocks(), second.GetStocks()), "mission full resource replay");
    const auto a = first.GetSamples(), b = second.GetSamples();
    Require(a.size() == b.size(), "mission replay population");
    for (std::size_t i = 0; i < a.size(); ++i)
        Require(a[i].id == b[i].id && a[i].position.x == b[i].position.x &&
            a[i].position.y == b[i].position.y && a[i].velocity.x == b[i].velocity.x &&
            a[i].velocity.y == b[i].velocity.y, "mission full sample replay");
    const auto fields_a = first.GetFields(), fields_b = second.GetFields();
    Require(fields_a.size() == fields_b.size(), "mission replay slots");
    for (std::size_t i = 0; i < fields_a.size(); ++i)
        Require(fields_a[i].kind == fields_b[i].kind && fields_a[i].slot == fields_b[i].slot &&
            fields_a[i].center.x == fields_b[i].center.x && fields_a[i].center.y == fields_b[i].center.y &&
            fields_a[i].radius == fields_b[i].radius && fields_a[i].strength == fields_b[i].strength &&
            fields_a[i].end.x == fields_b[i].end.x && fields_a[i].end.y == fields_b[i].end.y,
            "mission full field replay");
}

void CheckQuotaAndPrecedence() {
    // First spread infects the center and four cardinal neighbors. The reference
    // population has one sample in every cell; each contributes one harvest action.
    InspectorSession won{2048, {1, 4}, ReclamationMissionSettings{5, 1}};
    const auto initial = won.GetMission();
    Require(initial && initial->outcome == Outcome::active && initial->completed_tick == 0 &&
        initial->reclaimed == 0, "mission starts empty and active");
    const auto result = won.TryPump(100ms);
    const auto progress = won.GetMission();
    Require(result.advanced_ticks == 1 && result.status == Status::closed &&
        result.summary.completed_tick == 1 && result.summary.discarded_scaled_nanoseconds == 5'000'000'000,
        "quota stops catchup and discards all carried time");
    Require(progress->outcome == Outcome::won && progress->completed_tick == 1 && progress->reclaimed == 5 &&
        won.GetSnapshot().GetInfo()->completed_tick == 1 && won.GetSnapshot().GetInfo()->biomass->reserve == 5,
        "quota wins at exact deadline with the same retained boundary");
    const FieldEdit remove{FieldEditKind::remove, 0, {}, 0, 0};
    Require(won.TryAdmitFieldEdit(remove).status == CommandIngress::AdmissionStatus::closed,
        "terminal admission denied");
    won.Pause(); won.Resume();
    Require(won.TryPump(100ms).advanced_ticks == 0 && won.GetStatus() == Status::closed &&
        won.GetMission()->outcome == Outcome::won && won.GetMission()->reclaimed == 5,
        "terminal outcome cannot resume or change");
    won.Restart();
    Require(won.GetStatus() == Status::running && won.GetMission()->outcome == Outcome::active &&
        won.GetMission()->completed_tick == 0 && won.GetMission()->reclaimed == 0 &&
        won.GetSnapshot().GetInfo()->completed_tick == 0 && won.GetTrace().empty() &&
        won.GetSummary().ingress_pending == 0, "restart resets mission and retained run");

    InspectorSession lost{2048, {1, 0}, ReclamationMissionSettings{6, 1}};
    Require(lost.TryPump(100ms).advanced_ticks == 1 && lost.GetMission()->outcome == Outcome::lost &&
        lost.GetMission()->completed_tick == 1 && lost.GetMission()->reclaimed == 5,
        "unmet quota loses on deadline even with positive recovery");
}

void CheckSchedulesAndReplay() {
    const ReclamationMissionSettings settings{6, 2};
    InspectorSession combined{2048, {1, 4}, settings};
    InspectorSession divided{2048, {1, 4}, settings};
    const FieldEdit remove{FieldEditKind::remove, 0, {}, 0, 0};
    Require(combined.TryAdmitFieldEdit(remove).status == CommandIngress::AdmissionStatus::accepted &&
        divided.TryAdmitFieldEdit(remove).status == CommandIngress::AdmissionStatus::accepted,
        "mission receives replayable boundary command");
    Require(combined.TryPump(100ms).advanced_ticks == 2, "catchup stops at second quota boundary");
    static_cast<void>(divided.TryPump(20ms));
    static_cast<void>(divided.TryPump(20ms));
    Require(combined.GetMission()->outcome == Outcome::won && combined.GetMission()->completed_tick == 2 &&
        combined.GetMission()->reclaimed == 18 && divided.GetMission()->reclaimed == 18,
        "second cardinal expansion yields thirteen further contacts before deadline");
    SameFrame(combined.GetSnapshot(), divided.GetSnapshot());
    const std::vector<AppliedCommand> trace(combined.GetTrace().begin(), combined.GetTrace().end());
    Simulation oracle{2048, {{64, 32, 1}, 4, SteeringSettings{}, ResourceSettings{}}};
    HeadlessSession replay{oracle, {1, 4}};
    Require(replay.TryReplay(trace, 2).status == HeadlessSession::StepStatus::advanced,
        "mission terminal command trace replays independently");
    presentation::ScenarioSnapshot expected{2048, 4, 2048};
    Require(expected.TryCapture(oracle), "mission replay snapshot");
    SameFrame(expected, combined.GetSnapshot());
}

void CheckPauseBlockedAndLoss() {
    InspectorSession paused{0, {1, 1}, ReclamationMissionSettings{1, 2}};
    paused.Pause();
    Require(paused.TryPump(1s).advanced_ticks == 0 && paused.GetMission()->completed_tick == 0 &&
        paused.GetMission()->outcome == Outcome::active, "paused mission has no deadline progress");
    paused.Resume();
    Require(paused.TryPump(100ms).advanced_ticks == 2 && paused.GetMission()->outcome == Outcome::lost &&
        paused.GetMission()->completed_tick == 2 && paused.GetMission()->reclaimed == 0 &&
        paused.GetSnapshot().GetInfo()->completed_tick == 2, "zero population loses at exact boundary");
    auto copy = paused.GetMission(); copy->reclaimed = 999;
    Require(paused.GetMission()->reclaimed == 0, "mission observations are owned values");

    InspectorSession blocked{0, {1, 0}, ReclamationMissionSettings{1, 1}};
    const FieldEdit remove{FieldEditKind::remove, 0, {}, 0, 0};
    static_cast<void>(blocked.TryAdmitFieldEdit(remove));
    Require(blocked.TryPump(100ms).status == Status::blocked && blocked.GetMission()->completed_tick == 0 &&
        blocked.GetMission()->reclaimed == 0 && blocked.GetMission()->outcome == Outcome::active &&
        blocked.GetSnapshot().GetInfo()->completed_tick == 0, "failed boundary cannot advance mission");
    blocked.Resume();
    Require(blocked.TryPump(100ms).advanced_ticks == 0, "resume cannot bypass trace-full gate");
    blocked.Restart();
    Require(blocked.GetTrace().empty() && blocked.GetSummary().ingress_pending == 0 &&
        blocked.GetMission()->outcome == Outcome::active, "restart retires blocked queue");
}

void CheckInvalidMission() {
    for (const ReclamationMissionSettings settings : {
            ReclamationMissionSettings{0, 1}, ReclamationMissionSettings{1, 0},
            ReclamationMissionSettings{8193, 1},
            ReclamationMissionSettings{std::numeric_limits<std::uint64_t>::max(), 1}}) {
        bool rejected{};
        try { InspectorSession invalid{0, {1, 0}, settings}; }
        catch (const std::invalid_argument&) { rejected = true; }
        Require(rejected, "invalid mission quota/deadline rejected");
    }
    InspectorSession legal{0, {1, 0}, ReclamationMissionSettings{8192, 1}};
    Require(legal.GetMission()->settings.target_reclaimed == 8192, "full startup stock quota is legal");
    InspectorSession ordinary{0, {1, 0}};
    Require(!ordinary.GetMission() && ordinary.TryPump(100ms).advanced_ticks == 4 &&
        ordinary.GetStatus() == Status::running, "omitted mission preserves ordinary catchup");
}
}

int main() {
    try {
        CheckQuotaAndPrecedence(); CheckSchedulesAndReplay();
        CheckPauseBlockedAndLoss(); CheckInvalidMission();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
