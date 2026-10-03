# Fixed route provider and strategy receiving recipes

P6-M dispatch base `44701d6`; [phase plan](../../phases/phase6.md). The provider
extracts the existing CLI swept attractor without changing mission, resource,
steering, seed, population or spatial policy. It returns an owned optional edit,
retains no session/storage and allocates nothing. Root wires both CLI route and
actual mission-frame export to the same function.

`crucible::runtime::GetReferenceMissionRouteEdit(completed_tick)` requests slot 0,
radius 8, strength 4 at multiples of 60 completed ticks. Positions are
(8,8), (24,8), (40,8), (56,8), (8,24), (24,24), (40,24), (56,24), then repeat.
This reproduces the old recipe throughout the uint64 domain; the caller stops
requesting after a terminal outcome. Requests at completed ticks 0 and 60 apply
at boundaries 1 and 61. An accepted request is not an applied field or captured
frame; callers check admission before pumping and inspect the completed trace.

## Frozen additional receiving cases

Use default 2,048 samples, target 1,780 and deadline 900. All edits use slot 0 and
the existing live-tool radius/magnitude. Admit only while the mission remains active.
The following are architect-selected receiving experiments, not difficulty tuning.

| Case | Request at completed tick | Edit | Center | Radius | Strength | First eligible completed boundary |
|---|---:|---|---|---:|---:|---:|
| stationary | 0 | set | (32,16) | 8 | 4 | 1 |
| repel/reposition/erase | 0 | set | (32,16) | 8 | -4 | 1 |
| repel/reposition/erase | 120 | set | (48,16) | 8 | -4 | 121 |
| repel/reposition/erase | 240 | remove | (0,0) | 0 | 0 | 241 |

Stationary has no subsequent input. Repel/reposition/erase has no input beyond
the three rows. Removal uses canonical zero geometry. Freeze before measurement;
record resulting outcomes rather than presume wins or change quota to fit them.
Passive and the existing swept route remain receiving baselines.

## Oracles and evidence ownership

`runtime_reference_route` independently enumerates edits at 0/60/120/180/240/300/
360/420/480/900 and a near-maximum tick; absent requests include 59/239/479 and
UINT64_MAX. It checks owned-return isolation and valid live-tool values. Actual
session fixtures prove unapplied input at ticks 0/60, exact trace application at
1/61, equivalent 5ms+11,666,667ns versus 16,666,667ns partitions, and independent
full-state trace replay through tick 61. Metadata, stable IDs, positions/velocities,
fields, infection, stocks and ledger are compared.

Architect-owned CLI/export receiving covers complete terminal runs for all four
cases. Retain accepted/applied traces, terminal tick and ledger, full-state replay,
checkpoint observations and actual production-painter ACTIVE/WON/LOST captures.
Captions identify scenario, captured tick, applied commands and provenance. Shared
route consumption keeps CLI/export inputs identical; visual evidence must use
completed snapshots rather than anticipated edits. Numerical strategy evidence
prepares a playtest and does not establish human comprehension or balanced difficulty.

Worker validation: self cpp-review and `git diff --check` only; no build, test,
timing, commit or capture run. Root owns serialized Debug/Release/sanitizer and
headless/desktop/GPU receiving, actual outcome measurements and visual inspection.
No terminal outcome for the additional cases is claimed in this handoff.
