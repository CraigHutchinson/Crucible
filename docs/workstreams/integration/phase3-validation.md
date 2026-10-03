# Phase 3 delivery: finite reclamation and H2 receiving proof

Date: 2026-10-03. Architect plus two isolated implementation workers.
Planning PR 6 merged at `837a38002bbc189e7fe319b069a1a6cbba9f28c4`.
Local worker starting commit `71c14c0` is an ancestor of that merge; resource received
shared contracts as `7f0e75c`. Architect integrated main before combined execution.
Worker/integration SHAs below identify retained local handoffs. Connector publication
consolidates the verified tree in a new commit; PR/head and merge are recorded at closure.

## Delivered callers and ownership

| Owner | Handoff | Consumed result |
|---|---|---|
| resource_package / P3-01 | `399e945`, integrated as `b4c3dff` | Simulation consumes Reclamation; move-only Blight prepared lease atomically publishes infection, stock and conserved ledger |
| hex_adoption_package / P3-02 | `22a77d8`, integrated as `ef1e877` | Spatial Grid consumes pinned H2 mapping/candidates; Steering and Simulation retain exact stable-ID query behavior |
| Architect / P3-00/03 | Shared contracts `d5619d9`, pin/dispatch `e6c59f1`, integrated root changes | Post-move contacts, owned stock/ledger copies, complete replay, CLI conservation and actual-state SVG |

Resource configuration is optional and fixed at startup. Unconfigured scenarios retain
ordinary cardinal spread and the radial/separation rules. Stock belongs to rectangular
Blight contact cells, independent of hex bin occupancy. A successful action removes
one substrate quantum and adds one reserve quantum; mobile mass stays fixed.
Zero-stock reinfection can be cleared without credit. Stable cell/ID arbitration,
one proposal per sample and the successful-action global budget determine contention.
The [resource decision](../../decisions/phase3-resource-rules.md) is authoritative.

Reclamation owns startup-sized committed/pending stock, proposals and ID validation
scratch. PreparedStep owns an exclusive borrowed lease on Blight pending scratch;
abandonment releases it, commit consumes it. No ECS pointer or sample borrow escapes.
Simulation owns all coordinator state. Snapshot capture preflights every destination
including stock, then publishes metadata after copying. Owned frames retain prior
samples, fields, infection, stock and ledger after rejected capture.
Snapshot stock storage is startup-sized even for legacy frames; such frames expose
an empty stock span and absent ledger. Resource atomicity covers infection/material/
ledger, not already applied commands or movement. Unexpected tick exceptions stop
HeadlessSession and suppress completed-tick publication; continuation is unsupported.

## Numerical compatibility and review

H2 pin: `aaae5c2fc5731a23db94fa947bbb0182d0ea69fd`; other dependency pins unchanged.
Pointy layout uses origin (0,0), circumradius cell_size/sqrt(3) and the closed existing
physical rectangle. Cardinal infection adjacency is unchanged. Stable sample IDs,
bin storage, clamping, exact double squared-distance filtering and ascending output
remain application policy. Borrowed query output expires at next query/rebuild.

Review discovered two startup mismatches: H2's supported arithmetic domain is narrower,
and a dense covering rectangle can grow quadratically for tall thin physical worlds.
The architect approved a private rectangular compatibility path in the same Grid,
with checked hex cells <= 4*physical_cells+64. This explicitly refines the original
plan's no-dual-backend wording. The constant is a startup scaling guard, not a measured
optimum. Nonnearest environments preserve complete committed query behavior through
rectangular rebuild or exact scans; unsupported valid extreme queries scan committed
samples. [Spatial decision](../spatial/decisions.md) proves the outward corner-envelope
cover and records checked count/byte arithmetic. No selectable public backend was added.

Both workers and architect loaded cpp-write/cpp-review and the four shared references
from CraigHutchinson/Agentic-CPP. Project C++23/.hpp/standard-library conventions apply.
Architect independently reviewed the provider and spatial implementation file by file;
resource worker cross-reviewed spatial numerical/lifetime behavior; spatial worker
reviewed root composition, snapshot contracts, main, shared values and integration
fixtures. No unresolved MUST/SHOULD remains. Two root clarity findings were corrected:
no-steering indentation and explicit stock/ledger copy documentation. The unused
TryCopyStocks API was removed before integration because the owned snapshot consumes
GetStocks after its capacity precheck. No unused extraction/helper surface was added.

## Executed validation

Host: Linux x86-64, GCC 13.3.0, CMake 4.4.3, Ninja 1.13.2.
The architect serialized whole-tree configure/build/test work; workers used isolated
worktrees and only one granted focused spatial CPU slot. Commands:

```sh
cmake --preset debug
cmake --build --preset debug --parallel 2
ctest --preset debug
cmake --preset release
cmake --build --preset release --parallel 2
ctest --preset release
cmake --preset sanitize
cmake --build --preset sanitize --parallel 2
ctest --preset sanitize
```

| Gate | Actual result |
|---|---|
| Resource integration on original rectangular spatial index | Debug 16/16 passed before receiving spatial package |
| Combined Debug | 18/18 passed |
| Combined Release | 18/18 passed |
| Combined ASan/UBSan | All 18 tests passed with ASAN_OPTIONS=detect_leaks=0; one executable-permission repair and targeted rerun |
| Spatial worker focused GCC C++23 Debug | 3/3 passed, no authored warnings |
| Rectangular/hex resource full-state replay | Byte-identical traces at 0/8/2,048 populations, initial frame and every third tick through tick 60 |
| Legacy CLI | 150,000 entities / 60 ticks / checksum 225000 retained; default phase2 complete replay passes |
| Resource CLI | Complete tick-20 replay passes; initial 10240 = stock 7238 + mobile 2048 + reserve 954; harvested 954, work_actions 1104 |

