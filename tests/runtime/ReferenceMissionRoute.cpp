#include <crucible/runtime/ReferenceMissionRoute.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>

#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <crucible/runtime/ClockDriver.hpp>
#include <crucible/simulation.hpp>

namespace {
using namespace crucible;
using namespace crucible::runtime;
using namespace std::chrono_literals;

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
bool SameEdit(const FieldEdit& a, const FieldEdit& b) {
    return a.kind == b.kind && a.slot == b.slot && a.center.x == b.center.x && a.center.y == b.center.y &&
        a.radius == b.radius && a.strength == b.strength &&
        a.end.x == b.end.x && a.end.y == b.end.y;
}
void CheckIndependentEdits() {
    struct Expected { std::uint64_t tick; Position center; };
    // Enumeration is independent of the provider's division/modulo arithmetic.
    constexpr std::array expected{
        Expected{0, {8, 8}}, Expected{60, {24, 8}}, Expected{120, {40, 8}}, Expected{180, {56, 8}},
        Expected{240, {8, 24}}, Expected{300, {24, 24}}, Expected{360, {40, 24}}, Expected{420, {56, 24}},
        Expected{480, {8, 8}}, Expected{900, {56, 24}}, Expected{18'446'744'073'709'551'600ULL, {8, 24}}};
    for (const auto& example : expected) {
        const auto edit = GetReferenceMissionRouteEdit(example.tick);
        const FieldEdit oracle{FieldEditKind::set, 0, example.center, 8, 4};
        Require(edit && SameEdit(*edit, oracle) && edit->IsValid(4), "independent route edit");
    }
    for (const auto tick : std::array<std::uint64_t, 8>{1, 59, 61, 239, 241, 479, 481,
            std::numeric_limits<std::uint64_t>::max()})
        Require(!GetReferenceMissionRouteEdit(tick), "between-boundary route has no edit");
    auto owned = GetReferenceMissionRouteEdit(0);
    owned->center.x = 99;
    Require(GetReferenceMissionRouteEdit(0)->center.x == 8, "route returns an independent owned value");
}

void SameFrame(const presentation::ScenarioSnapshot& a, const presentation::ScenarioSnapshot& b) {
    const auto x = *a.GetInfo(), y = *b.GetInfo();
    Require(x.completed_tick == y.completed_tick && x.samples == y.samples && x.fields == y.fields &&
        x.cells == y.cells && x.grid.columns == y.grid.columns && x.grid.rows == y.grid.rows &&
        x.grid.cell_size == y.grid.cell_size && x.biomass == y.biomass, "route metadata replay");
    Require(std::ranges::equal(a.GetSamples(), b.GetSamples(), [](const auto& p, const auto& q) {
        return p.id == q.id && p.position.x == q.position.x && p.position.y == q.position.y &&
            p.velocity.x == q.velocity.x && p.velocity.y == q.velocity.y;
    }), "route sample replay");
    Require(std::ranges::equal(a.GetFields(), b.GetFields(), SameEdit) &&
        std::ranges::equal(a.GetBlight(), b.GetBlight()) &&
        std::ranges::equal(a.GetStocks(), b.GetStocks()), "route field/infection/stock replay");
}
void CheckBoundariesSchedulesAndReplay() {
    const Simulation::ScenarioOptions options{{64, 32, 1}, 4, SteeringSettings{}, ResourceSettings{}};
    Simulation combined{8, options}, divided{8, options}, replayed{8, options};
    HeadlessSession a{combined, {1, 2}}, b{divided, {1, 2}}, replay{replayed, {1, 2}};
    ClockDriver combined_clock{a}, divided_clock{b};
    presentation::ScenarioSnapshot observed{8, 4, 2048}, partitioned{8, 4, 2048}, expected{8, 4, 2048};
    for (std::uint64_t tick = 0; tick < 61; ++tick) {
        if (const auto edit = GetReferenceMissionRouteEdit(tick)) {
            Require(a.GetIngress().TryAdmit(std::span{&*edit, 1}).status == CommandIngress::AdmissionStatus::accepted &&
                b.GetIngress().TryAdmit(std::span{&*edit, 1}).status == CommandIngress::AdmissionStatus::accepted,
                "route request admitted before its boundary");
            Require(observed.TryCapture(combined), "pre-boundary route observation");
            const auto slot = observed.GetFields()[0];
            Require((tick == 0 && slot.kind == FieldEditKind::remove) ||
                (tick == 60 && slot.kind == FieldEditKind::set && slot.center.x == 8 && slot.center.y == 8),
                "admission does not apply the next route edit");
        }
        Require(combined_clock.TryPump(16'666'667ns).advanced_ticks == 1, "combined route boundary");
        Require(divided_clock.TryPump(5ms).advanced_ticks == 0 &&
            divided_clock.TryPump(11'666'667ns).advanced_ticks == 1, "partitioned same completed-tick input");
    }
    Require(a.GetTrace().size() == 2 && b.GetTrace().size() == 2 &&
        a.GetTrace()[0].tick == 1 && a.GetTrace()[1].tick == 61,
        "requests zero/sixty apply at boundaries one/sixty-one");
    const std::array trace_oracle{
        AppliedCommand{FieldEdit{FieldEditKind::set, 0, {8, 8}, 8, 4}, 1, 1},
        AppliedCommand{FieldEdit{FieldEditKind::set, 0, {24, 8}, 8, 4}, 2, 61}};
    const auto same_command = [](const AppliedCommand& p, const AppliedCommand& q) {
        return p.sequence == q.sequence && p.tick == q.tick && SameEdit(p.command.field, q.command.field);
    };
    Require(std::ranges::equal(a.GetTrace(), trace_oracle, same_command) &&
        std::ranges::equal(b.GetTrace(), trace_oracle, same_command), "independent applied route trace");
    Require(observed.TryCapture(combined) && partitioned.TryCapture(divided), "route completed observations");
    Require(observed.GetFields()[0].center.x == 24 && observed.GetFields()[0].center.y == 8,
        "second route edit applied after boundary sixty-one");
    SameFrame(observed, partitioned);
    Require(replay.TryReplay(a.GetTrace(), 61).status == HeadlessSession::StepStatus::advanced &&
        expected.TryCapture(replayed), "route accepted trace independently replays");
    SameFrame(observed, expected);
}
}
int main() {
    try { CheckIndependentEdits(); CheckBoundariesSchedulesAndReplay(); }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
