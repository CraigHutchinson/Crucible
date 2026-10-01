#pragma once

#include <crucible/contracts/GridConfig.hpp>
#include <crucible/contracts/SampleId.hpp>
#include <crucible/contracts/Position.hpp>
#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace crucible::spatial {
/// Owned stable identity and position gathered by Simulation at a tick boundary.
struct SpatialSample { SampleId id{}; Position position{}; };

/** Rectangular bins with complete, ID-sorted radius results; see docs/workstreams/spatial/design.md.
 * Simulation owns this object. Rebuild and query require exclusive access; no input borrows persist.
 */
class Grid {
public:
    /// Validates geometry/capacity and allocates all scratch; invalid startup input throws.
    Grid(GridConfig config, std::size_t sample_capacity);

    /// Rejects excess capacity, nonfinite positions or duplicate IDs without changing committed bins.
    [[nodiscard]] bool TryRebuild(std::span<const SpatialSample> samples) noexcept;

    /** Returns complete ascending IDs within inclusive radius of the clamped center.
     * Nonfinite center/radius or negative radius returns nullopt. Radius zero includes coincidence.
     * The owned result span expires on the next query or rebuild; consume it immediately.
     */
    [[nodiscard]] std::optional<std::span<const SampleId>> TryQuery(Position center, float radius) noexcept;

    /// Number of nonempty cells in the latest committed rebuild.
    [[nodiscard]] constexpr std::size_t GetOccupiedCellCount() const noexcept { return m_OccupiedCells; }

private:
    [[nodiscard]] Position ClampPosition(Position position) const noexcept;
    [[nodiscard]] std::size_t CellIndex(Position position) const noexcept;
    [[nodiscard]] std::size_t AxisCell(double coordinate, std::size_t cells) const noexcept;

    GridConfig m_Config;
    GridExtent m_Extent;
    std::vector<SpatialSample> m_Samples;
    std::vector<SpatialSample> m_Pending;
    std::vector<std::size_t> m_Counts;
    std::vector<std::size_t> m_Offsets;
    std::vector<std::size_t> m_Cursors;
    std::vector<std::size_t> m_Bins;
    std::vector<SampleId> m_Results;
    std::size_t m_SampleCount{};
    std::size_t m_OccupiedCells{};
};
}
