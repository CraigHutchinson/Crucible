#include <crucible/contracts/FieldEdit.hpp>
#include <crucible/contracts/GridConfig.hpp>
#include <crucible/contracts/SampleId.hpp>

#include <initializer_list>
#include <limits>

static_assert(static_cast<int>(crucible::FieldEditKind::set) == 0);
static_assert(static_cast<int>(crucible::FieldEditKind::remove) == 1);
static_assert(static_cast<int>(crucible::FieldEditKind::set_flow) == 2);

int main() {
    using namespace crucible;
    const GridConfig grid{3, 2, 4.0F};
    const auto extent = grid.TryValidate();
    if (!extent || extent->cells != 6 || extent->width != 12.0F || extent->height != 8.0F) return 1;
    if (GridConfig{0, 2, 1}.TryValidate() ||
        GridConfig{2, 0, 1}.TryValidate() || GridConfig{2, 2, 0}.TryValidate() ||
        GridConfig{2, 2, -1}.TryValidate()) return 2;
    const auto maximum = std::numeric_limits<std::size_t>::max();
    const auto infinity = std::numeric_limits<float>::infinity();
    const auto nan = std::numeric_limits<float>::quiet_NaN();
    if (GridConfig{maximum, 2, 1}.TryValidate() ||
        GridConfig{2, 2, infinity}.TryValidate() || GridConfig{2, 2, nan}.TryValidate() ||
        GridConfig{2, 2, std::numeric_limits<float>::max()}.TryValidate()) return 3;
    FieldEdit edit{FieldEditKind::set, 1, {2, 3}, 0, -2};
    if (!edit.IsValid(2)) return 4;
    if (edit.IsValid(1) || edit.IsValid(0)) return 11;
    const auto copied = edit;
    edit.center.x = nan;
    if (edit.IsValid(2) || !copied.IsValid(2) || copied.center.x != 2) return 5;
    edit = copied; edit.radius = -1;
    if (edit.IsValid(2)) return 6;
    edit = copied; edit.strength = infinity;
    if (edit.IsValid(2)) return 7;
    edit = copied; edit.kind = static_cast<FieldEditKind>(99);
    if (edit.IsValid(2)) return 8;
    edit.kind = FieldEditKind::remove;
    if (!edit.IsValid(2) || edit.IsValid(1)) return 9;
    if (!(SampleId{1} < SampleId{2}) || SampleId{1} != SampleId{1}) return 10;
    FieldEdit flow{FieldEditKind::set_flow, 1, {2, 3}, 4, 8, {10, 3}};
    if (!flow.IsValid(2) || flow.IsValid(1)) return 12;
    const auto owned_flow = flow;
    flow.end.x = nan;
    if (flow.IsValid(2) || owned_flow.end.x != 10) return 13;
    for (const auto invalid : {
             FieldEdit{FieldEditKind::set_flow, 1, {nan, 3}, 4, 8, {10, 3}},
             FieldEdit{FieldEditKind::set_flow, 1, {2, infinity}, 4, 8, {10, 3}},
             FieldEdit{FieldEditKind::set_flow, 1, {2, 3}, 4, 8, {nan, 3}},
             FieldEdit{FieldEditKind::set_flow, 1, {2, 3}, 4, 8, {10, infinity}},
             FieldEdit{FieldEditKind::set_flow, 1, {2, 3}, 4, 8, {2, 3}},
             FieldEdit{FieldEditKind::set_flow, 1, {2, 3}, -1, 8, {10, 3}},
             FieldEdit{FieldEditKind::set_flow, 1, {2, 3}, infinity, 8, {10, 3}},
             FieldEdit{FieldEditKind::set_flow, 1, {2, 3}, 4, -1, {10, 3}},
             FieldEdit{FieldEditKind::set_flow, 1, {2, 3}, 4, nan, {10, 3}}})
        if (invalid.IsValid(2)) return 14;
    flow = owned_flow; flow.radius = 0; flow.strength = 0;
    if (!flow.IsValid(2)) return 15;
    edit = copied; edit.end = {nan, infinity};
    if (!edit.IsValid(2)) return 16;
    edit.kind = FieldEditKind::remove;
    if (!edit.IsValid(2)) return 17;
    return 0;
}
