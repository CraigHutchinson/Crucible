# Crucible repository guidance

## Standing design methodology: top-down and bottom-up

User direction, 2026-10-05: develop the end-consumer experience and technical
foundations together, iterating until they meet at consumed contracts. Every phase
must balance both directions; technical groundwork alone is not the product plan.

Start top-down with a bounded concept or playable spike of the intended Crucible
experience: graphics, player interactions/UI and sound as applicable. Record the
observable behavior, fidelity, feedback, timing and ownership the consumer needs.
Use actual rendering, native interaction and audio receiving where supported;
identify conceptual/synthetic examples and participant evidence separately.

Work bottom-up from verified ECS, delivery, orchestration, diagnostics, geometry
and other existing sub0 capabilities. Feed measured costs, capacity/lifetime
limits and failure behavior back into the consumer concept. Weigh the intended
experience and engineering evidence together; revise either side as needed.

Meet in the middle through the smallest useful vertical slice: named consumer,
explicit interface/storage/ordering/error contract, end-to-end receiving and
review. Feed concrete product-neutral requirements and minimal reproductions
upstream to the appropriate Sub0 library; keep game policy local. Do not add
unconsumed APIs, copied infrastructure or speculative fidelity merely to match a
concept. Each phase plan/review records both directions, their reconciliation,
open requirements and how the findings shape the next iteration. Follow
docs/phases/README.md for the phase contract.

Interactive review follows automated receiving. Before the user's next run,
provide concise steps, expected results and the matching passed automated gates.
Use manual sessions to review comprehension, presentation and usability; reserve
computer control mainly for diagnosing a failed manual workflow. Clearly identify
any behavior whose automation or native receiving is still pending. Review root
docs as a fresh viewer: include a short, complete feature/usage checklist, launch
and controls, optional capabilities and their prerequisites, limits and review links.

The first milestone is validated integration of the sub0 stack. Resolve dependency,
toolchain, API, lifetime and build failures before adding gameplay systems.

Use clean C++23, RAII, explicit ownership and value semantics. Keep rendering and
platform APIs outside simulation code. Declare system data access and DAG dependencies;
do not assume the ECS or pub/sub broker is safe for arbitrary concurrent mutation.
Drain bounded input commands at tick boundaries. Structural changes must occur after
workers finish. Define overflow, teardown and borrowed-state lifetime behavior.

Use target-scoped CMake requirements and CPMAddPackage in cmake/Dependencies.cmake.
Pin full dependency commits in cmake/DependencyPins.cmake; Sub0Pub must come from
v2 and Sub0ECS from master (v2 is merged there and its branch deleted). Disable
dependency tests, examples, tools and benchmarks in consumers.
Use CMakePresets.json for local work and CI. Do not disable checksum or TLS verification.

Follow CONTRIBUTING.md and docs/benchmarking.md. Keep performance measurements advisory.
Add meaningful behavior and lifetime tests for integration changes; run Debug, Release,
and ASan/UBSan on supported platforms. Document what actually passed and limitations.
Run CTest through `python scripts/run_tests.py --preset <preset>`; its narrow
artifact preflight repairs restored executable modes before spending suite time.
Do not claim 100K+ entities at 60 FPS from an isolated integration loop.

## Parallel workstream sessions

Follow docs/phases/README.md. At every phase start reassess active workstream
division and record retain/consolidate/split/defer decisions, a small consumed
increment, shared contracts, core gates and bounded stretch in the phase plan.
Phases 1–8 have published increments; Phase8 recovered source/concept images merged
in PR21. Read docs/phases/README.md for current publication and receiving scope.
Phase9 investigation merged in PR22. Phase10 receives the selected structural loop;
read its current plan and the owned-library roadmap. No backlog implies dispatch.
Read docs/sprint-reviews/README.md and the latest review
before the next phase; carry unresolved follow-up IDs into its plan. Create an
in-progress phase-NN.md from the review template and complete it at phase close
with findings, evidence, retrospective, follow-up owner/gates and merge baseline.
Review docs/reuse/README.md at phase start/close. Feed concrete findings, fixtures
and measured improvements into existing sub0 projects; propose extraction only
around consumed product-neutral boundaries. Sub0HexGrid has standalone groundwork;
H2 finite regions/complete candidates are delivered upstream; Crucible adoption remains
backed by its own query/lifetime/replay evidence and a full H2 pin.
Respect quota: architect plus at most two workers, minimal repeated reviews/builds,
and core correctness/refinement before stretch. Close each completed phase with a
reviewable pushed PR, exact-head CI, merge to main and a verified baseline.
Capture and visually inspect examples of actual changed behavior at every iteration
where viable; follow docs/phases/README.md for provenance, reproduction and explicit
reasons when a visual does not apply. Include them in the sprint review.

Read docs/game-design.md for product intent and first-playable scope before choosing
implementation behavior. docs/concepts distinguishes generated visual proposals
from actual-state exports; neither defines simulation rules. Freeze numerical/resource rules with fixtures and
record consequential gameplay decisions before dependent streams implement them.

Read docs/workstreams/README.md and the stream brief. Claim paths and heavy CPU runs
in docs/ACTIVE_WORK_LOG.md before work. Streams own include/crucible/<stream>,
src/<stream>, tests/<stream> and docs/workstreams/<stream>. Scheduling and Runtime
are separate owners. Contract changes require Contracts owner and affected callers.
For hierarchy/acceleration, read docs/workstreams/hierarchy-boundaries.md.
Require one defining owner per contract and one edit owner per artifact. Private
spikes do not absorb another module; Integration assigns navigation research until
an evidence-backed ADR defines a production home. Preserve frozen gather epochs,
coherent publication and separate view/gameplay semantics.
Any added cross-cutting axis requires an all-stream responsibility audit before
dispatch: one defining owner/caller, disjoint paths, lifetimes and handoff gates.
Update the affected briefs, responsibility map and phase record together.

Integrator owns Simulation, ECS queries, main, root inventories, pins, presets, CI
and central docs. Submit shared edits as patch requests. Use local CMakeLists.txt
with explicit sources and stream CTest labels; no globs, dummy objects or stub APIs.
INTERFACE reservations are boundaries, not completed gameplay. Prefer per-session
worktrees/build trees and preserve other sessions' files/artifacts.

Load cpp-write before substantive C++ and cpp-review before integration. Completion
requires a production caller and actual acceptance evidence.
