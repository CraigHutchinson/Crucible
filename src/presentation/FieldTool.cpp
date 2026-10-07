#include <crucible/presentation/FieldTool.hpp>

#include <cmath>

namespace crucible::presentation {
std::optional<FieldEdit> TryBuildFieldEdit(
    FieldToolSettings settings, Position center, std::size_t field_capacity, Position end) noexcept {
    FieldEdit edit{FieldEditKind::remove, settings.slot};
    switch (settings.tool) {
    case FieldTool::attract:
    case FieldTool::repel:
    case FieldTool::flow:
        if (!std::isfinite(settings.radius) || !std::isfinite(settings.magnitude) ||
            settings.radius <= 0 || settings.magnitude <= 0) return std::nullopt;
        edit = {settings.tool == FieldTool::flow ? FieldEditKind::set_flow : FieldEditKind::set,
            settings.slot, center, settings.radius,
            settings.tool == FieldTool::repel ? -settings.magnitude : settings.magnitude,
            settings.tool == FieldTool::flow ? end : Position{}};
        break;
    case FieldTool::remove: break;
    default: return std::nullopt;
    }
    return edit.IsValid(field_capacity) ? std::optional{edit} : std::nullopt;
}
}
