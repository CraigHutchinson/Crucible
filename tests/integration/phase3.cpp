#include <crucible/simulation.hpp>
#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <crucible/runtime/ClockDriver.hpp>
#include <array>
#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <fstream>
#include <iomanip>
#include <limits>
#include <numeric>

namespace {
using namespace crucible;
using presentation::ScenarioSnapshot;
void Require(bool condition) { if (!condition) throw std::runtime_error("Phase three contract failed"); }
void Equal(const ScenarioSnapshot& a, const ScenarioSnapshot& b) {
    const auto x = *a.GetInfo(), y = *b.GetInfo();
    Require(x.completed_tick == y.completed_tick && x.samples == y.samples && x.fields == y.fields && x.cells == y.cells && x.biomass == y.biomass);
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
    Require(std::ranges::equal(a.GetStocks(), b.GetStocks()));
}
void Conserved(const ScenarioSnapshot& snapshot) {
    const auto ledger = *snapshot.GetInfo()->biomass;
    Require(std::accumulate(snapshot.GetStocks().begin(), snapshot.GetStocks().end(), std::uint64_t{}) == ledger.remaining_stock);
    Require(ledger.initial_total == ledger.remaining_stock + ledger.mobile_mass + ledger.reserve);
    Require(ledger.reserve == ledger.harvested);
    Require(ledger.harvested <= ledger.work_actions);
}
void Dump(const ScenarioSnapshot& snapshot, std::ostream* output) {
    if (!output) return;
    const auto info = *snapshot.GetInfo();
    const auto ledger = *info.biomass;
    *output << info.samples << ' ' << info.completed_tick << ' ' << ledger.initial_total << ' '
        << ledger.remaining_stock << ' ' << ledger.mobile_mass << ' ' << ledger.reserve << ' '
        << ledger.harvested << ' ' << ledger.work_actions << '\n' << std::hexfloat;
    for (const auto& p : snapshot.GetSamples())
        *output << p.id.value << ' ' << p.position.x << ' ' << p.position.y << ' ' << p.velocity.x << ' ' << p.velocity.y << '\n';
    for (const auto& f : snapshot.GetFields())
        *output << static_cast<int>(f.kind) << ' ' << f.slot << ' ' << f.center.x << ' ' << f.center.y << ' ' << f.radius << ' ' << f.strength << ' ' << f.end.x << ' ' << f.end.y << '\n';
    for (std::size_t i = 0; i < info.cells; ++i)
        *output << static_cast<int>(snapshot.GetBlight()[i]) << ' ' << snapshot.GetStocks()[i] << '\n';
}
void Run(std::size_t population, std::ostream* output) {
    using namespace std::chrono_literals;
    const Simulation::ScenarioOptions options{{64, 32, 1}, 2, SteeringSettings{}, ResourceSettings{}};
    Simulation first{population, options}, second{population, options}, replayed{population, options};
    runtime::HeadlessSession a{first, {8, 8}}, b{second, {8, 8}}, replay{replayed, {8, 8}};
    runtime::ClockDriver ca{a}, cb{b};
    ScenarioSnapshot sa{population, 2, 2048}, sb{population, 2, 2048}, sr{population, 2, 2048};
    ScenarioSnapshot retained{population, 2, 2048};
    Require(retained.TryCapture(first));
    Conserved(retained); Dump(retained, output);
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
        Equal(sa, sb); Conserved(sa); Dump(sa, output);
    }
    Require(replay.TryReplay(a.GetTrace(), 60).status == runtime::HeadlessSession::StepStatus::advanced);
    Require(sr.TryCapture(replayed)); Equal(sa, sr);
    Require(retained.GetInfo()->completed_tick == 0 && retained.GetBlight()[16 * 64 + 32] == 1);
    Require(retained.GetStocks()[0] == 4 && retained.GetInfo()->biomass->reserve == 0);
    if (population) Require(retained.GetSamples()[0].position.x == 0.5F);
    // Every failed capacity check precedes any destination write.
    std::array samples{SampleState{SampleId{999}, {99, 99}, {99, 99}}};
    std::array fields{FieldEdit{FieldEditKind::set, 99, {99, 99}, 99, 99}};
    std::array<std::uint8_t, 1> cells{99};
    Require(!first.TryCopyState({samples, fields, cells}));
    Require(samples[0].id.value == 999 && fields[0].slot == 99 && cells[0] == 99);
    Simulation too_large{population + 1, options};
    Require(!sa.TryCapture(too_large)); Equal(sa, sr);
    Simulation legacy{0}; Require(!sa.TryCapture(legacy) && !legacy.GetBiomassLedger()); Equal(sa, sr);
    ca.Close(); Require(ca.TryPump(1s).advanced_ticks == 0);
}
}
int main(int argc, char** argv) {
    std::ofstream dump;
    if (argc == 2) { dump.open(argv[1]); Require(static_cast<bool>(dump)); }
    auto* output = argc == 2 ? &dump : nullptr;
    Run(0, output); Run(8, output); Run(2048, output);
    using namespace crucible;
    // No steering: a one-cell world gives an independent hand-calculated transition.
    Simulation small{1, {{1, 1, 1}, 0, {}, ResourceSettings{2, 3, 5, 1}}};
    presentation::ScenarioSnapshot frame{1, 0, 1};
    Require(frame.TryCapture(small));
    Require(frame.GetInfo()->biomass == BiomassLedger{10, 2, 3, 5, 0, 0});
    small.tick(); Require(frame.TryCapture(small));
    Require(frame.GetInfo()->biomass == BiomassLedger{10, 1, 3, 6, 1, 1} && frame.GetBlight()[0] == 1);
    small.tick(); Require(frame.TryCapture(small));
    Require(frame.GetInfo()->biomass == BiomassLedger{10, 0, 3, 7, 2, 2} && frame.GetBlight()[0] == 0);
    small.tick(); Require(frame.TryCapture(small));
    Require(frame.GetInfo()->biomass == BiomassLedger{10, 0, 3, 7, 2, 2});
    // Stock capacity failure must precede writes even when all other spans fit.
    std::array samples{SampleState{SampleId{999}, {99, 99}, {99, 99}}};
    std::array<std::uint8_t, 1> cells{99};
    Require(!small.TryCopyState({samples, {}, cells, {}}));
    Require(samples[0].id.value == 999 && cells[0] == 99);
    Simulation disabled{1, {{1, 1, 1}, 0, {}, ResourceSettings{2, 3, 5, 0}}};
    for (int i = 0; i < 4; ++i) disabled.tick();
    Require(frame.TryCapture(disabled));
    Require(frame.GetInfo()->biomass == BiomassLedger{10, 2, 3, 5, 0, 0} && frame.GetBlight()[0] == 1);
    Simulation legacy{1, {{1, 1, 1}, 0}};
    Require(frame.TryCapture(legacy) && !frame.GetInfo()->biomass && frame.GetStocks().empty() && !legacy.GetBiomassLedger());
    bool overflow = false;
    try { Simulation bad{1, {{1, 1, 1}, 0, {}, ResourceSettings{1, 1, std::numeric_limits<std::uint64_t>::max(), 1}}}; }
    catch (const std::overflow_error&) { overflow = true; }
    Require(overflow);
    if (output) { dump.flush(); Require(static_cast<bool>(dump)); }
}
