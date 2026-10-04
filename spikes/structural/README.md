# Relay-density investigation

`relay_density.cpp` consumes the production Simulation, bounded HeadlessSession
input and owned ScenarioSnapshot. It investigates whether current tools can gather
32 mobile identities within radius4 of prospective relay (48.5,16.5). It does not
simulate fusion, losses, structures, protection, victory or human comprehension.
See the [structural rule proposal](../../docs/decisions/phase9-structural-proposal.md).

All three strategies use 2,048 samples, the existing 64×32 unit-cell arena, four
field slots, default steering, stock4/cell, mass1/sample and work cap64. Run directly
through tick900 without a mission latch so every strategy reaches the same boundary.
The schedules are fixed before execution:

| Strategy | Admitted before tick1 | Changes after admission |
|---|---|---|
| passive | None | None |
| radial | Slot0 attract at (48.5,16.5), radius8, strength4 | None |
| straight-flow | Slot0 from (24.5,16.5) to (48.5,16.5), radius8, strength4 | None |

**The uniform startup is already eligible:** an interior radius4 includes49 cell-center
samples, exceeding the proposed32-member cost before any input. This experiment
cannot establish that flow is necessary to unlock fusion. It reports signed count
change from startup and consecutive/longest eligible completed-tick runs, measured
at every tick and summarized at checkpoints. A result in which passive stays eligible
requires a subsequent bounded starting-density/relay-placement rule experiment;
stop that experiment when passive and admitted routes have a reproducible difference
in eligibility duration and both retain full-state replay. Do not change this spike's
cost after observing its result or claim human difficulty from a density difference.

The magnitude/radius match the live tools. Flow's capsule pushes along its tangent,
including its endpoint cap; it is not an attractor at the relay. A negative result
is useful evidence, not justification to change tuning silently. No pathfinding,
new input API or substitute experiment engine is introduced.

Run the `crucible_relay_density` target from the build tree. Stdout is JSONL: density
records at ticks0,60,120,240,480,900 and one final full-state replay result per
strategy. Capture it to an evidence file through the phase receiving workflow.
Every density row includes all six current biomass ledger fields and requires
conservation and exact production-radius-query/brute-force-snapshot parity.
An independent three-center fixture checks inclusive radius1 and the preceding
representable radius before the strategy runs. Replay compares geometry/metadata,
every ID/position/velocity, all field kinds/endpoints, infection, stock and ledger.
Any admission, invariant, query or replay failure produces a nonzero exit status.

Reaching the prospective threshold does not prove a fuse command, transition rules,
120-tick relay hold, balanced difficulty, performance or playability. The trace has
only actual field commands. Structural implementation remains a subsequent decision
using the independent hand-calculated transfer/loss proposal.
