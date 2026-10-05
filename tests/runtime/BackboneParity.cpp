// Receives the production Pub/Pipeline route against the direct coordinator.
// Every completed frame, mission, admission counter and command attempt agrees;
// restart, catch-up terminal closure and structural refusal are real callers.
#include <crucible/runtime/InspectorSession.hpp>
#include <crucible/runtime/ReferenceMissionRoute.hpp>
#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <chrono>
#include <iostream>
#include <stdexcept>
#ifdef CRUCIBLE_TEST_DIAGNOSTICS
#include <crucible/runtime/RuntimeDiagnostics.hpp>
#include <sstream>
#endif

using namespace crucible;
using namespace crucible::runtime;
namespace {
#ifdef CRUCIBLE_TEST_DIAGNOSTICS
constexpr auto diagnostics = InspectorSession::Diagnostics::bounded;
#else
constexpr auto diagnostics = InspectorSession::Diagnostics::disabled;
#endif
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void Equal(const InspectorSession& live, const InspectorSession& direct) {
    Require(live.GetSnapshot().HasEqualState(direct.GetSnapshot()), "full frame parity");
    Require(live.GetStatus() == direct.GetStatus(), "clock lifecycle parity");
    const auto a = live.GetSummary(), b = direct.GetSummary();
    Require(a.completed_tick == b.completed_tick && a.applied_commands == b.applied_commands &&
        a.ingress_pending == b.ingress_pending && a.ingress_accepted == b.ingress_accepted &&
        a.ingress_rejected == b.ingress_rejected &&
        a.discarded_scaled_nanoseconds == b.discarded_scaled_nanoseconds, "summary parity");
    const auto x = live.GetMission(), y = direct.GetMission();
    Require(x.has_value() == y.has_value(), "mission presence parity");
    if (x) Require(x->reclaimed == y->reclaimed && x->completed_tick == y->completed_tick &&
        x->outcome == y->outcome, "mission result parity");
    const auto p = live.GetTrace(), q = direct.GetTrace();
    Require(p.size() == q.size(), "trace length parity");
    for (std::size_t i = 0; i < p.size(); ++i) {
        const auto& c = p[i].command; const auto& d = q[i].command;
        Require(p[i].tick == q[i].tick && p[i].sequence == q[i].sequence && p[i].result == q[i].result &&
            c.action == d.action && c.generation == d.generation && c.field.kind == d.field.kind &&
            c.field.slot == d.field.slot && c.field.center.x == d.field.center.x && c.field.center.y == d.field.center.y &&
            c.field.end.x == d.field.end.x && c.field.end.y == d.field.end.y && c.field.radius == d.field.radius &&
            c.field.strength == d.field.strength, "complete trace parity");
    }
}
void Admission(CommandIngress::Admission a, CommandIngress::Admission b) {
    Require(a.status == b.status && a.first_sequence == b.first_sequence &&
        a.last_sequence == b.last_sequence, "admission receipt parity");
}
void Route(bool structural) {
    InspectorSession live{2048, {64,4096}, ReclamationMissionSettings{}, structural,
        InspectorSession::ExecutionPath::integrated, diagnostics};
    InspectorSession direct{2048, {64,4096}, ReclamationMissionSettings{}, structural,
        InspectorSession::ExecutionPath::direct};
    bool gathering{}, fused{};
    Equal(live,direct);
    const auto begin = std::chrono::steady_clock::now();
    while (live.GetMission()->outcome == ReclamationMissionOutcome::active) {
        const auto tick = live.GetMission()->completed_tick;
        if (!structural || (!gathering && live.GetMission()->reclaimed < 1780)) {
            if (const auto edit = GetReferenceMissionRouteEdit(tick))
                Admission(live.TryAdmitFieldEdit(*edit),direct.TryAdmitFieldEdit(*edit));
        } else if (!gathering) {
            const FieldEdit edit{FieldEditKind::set,0,{48.5F,16.5F},8,4};
            Admission(live.TryAdmitFieldEdit(edit),direct.TryAdmitFieldEdit(edit));
            gathering = true;
        }
        if (structural && gathering && !fused &&
            live.GetSnapshot().GetInfo()->structural->eligible_mobile >= StructuralSettings::cost) {
            Admission(live.TryFuseRelay(),direct.TryFuseRelay()); fused = true;
        }
        const auto a = live.TryPump(std::chrono::nanoseconds{16'666'667});
        const auto b = direct.TryPump(std::chrono::nanoseconds{16'666'667});
        Require(a.advanced_ticks == 1 && b.advanced_ticks == 1, "route advances one boundary");
        Equal(live,direct);
    }
    Require(live.GetMission()->outcome == ReclamationMissionOutcome::won, "route wins");
    const auto end = std::chrono::steady_clock::now();
    std::cout << "route structural=" << structural << " tick=" << live.GetMission()->completed_tick
        << " full-state/mission/trace=equal paired_receiving_ms="
        << std::chrono::duration<double,std::milli>(end-begin).count() << '\n';
}
void Backpressure() {
    InspectorSession live{64,{1,0},ReclamationMissionSettings{1780,4},true,
        InspectorSession::ExecutionPath::integrated, diagnostics};
    InspectorSession direct{64,{1,0},ReclamationMissionSettings{1780,4},true,
        InspectorSession::ExecutionPath::direct};
    Admission(live.TryFuseRelay(),direct.TryFuseRelay());
    const auto a=live.TryPump(std::chrono::seconds{1}), b=direct.TryPump(std::chrono::seconds{1});
    Require(a.advanced_ticks == 0 && b.advanced_ticks == 0 &&
        a.boundary_status == HeadlessSession::StepStatus::trace_full &&
        b.boundary_status == HeadlessSession::StepStatus::trace_full &&
        live.GetSummary().ingress_pending == 1, "production trace backpressure preserves boundary");
    Equal(live,direct);
}
void Lifecycle() {
    InspectorSession live{2048,{1,16},ReclamationMissionSettings{1780,4},true,
        InspectorSession::ExecutionPath::integrated, diagnostics};
    InspectorSession direct{2048,{1,16},ReclamationMissionSettings{1780,4},true,InspectorSession::ExecutionPath::direct};
    live.Pause(); direct.Pause();
    Admission(live.TryFuseRelay(),direct.TryFuseRelay());
    Admission(live.TryFuseRelay(),direct.TryFuseRelay()); // full
    static_cast<void>(live.TryPump(std::chrono::seconds{1}));
    static_cast<void>(direct.TryPump(std::chrono::seconds{1})); Equal(live,direct);
    live.Resume(); direct.Resume();
    static_cast<void>(live.TryPump(std::chrono::nanoseconds{16'666'667}));
    static_cast<void>(direct.TryPump(std::chrono::nanoseconds{16'666'667})); Equal(live,direct);
    Admission(live.TryShatterRelay(),direct.TryShatterRelay()); // empty refusal
    const auto a=live.TryPump(std::chrono::seconds{1}), b=direct.TryPump(std::chrono::seconds{1});
    Require(a.advanced_ticks == 3 && b.advanced_ticks == 3, "terminal stops catch-up"); Equal(live,direct);
    Admission(live.TryFuseRelay(),direct.TryFuseRelay()); // closed
    live.Restart(); direct.Restart(); Equal(live,direct);
    Admission(live.TryFuseRelay(),direct.TryFuseRelay());
    static_cast<void>(live.TryPump(std::chrono::nanoseconds{16'666'667}));
    static_cast<void>(direct.TryPump(std::chrono::nanoseconds{16'666'667})); Equal(live,direct);
    InspectorSession peer{2048,{1,16},ReclamationMissionSettings{1780,4},true};
    Require(peer.GetSummary().ingress_pending == 0 && peer.GetTrace().empty(), "live domains isolated");
}
#ifdef CRUCIBLE_TEST_DIAGNOSTICS
void ExhaustedDiagnostics() {
    InspectorSession live{64,{1,16},ReclamationMissionSettings{1780,4},true,
        InspectorSession::ExecutionPath::integrated, diagnostics};
    InspectorSession plain{64,{1,16},ReclamationMissionSettings{1780,4},true};
    live.Pause(); plain.Pause();
    Admission(live.TryFuseRelay(), plain.TryFuseRelay());
    for (int attempt = 0; attempt < 2000; ++attempt)
        Admission(live.TryFuseRelay(), plain.TryFuseRelay());
    Require(live.GetDiagnostics()->GetStatistics().dropped_records > 0,
        "real admission consumer exhausts bounded diagnostics");
    Equal(live, plain);
    live.Resume(); plain.Resume();
    static_cast<void>(live.TryPump(std::chrono::seconds{1}));
    static_cast<void>(plain.TryPump(std::chrono::seconds{1}));
    Equal(live, plain);
    std::ostringstream output;
    Require(live.GetDiagnostics()->WriteDecoded(output), "exhausted session prefix decodes");
    live.Restart(); plain.Restart(); Equal(live, plain);
    Require(live.GetDiagnostics()->GetStatistics().dropped_records > 0,
        "restart preserves bounded session loss counters");
}
void MissionSummaryCadence() {
    InspectorSession live{64,{1,16},ReclamationMissionSettings{1780,4},true,
        InspectorSession::ExecutionPath::integrated, diagnostics};
    for (int run = 0; run < 2; ++run) {
        Require(live.TryPump(std::chrono::seconds{1}).advanced_ticks == 4, "terminal cadence fixture");
        for (int attempt = 0; attempt < 3; ++attempt)
            Require(live.TryPump(std::chrono::seconds{1}).advanced_ticks == 0, "closed pump cannot re-summarize");
        if (run == 0) live.Restart();
    }
    std::ostringstream output;
    Require(live.GetDiagnostics()->WriteDecoded(output), "terminal summaries decode");
    const auto text = output.str();
    const auto first = text.find("schema=1 event=3 run=1 tick=4");
    const auto second = text.find("schema=1 event=3 run=2 tick=4");
    Require(first != std::string::npos && second != std::string::npos &&
        text.find("schema=1 event=3", second + 1) == std::string::npos &&
        text.find("diagnostics decoded=2 ") != std::string::npos,
        "exactly one summary per run despite catch-up and closed pumps");
    std::cout << text;
}
#endif
}
int main() {
    try { Backpressure(); Lifecycle(); Route(false); Route(true);
#ifdef CRUCIBLE_TEST_DIAGNOSTICS
        ExhaustedDiagnostics();
        MissionSummaryCadence();
#endif
    }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
