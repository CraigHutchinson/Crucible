# Sequential owned state inspection

Phase 2 implements the minimum read model used now by the headless driver, exact
replay fixture and SVG export. `Simulation::TryCopyState` checks all three capacities
before writing, then copies sorted SampleState values, field slots and row-major
Blight cells into caller-owned, mutually disjoint spans. Used lengths, geometry and
Simulation completed-tick identity are returned by value. Extra destination tails
remain untouched. Legacy mode and insufficient capacity write nothing.

Only the exclusive coordinator calls this API; it is not safe for a concurrent reader.
A copy observes current boundary edits, including edits made since the last tick.
Capture after a successfully completed session boundary for a completed replay frame.
Simulation tick identity counts all Simulation ticks; the session counter counts
its own boundaries. Production and replay start with fresh matching simulations.

ScenarioSnapshot owns startup-sized arrays and metadata. Capture allocates nothing;
a rejected capture preserves its last frame. GetSamples/GetFields/GetBlight borrow
only snapshot storage until the next successful capture or destruction. Copy/move
are disabled to preserve stable ownership and avoid moved-from metadata retaining
lengths after its vectors were moved. A captured frame survives later simulation ticks.

Fields retain active set payloads or canonical remove values in slot order. Blight
is byte-valued display state, not a resource balance or terrain-height representation.
Sample IDs represent this fixed population; destruction and generation semantics
remain a separate future contract.

The CLI `--export-svg path.svg` writes one actual tick-20 frame outside the tick loop,
using classic numeric locale and stable order. It reads only ScenarioSnapshot.
Cyan dots show samples, red cells show infection, green/orange rings show attraction/
repulsion. The prototype has no resource consumption or mission behavior. Export
I/O can allocate and fail; it is outside the simulation hot path and reports failure.

No GPU/window/input backend, snapshot exchange, reader lease or upload retirement
exists yet. Those W8b/W9 gates need real concurrent/display consumers first.
