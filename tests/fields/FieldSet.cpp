#include <crucible/fields/FieldSet.hpp>

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
}

int main() {
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
