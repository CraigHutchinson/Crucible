#include <crucible/contracts/FieldEdit.hpp>

#include <cmath>

namespace crucible {
bool FieldEdit::IsValid(std::size_t field_capacity) const noexcept {
    if (slot >= field_capacity) return false;
    switch (kind) {
    case FieldEditKind::remove: return true;
    case FieldEditKind::set:
        return std::isfinite(center.x) && std::isfinite(center.y) &&
            std::isfinite(radius) && radius >= 0.0F && std::isfinite(strength);
    }
    return false;
}
}
