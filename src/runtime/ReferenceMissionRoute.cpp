#include <crucible/runtime/ReferenceMissionRoute.hpp>

namespace crucible::runtime {
std::optional<FieldEdit> GetReferenceMissionRouteEdit(std::uint64_t completed_tick) noexcept {
    if (completed_tick % 60 != 0) return std::nullopt;
    return FieldEdit{FieldEditKind::set, 0,
        {static_cast<float>(8 + 16 * ((completed_tick / 60) % 4)),
         static_cast<float>(8 + 16 * ((completed_tick / 240) % 2))}, 8, 4};
}
}
