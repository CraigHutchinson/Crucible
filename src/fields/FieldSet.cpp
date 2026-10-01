#include <crucible/fields/FieldSet.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace crucible::fields {
FieldSet::FieldSet(std::size_t capacity) : m_Slots(capacity) {}

EditResult FieldSet::TryApplyEdit(const FieldEdit& edit) noexcept {
    if (edit.slot >= m_Slots.size()) return EditResult::slot_out_of_range;
    if (!edit.IsValid(m_Slots.size())) return EditResult::invalid;
    if (edit.kind == FieldEditKind::remove) m_Slots[edit.slot] = {};
    else m_Slots[edit.slot] = {true, edit.center, edit.radius, edit.strength};
    return EditResult::applied;
}

Acceleration FieldSet::Sample(Position position) const noexcept {
    if (!std::isfinite(position.x) || !std::isfinite(position.y)) return {};
    double x = 0.0, y = 0.0;
    for (const auto& slot : m_Slots) {
        if (!slot.occupied || slot.radius == 0.0F) continue;
        const double dx = static_cast<double>(slot.center.x) - position.x;
        const double dy = static_cast<double>(slot.center.y) - position.y;
        const double distance = std::hypot(dx, dy);
        if (distance == 0.0 || distance >= slot.radius) continue;
        const double magnitude = slot.strength * (1.0 - distance / slot.radius);
        x += magnitude * (dx / distance);
        y += magnitude * (dy / distance);
    }
    const double limit = std::numeric_limits<float>::max();
    return {static_cast<float>(std::clamp(x, -limit, limit)),
            static_cast<float>(std::clamp(y, -limit, limit))};
}
}
