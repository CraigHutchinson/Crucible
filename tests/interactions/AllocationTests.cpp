#include <crucible/interactions/Reclamation.hpp>

#include <array>
#include <cstdlib>
#include <iostream>
#include <new>

#include <crucible/blight/Grid.hpp>

namespace {
// Isolated executable: observation only, no production allocator or conditional path.
thread_local bool observe_allocations = false;
thread_local std::size_t observed_allocations = 0;
}

void* operator new(std::size_t bytes) {
    if (observe_allocations) ++observed_allocations;
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
    bool accepted = true;
    observe_allocations = true;
    for (int tick = 0; tick < 128; ++tick) {
        accepted = reclaim.TryStep(blight, samples) && accepted;
        accepted = reclaim.GetStocks().size() == 2 && accepted;
        accepted = reclaim.GetLedger().initial_total == 10 && accepted;
    }
    const std::array invalid{samples[0], samples[0]};
    accepted = !reclaim.TryStep(blight, invalid) && accepted;
    observe_allocations = false;
    if (!accepted || observed_allocations != 0) return 2;
    std::cout << "No ordinary new/new[] calls during bounded reclamation steps\n";
}
