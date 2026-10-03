# Crucible work breakdown for collaborative agents

Read [game design and project intent](game-design.md) before dispatch. Workstreams
serve its playable loop; numerical/resource and presentation decisions must close
at the listed gates before implementation. Concept art is illustrative.

Status: phase 5 adds a quota/deadline challenge to live inspection, finite reclamation
and verified H2 spatial reuse, 2026-10-03. W0/W1/W2/W3/W4/W5/W6 retain broader gates below;
finite stock-to-reserve work is the first W6 increment, not structural completion.
[Phase 3 evidence](workstreams/integration/phase3-validation.md) records delivered
scope; [architecture](architecture.md) and [workstream map](workstreams/README.md)
record technical boundaries and paths. Historical wave-1 evidence remains archived.

This is the capability backlog, not a fixed sprint allocation. Follow the
[phase workflow](phases/README.md) and completed [phase 5 packages](phases/phase5.md) and planned
[Phase 6 delegation](phases/phase6.md) for scope and revised ownership. Each phase reviews retain/consolidate/split/defer
choices before dispatch; package completion claims still require their full gates.

[Instancing design](decisions/render-instancing.md) scopes the next rendering
foundation: static quad, per-instance data, camera uniforms and explicit upload/
retirement ownership. Current SDL batching and the CPU spike do not deliver GPU
instancing or measured scale. Consolidate the first real backend/numerical contract,
then split platform receivers around consumed shader/resource requirements.

[Faction extensibility groundwork](decisions/faction-extensibility.md) reserves
more than two factions and controllable variants of either organism kind without
dispatching networking or new mechanics. Apply its gates before packet/ownership/
structural/remote-input contracts make two-side assumptions.

## Milestones and dependencies

| Milestone | Packages | Exit |
|---|---|---|
| M0 validated integration | W0 | Pinned API audit, reproducible builds and limitations |
| M1 headless runtime | W1, W2, W8a | Boundary commands, trace replay and lifecycle tests |
| M2 gameplay slice | W3–W6 | Sequential steering/Blight/consumption/fusion reference scenarios |
| M3 bounded parallelism | W7, W8b | Sequential/parallel agreement and ownership/join evidence |
| M4 playable measured slice | W9, W10 | Input/display works; full workload evidence published |

Critical path: W0 -> W1 -> W2 -> W3/W4 -> W5 -> W6 -> W7 -> W10.
W3/W4 can develop standalone fixtures after W1 while W2 progresses. W8a starts after
W1; W8b after W2/W8a; W9 after W6/W8b. W10 needs W7/W8b, plus W9 for FPS claims.

## Dispatchable packages

| ID / owner | Depends on | Exclusive paths | Deliverable and production wiring | Acceptance evidence |
|---|---|---|---|---|
| W0 integration | Existing foundation | CMake, presets, CI, stack test, validation docs | Audit exact pins and ECS/Pub/Pipeline/Log guarantees; remove redundant factory declaration; narrow Core stack linkage when Runtime arrives | Fresh Debug/Release, supported ASan/UBSan, stack round trip; Windows/Linux passed or outstanding explicitly recorded |
| W1 contracts / integrator | W0 | contracts/, contract tests, decision records | Scenario limits, tick/sequence/stable-ID, owned commands/results, phase summaries and snapshot values consumed by W2/W3/W4/W8 | Nonfinite/overflow dimensions rejected; lifetime/capacity contracts; every type has a named receiving caller |
| W2 runtime | W1 | runtime/, runtime tests | Bounded single-producer/consumer ingress, cutoff drain, accepted trace replay, clock/catch-up, pause/close, sequential Pipeline schedule; integrator wires main/Simulation | Queue full, atomic paint batches, arrival after cutoff, replay, catch-up cap, startup failure and shutdown during delivery |
| W3 spatial/fields | W1; integration W2 | spatial/, fields/, tests for each | Stable-ID bins, complete neighbor traversal, attractor/repulsor and painted-flow sampling consumed by W5/W6; both current and next-position rebuilds | Brute-force neighbor oracle; coincident/dense/border inputs, analytic field fixtures, capacity reporting and reused scratch |
| W4 cellular | W1; integration W2 | blight/, tests, rule record | Double-buffered cellular evolution with specified boundary/resource/field rules consumed by Simulation/W6 | Hand-calculated tiny grids, multi-step fixture, empty/full/border cases, partition-order agreement |
| W5 swarm | W2/W3 | swarm/, tests, math rule record | Separation/alignment/cohesion/field forces and fixed-step next-state integration consumed by Simulation | Independent reference math, symmetry, zero distance/velocity, finite-state invariants, clamp, two scenario scales |
| W6 interactions | W3/W4/W5 | interactions/, tests; event contracts via integrator | Consumption, one structure, fusion/shatter proposals from next-position bins; coordinator-only atomic structural commit | Competing consumers, overlapping candidates, stale IDs, proposal/entity exhaustion, exact resource balance and stable events |
| W7 scheduling | W0/W2/W6 | scheduling/, phase declarations and concurrency tests | Supported bounded executor; immutable gathered inputs, disjoint scratch partitions, stable reductions and sequential mode | One/multiple workers replay agreement, exact IDs/events/resources, numeric tolerance, failed job join, teardown stress and supported race checks |
| W8a telemetry | W1 | telemetry/, tests | Bounded phase summaries, Log adapter and drops; Runtime caller | Decoded tick/phase records, disabled path, overflow, orderly logger lifecycle |
| W8b snapshots | W2/W8a | presentation/ snapshot exchange/extraction, tests | Three owned slots, RAII read leases, newest-ready and skipped-publication counters; Simulation extracts after commit | Held reader across publications, supersession, no-free-slot, delayed upload release and shutdown with leases |
| W9 presentation | W6/W8b | presentation/ drawing/input, platform tests | Backend ADR; draw swarm/Blight/fields/structure; map input into W2; integrator wires desktop entry/dependencies | World/viewport mapping, overflow feedback, resize, stale frame indication, GPU lifetime and close; visual evidence per platform |
| W10 validation | W7/W8b; FPS also W9 | benchmarks/, capture scripts, workload fixtures and evidence docs | Preserve ECS microbenchmark; add full tick and frame scenarios at tiny/100K/150K scales | Correctness first; controlled alternating comparisons, tick p95/p99, memory/allocations, command age/drops, occupancy/worker settings, upload-inclusive frame evidence |

