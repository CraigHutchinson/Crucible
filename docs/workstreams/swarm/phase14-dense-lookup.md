# Phase14 dense identity lookup refinement

This first private refinement targets the complete-neighbor proposal kernel.
Measured proposal costs dominate the initial scale probes. Local correctness and
storage receiving passed; native performance receiving remains pending.

Both public steering paths validate the complete input's strict identity order,
finite values and world bounds before entering the shared private kernel. For a
nonempty strictly increasing integer sequence, `last-first == count-1` proves
that every identity in the interval exists. That proof permits a range-checked
`id-first` lookup into the same immutable input. Unknown IDs below the first
identity reject before unsigned subtraction; offsets outside the input reject
before access. Sparse inputs retain the existing exact `lower_bound` lookup.

There is no new public API, configuration option, approximation or storage.
Neighbor membership/order, coincidence direction, distance/weight arithmetic,
accumulation, field sampling, caps and conversions remain unchanged. Empty and
single-row inputs, arbitrary identity bases and structural identity holes remain
supported. The ordinary kernel/query allocation contract is unchanged.

`swarm_dense_lookup` uses order-preserving identity relabeling as an independent
bitwise oracle: identical physical state receives contiguous identities or sparse
identities forcing the existing search path. The numerical kernel uses identity
only for self membership and relative coincidence tie direction, so the relabeling
preserves those semantics. The fixture covers bases zero, one and near UINT64_MAX,
empty/single/23/257 rows, interior holes, unknown neighbor IDs on both sides of a
dense interval, dense in-place legacy output, complete-input validation outside a
partition, four advancing rounds in each of four rounding modes and actual
1/2/4-worker row graphs. Grid/field state
and geometric traversal order are the same in each compared arm.

The existing independent arithmetic/property oracle, partition/race/FP fixtures
and integrated state/replay tests remain separate gates. `swarm_partition_allocations`
now receives both sparse and contiguous identities with the same reused startup
buffers, preserving its strict ordinary-new assertion in Debug and Release.

Implementation, meaningful fixture and local manifest edits are confined to the
Swarm stream in the isolated `phase14-steering-refinement` worktree, based on
`84d7362fda87809f1360272209e09bb13dfee647`. Original Scheduling source/evidence is
preserved.

On 2026-10-09, isolated affected receiving passed Debug 6/6 (12.07 seconds) and
Release 6/6 (1.16 seconds): `swarm_steering`, `swarm_partition_rows`,
`swarm_partition_allocations`, `swarm_dense_lookup`,
`scheduling_row_floating_point` and `scheduling_row_allocations`. These are test
runner durations, not performance samples. Both builds receive sparse and dense
kernel/query first and steady calls with strict zero ordinary-new allocations.
Scheduling's real row kernel receives standard rounding modes and available x86
denormal controls (`0x8040`). Release orchestration first/steady/failure/recovery
ordinary-new counts and requested bytes were zero for 1/2/4 workers.

The existing checked-MSVC Debug orchestration qualification remains separate from
the strict kernel/query assertion. This run reported:

| Workers | First calls | First bytes | Total calls | Total bytes |
| --- | --- | --- | --- | --- |
| 1 | 7 | 112 | 480 | 7680 |
| 2 | 9 | 144 | 485 | 7760 |
| 4 | 8 | 128 | 484 | 7744 |

Those ordinary replaceable-new observations do not claim interception of aligned,
C-runtime or OS allocations. The prior Scheduling receipt attributes checked-MSVC
Debug allocations to Pipeline diagnostic string proxies; see
`../scheduling/phase14-row-partitions.md`. The four-worker counts can vary
with dispatch interleaving and are retained as observed.

The toolchain receipt records Visual Studio developer shell 18.7.0-insiders and
MSVC compiler 19.51.36246 from the unchanged `14.51.36231` toolset directory, with
embedded Debug information. Read-only dependency sources were received at
Pipeline `f730c4ec2973a449c45fbf9a74595414b9bf30e1`, Pub
`d566c71c47cc5aeba3ed0b615052dbe6fcd91f23` and HexGrid
`aaae5c2fc5731a23db94fa947bbb0182d0ea69fd`. Raw configure/build/test and prerequisite
receipts are preserved in this worktree's `build/phase14-dense-debug` and
`build/phase14-dense-release`; each test log is `dense-tests.log`.

Independent private-kernel source review found no MUST/SHOULD defects. This local
receiving does not claim sanitizer/platform, persistent-worker original FP flags
between jobs, or naturally unavailable FP installation/restoration failure
coverage. Full integrated/native receiving, matched-source worker comparisons and
G3/G5 acceptance belong to the architect. No speedup or milestone acceptance is
claimed here.
