#include <crucible/simulation.hpp>
#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <crucible/runtime/ClockDriver.hpp>
#include <array>
#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace {
using namespace crucible;
using presentation::ScenarioSnapshot;
void Require(bool condition) { if (!condition) throw std::runtime_error("Phase two contract failed"); }
void Equal(const ScenarioSnapshot& a, const ScenarioSnapshot& b) {
    const auto x = *a.GetInfo(), y = *b.GetInfo();
    Require(x.completed_tick == y.completed_tick && x.samples == y.samples && x.fields == y.fields && x.cells == y.cells);
    Require(x.grid.columns == y.grid.columns && x.grid.rows == y.grid.rows && x.grid.cell_size == y.grid.cell_size);
    for (std::size_t i = 0; i < x.samples; ++i) {
        const auto p = a.GetSamples()[i], q = b.GetSamples()[i];
        Require(p.id == q.id && p.position.x == q.position.x && p.position.y == q.position.y &&
                p.velocity.x == q.velocity.x && p.velocity.y == q.velocity.y);
    }
    for (std::size_t i = 0; i < x.fields; ++i) {
        const auto p = a.GetFields()[i], q = b.GetFields()[i];
        Require(p.kind == q.kind && p.slot == q.slot && p.center.x == q.center.x && p.center.y == q.center.y &&
                p.radius == q.radius && p.strength == q.strength &&
                p.end.x == q.end.x && p.end.y == q.end.y);
    }
    Require(std::ranges::equal(a.GetBlight(), b.GetBlight()));
}
void Run(std::size_t population) {
    using namespace std::chrono_literals;
    const Simulation::ScenarioOptions options{{64, 32, 1}, 2, SteeringSettings{}};
    Simulation first{population, options}, second{population, options}, replayed{population, options};
    runtime::HeadlessSession a{first, {8, 8}}, b{second, {8, 8}}, replay{replayed, {8, 8}};
    runtime::ClockDriver ca{a}, cb{b};
    ScenarioSnapshot sa{population, 2, 2048}, sb{population, 2, 2048}, sr{population, 2, 2048};
    ScenarioSnapshot retained{population, 2, 2048};
    Require(retained.TryCapture(first));
    for (int frame = 0; frame < 20; ++frame) {
        if (frame == 0 || frame == 10) {
            const std::array edits{FieldEdit{frame == 0 ? FieldEditKind::set : FieldEditKind::remove,
                0, {24, 16}, 20, 4}};
            Require(a.GetIngress().TryAdmit(edits).status == runtime::CommandIngress::AdmissionStatus::accepted);
            Require(b.GetIngress().TryAdmit(edits).status == runtime::CommandIngress::AdmissionStatus::accepted);
        }
        if (frame == 7) {
            ca.Pause(); cb.Pause();
            Require(ca.TryPump(500ms).advanced_ticks == 0 && cb.TryPump(500ms).advanced_ticks == 0);
            ca.Resume(); cb.Resume();
        }
        Require(ca.TryPump(50ms).advanced_ticks == 3);
        Require(cb.TryPump(10ms).advanced_ticks == 0);
        Require(cb.TryPump(40ms).advanced_ticks == 3);
        Require(sa.TryCapture(first) && sb.TryCapture(second));
        Equal(sa, sb);
    }
    Require(replay.TryReplay(a.GetTrace(), 60).status == runtime::HeadlessSession::StepStatus::advanced);
    Require(sr.TryCapture(replayed)); Equal(sa, sr);
    Require(retained.GetInfo()->completed_tick == 0 && retained.GetBlight()[16 * 64 + 32] == 1);
    if (population) Require(retained.GetSamples()[0].position.x == 0.5F);
    // Every failed capacity check precedes any destination write.
    std::array samples{SampleState{SampleId{999}, {99, 99}, {99, 99}}};
    std::array fields{FieldEdit{FieldEditKind::set, 99, {99, 99}, 99, 99}};
    std::array<std::uint8_t, 1> cells{99};
    Require(!first.TryCopyState({samples, fields, cells}));
    Require(samples[0].id.value == 999 && fields[0].slot == 99 && cells[0] == 99);
    Simulation too_large{population + 1, options};
    Require(!sa.TryCapture(too_large)); Equal(sa, sr);
    Simulation legacy{0}; Require(!sa.TryCapture(legacy)); Equal(sa, sr);
    ca.Close(); Require(ca.TryPump(1s).advanced_ticks == 0);
}
}
int main() { Run(0); Run(8); Run(2048); }
