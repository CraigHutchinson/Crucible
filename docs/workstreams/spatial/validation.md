# P3-02 spatial handoff and validation

Base: 71c14c0; branch phase3-spatial. Changes own spatial include/src/tests/docs and
local manifests only. Architect owns dependency/root wiring and integrated replay.
Sub0HexGrid receiving pin: aaae5c2fc5731a23db94fa947bbb0182d0ea69fd.
No new public APIs or shared contract changes. Local spatial target links the package
publicly because Grid's private value storage requires its geometry headers.

Existing spatial tests retain invalid input/startup, shuffled IDs, dense/coincident,
border/clamp, maximal finite radius and unchanged-on-rebuild-failure coverage.
Occupancy assertion now uses actual layout-specific expected cells; query oracle
remains independent. Added spatial_hex_queries exhaustively scans small worlds and
hex edge/vertex/adjacent-float fixtures at ordinary/tiny/large scales, checks inclusive
zero/world-wide/max-finite radii, sample-order independence and unique IDs. Independent
long-double nearest-center lattice search verifies normal-domain mappings without
production conversion/candidate helpers. Physical extremes include denorm_min,
min and max float cell scale; elongated world covers private startup memory guard.
Rounding changes cover startup/rebuild/query/restored-environment compatibility.

spatial_allocation_reuse intercepts ordinary global C++ new/new[] in a standalone
single-threaded executable and scopes recording around actual Grid rebuild/query/
rejection loops for hex and rectangular compatibility. It checks zero allocations;
it does not claim OS residency, every C library allocation, aligned allocation or
full-tick allocator behavior.

Focused execution under the architect's CPU reservation: GCC g++ -std=c++23 -O0
-g -Wall -Wextra -Wpedantic, with Crucible and pinned Sub0HexGrid include roots,
compiled Grid.cpp/GridHexTests.cpp/GridAllocationTests.cpp as three independent
executables. Each linked src/spatial/Grid.cpp, src/contracts/GridConfig.cpp and
upstream PointyLayout.cpp/regions/AxialRegion.cpp/candidates/CandidateCells.cpp.
All **3/3** executables passed with no compiler warnings; allocation interception
reported no allocation failure (zero recorded allocations). Temporary outputs live
in build/spatial-focused. No timing or cross-compiler bitwise identity is claimed.
Architect still runs registered spatial labels and supported unfiltered
Debug/Release/sanitizers plus integrated baseline/candidate full-state replay.

## cpp-write / cpp-review

Loaded both skills and all four generic C++ references; no unity overlay exists.
Project C++23/.hpp/standard-library conventions take precedence. Plan review retained
one owning concrete Grid and existing APIs; numerical/failure/payload contracts live
in decisions.md. Self-review L0: existing Simulation/Steering are actual consumers;
L1: geometry dependency is acyclic and no new abstraction; L2: unchanged bounded
ownership/ID/query surface; L3: standard algorithms, preallocated vectors, optional
checked geometry and explicit environment fallback. No outstanding MUST/SHOULD
finding in the authored surface. Independent architect code/numeric review and
execution remain separate gates; this is not an independent self-review claim.

Architecture/promotion judgment hands off to architect review. No concurrency change
is introduced; the existing exclusive synchronous owner remains required.
