#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <latch>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <thread>
#include <vector>

#include "crucible/scheduling/row_partitions.hpp"

namespace
{
using crucible::ExecutionSettings;
using crucible::scheduling::RowPartitions;
using crucible::scheduling::RowRange;
using Status = RowPartitions::RunStatus;

void check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

template<typename Error, typename Function>
void expectThrow(Function function, const char* message)
{
    try
    {
        function();
    }
    catch (const Error&)
    {
        return;
    }
    check(false, message);
}

void receiveCoverage(std::size_t capacity, ExecutionSettings settings)
{
    std::vector<unsigned int> hits(capacity);
    std::vector<std::optional<RowRange>> ranges(settings.partitions);
    std::atomic<std::size_t> calls{0};
    const auto coordinator = std::this_thread::get_id();
    RowPartitions execution{capacity, settings, [&](RowRange range)
    {
        ++calls;
        if ((std::this_thread::get_id() == coordinator) != (settings.workers == 1))
        {
            return false;
        }
        if (range.partitionIndex >= ranges.size() || range.rowCount == 0 ||
            range.firstRow > capacity || range.rowCount > capacity - range.firstRow)
        {
            return false;
        }
        ranges[range.partitionIndex] = range;
        for (std::size_t row = range.firstRow; row < range.firstRow + range.rowCount; ++row)
        {
            ++hits[row];
        }
        return true;
    }};
    check(calls == 0, "Startup priming invoked a real row callback");
    for (const auto rows : std::array{std::size_t{0}, std::min(capacity, std::size_t{1}),
             std::min(capacity, std::size_t{3}), capacity})
    {
        std::ranges::fill(hits, 0U);
        std::ranges::fill(ranges, std::nullopt);
        calls = 0;
        check(execution.tryRun(rows) == Status::complete, "Valid row graph failed");
        const auto active = std::min(rows, settings.partitions);
        check(calls == active, "Empty tasks invoked callbacks or a nonempty task was omitted");
        std::size_t first = 0;
        for (std::size_t index = 0; index < active; ++index)
        {
            const auto count = rows / active + (index < rows % active ? 1U : 0U);
            check(ranges[index] && ranges[index]->partitionIndex == index &&
                ranges[index]->firstRow == first && ranges[index]->rowCount == count,
                "Partition ranges were not balanced and contiguous");
            first += count;
        }
        check(first == rows, "Partition coverage missed its final row");
        for (std::size_t row = 0; row < capacity; ++row)
        {
            check(hits[row] == (row < rows ? 1U : 0U), "Row coverage overlap or tail write");
        }
    }
    const auto before = calls.load();
    check(execution.tryRun(capacity + 1) == Status::invalidRows && calls == before,
        "Excess rows were not rejected before callbacks");
    check(execution.tryRun(capacity) == Status::complete, "Invalid rows prevented valid reuse");
}

void receiveFailureJoin(bool throwing)
{
    std::latch heldEntered{1}, releaseHeld{1}, failureEntered{1};
    std::atomic<bool> fail{true}, returned{false}, heldFinished{false};
    RowPartitions execution{32, {2, 2}, [&](RowRange range)
    {
        if (!fail.load())
        {
            return true;
        }
        if (range.partitionIndex == 1)
        {
            heldEntered.count_down();
            releaseHeld.wait();
            heldFinished = true;
            return true;
        }
        heldEntered.wait();
        failureEntered.count_down();
        if (throwing)
        {
            throw std::runtime_error("Injected row failure");
        }
        return false;
    }};
    Status result = Status::complete;
    std::jthread coordinator{[&]
    {
        result = execution.tryRun(32);
        returned = true;
    }};
    failureEntered.wait();
    check(!returned.load(), "Failed row graph returned while another partition borrowed its context");
    releaseHeld.count_down();
    coordinator.join();
    check(result == Status::failed && heldFinished.load(), "Row failure was misclassified or not joined");
    fail = false;
    check(execution.tryRun(32) == Status::complete, "Joined row failure prevented valid graph reuse");
}
}

int main()
{
    for (const auto capacity : std::array{std::size_t{0}, std::size_t{1}, std::size_t{23}, std::size_t{257}})
    {
        for (const auto workers : std::array{std::size_t{1}, std::size_t{2}, std::size_t{4}})
        {
            for (const auto partitions : std::array{std::size_t{1}, std::size_t{2}, std::size_t{7}})
            {
                if (partitions <= std::max(capacity, std::size_t{1}))
                {
                    receiveCoverage(capacity, {workers, partitions});
                }
            }
        }
    }
    const auto function = [](RowRange) { return true; };
    expectThrow<std::invalid_argument>([&] { RowPartitions invalid{23, {0, 1}, function}; }, "Zero workers accepted");
    expectThrow<std::invalid_argument>([&] { RowPartitions invalid{23, {1, 0}, function}; }, "Unresolved partitions accepted");
    expectThrow<std::invalid_argument>([&] { RowPartitions invalid{23, {1, 24}, function}; }, "Excess partitions accepted");
    expectThrow<std::invalid_argument>([] { RowPartitions invalid{23, {1, 1}, {}}; }, "Empty callable accepted");
    expectThrow<std::length_error>([&]
    {
        RowPartitions invalid{23, {static_cast<std::size_t>(std::numeric_limits<int>::max()) + 1, 1}, function};
    }, "Unrepresentable workers launched");
    expectThrow<std::length_error>([&]
    {
        RowPartitions invalid{std::numeric_limits<std::size_t>::max(),
            {2, std::numeric_limits<std::uint32_t>::max()}, function};
    }, "Unrepresentable queue/task counts accepted");
    expectThrow<std::length_error>([&]
    {
        RowPartitions invalid{65537, {1, 65537}, function};
    }, "Pinned graph uint16 successor index limit was not rejected before construction");
    receiveFailureJoin(false);
    receiveFailureJoin(true);

    auto payload = std::make_shared<int>(17);
    const std::weak_ptr<int> retained = payload;
    {
        RowPartitions execution{23, {2, 2}, [owned = payload](RowRange) { return *owned == 17; }};
        payload.reset();
        check(!retained.expired(), "Startup callback capture was not retained");
        check(execution.tryRun(23) == Status::complete, "Captured callback state unavailable");
        check(!retained.expired(), "Reusable callback capture was destroyed before adapter destruction");
    }
    check(retained.expired(), "Adapter destruction retained callback state");
}
