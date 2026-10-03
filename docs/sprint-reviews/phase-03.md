# Phase 3 sprint review: finite reclamation and hex receiving proof

Status: complete and merged. Date: 2026-10-03.
Accountable reviewer: architect. [Plan](../phases/phase3.md),
[resource rule](../decisions/phase3-resource-rules.md),
[execution evidence](../workstreams/integration/phase3-validation.md).

## Intent, baseline and scope

Planning PR 6 merged at `837a38002bbc189e7fe319b069a1a6cbba9f28c4`.
Worker starting commit `71c14c0` is contained in that merge. Sub0HexGrid receiving
pin is `aaae5c2fc5731a23db94fa947bbb0182d0ea69fd`; upstream hierarchy PR 5
remains research. No unrelated dependency pin was refreshed.

Delivered outcome: post-move samples reclaim finite stock into reserve, clear
exhausted infection and expose a conserved owned ledger. Fixed population and
complete replay explain transfers. Spatial now consumes checked H2 geometry while
preserving the existing physical/query contract. Fusion/shatter, attrition, spawning,
mission outcome, playable input/rendering and concurrency remain deferred.

## Work delivered and delegation

Architect retained contracts, root wiring, Simulation, main, owned snapshots and
integration evidence. Two isolated workers owned disjoint packages. A consolidated
Blight/interactions under one publication owner (`399e945`, integrated `b4c3dff`).
B proved the application receiving contract rather than repeating upstream H2
(`22a77d8`, integrated `ef1e877`). A was integrated and tested on rectangular spatial
before B was promoted. Both workers and architect applied recovered cpp-write and
cpp-review skills plus shared references before substantive authoring/review.

This division kept geometry compatibility independent of resource correctness.
Separate fields, scheduling, telemetry and rendering workers were unnecessary for
this increment. Claims, isolated worktrees and serialized CPU ownership avoided
shared-surface competition; peer numerical/lifetime and root-consumer reviews
complemented architect review without adding agents.

## Findings and resolutions

| Finding / ID | Impact | Resolution / evidence | Remaining risk |
|---|---|---|---|
| P02-F01 stale H1 limitation | Repeating delivered upstream work | Consumed pinned H2, independent query fixtures and integrated full-state parity | No hierarchy or performance conclusion |
| Infection cannot represent biomass | Spread/reinfection could create unlimited reserve | Separate finite stock; checked ledger and independent partial/depleted/zero-credit oracles | Balance not playtested |
| Immediate Blight publication | Partial infection/resource transition | Exclusive move-only prepare/commit lease; abandonment preserves committed infection | Whole ECS tick rollback intentionally absent |
| H2 extreme query/startup domain | Could reject previously valid finite worlds/queries | Exact query scan and private rectangular compatibility; numeric/environment fixtures | Fallback cost not measured |
| Tall thin axial cover grows quadratically | Startup footprint could regress without bound | Checked 4*physical_cells+64 hex scaling guard; one public Grid | Constant is an evaluation guard, not optimum |
| Unused stock-copy API | Extra unconsumed surface | Removed TryCopyStocks; snapshot copies GetStocks after capacity preflight | Startup snapshot reserves stock bytes even for legacy frames |
| Root copy documentation/indentation | Obscured resource behavior | Two SHOULD findings corrected; independent final review clean | No unresolved MUST/SHOULD |
| Local LeakSanitizer cannot inspect /proc tasks | Default local sanitize execution fails at exit | Rerun ASan/UBSan with leak detection disabled; require normal hosted sanitizer CI | Local leak detection unsupported |

## Verification and useful artifacts

[Detailed commands, counts and limits](../workstreams/integration/phase3-validation.md)
record the actual receiving caller and ownership proof. Debug rectangular baseline
passed 16/16; combined Debug and Release passed 18/18. Local ASan/UBSan passed all 18 tests with leak detection disabled (17 in the
unfiltered run plus one permission-repaired targeted rerun). Exact-head hosted Linux/Windows Debug/Release and normal Linux ASan/UBSan each
passed 18/18 tests in five successful jobs. Hosted leak detection required no workaround. No historic result is substituted for a new run.

