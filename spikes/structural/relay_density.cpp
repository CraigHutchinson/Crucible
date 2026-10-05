// Production-consumer investigation: compare passive, radial and straight-flow
// concentration at one prospective relay. No fusion, protection or outcome model
// is simulated here; snapshots, radius queries and full replay are actual runtime.
#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <crucible/runtime/HeadlessSession.hpp>
#include <crucible/simulation.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>

namespace {
using namespace crucible;
constexpr std::size_t Population = 2048;
constexpr Position Relay{48.5F, 16.5F};
constexpr float Radius = 4;
constexpr std::size_t Required = 32;
constexpr std::array<std::uint64_t, 6> Checkpoints{0, 60, 120, 240, 480, 900};
constexpr Simulation::ScenarioOptions Options{{64, 32, 1}, 4, SteeringSettings{}, ResourceSettings{}};

void Require(bool condition, const char* reason) {
    if (!condition) throw std::runtime_error(reason);
}

std::size_t BruteCount(std::span<const SampleState> samples, Position center, float radius) {
    std::size_t count = 0;
    const double squared_radius = static_cast<double>(radius) * radius;
    for (const auto& sample : samples) {
        const double x = static_cast<double>(sample.position.x) - center.x;
        const double y = static_cast<double>(sample.position.y) - center.y;
        count += x * x + y * y <= squared_radius;
    }
    return count;
}

void SpatialFixture() {
    Simulation simulation{3, {{3, 1, 1}, 1, SteeringSettings{}, ResourceSettings{}}};
    presentation::ScenarioSnapshot frame{3, 1, 3};
    Require(frame.TryCapture(simulation), "hand-count fixture capture");
    // Startup centers (.5,.5), (1.5,.5), (2.5,.5) give exactly 3 inside
    // inclusive radius1, and exactly 1 inside the preceding representable radius.
    for (const auto radius : std::array{1.0F, std::nextafter(1.0F, 0.0F)}) {
        const std::size_t expected = radius == 1 ? 3 : 1;
        const auto query = simulation.TryCountNeighbors({1.5F, .5F}, radius);
        Require(query && *query == expected &&
            BruteCount(frame.GetSamples(), {1.5F, .5F}, radius) == expected,
            "independent inclusive-radius hand count");
    }
}

bool SameFrame(const presentation::ScenarioSnapshot& actual,
               const presentation::ScenarioSnapshot& replay) {
    const auto a = actual.GetInfo();
    const auto b = replay.GetInfo();
    if (!a || !b || a->completed_tick != b->completed_tick || a->samples != b->samples ||
        a->fields != b->fields || a->cells != b->cells || a->biomass != b->biomass ||
        a->grid.columns != b->grid.columns || a->grid.rows != b->grid.rows ||
        a->grid.cell_size != b->grid.cell_size) return false;
    return std::ranges::equal(actual.GetSamples(), replay.GetSamples(), [](const auto& x, const auto& y) {
        return x.id == y.id && x.position.x == y.position.x && x.position.y == y.position.y &&
            x.velocity.x == y.velocity.x && x.velocity.y == y.velocity.y;
    }) && std::ranges::equal(actual.GetFields(), replay.GetFields(), [](const auto& x, const auto& y) {
        return x.kind == y.kind && x.slot == y.slot && x.center.x == y.center.x &&
            x.center.y == y.center.y && x.radius == y.radius && x.strength == y.strength &&
            x.end.x == y.end.x && x.end.y == y.end.y;
    }) && std::ranges::equal(actual.GetBlight(), replay.GetBlight()) &&
        std::ranges::equal(actual.GetStocks(), replay.GetStocks());
}

void Run(std::string_view strategy, std::optional<FieldEdit> edit, std::uint64_t horizon) {
    Simulation simulation{Population, Options};
    runtime::HeadlessSession session{simulation, {8, 8}};
    presentation::ScenarioSnapshot frame{Population, 4, 2048};
    std::size_t checkpoint = 0;
    std::size_t startup_count = 0;
    std::size_t minimum_count = Population, maximum_count = 0;
    std::uint64_t eligible_run = 0, longest_eligible_run = 0;
    for (std::uint64_t tick = 0; tick <= horizon; ++tick) {
        Require(frame.TryCapture(simulation), "density observation capture");
        const auto info = frame.GetInfo();
        Require(info && info->biomass && info->completed_tick == tick, "density metadata");
        const auto ledger = *info->biomass;
        Require(ledger.initial_total == ledger.remaining_stock + ledger.mobile_mass + ledger.reserve,
            "density conserved biomass");
        const auto count = BruteCount(frame.GetSamples(), Relay, Radius);
        const auto query = simulation.TryCountNeighbors(Relay, Radius);
        Require(query && *query == count, "complete-radius query parity with snapshot brute force");
        minimum_count = std::min(minimum_count, count);
        maximum_count = std::max(maximum_count, count);
        if (tick == 0) startup_count = count;
        else {
            eligible_run = count >= Required ? eligible_run + 1 : 0;
            longest_eligible_run = std::max(longest_eligible_run, eligible_run);
        }
        if (tick == Checkpoints[checkpoint]) {
            std::cout << "{\"kind\":\"density\",\"strategy\":\"" << strategy << "\",\"tick\":" << tick
                << ",\"relay_x\":" << Relay.x << ",\"relay_y\":" << Relay.y
                << ",\"radius\":" << Radius << ",\"required\":" << Required
                << ",\"count\":" << count
                << ",\"minimum_count_through_tick\":" << minimum_count
                << ",\"maximum_count_through_tick\":" << maximum_count
                << ",\"delta_from_startup\":" << (static_cast<std::int64_t>(count) - static_cast<std::int64_t>(startup_count))
                << ",\"eligible_run_completed_ticks\":" << eligible_run
                << ",\"longest_eligible_run_completed_ticks\":" << longest_eligible_run
                << ",\"eligible\":" << (count >= Required ? "true" : "false")
                << ",\"ledger\":{\"initial_total\":" << ledger.initial_total
                << ",\"remaining_stock\":" << ledger.remaining_stock << ",\"mobile_mass\":" << ledger.mobile_mass
                << ",\"reserve\":" << ledger.reserve << ",\"harvested\":" << ledger.harvested
                << ",\"work_actions\":" << ledger.work_actions << "}}\n";
            ++checkpoint;
        }
        if (tick == horizon) break;
        if (tick == 0 && edit) {
            Require(session.GetIngress().TryAdmit(std::span{&*edit, std::size_t{1}}).status ==
                runtime::CommandIngress::AdmissionStatus::accepted, "study field admission");
        }
        Require(session.TryStep().status == runtime::HeadlessSession::StepStatus::advanced,
            "study completed boundary");
    }
    const auto trace = session.GetTrace();
    Require(trace.size() == (edit ? 1 : 0), "exact strategy command count");
    if (edit) Require(trace.front().tick == 1 && trace.front().sequence == 1,
        "one command applied at first completed boundary");
    Simulation replay_simulation{Population, Options};
    runtime::HeadlessSession replay_session{replay_simulation, {8, 8}};
    presentation::ScenarioSnapshot replay_frame{Population, 4, 2048};
    Require(replay_session.TryReplay(trace, horizon).status ==
        runtime::HeadlessSession::StepStatus::advanced && replay_frame.TryCapture(replay_simulation) &&
        SameFrame(frame, replay_frame), "full-state replay through final boundary");
    std::cout << "{\"kind\":\"replay\",\"strategy\":\"" << strategy << "\",\"tick\":"
        << horizon << ",\"commands\":" << trace.size() << ",\"status\":\"full-state-pass\"}\n";
}
}

int main(int argc, char** argv) {
    try {
        if (argc > 2 || (argc == 2 && std::string_view{argv[1]} != "--verify"))
            throw std::invalid_argument("usage: crucible_relay_density [--verify]");
        // Cross-platform receiving uses the same production paths and strategies,
        // bounded to 120 ticks. The full 900-tick investigation runs separately.
        const std::uint64_t horizon = argc == 2 ? 120 : Checkpoints.back();
        SpatialFixture();
        Run("passive", std::nullopt, horizon);
        Run("radial", FieldEdit{FieldEditKind::set, 0, Relay, 8, 4}, horizon);
        Run("straight-flow", FieldEdit{FieldEditKind::set_flow, 0, {24.5F, 16.5F}, 8, 4, Relay}, horizon);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "relay density investigation failed: " << error.what() << '\n';
        return 1;
    }
}
