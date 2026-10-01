# Blight worker handoff validation

Base prerequisite: accepted contract commit 7577e1d, 2026-10-01.

Implemented and registered `blight_grid` / `crucible_blight_tests`, label `blight`.
The hand-calculated fixtures check exact cells and count for center/corner seeds,
horizontal/vertical propagation, empty/dense/full/single-cell grids, reseeding after
a step, idempotent seeding, invalid coordinates, invalid geometry and max_size
storage rejection before allocation.

Worker checks actually run: clang-format on all three C++ files; `git diff --check`
passed. No compile, CTest, benchmark or sanitizer run was authorized for this worker.
Architect should run Debug and Release builds, `ctest --preset debug -L blight
--no-tests=error`, the unfiltered integrated suites and supported ASan/UBSan.

cpp-review self-review: no unresolved MUST findings. L0 identifies the named optional
Simulation caller, which is pending architect wiring rather than present in this
worker's owned files. L1 uses one concrete grid owning its invariant and no new
dependency beyond Contracts. L2 failure/ownership/thread coordination are documented;
no views or ordering-only init API escape. L3 checked bounds before indexing, startup
only allocation, complete next writes and noexcept swap/observers. Coordinate loops
are intentional because neighbor indices are the algorithm. constexpr observers were
considered: the runtime-owned grid is not constructed in constant evaluation, and
out-of-line implementation preserves a small header.

Remaining gaps: aggregate scenario memory admission belongs to the caller; no
allocation counter measurement, worker/partition execution, resource consumption,
field-dependent evolution or W6 interactions. No performance conclusion is claimed.