Rectangular and hex builds emitted byte-identical complete state for 0/8/2048
populations, initial frame and every third tick through tick 60. Independent checks
cover ledger arithmetic, spread/contact order, depletion/reinfection, stable priorities,
IDs, mapping seams/extremes/rounding, prepared lifetime and unchanged rejection.
Scoped resource and spatial ordinary C++ allocation probes observed zero calls.
Full ECS allocation freedom, private work-counter exhaustion and tick-failure injection
are not dynamically claimed. Replay is within one build, not cross-compiler identity.

Release legacy checksum remains 225000. Resource CLI complete replay conserves
10240 = 7238 stock + 2048 mobile + 954 reserve at tick 20, with 954 harvested and
1104 successful work actions. [Actual-state SVG](../concepts/phase3-state.svg) is an
owned capture, visually rendered and inspected; it is not concept art. No timing,
FPS, hierarchy benefit or game balance claim is made.

## Retrospective and reuse

Retain finite resource/contact/arbitration policy locally. Reuse H2 scalar geometry
and conservative candidates; keep IDs, exact filtering and bins in Crucible.
The consumed benefit is one checked geometry source and an HX-07 receiving pack.
The independent lane identified real domain and memory mismatches before promotion;
acceptance evidence justified a narrow internal compatibility refinement. Record
such refinements explicitly instead of preserving a plan that would reject working
worlds. A generic backend interface or copied upstream index was unnecessary.

The next recommended increment is first usable input/display: select one backend
through the existing ADR gate and consume these owned sequential frames. Keep one
presentation worker, one integration/runtime worker and the architect; defer resource
growth and hierarchy experiments until that increment or measurement supplies their
concrete need. This is a next-phase recommendation, not another dispatched sprint.

## Follow-ups

| Stable ID | Action / state | Accountable role / gate |
|---|---|---|
| P01-F03 | Finite ledger/reclamation gate satisfied; full W6 remains partial | Resource architect; structural capacity/identity/commit audit before growth or fusion |
| P02-F01 | Closed: upstream H2 and downstream receiving gates satisfied | Spatial architect; preserve independent oracle and replay on later changes |
| HX-07 | Delivered reproducible receiving fixture/trace pack in Crucible; no upstream issue/PR created | Geometry owner; consume [handoff](../workstreams/integration/phase3-validation.md) before hierarchy decisions |
| P03-F01 | Open: quantify compatibility-path/candidate/storage costs when measuring | Spatial/validation; complete controlled workload, fallback frequency and tail work |
| P01-F04 | Next recommendation: first input/display backend | Presentation; backend ADR, usable controls and visible feedback |
| P01-F05 | Deferred: structural/concurrent/measured scale | Integration/scheduling; capacity/generation/join and complete workload |
| P02-F02 | Deferred: concurrent snapshot exchange | Presentation; actual concurrent reader/upload first |
| P02-F03 | Deferred: height/mining/bridges/sphere | Game/geometry; conservation/traversability/metric gates |

## Closure

[PR 7](https://github.com/CraigHutchinson/Crucible/pull/7) merged at
`21f37867bebec46f215cf489d77eee4db63b0165`, from implementation head
`31fb1e49c3eae2652867009f8083d5688ab2e4dd`, after [CI](https://github.com/CraigHutchinson/Crucible/actions/runs/37089809757) passed all five jobs.
The local main checkout was verified clean and equal to origin/main at that merge.
This small documentation follow-up records the known merge SHA without changing code.

Worker branches/worktrees and useful local comparison/build artifacts are retained.
No unrelated branch or artifact was deleted; no upstream code or hierarchy programme
was changed. Start the next phase from merged main, not an archived worker handoff.
