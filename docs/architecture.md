# Crucible architecture

Crucible is a macro-RTS/swarm simulator for planetary reclamation. The player paints
currents and places attractors/repulsors to steer nanites against a cellular Blight.
Dense groups can fuse into structures, then shatter into a depleted swarm.

Target: 100,000–150,000 active entities and 60 FPS, measured rather than assumed.

The integration foundation uses Sub0ECS v2 query-oriented storage, Sub0Pipeline
DAG execution, Sub0Pub v2 typed delivery and Sub0Log telemetry. The executable and
benchmark currently exercise only ECS integration. The stack test proves typed
delivery -> deferred ECS tick -> ordered pipeline telemetry -> decoded log record.

Sub0Pub delivery is not automatically asynchronous. A bounded application-owned
handoff must separate UI publication from simulation mutation. Choose and test a
broker/queue contract for actual threads; the current smoke test uses one thread.
Sub0ECS v2 organizes storage around declared queries, not v1 archetype assumptions.

Planned tick dependencies:
1. Drain commands and apply structural changes while workers are quiescent.
2. Calculate double-buffered Blight chunks and rebuild the swarm spatial grid.
3. Evaluate flocking/fields from immutable snapshots into per-entity outputs.
4. Integrate positions once force calculations finish.
5. Resolve conflicts using owned cell partitions or reduction buffers; commit
   fusion/deaths only after workers finish.
6. Publish immutable display snapshots and record phase timings.

Declare read/write access and ownership for every stage. A DAG orders work but does
not by itself prove disjoint writes or remove the need for a safe handoff. Initial
headless execution is deterministic and sequential. A bounded worker pool, rendering,
boids, spatial hash, Blight, fusion and telemetry overlays remain future milestones.

Groundwork exit criteria: reproducible pinned consumer builds, clean dependency
options, successful API round trip, behavior tests, sanitizer checks, Release
benchmark capture, CI and documented limits before gameplay expansion.
