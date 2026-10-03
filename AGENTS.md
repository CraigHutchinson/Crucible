# Crucible repository guidance

The first milestone is validated integration of the sub0 stack. Resolve dependency,
toolchain, API, lifetime and build failures before adding gameplay systems.

Use clean C++23, RAII, explicit ownership and value semantics. Keep rendering and
platform APIs outside simulation code. Declare system data access and DAG dependencies;
do not assume the ECS or pub/sub broker is safe for arbitrary concurrent mutation.
Drain bounded input commands at tick boundaries. Structural changes must occur after
workers finish. Define overflow, teardown and borrowed-state lifetime behavior.

Use target-scoped CMake requirements and CPMAddPackage in cmake/Dependencies.cmake.
Pin full dependency commits in cmake/DependencyPins.cmake; Sub0Pub and Sub0ECS must
come from v2. Disable dependency tests, examples, tools and benchmarks in consumers.
Use CMakePresets.json for local work and CI. Do not disable checksum or TLS verification.

Follow CONTRIBUTING.md and docs/benchmarking.md. Keep performance measurements advisory.
Add meaningful behavior and lifetime tests for integration changes; run Debug, Release,
and ASan/UBSan on supported platforms. Document what actually passed and limitations.
Do not claim 100K+ entities at 60 FPS from an isolated integration loop.

## Parallel workstream sessions

Follow docs/phases/README.md. At every phase start reassess active workstream
division and record retain/consolidate/split/defer decisions, a small consumed
increment, shared contracts, core gates and bounded stretch in the phase plan.
Phases 1/2 are delivered; phase 3 work packages are in docs/phases/phase3.md.
Architecture preparation has started; implementation dispatch remains explicit. Do not infer
dispatch from a backlog. Read docs/sprint-reviews/README.md and the latest review
before the next phase; carry unresolved follow-up IDs into its plan. Create an
in-progress phase-NN.md from the review template and complete it at phase close
with findings, evidence, retrospective, follow-up owner/gates and merge baseline.
Review docs/reuse/README.md at phase start/close. Feed concrete findings, fixtures
and measured improvements into existing sub0 projects; propose extraction only
around consumed product-neutral boundaries. Sub0HexGrid has standalone groundwork;
H2 finite regions/complete candidates are delivered upstream; Crucible adoption remains
gated on its own query/lifetime/replay evidence, not pinned yet.
Respect quota: architect plus at most two workers, minimal repeated reviews/builds,
and core correctness/refinement before stretch. Close each completed phase with a
reviewable pushed PR, exact-head CI, merge to main and a verified baseline.

Read docs/game-design.md for product intent and first-playable scope before choosing
implementation behavior. docs/concepts distinguishes generated visual proposals
from actual-state exports; neither defines simulation rules. Freeze numerical/resource rules with fixtures and
record consequential gameplay decisions before dependent streams implement them.

Read docs/workstreams/README.md and the stream brief. Claim paths and heavy CPU runs
in docs/ACTIVE_WORK_LOG.md before work. Streams own include/crucible/<stream>,
src/<stream>, tests/<stream> and docs/workstreams/<stream>. Scheduling and Runtime
are separate owners. Contract changes require Contracts owner and affected callers.

Integrator owns Simulation, ECS queries, main, root inventories, pins, presets, CI
and central docs. Submit shared edits as patch requests. Use local CMakeLists.txt
with explicit sources and stream CTest labels; no globs, dummy objects or stub APIs.
INTERFACE reservations are boundaries, not completed gameplay. Prefer per-session
worktrees/build trees and preserve other sessions' files/artifacts.

Load cpp-write before substantive C++ and cpp-review before integration. Completion
requires a production caller and actual acceptance evidence.
