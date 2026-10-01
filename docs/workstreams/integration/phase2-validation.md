# Phase 2 delivery and validation

Date: 2026-10-01. Baseline main `65bb8c4`. Delivery branch `phase2-steer-inspect`.
[Phase plan](../../phases/phase2.md), [frozen contracts](../../decisions/phase2-execution.md).
[PR 4](https://github.com/CraigHutchinson/Crucible/pull/4) is the review/merge record;
its merge commit defines the next main baseline. Do not use isolated worker heads
as a complete application baseline.

## Delivered increment

Architect plus two GPT-6.1 Sol low workers; disjoint worktrees and no worker builds.
Swarm supplies stable-ID immutable-input separation plus radial force, Euclidean
acceleration/speed limits, reusable startup scratch and transactional publication.
Simulation consumes it when ScenarioOptions steering is engaged; the absent setting
preserves the prior scenario path. Original ECS-only workload is retained unchanged.

Runtime supplies injected elapsed-time ClockDriver: exact 60-Hz scaled arithmetic,
four-tick cap, fractional carry, explicit excess discard, pause/resume/close, invalid
elapsed rejection, saturating discarded counter and a blocked boundary latch.
HeadlessSession remains exact tick/replay authority. Main consumes owned summary.

ScenarioSnapshot consumes checked Simulation state copies: sorted IDs/positions/
velocities, all canonical field slots, row-major Blight and geometry/tick metadata.
Snapshots own their storage and survive subsequent simulation ticks. Capacity
rejection writes nothing, and failed capture retains the previous complete frame.
The headless CLI and combined replay test are real consumers. The actual-state
[SVG/PNG](../../concepts/phase2-state.md) is the delivered visual stretch.

## Traceable implementation

| Commit | Work |
|---|---|
| `a4e83f0` | Consumed shared sample/settings values and phase-start ownership/topology gate |
| `88b8a1e` | Integrated runtime handoff (worker `ad269ce`) |
| `e605903` | Integrated swarm handoff (worker `18dfe8e`) |
| `916014c` | Architect composition, owned copy/snapshot, exact full-state fixture and SVG caller |

Root reviewed actual worktree diffs before integration and committed exact owned
paths. Artifacts and older wave-1 worktrees were preserved. Completed worker branches
remain historical isolated handoffs; start future work from merged main.

## Verification

- MSVC 14.51 / VS 18 Debug: unfiltered **13/13** pass, including final exporter caller.
- MSVC Release: unfiltered **13/13** pass; CLI/export smoke pass.
- Ubuntu 24.04 / GCC 15 ASan + UBSan: unfiltered **13/13** pass with leak detection
  and halt-on-UB; final exporter CLI also executed under sanitizers.
- Original 150,000-entity, 60-tick checksum remains **225000**. This is correctness
  smoke, not a 60-FPS or throughput measurement.
- Independent swarm fixture compares the rule to all-pairs traversal and independently
  derived field sampling; includes tiny, dense/coincident 2,048, boundaries, caps,
  invalid/unknown membership, ordering, alias and unchanged-on-failure cases.
- `phase2_full_state`: populations 0/8/2048, 60 ticks, matching admitted trace under
  50ms versus 10+40ms schedules and pauses. Every checkpoint compares IDs, both
  positions/velocities, each field payload and every infection byte; exact replay
  also agrees at tick 60. Retained old frames, capacity failures and legacy rejection
  are covered. Exact agreement is within a matching compiler/build configuration.
- Controlled clock fixtures exercise fraction/cap/discard, arithmetic overflow with
  carried remainder, lifecycle/counters and trace-full blocked latch without sleeping.
- SVG: stable repeated MSVC Release export hash, valid XML, 2,048 sample circles plus
  one field ring and 800 infected cells. Invalid CLI/destination failures return 1.
  Offline rendered preview visually inspected; title, bounds, dots, infection and
  legend legible. No renderer/build dependency or software install added.

Local commands: `cmake --preset debug|release`, `cmake --build --preset ... --parallel 4`,
`ctest --preset ...`; GCC sanitizer configure used `CXX=g++-15`, test/CLI used
`ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1`.
Exact-head Linux/Windows Debug/Release and sanitizer CI are required by the PR gate.

## Review and limitations

cpp-write references were applied before authoring; cpp-review and actual consumer
inspection covered public boundaries, scratch, failure, naming and ownership.
Independent worker review caught implicit snapshot moves leaving source metadata
engaged after vector transfer; copy/move are disabled. The first executable gate
caught a nonexistent timing header in worker source/test; integration uses the
existing timing.hpp. GCC missing-initializer warnings were removed by the explicit
empty optional default. No outstanding MUST finding remains under the fixed-population,
single-coordinator contract. Hot paths contain no storage growth by static inspection;
no allocator interception or performance benchmark is claimed.

Unexpected Simulation failure stops the session/clock; it does not imply rollback
of already applied boundary edits. Failure/exception branches are inspected without
adding a test-only Simulation injection seam. Grid matching exact tick-start state
is a caller precondition verified by composition; provider cannot inspect all grid
geometry/membership. No cross-compiler bitwise replay promise is made.

## Retrospective and next phase

Consolidating runtime/telemetry and architect-owned contracts/presentation kept
coordination small and supplied actual consumers. Rectangular bins stayed because
Sub0HexGrid H1 has no complete bounded candidate traversal yet; no new dependency
was pinned. Reuse disposition is in the catalog; gameplay steering remains local.

Next proposed split is resource interactions/Blight, optional standalone HexGrid H2,
and architect integration/inspection. Freeze the material/consumption ledger before
resource code. Window/input backend, concurrent snapshot exchange, fusion/shatter,
relay objective, height/deformation/sphere, changing population and measured scale
remain open. See [phase 3 proposal](../../phases/phase3.md); it is not dispatched.
