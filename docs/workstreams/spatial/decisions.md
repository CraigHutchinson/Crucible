# P3-02 receiving decision: hex bins with complete compatibility

The physical GridConfig world remains its closed float rectangle. The evaluation
layout is pointy-top, origin (0,0), positive-y r, with double circumradius
cell_size/sqrt(3); cardinal Blight and movement bounds are unchanged. Crucible owns
stable IDs, bins, validation, exact point filtering and output scratch. It consumes
the pinned Sub0HexGrid H2 scalar package, never the application-only SpatialIndex.

## Covering region proof

The constructor creates a checked broad axial metadata rectangle from -INT32_MAX to
INT32_MAX on both axes. Its (2*INT32_MAX+1)^2 count fits uint64; no broad storage or
traversal is performed. Each physical corner is given to H2 CandidateCells with
radius zero. Its first and last cells are observed with O(1) checked slicing. Take
the componentwise minimum/maximum over the four padded intervals as resident bounds.

H2 Decision 0002 outward-rounds every world normalization, affine q/r operation and
its proved two-cell cube-repair envelope. For points in the physical rectangle,
world x/y lie between the corner extrema; q's extrema are at opposite corners and
r's extrema are at the horizontal edges. Monotonic IEEE binary64 operations imply
the outward endpoint bounds enclosing each corner's exact-input computed mapping
also enclose the corresponding extrema of every interior computed mapping. Union
bounds therefore include every rounded cell in the physical rectangle. This is a
reuse of H2's proved interval machinery, not bare nearest-cell corner mapping.

If a corner candidate interval touches a broad bound, clipping could hide an
unrepresentable mapping; the constructor declines hex geometry. Unsupported H2
arithmetic also declines it. Accepted region count must fit size_t, each container,
C+1 and complete checked byte payload. Geometry calculations require round-nearest.

## Compatibility and bounded startup scaling

A dense axial covering rectangle for a tall thin physical world can grow quadratically
with height. The architect approved checked hex count <= 4*physical_cells+64 as an
evaluation memory-scaling guard; it is not a measured optimum or a gameplay budget.
If this guard, mapping domain, rounding environment or byte limits cannot be met,
Grid retains its original rectangular indexing internally. There is one concrete
public Grid, no public backend selector, virtual interface or duplicate owner.
Scratch accommodates both address domains, so later environment changes require
neither allocation nor new public state.

Each rebuild validates/sorts/clamps into pending scratch, maps every sample before
publication, and builds one complete committed index. Nonnearest rebuilds use
rectangular assignments; a failed hex mapping switches all pending assignments to
rectangular. Invalid sample input preserves the prior committed mode and bins.
Nonnearest queries over committed hex bins perform an exact full scan. A restored
nearest environment still queries committed rectangular bins as rectangular until
another rebuild; it never mixes a new assignment with an old snapshot.

H2's supported query arithmetic is narrower than Crucible's finite float radius
contract. Unsupported valid queries scan every committed sample. Filtering retains
exactly double dx*dx+dy*dy <= double(radius)*radius and ascending SampleId output.
No silent truncation, no per-call allocation, no retained input borrow. Query spans
expire on next query/rebuild. All accesses require the existing exclusive caller.

## Receiving benefit and promotion gates

The consumed benefit is replacing locally maintained normal-domain assignment and
candidate geometry with the independently packaged H2 kernel, providing reproducible
fine hex assignments for HX-07. It is a reuse/receiving-contract benefit, not a
speedup claim. Spatial fixtures prove query behavior locally; architect integration
must still prove steering/resource replay and supported combined builds before
production promotion. If those fail, retain rectangular production and this evidence.
There is no hierarchy, navigation, renderer or sliced-tick implementation here.
