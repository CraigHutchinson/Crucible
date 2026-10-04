#pragma once

#include <crucible/contracts/Position.hpp>
#include <cstddef>
#include <type_traits>

namespace crucible {
/// Boundary edit operation; set/set_flow upsert a slot and remove is idempotent.
enum class FieldEditKind { set, remove, set_flow };

/// Owned bounded-slot command; radial strength is signed, flow strength is nonnegative.
struct FieldEdit {
    FieldEditKind kind{FieldEditKind::set};
    std::size_t slot{};
    Position center{};
    float radius{}, strength{};
    Position end{}; ///< Flow endpoint; radial set and remove ignore this payload.

    /** Checks the slot and active payload; flow endpoints must be finite and distinct.
     * @param[in] field_capacity Number of available slots; zero rejects every edit.
     * @return True when the edit addresses a slot and its active payload is valid.
     */
    [[nodiscard]] bool IsValid(std::size_t field_capacity) const noexcept;
};
static_assert(std::is_trivially_copyable_v<FieldEdit>);
}
