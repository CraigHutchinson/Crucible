#pragma once

#include <crucible/contracts/FieldEdit.hpp>
#include <crucible/contracts/Position.hpp>
#include <cstddef>
#include <vector>
#include <span>

namespace crucible::fields {
/// Acceleration in world units per second squared, integrated by Simulation using tick duration.
struct Acceleration { float x{}, y{}; };

/// Boundary application outcome; rejection preserves all slots.
enum class EditResult { applied, invalid, slot_out_of_range };

/** Fixed radial/straight-flow slots; commands and samples remain owned boundary values.
 * Simulation owns the object; edits and sampling require external synchronization.
 */
class FieldSet {
public:
    /// Allocates the fixed slot capacity once; allocation or oversized capacity can throw.
    explicit FieldSet(std::size_t capacity);

    /** Replaces one slot or clears it idempotently without growing storage.
     * @param[in] edit Command to copy; no reference is retained.
     * @return Applied, invalid payload, or unavailable slot; rejection preserves all slots.
     */
    [[nodiscard]] EditResult TryApplyEdit(const FieldEdit& edit) noexcept;

    /** Samples radial/capsule linear falloff, saturating stable slot sums to finite float range.
     * Radial coincidence and zero radius/strength contribute zero; flow points along its segment.
     * Nonfinite positions return zero; sampling allocates no storage.
     * @param[in] position World position at which to evaluate all occupied slots.
     * @return Summed acceleration; inactive and outside contributions are zero.
     */
    [[nodiscard]] Acceleration Sample(Position position) const noexcept;

    /** Copies slots in index order as active payloads or canonical remove commands.
     * @param[out] destination Caller-owned storage; excess elements remain unchanged.
     * @return True after copying all slots; false leaves insufficient storage unchanged.
     */
    [[nodiscard]] bool TryCopyEdits(std::span<FieldEdit> destination) const noexcept;
private:
    /// An occupied owned slot retains the complete active geometry.
    struct Slot {
        bool occupied{};
        FieldEditKind kind{FieldEditKind::set};
        Position center{};
        float radius{}, strength{};
        Position end{};
    };
    std::vector<Slot> m_Slots;
};
}
