#pragma once

#include <cstddef>
#include <optional>

#include <crucible/contracts/FieldEdit.hpp>

namespace crucible::presentation {
/// Discrete radial-field intent; admission and committed feedback remain coordinator-owned.
enum class FieldTool { attract, repel, remove };
/// Explicit field slot and positive world-unit radial parameters for the selected tool.
struct FieldToolSettings {
    FieldTool tool{FieldTool::attract};
    std::size_t slot{};
    float radius{8}, magnitude{4};
};

/** Builds an owned one-slot command without mutating gameplay or admitting input.
 * @param[in] settings Selected tool/slot; set tools require positive finite radius/magnitude.
 * @param[in] center World center; remove ignores geometric payload.
 * @param[in] field_capacity Actual fixed slot bound used by ingress/Simulation.
 * @return Valid signed set or canonical remove; nullopt for invalid intent or capacity.
 */
[[nodiscard]] std::optional<FieldEdit> TryBuildFieldEdit(
    FieldToolSettings settings, Position center, std::size_t field_capacity) noexcept;
}