Each package must specify concrete numerical rules before implementation, rather
than leaving its defining behavior to a later package. W6 validates complete
resource/capacity transitions before mutation and audits ECS allocation/exception behavior before promising atomic commit. W7 cannot enable concurrency before
the sequential gameplay oracle passes. W9 keeps headless operation available and
chooses one backend only after recording platform/dependency/upload requirements.
W10 reports missed targets honestly; an isolated integration loop is not 60 FPS.

## Team waves and shared ownership

For four agents reserve one integrator slot. After W0/W1, dispatch W2, W3 and W4
against frozen contracts. The next phase consolidates bounded W5 steering,
W2/minimal W8a and architect-owned inspection/integration; see the phase plan.
W6 follows W4/W5; W7 and W9 can overlap when prerequisites pass. Benchmark timing
runs alone on its reserved host. With fewer agents combine roles, preserving gates.
An unmet prerequisite permits fixture/design preparation, not a completed integration.

The integrator exclusively owns Simulation header/source, ECS query declarations,
main.cpp, root CMake, pins, CI and central docs. Contracts belong to W1 with integrator approval of cross-stream changes. Domain agents own
only listed compartments and their tests. Shared changes arrive as patch requests;
no competing edits. Parallel agent development does not imply parallel ECS mutation.

Claim paths and CPU reservations in [ACTIVE_WORK_LOG.md](ACTIVE_WORK_LOG.md). Prefer
one branch/worktree per package, record starting SHA and prerequisite SHAs, and never
stage another agent's work or clean their artifacts. Verify claims before heavy tests;
file-disjoint benchmarks still contend for hardware.

## Task brief and integration checklist

Every delegated brief includes:

1. Package ID, bounded outcome, base SHA and completed prerequisites.
2. Exclusive writable paths, read-only contracts and excluded scope.
3. Named production consumer and integration owner.
4. Input/output ownership, thread/capacity/failure/lifetime rules and oracle fixture.
5. Acceptance commands, required behavior and platform limitations.
6. Handoff: changed paths/SHAs, tests actually run, evidence paths, unresolved gaps,
   shared-surface patch requests and review findings addressed.

Load cpp-write before substantive C++ authoring; run cpp-review before integrating
new/substantially changed C++. Validate lifecycle/concurrency independently from math.
Merge prerequisites first, wire the real caller, run the combined unfiltered suite,
and update package status only when both caller and acceptance evidence exist.
A standalone fixture or passing compile is not completion. Keep benchmark runs
separate from sanitizer/build activity; follow benchmarking.md.

## Design review and assumptions

The design pass checks named consumers (L0), acyclic dependencies/ownership (L1),
and boundary/failure contracts (L2); it does not claim a code-style or runtime test
pass. Read-only independent review verified ECS query/component limits and Pipeline
orphan/executor behavior against pinned source, and identified the stale factory
comment in the stack test. The design incorporates those constraints.

Baseline choices made here are a bounded rectangular world, 60 Hz/four-tick catch-up,
a mutex value ring and three snapshot slots. They are implementation defaults to
validate, not measured optimal values. Gameplay tuning, graphics choice and dependency
threading guarantees remain explicit package gates. Human review should assess
product fit and platform-specific GPU/concurrency assumptions before release.
