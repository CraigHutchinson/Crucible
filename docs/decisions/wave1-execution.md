# Initial collaborative implementation wave

Date: 2026-10-01. Architect: primary agent. Workers: GPT-6.1 Sol, low reasoning,
as requested. At most three workers run alongside the architect in this session;
Blight follows the initial contract handoff. Four dedicated worktrees are reserved.

Execution complete for this first increment. [Combined handoff and verification](../workstreams/integration/wave1-validation.md)
record integrated commits, findings, test results and the next dispatch boundaries.

## Ownership and communication

Root owns central docs, Simulation, main and shared/root build wiring. The integration
worker owns W0 evidence and W1 contracts. Runtime, spatial/fields and Blight own only
their stream paths in dedicated worktrees. No worker edits dependency checkouts,
ACTIVE_WORK_LOG or other streams, changes pins, installs software, runs benchmarks,
starts heavy builds or spawns agents. Architect handles Git handoff and builds.

Workers use collaboration messages to announce proposed contracts, accepted changes,
blocked prerequisites and review-ready output. Send relevant decisions to root and
affected peers, including a short reason, affected symbols and action required.
Architect relays messages for the queued worker. Avoid repeated polling/status chatter.
Each worker records durable design/validation notes inside its own docs folder.

## Scope and common contract gate

Deliver the first integrated headless increment, retaining the existing Simulation
constructor/workload as a compatibility path. No flocking, fusion, parallel executor,
graphics or performance promotion in this wave. All new APIs need a named production
caller in the architect's combined integration, not just isolated tests.

Integration defines only consumed grid/scenario/command value contracts after reading
exact pins and consulting peers. Grid geometry is a finite rectangular domain with
clamped positions and no wraparound. Sizes/capacities must be validated for arithmetic
overflow and finite values; buffers are sized once. Entity identities are stable values,
never pointers/row offsets. Integration publishes the minimum agreed header shapes
before runtime/spatial implementation; root integrates that handoff before workers
depend on it. No speculative snapshot/structure/event contracts.

Runtime provides bounded owned input values, admission status, monotonic sequence and
tick cutoff, close/pause/replay behavior with explicit overflow tests. A real headless
session consumes admitted commands at tick boundaries. Begin sequentially; no Pub
threading assumption or threaded executor implementation. Optional Pub adaptation is
gated on exact audit and lifetime evidence, not required for the first increment.

Spatial/fields provides a reusable bounded stable-ID grid and radius traversal proven
against brute force, plus finite attractor/repulsor sampling and bounded owned field
edits. Agree command payloads with runtime/integration first. Root wires field edits
and sampling into the optional scenario tick, and consumes spatial diagnostics from
the headless scenario. No invented general query framework or incomplete neighbor results.

Blight defines its first simple cellular rule in a decision record and hand-calculated
fixtures before implementing current/next state. Prefer a monotone cardinal-neighbor
spread rule for the first bounded prototype; explicitly record that consumption/resource
accounting belongs to later interactions work. Root wires the completed step/observation
into the optional scenario tick. No external algorithm is implemented from recall.

## Handoff and verification

Workers load cpp-write references before code and self-review with cpp-review. They
report exact changed paths, contracts, tests requested and limitations, then stop
editing for architect review. Root snapshots status/diff, commits exact owned paths,
integrates prerequisites before dependents and reviews the combined result. Workers
do not commit/cherry-pick shared contracts without root coordination.

Debug/Release and supported ASan/UBSan suites run on serialized reservations; root
uses available MSVC and WSL GCC15. Record actual results and missing gates. Source
work may proceed in parallel with tests, but no worker shares a build tree. Performance
claims remain absent. Worktrees/artifacts stay available unless cleanup is explicitly
safe and no live task needs them.
