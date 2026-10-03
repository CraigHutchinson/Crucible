#pragma once

#include <cstdint>

namespace crucible {
/// Finite startup quantities and per-tick work limit for the reclamation scenario.
struct ResourceSettings {
    std::uint64_t initial_stock_per_cell{4}; ///< Biomass quanta, including initially clear cells.
    std::uint64_t mass_per_sample{1}; ///< Positive externally supplied mobile quanta per fixed sample.
    std::uint64_t initial_reserve{}; ///< Externally supplied reserve quanta included in the initial total.
    std::uint64_t actions_per_tick{64}; ///< Successful work limit; zero disables reclamation.
};
}
