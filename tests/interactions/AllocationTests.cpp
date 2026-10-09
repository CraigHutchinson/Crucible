#include <crucible/interactions/Reclamation.hpp>
#include <crucible/simulation.hpp>

#include <array>
#include <cstdlib>
#include <iostream>
#include <new>

#include <crucible/blight/Grid.hpp>

namespace {
// Isolated executable: observation only, no production allocator or conditional path.
thread_local bool observe_allocations = false;
thread_local std::size_t observed_allocations = 0;
thread_local std::size_t observed_bytes = 0;
}

void* operator new(std::size_t bytes) {
    if (observe_allocations) { ++observed_allocations; observed_bytes += bytes; }
    if (void* allocation = std::malloc(bytes == 0 ? 1 : bytes)) return allocation;
    throw std::bad_alloc{};
}
void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
void operator delete(void* allocation) noexcept { std::free(allocation); }
void operator delete[](void* allocation) noexcept { std::free(allocation); }
void operator delete(void* allocation, std::size_t) noexcept { std::free(allocation); }
void operator delete[](void* allocation, std::size_t) noexcept { std::free(allocation); }

int main() {
    crucible::blight::Grid blight{{2, 1, 1.0F}};
    crucible::interactions::Reclamation reclaim{{2, 1, 1.0F}, 2, {4, 1, 0, 2}};
    const std::array samples{
        crucible::SampleState{{2}, {1.5F, 0.5F}, {}},
        crucible::SampleState{{1}, {0.5F, 0.5F}, {}}};
    if (!blight.TrySeed(0, 0)) return 1;
    crucible::StructuralSettings structural{{.5F, .5F}, 0, 0, 120};
    crucible::Simulation simulation{64, {{1, 1, 1}, 1, crucible::SteeringSettings{},
        crucible::ResourceSettings{4, 1, 0, 0}, structural}};
    std::array<crucible::SampleState, 64> owned_samples{};
    std::array<crucible::FieldEdit, 1> owned_fields{};
    std::array<std::uint8_t, 1> owned_infection{};
    std::array<std::uint64_t, 1> owned_stock{};
    const crucible::StateCopyDestination owned{owned_samples, owned_fields, owned_infection, owned_stock};
    bool accepted = true;
    observe_allocations = true;
    for (int tick = 0; tick < 128; ++tick) {
        accepted = reclaim.TryStep(blight, samples) && accepted;
        accepted = reclaim.GetStocks().size() == 2 && accepted;
        accepted = reclaim.GetLedger().initial_total == 10 && accepted;
    }
    const std::array invalid{samples[0], samples[0]};
    accepted = !reclaim.TryStep(blight, invalid) && accepted;
    accepted = reclaim.TryAnchor(1) && accepted;
    accepted = reclaim.TryStep(blight, std::span{samples}.first(1),
        crucible::interactions::Reclamation::ProtectedArea{{.5F, .5F}, 1}) && accepted;
    accepted = reclaim.TryRelease(1, 0) && accepted;
    accepted = reclaim.TryStep(blight, std::span{samples}.first(1)) && accepted;
    // Component work remains strictly allocation-free on every toolchain.
    accepted = observed_allocations == 0 && accepted;
    // Whole production coordinator: all64 colocated at the exact-radius relay.
    // Include mobile-only steering/grid gathering and the owned capture boundary.
    accepted = simulation.TryFuseRelay() == crucible::StructuralCommandResult::applied && accepted;
    simulation.tick();
    auto captured = simulation.TryCopyState(owned);
    accepted = captured && captured->structural && captured->biomass && accepted;
    if (captured && captured->structural && captured->biomass)
        accepted = captured->structural->hold_ticks == 1 && captured->biomass->mobile_mass == 0 &&
            captured->biomass->structure_mass == 64 && accepted;
    accepted = simulation.TryCountNeighbors({.5F, .5F}, 1) == 0 && accepted;
    accepted = simulation.TryShatterRelay(1) == crucible::StructuralCommandResult::applied && accepted;
    accepted = observed_allocations == 0 && accepted;
    simulation.tick();
    const auto mobileTickAllocations = observed_allocations;
    const auto mobileTickBytes = observed_bytes;
    captured = simulation.TryCopyState(owned);
    accepted = captured && captured->structural && captured->biomass && accepted;
    if (captured && captured->structural && captured->biomass)
        accepted = captured->biomass->mobile_mass == 48 && captured->biomass->lost_mass == 16 &&
            captured->biomass->structure_mass == 0 && captured->biomass->remaining_stock == 4 &&
            captured->biomass->reserve == 0 && captured->biomass->initial_total == 68 &&
            captured->structural->hold_ticks == 0 && accepted;
    for (std::size_t index = 0; index < owned_samples.size(); ++index)
        accepted = owned_samples[index].activity == (index < 48 ? crucible::SampleActivity::mobile
            : crucible::SampleActivity::lost) && accepted;
    observe_allocations = false;
#if defined(_MSC_VER) && defined(_M_X64) && _ITERATOR_DEBUG_LEVEL > 0
    // Received Pipeline per-job std::string move creates one checked 16-byte proxy.
    // The empty anchored epoch skips dispatch; only the single mobile job pays it.
    constexpr std::size_t expectedOrchestrationCalls = 1, expectedOrchestrationBytes = 16;
#else
    constexpr std::size_t expectedOrchestrationCalls = 0, expectedOrchestrationBytes = 0;
#endif
    if (!accepted || mobileTickAllocations != expectedOrchestrationCalls || mobileTickBytes != expectedOrchestrationBytes ||
        observed_allocations != mobileTickAllocations || observed_bytes != mobileTickBytes) {
        std::cerr << "Reclamation/structural allocation receiving failed: calls=" << observed_allocations
            << " bytes=" << observed_bytes << " expectedOrchestrationCalls=" << expectedOrchestrationCalls << '\n';
        return 2;
    }
    std::cout << "Reclamation and structural edits/copy: zero ordinary allocations; mobile row orchestration calls="
        << mobileTickAllocations << " requestedBytes=" << mobileTickBytes << '\n';
}
