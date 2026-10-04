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
    else m_Slots[edit.slot] = {true, edit.kind, edit.center, edit.radius, edit.strength,
                             edit.kind == FieldEditKind::set_flow ? edit.end : Position{}};
    return EditResult::applied;
}

Acceleration FieldSet::Sample(Position position) const noexcept {
    if (!std::isfinite(position.x) || !std::isfinite(position.y)) return {};
    double x = 0.0, y = 0.0;
    for (const auto& slot : m_Slots) {
        if (!slot.occupied || slot.radius == 0.0F || slot.strength == 0.0F) continue;
        if (slot.kind == FieldEditKind::set_flow) {
            const double dx = static_cast<double>(slot.end.x) - slot.center.x;
            const double dy = static_cast<double>(slot.end.y) - slot.center.y;
            const double px = static_cast<double>(position.x) - slot.center.x;
            const double py = static_cast<double>(position.y) - slot.center.y;
            const double length = std::hypot(dx, dy);
            const double along = std::clamp((px * dx + py * dy) / (dx * dx + dy * dy), 0.0, 1.0);
            const double distance = std::hypot(px - along * dx, py - along * dy);
            if (distance >= slot.radius) continue;
            const double magnitude = slot.strength * (1.0 - distance / slot.radius);
            x += magnitude * (dx / length);
            y += magnitude * (dy / length);
            continue;
        }
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

namespace crucible::fields {
bool FieldSet::TryCopyEdits(std::span<FieldEdit> destination) const noexcept {
    if (destination.size() < m_Slots.size()) return false;
    for (std::size_t i = 0; i < m_Slots.size(); ++i) {
        const auto& slot = m_Slots[i];
        destination[i] = slot.occupied
            ? FieldEdit{slot.kind, i, slot.center, slot.radius, slot.strength, slot.end}
            : FieldEdit{FieldEditKind::remove, i};
    }
    return true;
}
}
