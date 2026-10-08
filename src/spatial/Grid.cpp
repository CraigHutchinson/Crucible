#include <crucible/spatial/Grid.hpp>

#include <algorithm>
#include <array>
#include <cfenv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <stdexcept>

#include <sub0hexgrid/candidates/CandidateCells.hpp>

namespace crucible::spatial {
namespace {
/** Reuses H2's outward rounding envelope at the affine extrema of the physical rectangle.
 * The checked broad region is metadata only; no allocation or cell scan is performed.
 */
std::optional<sub0hexgrid::AxialRegion> TryCoveringRegion(
    sub0hexgrid::PointyLayout layout, GridExtent extent) noexcept {
    constexpr auto limit = std::numeric_limits<std::int32_t>::max();
    const auto broad = sub0hexgrid::AxialRegion::TryCreate({-limit, -limit}, {limit, limit});
    sub0hexgrid::Axial minimum{limit, limit}, maximum{-limit, -limit};
    for (const auto corner : std::array{sub0hexgrid::Point{0, 0},
             sub0hexgrid::Point{extent.width, 0}, sub0hexgrid::Point{0, extent.height},
             sub0hexgrid::Point{extent.width, extent.height}}) {
        const auto candidates = sub0hexgrid::CandidateCells::TryCreate(layout, *broad, corner, 0);
        if (!candidates || candidates->GetCellCount() == 0) return std::nullopt;
        const auto first = *candidates->begin();
        const auto last = *candidates->TrySlice(candidates->GetCellCount() - 1, 1)->begin();
        // Clipping at the broad edge could conceal unrepresentable physical mappings.
        if (first.q == -limit || first.r == -limit || last.q == limit || last.r == limit)
            return std::nullopt;
        minimum.q = std::min(minimum.q, first.q);
        minimum.r = std::min(minimum.r, first.r);
        maximum.q = std::max(maximum.q, last.q);
        maximum.r = std::max(maximum.r, last.r);
    }
    return sub0hexgrid::AxialRegion::TryCreate(minimum, maximum);
}

bool IsStorageRepresentable(std::size_t cells, std::size_t samples) noexcept {
    constexpr auto maximum = std::numeric_limits<std::size_t>::max();
    constexpr auto per_sample = 2 * sizeof(SpatialSample) + 2 * sizeof(std::size_t) + sizeof(SampleId);
    constexpr auto per_cell = 3 * sizeof(std::size_t);
    if (samples > (maximum - sizeof(std::size_t)) / per_sample) return false;
    const auto sample_bytes = samples * per_sample + sizeof(std::size_t);
    return cells <= (maximum - sample_bytes) / per_cell;
}
}

Grid::Grid(GridConfig config, std::size_t sample_capacity) : m_Config(config) {
    const auto index_limit = std::vector<std::size_t>{}.max_size();
    const auto extent = config.TryValidate();
    if (!extent || extent->cells >= index_limit) throw std::invalid_argument("Invalid spatial grid geometry");
    if (sample_capacity > std::vector<SpatialSample>{}.max_size() ||
        sample_capacity > std::vector<SampleId>{}.max_size() || sample_capacity > index_limit ||
        !IsStorageRepresentable(extent->cells, sample_capacity))
        throw std::length_error("Spatial sample capacity exceeds storage limits");
    m_Extent = *extent;
    m_CellCount = m_Extent.cells;
    if (std::fegetround() == FE_TONEAREST) {
        const auto layout = sub0hexgrid::PointyLayout::TryCreate(config.cell_size / std::numbers::sqrt3);
        const auto region = layout ? TryCoveringRegion(*layout, m_Extent) : std::nullopt;
        constexpr auto maximum = std::numeric_limits<std::size_t>::max();
        const auto cap = m_Extent.cells <= (maximum - 64) / 4 ? m_Extent.cells * 4 + 64 : maximum;
        if (region && region->GetCellCount() < index_limit && region->GetCellCount() <= cap &&
            IsStorageRepresentable(static_cast<std::size_t>(region->GetCellCount()), sample_capacity)) {
            m_Layout = layout;
            m_Region = region;
            m_CellCount = std::max(m_CellCount, static_cast<std::size_t>(region->GetCellCount()));
        }
    }
    m_Samples.resize(sample_capacity);
    m_Pending.resize(sample_capacity);
    m_SampleCells.resize(sample_capacity);
    m_Counts.resize(m_CellCount);
    m_Offsets.resize(m_CellCount + 1);
    m_Cursors.resize(m_CellCount);
    m_Bins.resize(sample_capacity);
    m_Results.resize(sample_capacity);
}

Position Grid::ClampPosition(Position position) const noexcept {
    return {std::clamp(position.x, 0.0F, m_Extent.width),
            std::clamp(position.y, 0.0F, m_Extent.height)};
}

std::size_t Grid::AxisCell(double coordinate, std::size_t cells) const noexcept {
    const double cell = std::floor(std::max(0.0, coordinate) / m_Config.cell_size);
    if (cell >= static_cast<double>(cells - 1)) return cells - 1;
    return static_cast<std::size_t>(cell);
}

std::size_t Grid::RectangularCellIndex(Position position) const noexcept {
    return AxisCell(position.y, m_Config.rows) * m_Config.columns + AxisCell(position.x, m_Config.columns);
}

std::optional<std::size_t> Grid::TryHexCellIndex(Position position) const noexcept {
    const auto cell = m_Layout->TryCellAt({position.x, position.y});
    const auto index = cell ? m_Region->TryIndex(*cell) : std::nullopt;
    if (!index) return std::nullopt;
    return static_cast<std::size_t>(*index);
}

void Grid::PrepareCellIndices(std::span<const SpatialSample> pending) noexcept {
    m_HexBins = m_Layout.has_value() && std::fegetround() == FE_TONEAREST;
    if (m_HexBins) {
        for (std::size_t row = 0; row < pending.size(); ++row) {
            const auto index = TryHexCellIndex(pending[row].position);
            if (!index) { m_HexBins = false; break; }
            m_SampleCells[row] = *index;
        }
    }
    if (!m_HexBins)
        for (std::size_t row = 0; row < pending.size(); ++row)
            m_SampleCells[row] = RectangularCellIndex(pending[row].position);
}

bool Grid::TryRebuild(std::span<const SpatialSample> samples) noexcept {
    if (samples.size() > m_Samples.size()) return false;
    if (!std::ranges::all_of(samples, [](const auto& sample) {
            return std::isfinite(sample.position.x) && std::isfinite(sample.position.y);
        })) return false;
    std::copy(samples.begin(), samples.end(), m_Pending.begin());
    auto pending = std::span{m_Pending}.first(samples.size());
    std::ranges::sort(pending, {}, &SpatialSample::id);
    if (std::ranges::adjacent_find(pending, [](const auto& left, const auto& right) {
            return left.id == right.id;
        }) != pending.end()) return false;
    for (auto& sample : pending) sample.position = ClampPosition(sample.position);
    PrepareCellIndices(pending);
    m_Samples.swap(m_Pending);
    m_SampleCount = samples.size();
    std::ranges::fill(m_Counts, 0);
    for (const auto cell : std::span{m_SampleCells}.first(m_SampleCount)) ++m_Counts[cell];
    m_Offsets[0] = 0;
    m_OccupiedCells = 0;
    for (std::size_t cell = 0; cell < m_CellCount; ++cell) {
        m_Offsets[cell + 1] = m_Offsets[cell] + m_Counts[cell];
        m_Cursors[cell] = m_Offsets[cell];
        if (m_Counts[cell] != 0) ++m_OccupiedCells;
    }
    for (std::size_t sample = 0; sample < m_SampleCount; ++sample)
        m_Bins[m_Cursors[m_SampleCells[sample]]++] = sample;
    return true;
}

std::optional<std::span<const SampleId>> Grid::TryQuery(Position center, float radius) noexcept {
    return tryQuery(center, radius, m_Results);
}

std::optional<std::span<const SampleId>> Grid::tryQuery(
    Position center, float radius, std::span<SampleId> callerScratch) const noexcept
{
    if (!std::isfinite(center.x) || !std::isfinite(center.y) || !std::isfinite(radius) || radius < 0.0F)
        return std::nullopt;
    if (callerScratch.size() < m_SampleCount) return std::nullopt;
    center = ClampPosition(center);
    const double distance = radius;
    std::size_t result_count = 0;
    const auto consider = [&](const SpatialSample& sample) {
        const double dx = static_cast<double>(sample.position.x) - center.x;
        const double dy = static_cast<double>(sample.position.y) - center.y;
        if (dx * dx + dy * dy <= distance * distance) callerScratch[result_count++] = sample.id;
    };
    const auto visit = [&](std::size_t cell) {
        for (auto bin = m_Offsets[cell]; bin < m_Offsets[cell + 1]; ++bin)
            consider(m_Samples[m_Bins[bin]]);
    };
    if (m_HexBins) {
        const auto candidates = std::fegetround() == FE_TONEAREST
            ? sub0hexgrid::CandidateCells::TryCreate(*m_Layout, *m_Region, {center.x, center.y}, distance)
            : std::nullopt;
        if (candidates)
            for (const auto cell : *candidates) visit(static_cast<std::size_t>(*m_Region->TryIndex(cell)));
        else
            for (const auto& sample : std::span{m_Samples}.first(m_SampleCount)) consider(sample);
    } else {
        const auto min_x = AxisCell(static_cast<double>(center.x) - distance, m_Config.columns);
        const auto max_x = AxisCell(static_cast<double>(center.x) + distance, m_Config.columns);
        const auto min_y = AxisCell(static_cast<double>(center.y) - distance, m_Config.rows);
        const auto max_y = AxisCell(static_cast<double>(center.y) + distance, m_Config.rows);
        for (std::size_t y = min_y; y <= max_y; ++y)
            for (std::size_t x = min_x; x <= max_x; ++x) visit(y * m_Config.columns + x);
    }
    auto results = callerScratch.first(result_count);
    std::ranges::sort(results);
    return std::span<const SampleId>{results};
}
}
