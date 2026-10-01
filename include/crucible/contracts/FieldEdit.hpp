#pragma once

#include <crucible/contracts/Position.hpp>
#include <cstddef>
#include <type_traits>

namespace crucible {
/// Boundary edit operation; set upserts a slot and remove is idempotent.
enum class FieldEditKind { set, remove };

/// Owned command for a bounded field slot; positive strength attracts, negative repels.
struct FieldEdit {
    FieldEditKind kind{FieldEditKind::set};
    std::size_t slot{};
    Position center{};
    float radius{}, strength{};

    /// Checks slot capacity, operation and set payload. Remove ignores geometric payload.
    [[nodiscard]] bool IsValid(std::size_t field_capacity) const noexcept;
};
static_assert(std::is_trivially_copyable_v<FieldEdit>);
}
