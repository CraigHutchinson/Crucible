#include <crucible/spatial/Grid.hpp>

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <utility>

namespace {
// Standalone single-threaded instrumentation observes ordinary C++ vector/sort allocations.
thread_local bool record_allocations{};
thread_local std::size_t allocation_count{};

class AllocationScope {
public:
    AllocationScope() noexcept { allocation_count = 0; record_allocations = true; }
    ~AllocationScope() { record_allocations = false; }
    AllocationScope(const AllocationScope&) = delete;
    AllocationScope& operator=(const AllocationScope&) = delete;
};
}

void* operator new(std::size_t size) {
    if (record_allocations) ++allocation_count;
    if (auto* memory = std::malloc(size == 0 ? 1 : size)) return memory;
    throw std::bad_alloc{};
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

int main() {
    std::array<crucible::spatial::SpatialSample, 2048> samples{};
    for (std::size_t row = 0; row < samples.size(); ++row)
        samples[row] = {{samples.size() - row}, {static_cast<float>(row % 16) * 0.5F,
                                               static_cast<float>(row % 13) * 0.5F}};
    crucible::spatial::Grid grid{{8, 8, 1}, samples.size()};
    crucible::spatial::Grid fallback{{1, 4096, 1}, samples.size()};
    std::array<crucible::SampleId, 2048> caller_scratch{};
    bool valid = true;
    {
        AllocationScope scoped;
        for (int repeat = 0; repeat < 16; ++repeat) {
            valid = grid.TryRebuild(samples) && fallback.TryRebuild(samples) && valid;
            for (float radius : {0.0F, 0.1F, 1.0F, 4.0F, std::numeric_limits<float>::max()}) {
                const auto hits = grid.TryQuery({4, 4}, radius);
                const auto fallback_hits = fallback.TryQuery({0.5F, 4}, radius);
                const auto caller_hits = std::as_const(grid).tryQuery({4, 4}, radius, caller_scratch);
                valid = hits.has_value() && fallback_hits.has_value() && caller_hits.has_value() && valid;
            }
            const auto saved_id = samples.back().id;
            samples.back().id = samples.front().id;
            valid = !grid.TryRebuild(samples) && valid;
            samples.back().id = saved_id;
        }
    }
    if (!valid || allocation_count != 0) {
        std::cerr << "rebuild/query/rejection allocated or rejected valid fixture: " << allocation_count << '\n';
        return 1;
    }
}
