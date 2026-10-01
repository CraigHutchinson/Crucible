# Wave 1 pinned-source audit

Read-only audit on 2026-10-01 using `git show` of the full revisions in
`cmake/DependencyPins.cmake`; no dependency HEAD assumptions or dependency edits.
This is source evidence, not a threaded acceptance run.

## ECS 8391f81fd74a016564b4711b074eb286d3c5e14b

`include/sub0ecs/entity.hpp`: Entity is 24-bit index plus 8-bit version. Release
increments version modulo 256 and reuses slots LIFO. Stale detection therefore has
an ABA limit after 256 reuses. Allocator create masks indices without a capacity
failure result; Crucible must bound creation before the 24-bit domain is exhausted.
Allocator alive compares version and index, not explicit occupancy; application code
must not fabricate a handle using the current version of an unallocated/free slot.
A sample ID is an application-owned value, not a claim of stronger ECS generation
semantics. Wave 1 has no destruction/recycling; stable sample IDs can be assigned once.

`include/sub0ecs/store/world.hpp`: reserve reserves records only, not entity version
or partition columns. create registers types, creates the entity, pushes a row and
stores values; vector/hash/partition allocation can throw after partial mutation.
Do not claim bounded/no-allocation or transactional structural mutation from reserve.
Constraints: trivially copyable <=64-byte components, <=64 component types per World
type and <=32 declared queries. each<Cs...> statically requires an exact declared
Query<Cs...>. each passes component references without handles; a SampleId component
must join a declared query if identities are gathered alongside positions.

`world.hpp` find/each and moveTo/destroy, `store/column.hpp` and `store/partition.hpp`:
component pointers/references borrow dense/side storage. Growth and structural
migration/swap removal invalidate values or their entity association. No general
concurrent mutation lock exists. Gather owned values while quiescent; release all
borrows before structural changes. Do not depend on dense row ordering.

## Pub 2cd3daf15e44c9429fd2cfbc4ca2fcb723b77ced

`include/sub0pub/broker/publish.hpp`, `subscribe.hpp`, `broker_impl.hpp`, `table.hpp`:
publish invokes filter/receive synchronously on the publisher thread, passing a
const reference valid for that call. Concurrent publishers can invoke the same
subscriber concurrently; a broker Lock protects table bookkeeping, not application
callback state. Copy into the bounded ingress under its own mutex.

A concurrent configuration requires Snapshot and ThreadLocalContext. It does not
register in the base constructor: trySubscribe must follow complete derived
construction. Disconnect removes active snapshot entries then waits for callbacks
on other threads, excluding its own dispatch. Most-derived destruction must call
disconnect while callback members and ingress remain alive. Base destruction is too
late for concurrent derived lifetime. Mutual cross-disconnect inside simultaneous
callbacks can deadlock (explicit broker warning); defer teardown to coordinator.
Unlocked configurations are single-threaded, even though optional thread checks can
detect overlap. These source guarantees do not substitute for a Crucible threaded
lifetime/race test. Optional Pub adapter is deferred in this increment.

## Pipeline f6f54c623908649e8daac3613545062cf08b3822

`include/sub0pipeline/sub0pipeline.hpp`, `run_scope.hpp`, `src/sub0pipeline.cpp`:
IExecutor dispatch must eventually invoke completion; wait_all waits for bodies AND
completion callbacks. runImpl calls executor.wait_all before its normal result.
Timed noncooperative jobs may outlive run, including inline runs; join_orphans is
required before borrowed captures, executor, observer or Pipeline are destroyed.
Join can block indefinitely; never join from jobs or mutate/start work at teardown.
RunScope owns a run thread, stops/joins on destruction, and reaps orphans unless the
run was rejected kBusy. Graph construction is single-threaded. Wave 1 uses untimed
sequential work only; no nested waits or parallel/thread safety claim.

Exception/dispatch-throw paths are not established as a consumer rollback guarantee
by this audit. Root must retain valid owned state and gate executor adaptation with
failure injection before enabling parallel execution.
