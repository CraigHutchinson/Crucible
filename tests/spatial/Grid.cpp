#include <crucible/spatial/Grid.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <set>
#include <numbers>
#include <vector>

namespace {
void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}
std::vector<crucible::SampleId> Reference(std::span<const crucible::spatial::SpatialSample> samples,
                                        crucible::Position center, float radius) {
    center.x = std::clamp(center.x, 0.0F, 14.0F);
    center.y = std::clamp(center.y, 0.0F, 10.0F);
    std::vector<crucible::SampleId> result;
    for (const auto& sample : samples) {
        const double dx = static_cast<double>(std::clamp(sample.position.x, 0.0F, 14.0F)) - center.x;
        const double dy = static_cast<double>(std::clamp(sample.position.y, 0.0F, 10.0F)) - center.y;
        if (dx * dx + dy * dy <= static_cast<double>(radius) * radius) result.push_back(sample.id);
    }
    std::ranges::sort(result);
    return result;
}
void Compare(crucible::spatial::Grid& grid, std::span<const crucible::spatial::SpatialSample> samples,
             crucible::Position center, float radius) {
    const auto expected = Reference(samples, center, radius);
    const auto actual = grid.TryQuery(center, radius);
    Check(actual.has_value(), "valid radius query rejected");
    Check(std::ranges::equal(*actual, expected), "radius query differs from complete brute-force result");
}
}

int main() {
    using namespace crucible;
    using namespace crucible::spatial;
    Grid grid{{7, 5, 2.0F}, 1600};
    Check(grid.TryRebuild({}), "empty rebuild rejected");
    Check(grid.GetOccupiedCellCount() == 0, "empty occupied cells");
    Compare(grid, {}, {0, 0}, 100.0F);
    std::vector<SpatialSample> samples;
    for (std::uint64_t i = 0; i < 1024; ++i) samples.push_back({{2000 - i}, {6, 4}});
    for (std::uint64_t i = 0; i < 512; ++i)
        samples.push_back({{10000 + i}, {static_cast<float>(i % 29) * 0.5F,
                                        static_cast<float>((i * 11) % 21) * 0.5F}});
    samples.push_back({{50000}, {-100, -100}});
    samples.push_back({{50001}, {100, 100}});
    samples.push_back({{50002}, {14, 0}});
    samples.push_back({{50003}, {0, 10}});
    Check(grid.TryRebuild(samples), "dense rectangular rebuild rejected");
    // Occupancy is a representation diagnostic; query identities still use the independent scan.
    const auto layout = sub0hexgrid::PointyLayout::TryCreate(2.0 / std::numbers::sqrt3);
    std::set<std::pair<std::int32_t, std::int32_t>> occupied_cells;
    for (const auto& sample : samples) {
        const auto cell = layout->TryCellAt({std::clamp(sample.position.x, 0.0F, 14.0F),
                                           std::clamp(sample.position.y, 0.0F, 10.0F)});
        Check(cell.has_value(), "fixture mapping rejected");
        occupied_cells.emplace(cell->q, cell->r);
    }
    Check(grid.GetOccupiedCellCount() == occupied_cells.size(), "hex occupancy differs from mapped fixture");
    for (const auto center : {Position{6, 4}, Position{0, 0}, Position{14, 10}, Position{14, 0},
                              Position{0, 10}, Position{-9, -3}, Position{50, 50}, Position{3.1F, 7.7F}})
        for (float radius : {0.0F, 0.1F, 1.0F, 2.0F, 2.01F, 5.3F, 17.0F, std::numeric_limits<float>::max()})
            Compare(grid, samples, center, radius);
    const auto dense = grid.TryQuery({6, 4}, 0.0F);
    Check(dense && dense->size() >= 1024, "dense coincident results truncated");
    std::ranges::reverse(samples);
    Check(grid.TryRebuild(samples), "shuffled rebuild rejected");
    Compare(grid, samples, {6, 4}, 5.3F);
    const auto occupied = grid.GetOccupiedCellCount();
    auto invalid = samples;
    invalid.back().id = invalid.front().id;
    Check(!grid.TryRebuild(invalid), "duplicate ID accepted");
    invalid = samples;
    invalid.back().position.x = std::numeric_limits<float>::infinity();
    Check(!grid.TryRebuild(invalid), "nonfinite position accepted");
    invalid.resize(1601);
    Check(!grid.TryRebuild(invalid), "capacity overflow accepted");
    Check(grid.GetOccupiedCellCount() == occupied, "rejection damaged committed grid");
    Compare(grid, samples, {6, 4}, 5.3F);
    Check(!grid.TryQuery({0, 0}, -1), "negative radius accepted");
    Check(!grid.TryQuery({0, 0}, std::numeric_limits<float>::infinity()), "infinite radius accepted");
    Check(!grid.TryQuery({std::numeric_limits<float>::quiet_NaN(), 0}, 1), "NaN center accepted");
    Check(grid.TryRebuild({}), "reset to empty rejected");
    Compare(grid, {}, {6, 4}, 100);
    for (auto config : {GridConfig{0, 1, 1}, GridConfig{1, 0, 1}, GridConfig{1, 1, 0},
                        GridConfig{1, 1, -1}, GridConfig{2, 2, std::numeric_limits<float>::max()},
                        GridConfig{std::numeric_limits<std::size_t>::max(), 2, 1}}) {
        bool rejected = false;
        try { Grid bad{config, 0}; } catch (const std::invalid_argument&) { rejected = true; }
        Check(rejected, "invalid grid startup accepted");
    }
    bool rejected_capacity = false;
    try { Grid bad{{1, 1, 1}, std::numeric_limits<std::size_t>::max()}; }
    catch (const std::length_error&) { rejected_capacity = true; }
    Check(rejected_capacity, "overflowed sample storage accepted");
    Grid single{{1, 1, 0.5F}, 1};
    const SpatialSample edge{{1}, {1, -1}};
    Check(single.TryRebuild(std::span{&edge, 1}), "single cell rebuild failed");
    const auto clamped = single.TryQuery({0.5F, 0}, 0);
    Check(clamped && clamped->size() == 1, "single cell clamp/coincidence failed");
}
