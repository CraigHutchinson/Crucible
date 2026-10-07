# Pure camera and discrete field intent

Camera2D maps the finite GridConfig world to a supplied immutable logical viewport,
preserving aspect ratio. Native input adapters perform pixel/DPI conversion first;
pure values contain no window handles or renderer dependencies. The receiving
inspector uses logical canvas 1280x720 and viewport {24,96,1232,520}; the constructor
accepts other rectangles to keep affine fixtures independent.

Fit centers the world and restores relative zoom one. Logical drag moves the center
oppositely; zoom preserves the underlying double-precision anchor until world-edge
constraints intervene. Zoom clamps to 1..16. Unzoomed axes stay centered; zoomed axes
keep their visible span inside the world. Reverse mapping rejects nonfinite input,
half-open viewport exterior and world exterior/letterboxing, rather than turning
outside clicks into edge placement. Forward projection is unclipped so the painter
owns viewport clipping. Invalid operations preserve camera state. ResetFit is consumed
by fit/restart controls; no mutable viewport API is introduced without a caller.

FieldTool settings explicitly select one bounded slot. Attract/repel require positive
finite radius and magnitude and produce signed owned FieldEdit values through the
existing validation contract. Remove ignores geometry and emits a canonical remove.
The builder does not admit commands or inspect stale field snapshots. The desktop
coordinator owns preview, admission status, queued sequence confirmation and fresh
restart; successful queue admission is distinct from committed world state.

Independent fixtures use an 8x4 world in a 400x400 viewport: scale 50, world origin
at (0,100), center at (200,200); anchored 2x zoom moves center to world (3,2), and a
100-unit drag reaches the left center constraint (2,2). Separate tests cover the
production offset viewport, inversion, unclipped projections, edge constraints,
zoom bounds, invalid-preservation, checked constructor arithmetic and signed/canonical
field intent. These pure fixtures do not claim native DPI/touch/lifecycle coverage.

Authoring applied cpp-write and self-review applied cpp-review plus the four shared
references; project C++23, standard-library and .hpp conventions govern. No unresolved
MUST finding remains. Camera state owns no borrows, helpers express its affine and
center invariants, and FieldTool reuses FieldEdit validation. Cheap camera observers
are constexpr. The architect wires actual inspector/painter callers and registers
source/test manifests. No worker build/test/sanitizer run is claimed here; combined
executable and native/software gates belong to integration. git diff --check passed.
