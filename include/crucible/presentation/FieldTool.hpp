#pragma once

#include <cstddef>
#include <optional>

#include <crucible/contracts/FieldEdit.hpp>

namespace crucible::presentation {
/// Field intent; admission and committed feedback remain coordinator-owned.
enum class FieldTool { attract, repel, remove, flow };
/// Explicit field slot and positive world-unit field parameters for the selected tool.
struct FieldToolSettings {
    FieldTool tool{FieldTool::attract};
    std::size_t slot{};
    float radius{8}, magnitude{4};
};

/** Builds an owned one-slot command without mutating gameplay or admitting input.
 * @param[in] settings Selected tool/slot; set tools require positive finite radius/magnitude.
 * @param[in] center World center or flow start; remove ignores geometric payload.
 * @param[in] field_capacity Actual fixed slot bound used by ingress/Simulation.
 * @param[in] end World flow endpoint; radial/remove ignore it.
 * @return Valid signed radial/positive flow set or canonical remove; nullopt for invalid intent or capacity.
 */
[[nodiscard]] std::optional<FieldEdit> TryBuildFieldEdit(
    FieldToolSettings settings, Position center, std::size_t field_capacity, Position end = {}) noexcept;
}
