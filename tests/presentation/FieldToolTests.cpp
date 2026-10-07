#include <crucible/presentation/FieldTool.hpp>

#include <array>
#include <iostream>
#include <limits>

namespace {
using crucible::FieldEditKind;
using crucible::presentation::FieldTool;
using crucible::presentation::FieldToolSettings;
using crucible::presentation::TryBuildFieldEdit;
#define CHECK(expression) do { if (!(expression)) { std::cerr << #expression << '\n'; return false; } } while (false)

bool Fixtures() {
    for (const auto tool : {FieldTool::attract, FieldTool::repel}) {
        const auto edit = TryBuildFieldEdit({tool, 2, 8, 4}, {12, 6}, 4);
        CHECK(edit && edit->kind == FieldEditKind::set && edit->slot == 2 &&
            edit->center.x == 12 && edit->center.y == 6 && edit->radius == 8 &&
            edit->strength == (tool == FieldTool::attract ? 4 : -4) && edit->IsValid(4));
    }
    const auto nan = std::numeric_limits<float>::quiet_NaN();
    const auto infinity = std::numeric_limits<float>::infinity();
    const auto remove = TryBuildFieldEdit({FieldTool::remove, 3, nan, infinity}, {nan, infinity}, 4);
    CHECK(remove && remove->kind == FieldEditKind::remove && remove->slot == 3 &&
        remove->center.x == 0 && remove->center.y == 0 && remove->radius == 0 && remove->strength == 0);
    CHECK(!TryBuildFieldEdit({FieldTool::remove, 4, 8, 4}, {}, 4));
    CHECK(!TryBuildFieldEdit({}, {}, 0));
    CHECK(!TryBuildFieldEdit({static_cast<FieldTool>(99), 0, 8, 4}, {}, 4));
    for (const auto tool : {FieldTool::attract, FieldTool::repel}) {
        for (const float value : {0.0F, -1.0F, infinity, nan}) {
            CHECK(!TryBuildFieldEdit({tool, 0, value, 4}, {}, 4));
            CHECK(!TryBuildFieldEdit({tool, 0, 8, value}, {}, 4));
        }
        CHECK(!TryBuildFieldEdit({tool, 4, 8, 4}, {}, 4));
        CHECK(!TryBuildFieldEdit({tool, 0, 8, 4}, {nan, 0}, 4));
        CHECK(!TryBuildFieldEdit({tool, 0, 8, 4}, {0, infinity}, 4));
        const auto extreme = TryBuildFieldEdit({tool, 0, std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max()}, {}, 4);
        CHECK(extreme && extreme->IsValid(4));
    }
    const auto flow = TryBuildFieldEdit({FieldTool::flow, 2, 8, 4}, {2, 3}, 4, {10, 3});
    CHECK(flow && flow->kind == FieldEditKind::set_flow && flow->center.x == 2 && flow->end.x == 10 &&
          flow->radius == 8 && flow->strength == 4 && flow->slot == 2);
    CHECK(!TryBuildFieldEdit({FieldTool::flow, 0, 8, 4}, {2, 3}, 4, {2, 3}));
    CHECK(!TryBuildFieldEdit({FieldTool::flow, 0, 8, 4}, {2, 3}, 4, {nan, 3}));
    CHECK(!TryBuildFieldEdit({FieldTool::flow, 0, 8, 4}, {2, 3}, 4, {2, infinity}));
    for (const float value : {0.F, -1.F, nan, infinity}) {
        CHECK(!TryBuildFieldEdit({FieldTool::flow, 0, value, 4}, {2, 3}, 4, {10, 3}));
        CHECK(!TryBuildFieldEdit({FieldTool::flow, 0, 8, value}, {2, 3}, 4, {10, 3}));
    }
    const auto radial = TryBuildFieldEdit({FieldTool::attract, 0, 8, 4}, {2, 3}, 4, {nan, infinity});
    CHECK(radial && radial->end.x == 0 && radial->end.y == 0);
    const FieldToolSettings defaults{};
    CHECK(defaults.tool == FieldTool::attract && defaults.slot == 0 && defaults.radius == 8 && defaults.magnitude == 4);
    return true;
}
}
int main() {
    if (!Fixtures()) return 1;
    std::cout << "Field tools produce valid signed and canonical owned edits\n";
}
