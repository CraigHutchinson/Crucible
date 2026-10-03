#include <crucible/spatial/Grid.hpp>

#include <algorithm>
#include <array>
#include <cfenv>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <numbers>
#include <vector>

namespace {
void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

std::vector<crucible::SampleId> Scan(crucible::GridExtent extent,
    std::span<const crucible::spatial::SpatialSample> samples, crucible::Position center, float radius) {
    center.x = std::clamp(center.x, 0.0F, extent.width);
    center.y = std::clamp(center.y, 0.0F, extent.height);
    const double squared_radius = static_cast<double>(radius) * radius;
    std::vector<crucible::SampleId> ids;
    for (const auto& sample : samples) {
        const double dx = static_cast<double>(std::clamp(sample.position.x, 0.0F, extent.width)) - center.x;
        const double dy = static_cast<double>(std::clamp(sample.position.y, 0.0F, extent.height)) - center.y;
        if (dx * dx + dy * dy <= squared_radius) ids.push_back(sample.id);
    }
    std::ranges::sort(ids);
    return ids;
}

void Compare(crucible::spatial::Grid& grid, crucible::GridExtent extent,
    std::span<const crucible::spatial::SpatialSample> samples, crucible::Position center, float radius) {
    const auto expected = Scan(extent, samples, center, radius);
    const auto actual = grid.TryQuery(center, radius);
    Check(actual && std::ranges::equal(*actual, expected), "hex queries differ from independent exact scan");
    Check(std::ranges::adjacent_find(*actual) == actual->end(), "duplicate identity emitted across cells");
}

// Independent lattice center search: no cube-rounding, candidate or region helper is used.
void CheckNearestCenter(sub0hexgrid::PointyLayout layout, crucible::Position point) {
    const auto cell = layout.TryCellAt({point.x, point.y});
    Check(cell.has_value(), "representable point mapping rejected");
    const long double scale = layout.GetRadius();
    const auto distance = [&](std::int32_t q, std::int32_t r) {
        const long double x = scale * std::sqrt(3.0L) * (q + 0.5L * r);
        const long double y = scale * 1.5L * r;
        return (x - point.x) * (x - point.x) + (y - point.y) * (y - point.y);
    };
    const auto chosen = distance(cell->q, cell->r);
    long double nearest = std::numeric_limits<long double>::max();
    for (std::int32_t r = -8; r <= 24; ++r)
        for (std::int32_t q = -16; q <= 24; ++q) nearest = std::min(nearest, distance(q, r));
    // Computed-double seam choices may select either analytic nearest center.
    const auto tolerance = 128 * std::numeric_limits<double>::epsilon() * scale * scale;
    Check(chosen <= nearest + tolerance, "world mapping is not an independently nearest center");
}

void Exhaustive(crucible::GridConfig config) {
    const auto extent = *config.TryValidate();
    crucible::spatial::Grid grid{config, 2048};
    std::vector<crucible::spatial::SpatialSample> samples;
    const auto add = [&](float x, float y) {
        samples.push_back({{10000 - samples.size()}, {x, y}});
    };
    for (std::size_t y = 0; y <= 16; ++y)
        for (std::size_t x = 0; x <= 16; ++x)
            add(extent.width * (static_cast<float>(x) / 16), extent.height * (static_cast<float>(y) / 16));
    const auto layout = *sub0hexgrid::PointyLayout::TryCreate(config.cell_size / std::numbers::sqrt3);
    for (int r = 0; r <= 5; ++r) {
        for (int q = -2; q <= 5; ++q) {
            const auto x = static_cast<float>(layout.GetRadius() * std::numbers::sqrt3 * (q + 0.5 * r));
            const auto y = static_cast<float>(layout.GetRadius() * 1.5 * r);
            for (int corner = 0; corner < 6; ++corner) {
                const auto angle = std::numbers::pi / 3 * corner + std::numbers::pi / 6;
                const auto vx = static_cast<float>(x + layout.GetRadius() * std::cos(angle));
                const auto vy = static_cast<float>(y + layout.GetRadius() * std::sin(angle));
                add(vx, vy);
                add(std::nextafter(vx, -std::numeric_limits<float>::infinity()), vy);
                add(std::nextafter(vx, std::numeric_limits<float>::infinity()), vy);
                add((x + vx) * 0.5F, (y + vy) * 0.5F);
            }
        }
    }
    add(-config.cell_size, -config.cell_size);
    add(extent.width + config.cell_size, extent.height + config.cell_size);
    Check(grid.TryRebuild(samples), "seam fixture rebuild failed");
    for (const auto& sample : samples) {
        const auto center = sample.position;
        for (float radius : {0.0F, config.cell_size * 0.01F, config.cell_size,
                             config.cell_size * 4, std::numeric_limits<float>::max()})
            Compare(grid, extent, samples, center, radius);
    }
    if (config.cell_size == 1.0F)
        for (const auto& sample : samples)
            if (sample.position.x >= 0 && sample.position.y >= 0 &&
                sample.position.x <= extent.width && sample.position.y <= extent.height)
                CheckNearestCenter(layout, sample.position);
    std::ranges::reverse(samples);
    Check(grid.TryRebuild(samples), "seam shuffle rebuild failed");
    Compare(grid, extent, samples, {0, 0}, std::numeric_limits<float>::max());
}

void PhysicalExtremes() {
    for (float scale : {std::numeric_limits<float>::denorm_min(), std::numeric_limits<float>::min(),
                        std::numeric_limits<float>::max()}) {
        const crucible::GridConfig config{1, 1, scale};
        const auto extent = *config.TryValidate();
        const std::array samples{crucible::spatial::SpatialSample{{1}, {0, 0}},
                                crucible::spatial::SpatialSample{{2}, {scale, scale}}};
        crucible::spatial::Grid grid{config, samples.size()};
        Check(grid.TryRebuild(samples), "finite physical extreme startup/rebuild rejected");
        for (auto center : {crucible::Position{0, 0}, crucible::Position{scale, scale}})
            for (float radius : {0.0F, scale, std::numeric_limits<float>::max()})
                Compare(grid, extent, samples, center, radius);
    }
    const crucible::GridConfig config{1, 4096, 1};
    const auto extent = *config.TryValidate();
    const std::array samples{crucible::spatial::SpatialSample{{3}, {0.5F, 0}},
                            crucible::spatial::SpatialSample{{1}, {0.5F, 2048}},
                            crucible::spatial::SpatialSample{{2}, {1, 4096}}};
    crucible::spatial::Grid elongated{config, samples.size()};
    Check(elongated.TryRebuild(samples), "elongated physical world rejected");
    Check(elongated.GetOccupiedCellCount() == 3, "rectangular memory guard fallback occupancy");
    for (float radius : {0.0F, 1.0F, 4096.0F, std::numeric_limits<float>::max()})
        Compare(elongated, extent, samples, {0.5F, 2048}, radius);
}

void RoundingChanges() {
    const int saved = std::fegetround();
    Check(std::fesetround(FE_TONEAREST) == 0, "cannot set reference rounding");
    const crucible::GridConfig config{4, 4, 1};
    const auto extent = *config.TryValidate();
    const std::array samples{crucible::spatial::SpatialSample{{3}, {0.5F, 0}},
                            crucible::spatial::SpatialSample{{1}, {2, 2}},
                            crucible::spatial::SpatialSample{{2}, {4, 4}}};
    crucible::spatial::Grid grid{config, samples.size()};
    Check(grid.TryRebuild(samples), "round-nearest rebuild failed");
    for (int mode : {FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO}) {
        Check(std::fesetround(mode) == 0, "cannot set supported rounding mode");
        Compare(grid, extent, samples, {0.5F, 0}, 0);
        Compare(grid, extent, samples, {0, 0}, 4);
        Check(grid.TryRebuild(samples), "changed-rounding rebuild rejected");
        crucible::spatial::Grid constructed_outside_nearest{config, samples.size()};
        Check(constructed_outside_nearest.TryRebuild(samples), "nonnearest constructor rejected");
        Compare(constructed_outside_nearest, extent, samples, {0, 0}, 4);
        Check(std::fesetround(FE_TONEAREST) == 0, "cannot restore reference rounding");
        Compare(grid, extent, samples, {0, 0}, 4);
        Check(grid.TryRebuild(samples), "restored-rounding rebuild failed");
        Compare(grid, extent, samples, {0, 0}, 4);
    }
    Check(std::fesetround(saved) == 0, "cannot restore caller rounding");
}
}

int main() {
    Exhaustive({7, 5, 1.0F});
    Exhaustive({3, 2, 1.0e-20F});
    Exhaustive({3, 2, 1.0e20F});
    PhysicalExtremes();
    RoundingChanges();
}
