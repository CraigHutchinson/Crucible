#include <crucible/spatial/Grid.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace crucible::spatial {
Grid::Grid(GridConfig config, std::size_t sample_capacity) : m_Config(config) {
    const auto index_limit = std::vector<std::size_t>{}.max_size();
    const auto extent = config.TryValidate();
    if (!extent || extent->cells >= index_limit) throw std::invalid_argument("Invalid spatial grid geometry");
    if (sample_capacity > std::vector<SpatialSample>{}.max_size() ||
        sample_capacity > std::vector<SampleId>{}.max_size() || sample_capacity > index_limit)
        throw std::length_error("Spatial sample capacity exceeds storage limits");
    m_Extent = *extent;
    m_Samples.resize(sample_capacity);
    m_Pending.resize(sample_capacity);
    m_Counts.resize(m_Extent.cells);
    m_Offsets.resize(m_Extent.cells + 1);
    m_Cursors.resize(m_Extent.cells);
    m_Bins.resize(sample_capacity);
    m_Results.resize(sample_capacity);
}

Position Grid::ClampPosition(Position position) const noexcept {
    return {std::clamp(position.x, 0.0F, m_Extent.width),
            std::clamp(position.y, 0.0F, m_Extent.height)};
}

std::size_t Grid::AxisCell(double coordinate, std::size_t cells) const noexcept {
    // Test before conversion: a rounded large dimension must never produce an out-of-range cast.
    const double cell = std::floor(std::max(0.0, coordinate) / m_Config.cell_size);
    if (cell >= static_cast<double>(cells - 1)) return cells - 1;
    return static_cast<std::size_t>(cell);
}

std::size_t Grid::CellIndex(Position position) const noexcept {
    return AxisCell(position.y, m_Config.rows) * m_Config.columns + AxisCell(position.x, m_Config.columns);
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
    m_Samples.swap(m_Pending);
    m_SampleCount = samples.size();
    std::ranges::fill(m_Counts, 0);
    for (const auto& sample : std::span{m_Samples}.first(m_SampleCount)) ++m_Counts[CellIndex(sample.position)];
    m_Offsets[0] = 0;
    m_OccupiedCells = 0;
    for (std::size_t cell = 0; cell < m_Extent.cells; ++cell) {
        m_Offsets[cell + 1] = m_Offsets[cell] + m_Counts[cell];
        m_Cursors[cell] = m_Offsets[cell];
        if (m_Counts[cell] != 0) ++m_OccupiedCells;
    }
    for (std::size_t sample = 0; sample < m_SampleCount; ++sample)
        m_Bins[m_Cursors[CellIndex(m_Samples[sample].position)]++] = sample;
    return true;
}

std::optional<std::span<const SampleId>> Grid::TryQuery(Position center, float radius) noexcept {
    if (!std::isfinite(center.x) || !std::isfinite(center.y) || !std::isfinite(radius) || radius < 0.0F)
        return std::nullopt;
    center = ClampPosition(center);
    const double distance = radius;
    const auto min_x = AxisCell(static_cast<double>(center.x) - distance, m_Config.columns);
    const auto max_x = AxisCell(static_cast<double>(center.x) + distance, m_Config.columns);
    const auto min_y = AxisCell(static_cast<double>(center.y) - distance, m_Config.rows);
    const auto max_y = AxisCell(static_cast<double>(center.y) + distance, m_Config.rows);
    std::size_t result_count = 0;
    for (std::size_t y = min_y; y <= max_y; ++y) {
        for (std::size_t x = min_x; x <= max_x; ++x) {
            const auto cell = y * m_Config.columns + x;
            for (auto bin = m_Offsets[cell]; bin < m_Offsets[cell + 1]; ++bin) {
                const auto& sample = m_Samples[m_Bins[bin]];
                const double dx = static_cast<double>(sample.position.x) - center.x;
                const double dy = static_cast<double>(sample.position.y) - center.y;
                if (dx * dx + dy * dy <= distance * distance) m_Results[result_count++] = sample.id;
            }
        }
    }
    auto results = std::span{m_Results}.first(result_count);
    std::ranges::sort(results);
    return std::span<const SampleId>{results};
}


}
