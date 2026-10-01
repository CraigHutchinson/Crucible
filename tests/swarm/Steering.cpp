#include <crucible/swarm/Steering.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

#include <crucible/contracts/FieldEdit.hpp>
#include <crucible/contracts/Tick.hpp>
#include <crucible/fields/FieldSet.hpp>
#include <crucible/spatial/Grid.hpp>

namespace {
using namespace crucible;

void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

bool Equal(const SampleState& left, const SampleState& right) {
    return left.id == right.id && left.position.x == right.position.x &&
           left.position.y == right.position.y && left.velocity.x == right.velocity.x &&
           left.velocity.y == right.velocity.y;
}

bool Near(float left, float right) {
    return std::abs(static_cast<double>(left) - right) <=
           2.0e-6 * std::max({1.0, std::abs(static_cast<double>(left)), std::abs(static_cast<double>(right))});
}

// Deliberately scans every immutable sample and computes fields independently of Grid/FieldSet.
std::vector<SampleState> Reference(std::span<const SampleState> input, GridConfig config,
                                   SteeringSettings settings, std::span<const FieldEdit> edits) {
    std::vector<SampleState> result;
    const auto extent = *config.TryValidate();
    for (const auto& sample : input) {
        double sx = 0, sy = 0;
        std::size_t count = 0;
        for (const auto& neighbor : input) {
            if (sample.id == neighbor.id) continue;
            const double dx = static_cast<double>(sample.position.x) - neighbor.position.x;
            const double dy = static_cast<double>(sample.position.y) - neighbor.position.y;
            const double squared = dx * dx + dy * dy;
            if (squared > static_cast<double>(settings.neighbor_radius) * settings.neighbor_radius) continue;
            ++count;
            if (squared == 0.0) sx += sample.id < neighbor.id ? -1.0 : 1.0;
            else {
                const double distance = std::sqrt(squared);
                const double coefficient = std::max(0.0, 1.0 - distance / settings.neighbor_radius) / distance;
                sx += dx * coefficient;
                sy += dy * coefficient;
            }
        }
        if (count != 0) {
            sx = sx / static_cast<double>(count) * settings.separation_strength;
            sy = sy / static_cast<double>(count) * settings.separation_strength;
        }
        double fx = 0, fy = 0;
        for (const auto& edit : edits) {
            if (edit.radius == 0.0F) continue;
            const double dx = static_cast<double>(edit.center.x) - sample.position.x;
            const double dy = static_cast<double>(edit.center.y) - sample.position.y;
            const double distance = std::sqrt(dx * dx + dy * dy);
            if (distance == 0.0 || distance >= edit.radius) continue;
            const double coefficient = edit.strength * (1.0 - distance / edit.radius) / distance;
            fx += dx * coefficient;
            fy += dy * coefficient;
        }
        const double float_limit = std::numeric_limits<float>::max();
        const double ax = sx + static_cast<float>(std::clamp(fx, -float_limit, float_limit));
        const double ay = sy + static_cast<float>(std::clamp(fy, -float_limit, float_limit));
        const double acceleration = std::sqrt(ax * ax + ay * ay);
        const double force_scale = acceleration == 0.0 ? 1.0 :
            std::min(1.0, settings.maximum_acceleration / acceleration);
        double vx = sample.velocity.x + ax * force_scale * tick_seconds;
        double vy = sample.velocity.y + ay * force_scale * tick_seconds;
        const double speed = std::sqrt(vx * vx + vy * vy);
        const double speed_scale = speed == 0.0 ? 1.0 : std::min(1.0, settings.maximum_speed / speed);
        vx *= speed_scale;
        vy *= speed_scale;
        result.push_back({sample.id,
            {static_cast<float>(std::min(static_cast<double>(extent.width),
                                        std::max(0.0, sample.position.x + vx * tick_seconds))),
             static_cast<float>(std::min(static_cast<double>(extent.height),
                                        std::max(0.0, sample.position.y + vy * tick_seconds)))},
            {static_cast<float>(vx), static_cast<float>(vy)}});
    }
    return result;
}

std::vector<spatial::SpatialSample> Gather(std::span<const SampleState> samples) {
    std::vector<spatial::SpatialSample> result;
    for (const auto& sample : samples) result.push_back({sample.id, sample.position});
    return result;
}

void Compare(std::span<const SampleState> input, std::span<const SampleState> actual,
             std::span<const SampleState> expected, GridConfig config, SteeringSettings settings) {
    Check(actual.size() == expected.size(), "output/reference count differs");
    const auto extent = *config.TryValidate();
    for (std::size_t row = 0; row < actual.size(); ++row) {
        const auto& sample = actual[row];
        Check(sample.id == expected[row].id && Near(sample.position.x, expected[row].position.x) &&
              Near(sample.position.y, expected[row].position.y) && Near(sample.velocity.x, expected[row].velocity.x) &&
              Near(sample.velocity.y, expected[row].velocity.y), "steering differs from brute-force reference");
        Check(std::isfinite(sample.position.x) && std::isfinite(sample.position.y) &&
              std::isfinite(sample.velocity.x) && std::isfinite(sample.velocity.y), "nonfinite steering output");
        Check(sample.position.x >= 0 && sample.position.x <= extent.width &&
              sample.position.y >= 0 && sample.position.y <= extent.height, "position outside world");
        Check(std::hypot(sample.velocity.x, sample.velocity.y) <= settings.maximum_speed * (1.0 + 2.0e-6),
              "Euclidean speed cap violated");
        // A preexisting overspeed velocity can be reduced by more than the acceleration cap.
        if (std::hypot(input[row].velocity.x, input[row].velocity.y) <= settings.maximum_speed)
            Check(std::hypot(static_cast<double>(sample.velocity.x) - input[row].velocity.x,
                             static_cast<double>(sample.velocity.y) - input[row].velocity.y) <=
                  settings.maximum_acceleration * tick_seconds + settings.maximum_speed * 2.0e-6,
                  "Euclidean acceleration cap violated");
    }
}

void RunFixture(std::vector<SampleState> input, GridConfig config, SteeringSettings settings,
                std::span<const FieldEdit> edits = {}) {
    fields::FieldSet fields{edits.size()};
    for (const auto& edit : edits)
        Check(fields.TryApplyEdit(edit) == fields::EditResult::applied, "fixture field rejected");
    swarm::Steering steering{config, settings, input.size()};
    spatial::Grid grid{config, input.size()};
    auto spatial_input = Gather(input);
    Check(grid.TryRebuild(spatial_input), "fixture rebuild rejected");
    const auto expected = Reference(input, config, settings, edits);
    std::vector<SampleState> output(input.size());
    auto* const destination = output.data();
    for (int repetition = 0; repetition < 3; ++repetition) {
        Check(steering.TryCompute(input, fields, grid, output), "valid fixture rejected");
        Compare(input, output, expected, config, settings);
        Check(output.data() == destination, "destination storage changed");
    }
    const auto first = output;
    std::ranges::reverse(spatial_input);
    Check(grid.TryRebuild(spatial_input), "reverse grid gather rejected");
    Check(steering.TryCompute(input, fields, grid, output), "reverse grid order rejected");
    Check(std::ranges::equal(output, first, Equal), "grid gather order changed output");
    auto in_place = input;
    Check(steering.TryCompute(in_place, fields, grid, in_place), "in-place computation rejected");
    Check(std::ranges::equal(in_place, first, Equal), "in-place alias changed output");
}

void CheckFailureContracts() {
    const GridConfig config{8, 8, 1};
    const SteeringSettings settings{1, 6, 16, 4};
    fields::FieldSet fields{0};
    spatial::Grid grid{config, 4};
    swarm::Steering steering{config, settings, 2};
    const std::array<SampleState, 2> input{{{{7}, {1, 1}, {}}, {{1000001}, {6, 6}, {}}}};
    Check(grid.TryRebuild(Gather(input)), "failure fixture rebuild rejected");
    const SampleState sentinel{{9999999}, {2, 3}, {4, 5}};
    std::array<SampleState, 3> output{sentinel, sentinel, sentinel};
    const auto before = output;
    const auto unchanged = [&] { return std::ranges::equal(output, before, Equal); };
    Check(!steering.TryCompute(input, fields, grid, std::span{output}.first(1)) && unchanged(),
          "short output changed destination");
    const std::array<SampleState, 3> excess{{input[0], input[1], {{1000002}, {7, 7}, {}}}};
    Check(!steering.TryCompute(excess, fields, grid, output) && unchanged(), "excess input changed destination");
    for (int invalid_kind = 0; invalid_kind < 8; ++invalid_kind) {
        auto invalid = input;
        switch (invalid_kind) {
        case 0: invalid[1].id = invalid[0].id; break;
        case 1: std::ranges::reverse(invalid); break;
        case 2: invalid[1].position.x = std::numeric_limits<float>::quiet_NaN(); break;
        case 3: invalid[1].position.y = std::numeric_limits<float>::infinity(); break;
        case 4: invalid[1].velocity.x = std::numeric_limits<float>::quiet_NaN(); break;
        case 5: invalid[1].velocity.y = std::numeric_limits<float>::infinity(); break;
        case 6: invalid[1].position.x = -0.1F; break;
        case 7: invalid[1].position.y = 8.1F; break;
        }
        Check(!steering.TryCompute(invalid, fields, grid, output) && unchanged(),
              "invalid input accepted or changed destination");
    }
    auto wrong_grid = Gather(input);
    wrong_grid.push_back({{7777777}, input[1].position});
    Check(grid.TryRebuild(wrong_grid), "unknown-neighbor fixture rejected");
    Check(!steering.TryCompute(input, fields, grid, output) && unchanged(),
          "late unknown neighbor published partial output");
    Check(grid.TryRebuild({}), "empty grid rebuild rejected");
    Check(!steering.TryCompute(input, fields, grid, output) && unchanged(), "missing self accepted");
    Check(steering.TryCompute({}, fields, grid, output) && unchanged(), "empty input changed output");
    Check(grid.TryRebuild(Gather(input)), "restore grid failed");
    Check(steering.TryCompute(input, fields, grid, output), "valid call after failures rejected");
    Check(Equal(output.back(), sentinel), "success changed output tail");
    swarm::Steering empty{config, settings, 0};
    Check(empty.TryCompute({}, fields, grid, {}), "zero-capacity empty input rejected");
}

void CheckStartupContracts() {
    const GridConfig config{8, 8, 1};
    const auto infinity = std::numeric_limits<float>::infinity();
    const auto nan = std::numeric_limits<float>::quiet_NaN();
    for (const auto invalid : {SteeringSettings{0, 1, 1, 1}, SteeringSettings{-1, 1, 1, 1},
         SteeringSettings{nan, 1, 1, 1}, SteeringSettings{infinity, 1, 1, 1},
         SteeringSettings{1, -1, 1, 1}, SteeringSettings{1, nan, 1, 1},
         SteeringSettings{1, infinity, 1, 1}, SteeringSettings{1, 1, 0, 1},
         SteeringSettings{1, 1, -1, 1}, SteeringSettings{1, 1, nan, 1},
         SteeringSettings{1, 1, infinity, 1}, SteeringSettings{1, 1, 1, 0},
         SteeringSettings{1, 1, 1, -1}, SteeringSettings{1, 1, 1, nan},
         SteeringSettings{1, 1, 1, infinity}}) {
        bool rejected = false;
        try { swarm::Steering bad{config, invalid, 0}; }
        catch (const std::invalid_argument&) { rejected = true; }
        Check(rejected, "invalid steering setting accepted");
    }
    for (const auto invalid : {GridConfig{0, 8, 1}, GridConfig{8, 0, 1}, GridConfig{8, 8, 0},
                              GridConfig{8, 8, infinity}, GridConfig{8, 8, nan},
                              GridConfig{2, 2, std::numeric_limits<float>::max()}}) {
        bool rejected = false;
        try { swarm::Steering bad{invalid, {}, 0}; }
        catch (const std::invalid_argument&) { rejected = true; }
        Check(rejected, "invalid steering world accepted");
    }
    bool rejected_capacity = false;
    try { swarm::Steering bad{config, {}, std::numeric_limits<std::size_t>::max()}; }
    catch (const std::length_error&) { rejected_capacity = true; }
    Check(rejected_capacity, "overflowed startup capacity accepted");
}

void CheckAnalyticPairs() {
    const GridConfig config{8, 8, 1};
    const SteeringSettings settings{2, 4, 16, 4};
    fields::FieldSet fields{0};
    spatial::Grid grid{config, 3};
    swarm::Steering steering{config, settings, 3};
    std::array<SampleState, 3> input{{{{7}, {3, 3}, {}}, {{1000001}, {4, 3}, {}},
                                     {{1000009}, {5, 3}, {}}}};
    std::array<SampleState, 3> output{};
    Check(grid.TryRebuild(Gather(input)), "analytic fixture rebuild failed");
    Check(steering.TryCompute(input, fields, grid, output), "analytic fixture failed");
    Check(Near(output.front().velocity.x, -tick_seconds) && output.front().velocity.y == 0 &&
          output[1].velocity.x == 0 && Near(output.back().velocity.x, tick_seconds),
          "inclusive zero-weight neighbor missing from average or pair symmetry broken");
    input[1].position = input[0].position;
    const auto pair = std::span{input}.first(2);
    Check(grid.TryRebuild(Gather(pair)), "coincident analytic rebuild failed");
    Check(steering.TryCompute(pair, fields, grid, output), "coincident analytic compute failed");
    Check(Near(output[0].velocity.x, -4 * tick_seconds) && Near(output[1].velocity.x, 4 * tick_seconds) &&
          output[0].velocity.y == 0 && output[1].velocity.y == 0,
          "coincident pair did not use antisymmetric ID x-axis direction");
}
}

