#include <array>
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <new>

#include "crucible/scheduling/row_partitions.hpp"

namespace
{
// This process observes ordinary replaceable new/new[] on all pool threads.
std::atomic<bool> observeAllocations{false};
std::atomic<std::size_t> allocationCount{0};
std::atomic<std::size_t> requestedBytes{0};
}

void* operator new(std::size_t bytes)
{
    if (observeAllocations.load(std::memory_order_relaxed))
    {
        allocationCount.fetch_add(1, std::memory_order_relaxed);
        requestedBytes.fetch_add(bytes, std::memory_order_relaxed);
    }
    if (auto* allocation = std::malloc(bytes == 0 ? 1 : bytes))
    {
        return allocation;
    }
    throw std::bad_alloc{};
}
void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
void operator delete(void* allocation) noexcept { std::free(allocation); }
void operator delete[](void* allocation) noexcept { std::free(allocation); }
void operator delete(void* allocation, std::size_t) noexcept { std::free(allocation); }
void operator delete[](void* allocation, std::size_t) noexcept { std::free(allocation); }

int main()
{
    using crucible::scheduling::RowPartitions;
    using crucible::scheduling::RowRange;
    using Status = RowPartitions::RunStatus;
#if defined(_MSC_VER) && _ITERATOR_DEBUG_LEVEL > 0
    // Received Pipeline error-context moves allocate checked std::string proxies.
    constexpr bool requireZero = false;
#else
    constexpr bool requireZero = true;
#endif
    for (const std::size_t workers : {1U, 2U, 4U})
    {
        std::array<std::size_t, 128> output{};
        std::atomic<bool> fail{false};
        RowPartitions execution{output.size(), {workers, 7}, [&](RowRange range)
        {
            if (fail.load(std::memory_order_relaxed))
            {
                return false;
            }
            for (std::size_t row = range.firstRow; row < range.firstRow + range.rowCount; ++row)
            {
                output[row] = row + 1;
            }
            return true;
        }};
        const auto* address = output.data();
        allocationCount = 0;
        requestedBytes = 0;
        bool valid = true;
        std::size_t firstActualCount = 0;
        std::size_t firstActualBytes = 0;
        observeAllocations = true;
        // The first actual computation is observed, not an unmeasured warm run.
        for (int repetition = 0; repetition < 32; ++repetition)
        {
            valid = execution.tryRun(output.size()) == Status::complete && valid;
            if (repetition == 0)
            {
                firstActualCount = allocationCount.load();
                firstActualBytes = requestedBytes.load();
            }
            valid = execution.tryRun(output.size() + 1) == Status::invalidRows && valid;
            valid = execution.tryRun(0) == Status::complete && valid;
            fail = true;
            valid = execution.tryRun(output.size()) == Status::failed && valid;
            fail = false;
            valid = execution.tryRun(3) == Status::complete && valid;
        }
        observeAllocations = false;
        if (!valid || (requireZero && allocationCount.load() != 0) || address != output.data())
        {
            std::cerr << "Row graph first/warm/failure storage reuse failed; ordinary new calls="
                << allocationCount.load() << '\n';
            return 1;
        }
        std::cout << "workers=" << workers << " firstActualOrdinaryNewCalls=" << firstActualCount
            << " firstActualRequestedBytes=" << firstActualBytes << " totalRequestedBytes=" << requestedBytes.load()
            << " requireZero=" << requireZero << " totalOrdinaryNewCalls=" << allocationCount.load() << '\n';
        for (std::size_t row = 0; row < output.size(); ++row)
        {
            if (output[row] != row + 1)
            {
                return 2;
            }
        }
    }
}
