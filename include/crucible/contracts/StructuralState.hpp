#pragma once

#include <crucible/contracts/Position.hpp>
#include <crucible/contracts/SampleId.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace crucible {
/// Startup-owned single-relay rules. Structural scenarios require unit sample mass.
struct StructuralSettings {
    Position relay_center{48.5F, 16.5F};
    float eligibility_radius{4};
    float protection_radius{2};
    std::uint64_t hold_ticks{120};
    static constexpr std::size_t cost = 64;
    static constexpr std::size_t refund = 48;
};
/// Normal domain refusal never fails the runtime coordinator or debits biomass.
enum class StructuralCommandResult {
    applied, disabled, occupied, insufficient_mass, empty, stale_generation, generation_exhausted
};
/// Owned relay observation; empty slots have zero members and hold, but retain generation.
struct StructuralState {
    StructuralSettings settings{};
    bool occupied{};
    std::uint64_t generation{};
    std::uint64_t hold_ticks{};
    std::size_t eligible_mobile{};
    std::array<SampleId, StructuralSettings::cost> members{};
};
static_assert(std::is_trivially_copyable_v<StructuralState>);
}
