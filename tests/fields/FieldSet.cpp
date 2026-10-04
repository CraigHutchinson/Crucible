#include <crucible/fields/FieldSet.hpp>

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}
void CheckSample(const crucible::fields::FieldSet& fields, crucible::Position position,
                 crucible::fields::Acceleration expected) {
    const auto actual = fields.Sample(position);
    Check(std::abs(actual.x - expected.x) < 0.00001F && std::abs(actual.y - expected.y) < 0.00001F,
          "field sample differs from analytic expectation");
}
void FlowFixtures() {
    using namespace crucible;
    using namespace crucible::fields;
    FieldSet fields{2};
    FieldEdit flow{FieldEditKind::set_flow, 0, {2, 3}, 4, 8, {10, 3}};
    Check(fields.TryApplyEdit(flow) == EditResult::applied, "flow upsert");
    flow.end = {99, 99};
    for (const auto point : {Position{6, 3}, Position{2, 3}, Position{10, 3}})
        CheckSample(fields, point, {8, 0});
    for (const auto point : {Position{6, 5}, Position{0, 3}, Position{12, 3}})
        CheckSample(fields, point, {4, 0});
    CheckSample(fields, {6, 7}, {});
    CheckSample(fields, {-3, 3}, {});
    const auto rounded_cap_force = static_cast<float>(8.0 * (1.0 - std::sqrt(8.0) / 4.0));
    CheckSample(fields, {0, 1}, {rounded_cap_force, 0});
    CheckSample(fields, {12, 5}, {rounded_cap_force, 0});
    const auto nan = std::numeric_limits<float>::quiet_NaN();
    const auto infinity = std::numeric_limits<float>::infinity();
    for (const auto invalid : {
             FieldEdit{FieldEditKind::set_flow, 0, {2, 3}, 4, 8, {2, 3}},
             FieldEdit{FieldEditKind::set_flow, 0, {2, 3}, 4, 8, {nan, 3}},
             FieldEdit{FieldEditKind::set_flow, 0, {2, 3}, 4, -8, {10, 3}},
             FieldEdit{FieldEditKind::set_flow, 0, {2, 3}, -4, 8, {10, 3}}}) {
        Check(fields.TryApplyEdit(invalid) == EditResult::invalid, "invalid flow mutation");
        CheckSample(fields, {6, 5}, {4, 0});
    }
    Check(fields.TryApplyEdit({FieldEditKind::set_flow, 2, {2, 3}, 4, 8, {10, 3}}) ==
        EditResult::slot_out_of_range, "flow must not grow slots");
    CheckSample(fields, {nan, 3}, {});
    CheckSample(fields, {6, infinity}, {});
    std::array<FieldEdit, 2> observed{};
    Check(fields.TryCopyEdits(observed) && observed[0].kind == FieldEditKind::set_flow &&
        observed[0].end.x == 10 && observed[0].end.y == 3 &&
        observed[1].kind == FieldEditKind::remove, "owned flow observation");
    const auto retained = observed;
    std::array too_small{FieldEdit{FieldEditKind::set, 99, {99, 99}, 99, 99, {99, 99}}};
    Check(!fields.TryCopyEdits(too_small) && too_small[0].slot == 99 && too_small[0].end.x == 99,
        "insufficient flow observation unchanged");
    Check(fields.TryApplyEdit({FieldEditKind::set_flow, 0, {10, 3}, 4, 8, {2, 3}}) == EditResult::applied,
        "reverse flow");
    CheckSample(fields, {6, 5}, {-4, 0});
    Check(retained[0].end.x == 10, "retained flow observation unchanged");
    Check(fields.TryApplyEdit({FieldEditKind::set_flow, 1, {2, 3}, 4, 8, {10, 3}}) == EditResult::applied,
        "opposing overlap");
    CheckSample(fields, {6, 5}, {});
    Check(fields.TryApplyEdit({FieldEditKind::remove, 1}) == EditResult::applied, "clear overlap companion");
    Check(fields.TryApplyEdit({FieldEditKind::set_flow, 0, {2, 3}, 4, 8, {2, 11}}) == EditResult::applied,
        "vertical flow");
    CheckSample(fields, {4, 7}, {0, 4});
    Check(fields.TryApplyEdit({FieldEditKind::set_flow, 0, {0, 0}, 4, 10, {6, 8}}) == EditResult::applied,
        "diagonal flow");
    CheckSample(fields, {3, 4}, {6, 8});
    CheckSample(fields, {1.4F, 5.2F}, {3, 4});
    Check(fields.TryApplyEdit({FieldEditKind::set_flow, 0, {2, 3}, 4, 8, {10, 3}}) == EditResult::applied &&
        fields.TryApplyEdit({FieldEditKind::set, 1, {6, 7}, 4, 8}) == EditResult::applied, "mixed flow/radial slots");
    CheckSample(fields, {6, 5}, {4, 4});
    for (const auto edit : {FieldEdit{FieldEditKind::set_flow, 0, {2, 3}, 0, 8, {10, 3}},
                           FieldEdit{FieldEditKind::set_flow, 0, {2, 3}, 4, 0, {10, 3}}}) {
        Check(fields.TryApplyEdit(edit) == EditResult::applied, "inactive flow accepted");
        CheckSample(fields, {6, 5}, {0, 4});
    }
    Check(fields.TryApplyEdit({FieldEditKind::set, 0, {2, 3}, 4, -8, {nan, infinity}}) == EditResult::applied &&
        fields.TryCopyEdits(observed) && observed[0].end.x == 0 && observed[0].end.y == 0,
        "radial replacement canonical endpoint");
    Check(fields.TryApplyEdit({FieldEditKind::remove, 0, {nan, nan}, nan, nan, {nan, nan}}) == EditResult::applied &&
        fields.TryCopyEdits(observed) && observed[0].kind == FieldEditKind::remove && observed[0].end.x == 0 &&
        observed[0].end.y == 0, "remove canonical endpoint");
    Check(fields.TryApplyEdit({FieldEditKind::remove, 0}) == EditResult::applied, "flow erase idempotent");
    const auto maximum = std::numeric_limits<float>::max();
    for (std::size_t slot = 0; slot < 2; ++slot)
        Check(fields.TryApplyEdit({FieldEditKind::set_flow, slot, {-maximum, 0}, maximum, maximum, {maximum, 0}}) ==
            EditResult::applied, "extreme flow accepted");
    const auto saturated = fields.Sample({0, 0});
    Check(saturated.x == maximum && saturated.y == 0, "extreme subtraction and flow saturation");
    CheckSample(fields, {0, maximum}, {});
    Check(fields.TryApplyEdit({FieldEditKind::remove, 1}) == EditResult::applied, "clear saturation companion");
    const auto tiny = std::numeric_limits<float>::denorm_min();
    Check(fields.TryApplyEdit({FieldEditKind::set_flow, 0, {}, 1, 2, {tiny, 0}}) == EditResult::applied,
        "no implicit minimum segment length");
    CheckSample(fields, {}, {2, 0});
    CheckSample(fields, {tiny, 0}, {2, 0});
    Check(fields.TryApplyEdit({FieldEditKind::set_flow, 0, {}, 1, 2, {tiny, tiny}}) == EditResult::applied,
        "smallest diagonal segment accepted");
    const auto diagonal_force = static_cast<float>(std::sqrt(2.0));
    CheckSample(fields, {}, {diagonal_force, diagonal_force});
    CheckSample(fields, {tiny, tiny}, {diagonal_force, diagonal_force});
}

}

