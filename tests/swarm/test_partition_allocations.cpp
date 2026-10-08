#include <array>
#include <cstdlib>
#include <iostream>
#include <new>
#include <span>

#include "crucible/fields/FieldSet.hpp"
#include "crucible/spatial/Grid.hpp"
#include "crucible/swarm/Steering.hpp"

namespace
{
// This isolated executable observes ordinary replaceable C++ allocation calls only.
thread_local bool observeAllocations{};
thread_local std::size_t allocationCount{};
}

void* operator new(std::size_t bytes)
{
    if (observeAllocations) ++allocationCount;
    if (auto* allocation = std::malloc(bytes == 0 ? 1 : bytes)) return allocation;
    throw std::bad_alloc{};
}
void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
void operator delete(void* allocation) noexcept { std::free(allocation); }
void operator delete[](void* allocation) noexcept { std::free(allocation); }
void operator delete(void* allocation, std::size_t) noexcept { std::free(allocation); }
void operator delete[](void* allocation, std::size_t) noexcept { std::free(allocation); }

int main()
{
    using namespace crucible;
    std::array<SampleState, 64> input{}, pending{};
    std::array<spatial::SpatialSample, 64> gathered{};
    std::array<SampleId, 64> scratch{};
    for (std::size_t row = 0; row < input.size(); ++row) {
        input[row] = {{1001 + row * 17}, {static_cast<float>(row % 8), static_cast<float>(row / 8)}, {1, -1}};
        gathered[row] = {input[row].id, input[row].position};
    }
    spatial::Grid grid{{8, 8, 1}, input.size()};
    swarm::Steering steering{{8, 8, 1}, SteeringSettings{}, input.size()};
    fields::FieldSet fields{0};
    if (!grid.TryRebuild(gathered)) return 1;
    const auto* const queryAddress = scratch.data();
    const auto* const pendingAddress = pending.data();
    bool valid = true;
    observeAllocations = true;
    for (int repetition = 0; repetition < 64; ++repetition) {
        for (const std::size_t first : {0U, 17U, 39U}) {
            const auto count = first == 0 ? 17 : first == 17 ? 22 : 25;
            valid = steering.tryComputeRows(input, fields, grid, first,
                std::span{pending}.subspan(first, count), scratch) && valid;
        }
        valid = !steering.tryComputeRows(input, fields, grid, input.size(),
            std::span{pending}.first(1), scratch) && valid;
        valid = !steering.tryComputeRows(input, fields, grid, 0,
            pending, std::span{scratch}.first(63)) && valid;
        valid = steering.TryCompute(input, fields, grid, pending) && valid;
    }
    observeAllocations = false;
    if (!valid || allocationCount != 0 || scratch.data() != queryAddress || pending.data() != pendingAddress) {
        std::cerr << "Kernel/query/rejection storage reuse failed; ordinary new calls=" << allocationCount << '\n';
        return 2;
    }
}
