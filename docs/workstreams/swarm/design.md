# Bounded local steering

The [phase 2 execution decision](../../decisions/phase2-execution.md) freezes this
project-defined rule. Simulation's optional steering scenario is the named caller;
the architect owns its wiring. No external flocking algorithm is being ported.

`Steering(GridConfig, SteeringSettings, sample_capacity)` validates the startup
world, finite positive neighbor radius/acceleration/speed caps and finite nonnegative
separation strength. Zero capacity supports an empty population. One output vector
is sized at startup. `TryCompute` neither grows storage nor retains input borrows.

Simulation gathers strictly ascending, distinct SampleIds and finite position/velocity
values from the same tick-start state. Positions must lie in the closed validated
rectangle. It rebuilds the matching rectangular Grid from those exact values before
calling `TryCompute`. This is a caller precondition: Grid has no public configuration
or committed-position inspection. Missing self and unknown query IDs are rejected,
but those checks do not certify every stale or mismatched grid. The method consumes
each borrowed ID-sorted query result before the next query and maps IDs with a
checked binary search; SampleId is never used as an array offset.

For each non-self neighbor inside the inclusive radius, add its outward unit vector
times `1 - distance / radius`. Exact coincidence contributes minus x for the smaller
ID and plus x for the larger ID. Average over all non-self neighbors, including the
zero-weight neighbors exactly at the radius, and multiply by separation strength.
An empty neighborhood contributes zero. Sum existing FieldSet radial acceleration,
cap Euclidean acceleration, integrate velocity with `tick_seconds`, cap Euclidean
speed, integrate position, then clamp position to the world rectangle. Clipping
does not zero or reflect velocity. All inputs remain unchanged during computation.

Intermediate arithmetic uses double and the reduction follows ascending IDs.
Nonnegative clamping of a falloff at the radius avoids tiny inward contributions
from floating-point roundoff. The final float conversion may round a capped vector
slightly above its mathematical magnitude; fixtures allow float rounding tolerance.
FieldSet supplies its existing finite saturated float acceleration components.

Every next sample is staged in owned reusable scratch. Only after all input/query
checks and all sample calculations succeed is the input-sized output prefix copied.
The entire destination remains unchanged on failure; any output tail stays unchanged
on success. Input/output overlap is supported because publication follows the last
input read. The object and grid require exclusive coordinator access; neither ECS
nor renderer state is exposed.

Alignment/cohesion, painted flow, hex migration, obstacles and worker execution are
outside this increment. Rectangular completeness is inherited from the existing Grid.
