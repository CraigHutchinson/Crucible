# Workstream map and parallel session guide

This is the entry point for parallel Crucible sessions. Architecture is the contract
authority. A reserved folder/target does not mean its gameplay package is implemented.
Read [game design](../game-design.md) for player intent and the first playable slice;
use [concepts](../concepts/README.md) as visual exploration rather than game rules.
Active assignments are phase-specific: see the [phase workflow](../phases/README.md)
and [current Phase13 package](../phases/phase13.md). Retain these module folders while
consolidating small coupled packages under a single phase owner when useful.
Read the [hierarchy/acceleration responsibility map](hierarchy-boundaries.md) with
every brief. Spatial owns occupied traversal; upstream H geometric grouping/coverage;
Presentation view caches/policy; phase-assigned navigation owns route research.
Runtime chooses scheduling policy, Scheduling executes/joins, Integration publishes.
Experiment ownership does not transfer production module ownership.

## Ownership map

| Stream | Package | Brief | Target / current foundation |
|---|---|---|---|
| Integration | W0 | [Integration](integration/README.md) | Core; ECS world and shared wiring |
| Contracts | W1 | [Contracts](contracts/README.md) | Contracts; geometry, field edits, stable sample/state values, steering/resource settings, biomass ledger, copy destinations, timing |
| Runtime | W2 | [Runtime](runtime/README.md) | Runtime; bounded ingress, cutoff, pause/close, tick trace replay, bounded fixed-step clock and summary |
| Spatial | W3 | [Spatial](spatial/README.md) | Spatial; stable-ID bins, pinned H2 geometry and complete exact radius queries |
| Fields | W3 | [Fields](fields/README.md) | Fields; bounded radial attractor/repulsor slots |
| Blight | W4 | [Blight](blight/README.md) | Blight; double-buffered cardinal spread and exclusive prepared-step lease |
| Swarm | W5 | [Swarm](swarm/README.md) | Swarm; fixed integration and bounded immutable-input separation/radial steering |
| Interactions | W6 | [Interactions](interactions/README.md) | Interactions; finite reclamation and fixed-identity fusion/shatter; Integration coordinates commits |
| Scheduling | W7 | [Scheduling](scheduling/README.md) | Scheduling; reserved |
| Telemetry | W8a | [Telemetry](telemetry/README.md) | Telemetry; adapter target reserved; current owned summary lives with ClockDriver |
| Presentation | W8b/W9 | [Presentation](presentation/README.md) | Presentation; owned snapshots, camera/tools, SDL ScenePainter and optional offscreen Vulkan; concurrent exchange deferred |
| Validation | W10 | [Validation](validation/README.md) | Opt-in crucible_bench; ECS microbenchmark |

CMake aliases use the Crucible:: prefix. Spatial/Fields split W3 into distinct path
owners. Presentation owns snapshots before rendering. Scheduling is a separate
path owner from Runtime; changing runtime files requires a handoff.

The first four dispatch streams were Integration/Contracts, Runtime, Spatial/Fields
and Blight. Their [combined handoff](integration/wave1-validation.md) records current
evidence and remaining package gates. Dedicated branches/worktrees are retained at
.worktrees/{integration,runtime,spatial,blight}; inspect status and claims before reuse.

## Folder pattern and shared ownership

```text
docs/workstreams/<stream>/README.md     scope, prerequisites, ownership and gates
include/crucible/<stream>/             self-contained public headers
src/<stream>/CMakeLists.txt            explicit source list and dependencies
src/<stream>/                         implementation and private headers
tests/<stream>/CMakeLists.txt          local executables, fixtures and CTest labels
```

Integration/Validation use existing root wiring, integration tests and benchmarks
instead of duplicate code modules. Add design.md, decisions.md and validation.md
inside stream docs when actual decisions/evidence exist. Cross-stream decisions go
in docs/decisions; central architecture/backlog changes go through the integrator.

The integrator owns Simulation, ECS queries, main, root build inventories, pins,
presets, CI and central docs. Domain sessions edit only claimed paths and submit
shared changes as patch requests. Contract changes require the Contracts owner and
affected callers to agree. Parallel development never authorizes parallel ECS mutation.

Read AGENTS.md, architecture.md, work-breakdown.md and the stream brief. Inspect
status/worktrees/ACTIVE_WORK_LOG, then record owner, paths, branch/worktree, base SHA,
prerequisites and CPU reservation. Prefer separate worktrees and per-worktree build
directories; never share writable build trees or stage another session's files.
CPM dependency sources stay read-only. Follow the backlog task-brief/handoff checklist.

## Local build and test registration

Every stream manifest is already in the central inventory. Do not use source globs.
When adding the first compiled source, change the local manifest from:

```cmake
crucible_add_workstream(crucible_spatial Spatial)
target_link_libraries(crucible_spatial INTERFACE Crucible::Contracts)
```

to:

```cmake
crucible_add_workstream(crucible_spatial Spatial grid.cpp)
target_link_libraries(crucible_spatial PUBLIC Crucible::Contracts)
```

The helper selects STATIC with sources, INTERFACE without them. Header-only code
stays INTERFACE; never add dummy objects. Use PRIVATE for dependencies absent from
public headers, and declare only consumed dependencies. Streams inherit project
C++23/warnings and enabled sanitizer settings. Backend/pin changes need integration.

Reserved INTERFACE targets have no build artifact or direct Ninja build target.
Use the preset configure/full build to check their graph. Once compiled, a stream's
crucible_<stream> target can be built directly. Graph checks prove no gameplay math.

Add tests to the local tests/<stream>/CMakeLists.txt:

```cmake
add_executable(crucible_spatial_tests grid.cpp)
target_link_libraries(crucible_spatial_tests PRIVATE Crucible::Spatial)
add_test(NAME spatial_grid COMMAND crucible_spatial_tests)
set_tests_properties(spatial_grid PROPERTIES LABELS spatial)
```

```sh
cmake --preset debug
cmake --build --preset debug --parallel 4
ctest --preset debug -L runtime --no-tests=error
ctest --preset debug
```

Replace runtime after the stream registers tests. no-tests=error prevents an empty
reservation from masquerading as a pass. Filtered checks aid iteration; integration
requires the unfiltered suite. See CONTRIBUTING.md for Release/sanitizers. Reserve
CPU before heavy work; timings run alone per benchmarking.md.

## Completion

Load cpp-write before substantive C++ and cpp-review before integration. Add owned
values/APIs only with a named caller, never stub success or framework scaffolding.
Foundation is separate from package completion: richer steering,
resource interactions, workers, snapshot leases and rendering still require gates.
Handoff records SHAs/paths, consumer
wiring, actual test results, evidence, shared patches and limits. Update claims.

## Sub0 evolution ownership

Use the [owned-library roadmap](../reuse/sub0-roadmap.md) at every dispatch. Integration
owns ECS/pin promotion; Spatial carries HexGrid numerical/terrain requirements;
Runtime owns Pub bounded receiving; Scheduling owns Pipeline dispatch/join evidence;
Telemetry owns Log bounded diagnostic receiving. Each handoff names a real consumer,
neutral requirement and exact-version fixture. Architect prioritizes upstream work
alongside gameplay; current test-only adoption is a baseline, not a permanent design.
Paging/cache requirements come from measured world storage/residency, with lifetime
and eviction receiving before adoption. Game rules stay outside library APIs.
