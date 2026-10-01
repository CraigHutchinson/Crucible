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
 * Simulation owns this object and its reusable output scratch. Calls require exclusive access
 * to this object and the supplied grid. No input, field or grid borrow persists after a call.
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

private:
    GridExtent m_Extent;
    SteeringSettings m_Settings;
    std::vector<SampleState> m_Output;
};
}
