# Sub0HexGrid: standalone groundwork and adoption specification

Status: standalone initialization, 2026-10-01. The user supplied the existing empty
[Sub0HexGrid repository](https://github.com/CraigHutchinson/Sub0HexGrid) and authorized
requirements, architecture and initial code groundwork. H0 architecture is 9a4e604;
H1 kernel is e9d4231, reviewed/validated locally and delivered through
[PR 1](https://github.com/CraigHutchinson/Sub0HexGrid/pull/1). No Crucible dependency pin
or migration has been added. Catalog entry: [R05](README.md).

The first library increment supplies checked axial neighbors, wide distance,
validated pointy-top world mapping, an example, independent numeric fixtures and
a relocated CMake package consumer. MSVC Debug/Release and WSL GCC 15 ASan/UBSan
each pass 3/3 tests. Detailed requirements/architecture and evidence now live in
the standalone project. Finite regions and complete radius candidates remain its
next gate before replacing Crucible's spatial backend.

The user's [terrain/world extension](../decisions/terrain-and-world-extension.md)
is also preserved in the library's future-surfaces requirements: planar height
first, possible near-uniform spherical subdivision later. Neither is implemented.

## Purpose and receiving consumer

Provide product-neutral hexagonal coordinates, topology and world-space geometry
for C++ consumers. Crucible's spatial module is the first proposed receiving caller:
it assigns swarm positions to cells and enumerates candidate cells for complete
radius queries. Stable sample IDs, bin storage, neighbor result ownership and swarm
behavior remain Crucible responsibilities in the initial extraction boundary.

A cellular world module could also consume the topology, but moving Blight to hex
is a separate gameplay decision. Choosing hex bins for continuous swarm positions
does not automatically replace Blight's current cardinal spread or its fixtures.

## Proposed minimal boundary

| Capability | Candidate library responsibility | Application responsibility |
|---|---|---|
| Coordinates | Value-based axial/cube coordinate representation and validated arithmetic | Mapping coordinates to application-owned storage/IDs |
| Topology | Six-neighbor enumeration and distance/ring/region operations only as consumed | Spread rules, traversal costs, terrain policy, units and pathfinding decisions |
| Geometry | Explicit layout/orientation, cell scale/origin, center/corner mapping and world-to-cell selection | Camera/viewport mapping, rendering, input interpretation and world clipping policy |
| Candidate traversal | A bounded method to enumerate cells intersecting a query region when required by the first caller | Exact point-distance filtering and complete stable-ID results |

Start with the operations the first caller actually uses. Rings, shapes, rotations,
paths or serial formats are not pre-added simply because a grid library could offer
them. A generic owning world/container and concurrent spatial index are outside the
minimal library. Runtime/template orientation choice is decided from consumer needs,
not exposed as two options before either has a caller.

## Decisions required before implementation

- Choose coordinate scalar/range and explicit overflow failure; do not infer storage
  capacity from unbounded coordinate mathematics.
- Choose pointy/flat orientation, define scale (radius/width convention), origin and
  the exact finite region shape. A hex cell topology can live in a bounded rectangular
  world; the boundary must be specified independently.
- Define coordinate-to-world and world-to-cell behavior at edges, vertices, ties,
  negative positions and large finite coordinates. Use a documented deterministic
  tie rule; reject nonfinite/invalid geometry before mutation.
- Specify complete candidate traversal for inclusive radius queries, then exact
  point filtering. No assumption that six immediate neighbors cover a large radius.
- Choose stable enumeration order and storage mapping without coupling to ECS row
  offsets or Crucible's SampleId. Document borrowed/output capacity semantics.
- Separate the continuous swarm index decision from cellular gameplay adjacency,
  world edge clipping and terrain collision. Each migration needs its own fixture.

Algorithms and conversion formulas must be checked against primary/reference sources
before implementation, with conventions derived for the chosen layout. This proposal
does not prescribe formulas from recall. Record sources and uncertainties in the
design handoff; avoid a broad literature task under a small phase budget.

## Quality and acceptance

Proposed baseline is C++23, explicit value/ownership semantics, no hidden global
state, target-scoped CMake and a separately consumable package. Coordinate/topology
operations should be allocation-free; region/query enumeration needs caller-owned
or startup-sized output and explicit capacity failure. Do not claim these properties
until they are implemented and checked.

Required fixtures: origin and negative coordinates; each of six directions;
coordinate/world round trips; exact edge/vertex ties; symmetry and distance/ring
properties for consumed operations; finite/nonfinite/extreme values and arithmetic
overflow; bounded region clipping; complete radius candidates against independent
brute force; deterministic ordering and insufficient output capacity.

Require cpp-write before public APIs and cpp-review of the design/code, supported
Debug/Release/sanitizers and an independent package consumer. Crucible must validate
its actual adopted grid queries against brute force at tiny/dense/border scales and
replay its integrated scenario after migration. Performance conclusions require
controlled measurements on real scenario inputs; six-neighbor topology alone says
nothing about the cost of a point-radius query.

## Extraction and migration gates

1. Record a topology ADR and one consumed geometry contract within phase 2's budget.
   Retain the rectangular baseline if unresolved; do not stall useful inspection/clock work.
2. Verify the first receiving caller and any genuine second consumer; compare existing
   reusable options before choosing a new project. A second consumer strengthens the
   extraction case but should not be invented for paperwork.
3. Standalone H0/H1 initialization is authorized and delivered separately. Agree the
   next region/candidate facet and its receiving caller; no new generic surface API
   or application migration is implied by the initialized repository.
4. Validate that facet and independent consumer, then integrate the pinned result
   into Crucible through an explicit spatial migration package.
5. Re-record query/replay evidence and catalog links. Remove duplicated geometry only
   after equivalence/adopted semantics are verified. Update Blight separately if chosen.

Useful phase 2 output can be just the validated design boundary and a decision to
extract next. Do not spend the core increment's remaining review quota on a broad
hex engine, graphics mesh system, navigation framework or speculative optimizations.