Default local sanitizer execution failed at LeakSanitizer process inspection because
/proc/<pid>/task is restricted by this managed host. The adjusted unfiltered run
passed 17 tests and could not launch stack_round_trip after its executable bit was
removed; restoring that local binary permission and ctest --rerun-failed passed
the remaining test. This changes no project source, preset or hosted CI policy.
Normal hosted sanitizer CI remains a required merge gate.

The 13 historical tests remain registered and passing. Five new registered tests
cover resource integration, independent reclamation arithmetic and allocations,
independent hex queries and spatial allocation reuse. Hand fixtures cover spread
before contact, depletion/zero credit/reinfection, fixed IDs, shuffled input,
cell-first global arbitration, edge/next-float/clamped contact, invalid/nonfinite
inputs, startup products/sums, max-total harvest and abandoned/moved/committed leases.
Integration includes pause, different elapsed schedules, admitted trace replay,
complete sample/velocity/field/infection/material/ledger comparison, owned retention,
stock-capacity rejection, disabled work and no-steering hand-calculated transfers.
Spatial queries independently compare all IDs to scan oracles across seams, vertices,
physical extremes, elongated worlds and rounding transitions. See worker handoffs.

The two isolated allocation probes observe zero ordinary global C++ new/new[] calls
in scoped actual resource/Blight and spatial rebuild/query/rejection operations after
startup. They do not claim every aligned/C allocation, OS residency or full ECS-tick
allocation freedom. Cumulative work-counter exhaustion is checked and reviewed but
cannot feasibly be driven from zero to uint64 max; no test-only mutation hook was
introduced. The stopped tick-exception path is reviewed, not fault-injected through
private counters. Exact replay is within one build, not a cross-compiler bitwise claim.
Pinned Sub0Pipeline emits an existing missing-field-initializer warning; no unrelated
upstream pin or source change was made. No timing or FPS conclusion is claimed.

## Reproducing the rectangular/hex comparison and HX-07 handoff

The integration executable optionally serializes all frames with hexfloat sample/
field values, every infection/stock pair and every ledger field:

```sh
./build/debug/tests/integration/crucible_phase3_test hex-state.txt
```

For the rectangular comparator, make an isolated worktree at the delivered phase-3
head and restore only include/crucible/spatial/Grid.hpp, src/spatial/Grid.cpp and
src/spatial/CMakeLists.txt from the planning merge `837a380`. Configure Debug there,
build the crucible_phase3_test target, run it with rectangular-state.txt, then cmp
the two outputs. Retain both inputs/build options; do not overwrite the main worktree.
The actual sequential pre/post-integration comparison produced 172389 lines and
2917785 bytes each, SHA256 `f90b7e04a4cb23a4627b645c21e95a0718aab69070d55136f746860bf6300a82`.

HX-07 is a reproducible receiving pack, retained here rather than copied into upstream
research. Physical world is 64x32 at scale 1; IDs are fixed 1..N; ECS pin remains
`8391f81fd74a016564b4711b074eb286d3c5e14b`. Motion queries radius 1.5 once per sample
per tick, consumes the complete ascending span immediately, and rebuilds the immutable
tick-start and post-move indexes. Trace contains set slot 0 at the first boundary
and remove at tick 31. Fifty milliseconds versus 10+40 milliseconds yields three
60Hz ticks; pause pumps produce none. Full traces supply exact position/velocity/ID
frames and command epochs with coordinator-exclusive publication.

Measured changed-position fraction between the retained three-tick frames is 100%
for population 8 and 2047/2048 to 100% for population 2048; empty population has no
fraction. These are three-tick interval observations, not per-tick movement rates.
No measured candidate/output-work distribution, tail budget or hierarchy advantage
is supplied. HX-05/HX-06 and hierarchy experiments remain upstream-owned; no hierarchy,
navigation, renderer or sliced-tick consumer was selected. The receiving benefit is
shared checked H2 geometry with proven application compatibility, not a speedup.

## Actual-state view and closure

[phase3-state.svg](../../concepts/phase3-state.svg) is generated from the owned tick-20
resource snapshot by the release CLI, not concept art. CairoSVG rendering was
visually inspected; XML independently counts 2048 cyan samples, 752 infection
rectangles and one active field. It shows 2048 sample positions,
752 infected cells, the surviving repulsive field and the conservation equation.
The separate default legacy phase2 scenario retains 800 infected cells at tick 20.
Fusion, growth, attrition, mission outcome and playable input/rendering remain deferred.

[Implementation PR 7](https://github.com/CraigHutchinson/Crucible/pull/7) merged
at `21f37867bebec46f215cf489d77eee4db63b0165` after [exact-head CI](https://github.com/CraigHutchinson/Crucible/actions/runs/37089809757)
passed at `31fb1e49c3eae2652867009f8083d5688ab2e4dd`. Linux and Windows Debug/Release and normal
Linux ASan/UBSan each passed all 18 registered tests (five successful jobs). Hosted
sanitizer coverage includes normal leak detection; no local workaround was committed.
The local checkout was fast-forwarded to origin/main and verified clean at that merge.

Worker branches/worktrees, local source handoffs and comparison/build artifacts are
retained. Connector publication consolidated the identical verified local tree
`b32e3289a783316173f9247a40bad27da4776099`; no files were omitted. This closure
update is documentation only and does not authorize branch/artifact deletion.