int main() {
    const GridConfig config{8, 6, 2};
    const SteeringSettings settings{2, 6, 16, 4};
    RunFixture({}, config, settings);
    RunFixture({{{700000}, {5, 5}, {1, -1}}}, config, settings);
    RunFixture({{{7}, {5, 5}, {}}, {{1000001}, {6, 5}, {}}}, config, settings);
    RunFixture({{{7}, {5, 5}, {}}, {{1000001}, {5, 5}, {}}}, config, settings);
    RunFixture({{{7}, {5, 5}, {}}, {{1000001}, {6, 5}, {}}, {{1000009}, {7, 5}, {}}},
               config, {2, 4, 16, 4});
    RunFixture({{{7}, {0, 0}, {-4, -4}}, {{1000001}, {16, 12}, {4, 4}},
                {{1000009}, {0, 12}, {-4, 4}}, {{1000011}, {16, 0}, {4, -4}}}, config, settings);
    const std::array<FieldEdit, 2> edits{{{FieldEditKind::set, 0, {8, 6}, 10, 12},
                                         {FieldEditKind::set, 1, {2, 2}, 6, -9}}};
    RunFixture({{{7}, {5, 5}, {1, -1}}, {{1000001}, {6, 5}, {-1, 2}},
                {{1000009}, {8, 6}, {}}}, config, settings, edits);
    const auto huge = std::numeric_limits<float>::max();
    const std::array<FieldEdit, 2> strong_fields{{{FieldEditKind::set, 0, {8, 8}, 20, huge},
                                                {FieldEditKind::set, 1, {8, 8}, 20, huge}}};
    RunFixture({{{7}, {5, 5}, {}}}, config, {2, huge, 0.5F, 0.002F}, strong_fields);
    RunFixture({{{7}, {5, 5}, {}}, {{1000001}, {5, 5}, {}}}, config,
               {std::numeric_limits<float>::denorm_min(), huge, 1, 1});
    RunFixture({{{7}, {5, 5}, {}}}, config, {2, 0, 16, 4}, edits);
    std::vector<SampleState> dense;
    for (std::uint64_t index = 0; index < 2048; ++index) {
        const Position position = index < 1024 ? Position{8, 6} :
            Position{static_cast<float>(index % 64) * 0.25F,
                     static_cast<float>((index / 64) % 32) * 0.375F};
        dense.push_back({{1000000000 + index * 997}, position,
                         {static_cast<float>(index % 3) - 1, static_cast<float>(index % 5) - 2}});
    }
    RunFixture(dense, config, settings, edits);
    CheckFailureContracts();
    CheckStartupContracts();
    CheckAnalyticPairs();
}
