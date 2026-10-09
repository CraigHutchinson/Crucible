#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include <crucible/contracts/GridConfig.hpp>
#include <crucible/contracts/SampleState.hpp>
#include <crucible/contracts/SteeringSettings.hpp>

namespace crucible::fields { class FieldSet; }
namespace crucible::spatial { class Grid; }

namespace crucible::swarm {
/** Computes bounded separation and radial steering from one immutable tick-start state.
 * Simulation owns this object and its reusable output scratch. TryCompute requires exclusive
 * access; caller-scratch row computation shares immutable input/settings/index only.
 * No input, field or grid borrow persists after a call.
 * The numerical rule is specified in docs/workstreams/swarm/design.md.
 */
class Steering {
public:
    /// Validates geometry/settings and allocates fixed output scratch; invalid startup values throw.
    Steering(GridConfig config, SteeringSettings settings, std::size_t sample_capacity);

    /** Writes the input-sized prefix on success, leaving any output tail untouched.
     * Input must be strictly ID-sorted, finite and inside this object's closed world bounds.
     * The caller must rebuild a matching grid from exactly these tick-start positions first.
     * Input/output may overlap: all computation finishes in owned scratch before publication.
     * @return False for insufficient capacity, invalid input or inconsistent query membership;
     * the whole destination remains unchanged on failure. No storage grows during this call.
     */
    [[nodiscard]] bool TryCompute(std::span<const SampleState> input,
                                  const fields::FieldSet& fields, spatial::Grid& grid,
                                  std::span<SampleState> output) noexcept;

    /** Computes one staged row range while retaining complete neighbor input.
     * @param fullInput Complete strictly ID-sorted tick-start samples, finite and in bounds.
     * @param fields Immutable field values for the same tick-start epoch.
     * @param constGrid Immutable index rebuilt from exactly fullInput positions/IDs.
     * @param firstRow First input row to compute; pendingRows.size() is the range length.
     * @param pendingRows Caller-owned staging, disjoint from fullInput and all active outputs.
     * @param queryScratch Caller-owned query storage for every committed index sample,
     * disjoint from input, outputs, index and other active queries.
     * @return True on complete range computation. Invalid range/input, overlap or insufficient
     * storage rejects before writes. A query/membership failure may leave staging partial;
     * the coordinator must discard all pending ranges if any task fails.
     * @note No allocation or owned scratch mutation. Concurrent calls require immutable
     * input/fields/index and distinct output/query storage, joined before publication,
     * mutation or destruction. All borrows end when this call returns.
     */
    [[nodiscard]] bool tryComputeRows(std::span<const SampleState> fullInput,
        const fields::FieldSet& fields, const spatial::Grid& constGrid, std::size_t firstRow,
        std::span<SampleState> pendingRows, std::span<SampleId> queryScratch) const noexcept;

private:
    GridExtent m_Extent;
    SteeringSettings m_Settings;
    std::vector<SampleState> m_Output;
};
}
