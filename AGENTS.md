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
