#pragma once

#include <cstddef>
#include <functional>
#include <memory>

#include "crucible/contracts/execution_settings.hpp"

namespace crucible::scheduling
{
/** One contiguous output range and its exclusive startup scratch slot. */
struct RowRange
{
    std::size_t firstRow{};
    std::size_t rowCount{};
    std::size_t partitionIndex{};
};

/** Startup-owned row computation returning false when its staging must be discarded.
 * Simultaneous invocations share the callable: only each supplied range and scratch
 * slot may be mutated. Captured input, fields and index stay immutable until joined
 * return. Callback bodies and captured state must remain valid until destruction.
 * Callback throws become failed work; captured target destructors must not throw.
 */
using RowFunction = std::function<bool(RowRange)>;

/** Executes an untimed, startup-bounded row graph synchronously for Simulation.
 * The coordinator owns input, unpublished output and one complete query scratch
 * buffer per partition. This object retains only the startup callable; it owns
 * its graph, task metadata and optional worker pool. See the consumed contract in
 * docs/decisions/phase14-scale-contracts.md#c2-scheduling-and-failure-joins.
 * All calls and destruction require exclusive, non-reentrant coordinator access.
 * A callback must not recursively run, destroy or replace this object.
 */
class RowPartitions
{
public:
    /** Joined outcome; only complete permits authoritative output publication. */
    enum class RunStatus
    {
        complete,
        invalidRows,
        unsupportedFloatingPoint,
        failed
    };

    /** Maximum graph partitions supported by the received Pipeline node-index domain. */
    static constexpr std::size_t maxPartitions{65536};

    /** Validates bounds, allocates storage and primes an inactive graph at startup.
     * @param rowCapacity Maximum input rows; zero permits one empty partition.
     * @param settings Positive resolved worker/partition counts; partitions must
     * not exceed max(rowCapacity, 1) or maxPartitions. One worker executes on
     * the coordinator.
     * @param rowFunction Owned computation; no invocation occurs during startup.
     * @throws std::invalid_argument Empty callable or invalid counts.
     * @throws std::length_error Unrepresentable worker/task/storage counts.
     * Allocation and worker launch failures also propagate before a usable object
     * exists. Started workers are joined during construction failure cleanup.
     */
    RowPartitions(std::size_t rowCapacity, ExecutionSettings settings, RowFunction rowFunction);

    /** Joins and destroys the owned pool before releasing graph and callback state. */
    ~RowPartitions();

    RowPartitions(const RowPartitions&) = delete;
    RowPartitions& operator=(const RowPartitions&) = delete;
    RowPartitions(RowPartitions&&) = delete;
    RowPartitions& operator=(RowPartitions&&) = delete;

    /** Computes balanced, disjoint ranges and joins accepted work before returning.
     * @param rows Actual immutable input prefix, no larger than startup capacity.
     * @return Complete on success or empty input; invalidRows rejects before work;
     * unsupportedFloatingPoint permits sequential recomputation of all staging
     * only after a successful graph join. Callback, graph or submission failure
     * returns failed and requires discarding all staging. A worker restoration
     * failure poisons the adapter: every later call, including zero rows, fails.
     * @note Pool jobs receive and restore the coordinator FP environment. Inline
     * execution preserves ordinary coordinator exception-flag effects. Every
     * body, completion callback and accepted callable target finishes before
     * return; no input/output borrow survives it. Normal failed work is reusable
     * after joining, but the Runtime consumer retains its own fail-stop policy.
     */
    [[nodiscard]] RunStatus tryRun(std::size_t rows);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
