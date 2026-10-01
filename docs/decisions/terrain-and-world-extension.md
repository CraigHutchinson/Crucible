# World patches, terrain height and later spherical worlds

Date: 2026-10-01. Direction supplied by the user; numerical rules and implementation
remain future increments. Keep this record when reviewing phase scope and reuse.

## Near-term world intent

The game shows a small local view of a larger world. Begin with a bounded planar
patch, with varying terrain height for visual interest and eventual movement cost.
This is a height field over a surface, not a commitment to arbitrary 3D flight or
volumetric terrain. The current simulation remains flat until a consumed terrain
increment defines state, sampling, traversal and owned display data.

World resource may be consumed/mined. Removing that material can lower terrain and
form depressions. Fusing nanites may create a permanent terrain element, such as a
bridge, rather than only the currently proposed reversible holding lattice. These
are distinct transition types: a permanent bridge must not silently inherit a
lattice's shatter/recovery behavior.

## Decisions before a terrain increment

- Define height samples (cell centers, vertices or another representation), units,
  finite ranges/floor and continuity at cell/patch edges. Keep world coordinates
  separate from viewport coordinates and local render precision.
- Define resource-to-height conversion and a material ledger. Mining lowers only
  the mass actually removed; contention, exhausted capacity or failed mutation
  cannot consume twice or create terrain for free.
- Define traversal/gradient costs, maximum traversable step/slope, depressions and
  bridge support/connectivity. Visual height must agree with traversability once
  it affects movement. Preserve deterministic neighbor/order and replay evidence.
- Define permanent fusion's material cost, lifecycle and any later erosion/removal.
  Validate complete structural/material transitions before commit, separately from
  the mobile swarm's reversible lattice and its recovery loss.
- Copy terrain changes into owned snapshots; add fixtures for borders, depletion,
  movement cost, blocked paths, competing miners and failed bridge creation.

Terrain height, resource balance and construction policy stay in Crucible initially.
Geometry/topology operations with real independent consumers may feed reusable sub0
libraries; see the [reuse catalog](../reuse/README.md). Do not start streaming,
deformation, bridge/pathfinding or physics work just to enrich the first visual export.

## Preserved spherical extension

A later world may use a hexasphere-like or other near-uniform spherical subdivision,
with height and erosion/resource dynamics on that surface. Preserve the requirement,
but do not assume an infinite planar axial lattice maps globally to a sphere. A
spherical surface needs its own cell identity, adjacency, exceptional-cell/seam
behavior, metric, projection and height direction. Uniform six-neighbor rules and
planar Euclidean distances cannot simply be carried across as universal contracts.

The [Sub0HexGrid specification](../reuse/Sub0HexGrid.md) records this extension.
Its planar kernel should remain simple. A spherical facet requires a sourced design,
actual receiving caller and reference topology/geometry fixtures before API/code.
A height field may apply to both plane and sphere, but game-specific erosion and
construction rules remain separate from reusable surface geometry.

This direction expands the longer-term design. Phase 2 still prioritizes bounded
steering, clock and owned inspection; initial visual export may show the current
flat state. Terrain and spherical capability are not required for its exit.
