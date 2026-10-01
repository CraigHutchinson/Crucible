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

/** Fixed radial field slots; see docs/workstreams/fields/design.md for the prototype falloff rule.
 * Simulation owns the object; edits and sampling require external synchronization.
 */
class FieldSet {
public:
    /// Allocates the fixed slot capacity once; allocation or oversized capacity can throw.
    explicit FieldSet(std::size_t capacity);

    /// Set upserts a slot, remove clears it idempotently; neither grows storage.
    [[nodiscard]] EditResult TryApplyEdit(const FieldEdit& edit) noexcept;

    /** Samples signed linear radial falloff, saturating component sums to finite float range.
     * Coincidence and zero-radius fields contribute zero. Nonfinite positions return zero.
     */
    [[nodiscard]] Acceleration Sample(Position position) const noexcept;

    /// Copies slots in index order: active set payloads or canonical remove commands.
    /// Insufficient capacity leaves destination unchanged; storage remains caller-owned.
    [[nodiscard]] bool TryCopyEdits(std::span<FieldEdit> destination) const noexcept;
private:
    /// An occupied owned slot retains only the set payload.
    struct Slot { bool occupied{}; Position center{}; float radius{}, strength{}; };
    std::vector<Slot> m_Slots;
};
}
