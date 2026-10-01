# Phase 2 ownership and consumed contracts

Date: 2026-10-01. Base: main 65bb8c4. Architect plus two GPT-6.1 Sol low workers.
Read the phase 2 plan, game design and active work log. No worker builds, benchmarks,
Git handoffs, installs, dependency edits or additional agents. Root owns these gates.

Swarm worker owns swarm headers/source/tests/docs only. Runtime worker owns runtime
and telemetry headers/source/tests/docs only. Architect owns shared contracts,
Simulation/main, state inspection/presentation, field-copy observation, integration
tests and central wiring/docs. Both workers message root and the affected peer at
contract changes and review-ready handoff; keep durable stream decision/validation notes.

## Topology review

Sub0HexGrid 59eddb5 has verified scalar geometry but no bounded regions or complete
radius candidates. Retain the current rectangular bins for this phase's steering
oracle. Future adoption favors investigating pointy-top geometry through the existing
stable-ID radius seam, gated by H2 and brute-force/replay evidence. No second backend,
library pin, Blight adjacency change, height/sphere dynamics or resource code is added.
This is a deliberate phase-start deferral, not abandonment of hex world intent.

## Swarm contract and rule

Shared SampleState is an owned ID/position/velocity value; SteeringSettings holds
startup radius, separation strength and finite acceleration/speed caps. ScenarioOptions
will gain an optional steering setting; absence preserves the existing scenario path.

Worker supplies swarm::Steering(GridConfig, SteeringSettings, sample_capacity) and
TryCompute(span<const SampleState>, const fields::FieldSet&, spatial::Grid&,
span<SampleState>) returning bool. Constructor validates/allocates startup scratch.
Input is strictly ID-sorted, finite and in the matching world; coordinator rebuilds
the grid from these exact positions before calling. Never infer a row from SampleId.
Output capacity failure or invalid input leaves output unchanged. Stage outputs in
owned reused scratch and copy only on success. Consume each grid result before the
next query. All neighbors read the same input, not progressively updated positions.

Project-defined separation: ignore self; neighbors inside the inclusive radius
contribute an outward unit vector weighted by (1-distance/radius). Exact coincidence
uses the x axis, smaller ID left and larger ID right, giving an antisymmetric pair.
Average over non-self neighbors and multiply by separation strength; add sampled
radial acceleration, cap Euclidean acceleration, integrate velocity at tick_seconds,
cap Euclidean speed, then integrate and clamp positions to validated world extents.
Use double intermediates and stable ID reduction order. Empty neighbors give zero
separation. This small explicit rule is not a ported external flocking algorithm.

## Runtime contract

Runtime worker supplies an exclusive coordinator clock driver borrowing HeadlessSession,
with injected elapsed time, four-tick catch-up, fractional carry, explicit discard,
pause/resume/close and blocked/error behavior. Choose and document safe elapsed-time
arithmetic and invalid/overflow input policy before code; no sleeping tests. It owns
the minimum consumed telemetry summary, including completed tick, applied count,
ingress pending/rejection and discarded time. Notify root of header/API shape early.
No Pub/Pipeline/Log adapter or threaded executor is required. Exact trace replay stays
independent of wall time. Root wires a real main caller and full-state oracle.

## Architect inspection and integration

Simulation owns completed tick identity and exposes a checked copy into caller-owned
pre-sized sample/field/cell destinations. Validate all capacities before any writes;
return owned geometry/count metadata on success. Samples are ID-sorted copies, not
ECS/scratch borrows. Snapshot storage survives subsequent ticks. Root integrates
swarm scratch before coordinator commit and checks exact full-state replay.
The legacy ECS-only workload remains available. Visual export is stretch after core
review/fixtures/combined Debug/Release/sanitizer gates. No dummy success or new
unconsumed public surface, and no performance conclusions from these checks.
