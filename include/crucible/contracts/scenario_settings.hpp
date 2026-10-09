#pragma once

#include <cstddef>
#include <optional>

#include "crucible/contracts/GridConfig.hpp"
#include "crucible/contracts/ResourceSettings.hpp"
#include "crucible/contracts/SteeringSettings.hpp"
#include "crucible/contracts/StructuralState.hpp"

namespace crucible {
/** Owns the startup geometry and population shared by simulation and its receivers.
 * Every buffer derives its capacity from this value; runtime growth is unsupported.
 * Validation and allocation occur when the receiving session is constructed.
 * Mission and desktop presentation policy remain outside this platform-free value.
 */
struct ScenarioSettings {
    std::size_t population{2048}; ///< Fixed identity count, including anchored/lost identities.
    GridConfig grid{64, 32, 1.0F}; ///< Physical world and finite substrate dimensions.
    std::size_t fieldCapacity{4}; ///< Startup field-slot and snapshot capacity.
    SteeringSettings steering{}; ///< Unchanged complete-neighbor steering rule.
    ResourceSettings resources{}; ///< Conserved finite stock and work budget.
    std::optional<StructuralSettings> structural{}; ///< Optional fixed-identity relay rule.
};
}
