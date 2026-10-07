#pragma once

#include <crucible/contracts/FieldEdit.hpp>

#include <cstdint>
#include <optional>

namespace crucible::runtime {
/** Returns the fixed swept-attractor example used by the reference mission CLI/export.
 * @param[in] completed_tick Last successfully completed tick, before admitting input
 * for the next boundary. The route repeats over the entire uint64 tick domain.
 * @return Owned slot-zero edit at multiples of 60; nullopt between requests.
 * @note Radius 8 and strength 4 match the live tool. The function neither admits
 * nor applies input; callers check admission and stop requesting after an outcome.
 * Request tick 0 applies at completed boundary 1, and request tick 60 at boundary 61.
 */
[[nodiscard]] std::optional<FieldEdit> GetReferenceMissionRouteEdit(std::uint64_t completed_tick) noexcept;
}