int main() {
    FlowFixtures();
    using namespace crucible;
    using namespace crucible::fields;
    FieldSet fields{2};
    CheckSample(fields, {1, 0}, {});
    Check(fields.TryApplyEdit({FieldEditKind::set, 0, {0, 0}, 4, 8}) == EditResult::applied, "set failed");
    CheckSample(fields, {1, 0}, {-6, 0});
    CheckSample(fields, {0, 2}, {0, -4});
    CheckSample(fields, {4, 0}, {});
    CheckSample(fields, {8, 0}, {});
    CheckSample(fields, {0, 0}, {});
    Check(fields.TryApplyEdit({FieldEditKind::set, 0, {0, 0}, 4, -8}) == EditResult::applied, "upsert failed");
    CheckSample(fields, {1, 0}, {6, 0});
    Check(fields.TryApplyEdit({FieldEditKind::set, 1, {0, 0}, 4, 8}) == EditResult::applied, "second slot failed");
    CheckSample(fields, {1, 0}, {});
    Check(fields.TryApplyEdit({FieldEditKind::set, 2, {0, 0}, 4, 8}) == EditResult::slot_out_of_range,
          "full field array grew");
    CheckSample(fields, {1, 0}, {});
    Check(fields.TryApplyEdit({FieldEditKind::remove, 1}) == EditResult::applied, "remove failed");
    Check(fields.TryApplyEdit({FieldEditKind::remove, 1}) == EditResult::applied, "idempotent remove failed");
    CheckSample(fields, {1, 0}, {6, 0});
    const auto nan = std::numeric_limits<float>::quiet_NaN();
    const auto infinity = std::numeric_limits<float>::infinity();
    for (auto invalid : {FieldEdit{FieldEditKind::set, 0, {nan, 0}, 4, 8},
                         FieldEdit{FieldEditKind::set, 0, {0, infinity}, 4, 8},
                         FieldEdit{FieldEditKind::set, 0, {0, 0}, -1, 8},
                         FieldEdit{FieldEditKind::set, 0, {0, 0}, infinity, 8},
                         FieldEdit{FieldEditKind::set, 0, {0, 0}, 4, nan},
                         FieldEdit{static_cast<FieldEditKind>(99), 0}}) {
        Check(fields.TryApplyEdit(invalid) == EditResult::invalid, "invalid field edit accepted");
        CheckSample(fields, {1, 0}, {6, 0});
    }
    Check(fields.TryApplyEdit({FieldEditKind::set, 0, {0, 0}, 0, 8}) == EditResult::applied, "zero radius rejected");
    CheckSample(fields, {0, 0}, {});
    CheckSample(fields, {1, 0}, {});
    CheckSample(fields, {nan, 0}, {});
    CheckSample(fields, {0, infinity}, {});
    Check(fields.TryApplyEdit({FieldEditKind::remove, 0, {nan, nan}, nan, nan}) == EditResult::applied,
          "remove incorrectly interpreted ignored geometry");
    Check(fields.TryApplyEdit({FieldEditKind::set, 0, {0, 0}, 10, 10}) == EditResult::applied, "diagonal set failed");
    const float diagonal = -static_cast<float>((10.0 - std::sqrt(2.0)) / std::sqrt(2.0));
    CheckSample(fields, {1, 1}, {diagonal, diagonal});
    const float maximum = std::numeric_limits<float>::max();
    for (std::size_t slot = 0; slot < 2; ++slot)
        Check(fields.TryApplyEdit({FieldEditKind::set, slot, {0, 0}, maximum, maximum}) == EditResult::applied,
              "finite maximal field rejected");
    const auto saturated = fields.Sample({1, 0});
    Check(saturated.x == -maximum && saturated.y == 0 && std::isfinite(saturated.x), "sum saturation not finite");
    const auto extreme = fields.Sample({-maximum, maximum});
    Check(std::isfinite(extreme.x) && std::isfinite(extreme.y), "extreme sample nonfinite");
    FieldSet empty{0};
    Check(empty.TryApplyEdit({FieldEditKind::set, 0, {}, 1, 1}) == EditResult::slot_out_of_range,
          "zero capacity accepted slot");
    CheckSample(empty, {}, {});
}
