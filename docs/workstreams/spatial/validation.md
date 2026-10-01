# Spatial handoff and validation

Changed owned files: include/crucible/spatial/Grid.hpp, src/spatial/Grid.cpp,
src/spatial/CMakeLists.txt, tests/spatial/Grid.cpp, tests/spatial/CMakeLists.txt,
and this stream README/design/validation. Base 978190c; worker made no Git mutations.

The architect-owned Simulation optional scenario is the planned production caller:
construct Grid once, gather stable samples into reused storage, check TryRebuild,
immediately consume TryQuery results and GetOccupiedCellCount for headless diagnostics.
Contracts required: GridConfig::TryValidate(), SampleId comparison/value, Position.
The caller owns a scenario memory budget; Grid checks individual container size limits
and allocating construction can fail. No heap allocations occur in the written rebuild
or query paths; this is source review, not an instrumented allocation measurement.

Fixtures register spatial_grid with CTest label spatial. They compare complete sorted
results against brute force for 1540 samples including 1024 coincident samples, every
cell of a 7x5 rectangle, four boundaries, clamped external positions, shuffled IDs,
zero and arbitrary/multi-cell radii including maximal finite radius. Rejection preserves
committed state for duplicate IDs/nonfinite positions/excess capacity. Startup overflow,
empty, single-cell and query validation are covered.

No compilation or tests were run by this worker; root owns CPU reservations. Request
Debug and Release spatial_grid, then supported ASan/UBSan and unfiltered combined suites.
No throughput/performance conclusion is offered. Query result spans expire on next
query/rebuild and require immediate consumption; access is externally synchronized.

cpp-review self-review against base-owned changes: L0 production wiring is explicitly
pending architect integration; L1 concrete owning Grid has no speculative interface;
L2 complete result/lifetime/rejection contracts are documented; L3 nodiscard/noexcept,
std::span, ranges, owned startup scratch and matching class filenames checked. No open
MUST/SHOULD findings in the worker surface. Review integration callers before landing.
