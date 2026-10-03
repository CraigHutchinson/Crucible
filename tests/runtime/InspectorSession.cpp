#include <crucible/runtime/InspectorSession.hpp>
#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <crucible/simulation.hpp>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
using namespace std::chrono_literals;
using namespace crucible;
using namespace crucible::runtime;
void Check() {
    InspectorSession owner{8, {1, 2}};
    const FieldEdit edit{FieldEditKind::set, 0, {16, 16}, 8, 4};
    owner.Pause();
    Require(owner.TryAdmitFieldEdit(edit).status == CommandIngress::AdmissionStatus::accepted, "paused admission");
    Require(owner.TryAdmitFieldEdit(edit).status == CommandIngress::AdmissionStatus::full, "bounded rejection");
    Require(owner.TryPump(1s).advanced_ticks == 0 && owner.GetSnapshot().GetInfo()->completed_tick == 0 &&
        owner.GetSnapshot().GetFields()[0].kind == FieldEditKind::remove, "acceptance is not committed frame");
    owner.Resume();
    Require(owner.TryPump(50ms).advanced_ticks == 3 && owner.GetTrace().size() == 1 &&
        owner.GetSnapshot().GetInfo()->completed_tick == 3 && owner.GetSnapshot().GetFields()[0].kind == FieldEditKind::set,
        "completed frame and trace");
    const std::vector<AppliedCommand> trace(owner.GetTrace().begin(), owner.GetTrace().end());
    Simulation oracle{8, {{64, 32, 1.0F}, 4, SteeringSettings{}, ResourceSettings{}}};
    HeadlessSession replay{oracle, {1, 2}};
    Require(replay.TryReplay(trace, 3).status == HeadlessSession::StepStatus::advanced, "copied trace replays");
    presentation::ScenarioSnapshot expected{8, 4, 2048};
    Require(expected.TryCapture(oracle), "oracle frame");
    Require(std::ranges::equal(expected.GetBlight(), owner.GetSnapshot().GetBlight()) &&
        std::ranges::equal(expected.GetStocks(), owner.GetSnapshot().GetStocks()), "replay infection and stock");
    const auto a = expected.GetSamples(), b = owner.GetSnapshot().GetSamples();
    for (std::size_t i = 0; i < a.size(); ++i) {
        Require(a[i].position.x == b[i].position.x && a[i].position.y == b[i].position.y &&
            a[i].velocity.x == b[i].velocity.x && a[i].velocity.y == b[i].velocity.y, "replay samples");
    }
    owner.Pause();
    static_cast<void>(owner.TryAdmitFieldEdit(edit));
    owner.Restart();
    Require(owner.GetStatus() == ClockDriver::Status::running && owner.GetSummary().ingress_pending == 0 &&
        owner.GetTrace().empty() && owner.GetSnapshot().GetInfo()->completed_tick == 0, "restart fresh lifetime");
    owner.Close();
    Require(owner.TryAdmitFieldEdit(edit).status == CommandIngress::AdmissionStatus::closed &&
        owner.TryPump(1s).advanced_ticks == 0, "close cannot advance");
    InspectorSession blocked{0, {1, 0}};
    static_cast<void>(blocked.TryAdmitFieldEdit(edit));
    Require(blocked.TryPump(20ms).status == ClockDriver::Status::blocked &&
        blocked.GetSnapshot().GetInfo()->completed_tick == 0 && blocked.GetSummary().ingress_pending == 1,
        "trace capacity blocks without claiming application");
    blocked.Restart();
    Require(blocked.TryPump(20ms).advanced_ticks == 1, "restart clears blocked run");
}
}
int main() { try { Check(); } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; } }
