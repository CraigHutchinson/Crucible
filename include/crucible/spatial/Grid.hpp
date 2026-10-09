#pragma once

#include <crucible/contracts/GridConfig.hpp>
#include <crucible/contracts/SampleId.hpp>
#include <crucible/contracts/Position.hpp>
#include <cstddef>
#include <optional>
#include <span>
#include <vector>

#include <sub0hexgrid/PointyLayout.hpp>
#include <sub0hexgrid/regions/AxialRegion.hpp>

namespace crucible::spatial {
/// Owned stable identity and position gathered by Simulation at a tick boundary.
struct SpatialSample { SampleId id{}; Position position{}; };

/** Bounded spatial bins with complete, ID-sorted radius results.
 * Simulation owns this object. Rebuild and owned-scratch queries require exclusive access.
 * Caller-scratch queries may share an immutable index; no input borrows persist.
 */
class Grid {
public:
    /** Validates physical geometry/capacity and allocates all scratch at startup.
     * @param[in] config Closed rectangular physical world and bin scale.
     * @param[in] sample_capacity Maximum committed samples and complete query results.
     * @throws std::invalid_argument Invalid geometry.
     * @throws std::length_error Unrepresentable storage; allocation can also fail.
     */
    Grid(GridConfig config, std::size_t sample_capacity);

    /** Replaces bins after validating and clamping every sample to the physical world.
     * @param[in] samples Borrowed only during this call; arbitrary identity order is accepted.
     * @return False for excess capacity, nonfinite positions or duplicate IDs, preserving committed state.
     */
    [[nodiscard]] bool TryRebuild(std::span<const SpatialSample> samples) noexcept;

    /** Returns complete ascending IDs within inclusive radius of the clamped center.
     * Nonfinite center/radius or negative radius returns nullopt. Radius zero includes coincidence.
     * The owned result span expires on the next query or rebuild; consume it immediately.
     * @param[in] center Finite world position, clamped consistently with samples.
     * @param[in] radius Finite nonnegative world radius; every such float value is supported.
     * @return Complete IDs in owned scratch, or nullopt for invalid query input.
     */
    [[nodiscard]] std::optional<std::span<const SampleId>> TryQuery(Position center, float radius) noexcept;

    /** Queries an immutable index into caller-owned complete-result storage.
     * @param center Finite physical position, clamped to the closed world bounds.
     * @param radius Finite nonnegative inclusive radius; zero includes coincidence.
     * @param callerScratch Storage for every committed sample, disjoint from index storage
     * and other concurrent queries' scratch. A larger unused tail remains unchanged.
     * @return Complete ascending IDs in callerScratch, or nullopt on invalid input or
     * insufficient storage, with callerScratch unchanged on rejection.
     * @note Concurrent calls require an immutable index and distinct scratch. Rebuild,
     * destruction and owned-scratch calls must not overlap them. The result borrows
     * callerScratch until it is overwritten or destroyed; this call retains nothing.
     */
    [[nodiscard]] std::optional<std::span<const SampleId>> tryQuery(
        Position center, float radius, std::span<SampleId> callerScratch) const noexcept;

    /// Number of nonempty cells in the latest committed rebuild.
    [[nodiscard]] constexpr std::size_t GetOccupiedCellCount() const noexcept { return m_OccupiedCells; }

private:
    [[nodiscard]] Position ClampPosition(Position position) const noexcept;
    [[nodiscard]] std::size_t RectangularCellIndex(Position position) const noexcept;
    [[nodiscard]] std::optional<std::size_t> TryHexCellIndex(Position position) const noexcept;
    void PrepareCellIndices(std::span<const SpatialSample> pending) noexcept;
    [[nodiscard]] std::size_t AxisCell(double coordinate, std::size_t cells) const noexcept;

    GridConfig m_Config;
    GridExtent m_Extent;
    std::optional<sub0hexgrid::PointyLayout> m_Layout;
    std::optional<sub0hexgrid::AxialRegion> m_Region;
    std::size_t m_CellCount{};
    bool m_HexBins{};
    std::vector<SpatialSample> m_Samples;
    std::vector<SpatialSample> m_Pending;
    std::vector<std::size_t> m_SampleCells;
    std::vector<std::size_t> m_Counts;
    std::vector<std::size_t> m_Offsets;
    std::vector<std::size_t> m_Cursors;
    std::vector<std::size_t> m_Bins;
    std::vector<SampleId> m_Results;
    std::size_t m_SampleCount{};
    std::size_t m_OccupiedCells{};
};
}
