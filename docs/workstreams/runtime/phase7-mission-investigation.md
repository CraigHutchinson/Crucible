# Phase 7 mission timing investigation

Status: local study/visual inspection and hosted receiving complete; original local
evidence recovery/publication remains P07-F01 after execution disconnect. See [receiving](../integration/phase7-validation.md).
This is deterministic timing evidence and preparation for human observation, not
human validation, difficulty tuning or a random-seed experiment.

## Frozen question and receiver

Does the reference sweep's command timing change its outcome on the existing
fixed startup world? The quota stays 1780 recovered biomass, deadline 900 completed
ticks, population 2048, four field slots and unchanged resource/steering rules.
There is no configurable random seed in this receiver.

The existing four-case [Phase 6 receiver](phase6-examples.md) remains untouched.
Its passive, reference sweep, stationary and repel/reposition/erase outcomes are
references, not predictions about the three new schedules.

| Additional case | First request tick | Request cadence | First two applied ticks |
|---|---:|---:|---|
| cadence30-start0 | 0 | 30 | 1, 31 |
| cadence120-start0 | 0 | 120 | 1, 121 |
| cadence60-start60 | 60 | 60 | 61, 121 |

At request index k, the receiver consumes
`GetReferenceMissionRouteEdit(k * 60)`. It retains slot 0, radius 8, strength 4 and
the complete reference coordinate sequence. Moving cadence changes timing, not
route order. An active run requests only before tick 900: at most 30 edits per
case and 2700 forward ticks across all three cases, plus terminal replay.
The coordinator stops at its first terminal boundary; no post-terminal requests
are part of a schedule.

## Reproduction and evidence

After building `crucible_mission_sensitivity`, run:

```sh
<build>/tests/integration/crucible_mission_sensitivity --schedule-only
<build>/tests/integration/crucible_mission_sensitivity --study
<build>/tests/integration/crucible_mission_sensitivity --study --export NEW_DIRECTORY
```

Default CTest receives only explicit `--schedule-only`. It enumerates independent
coordinates, absent and extreme inputs and actual request/application boundaries
in small coordinator runs. The expensive three-case study is an explicit Release
investigation; original four-strategy CI acceptance stays intact. Missing modes,
unknown flags, extra arguments, empty export paths and existing export directories
fail explicitly. There is no selector that silently omits cases.

Study observations check finite-resource conservation at every completed tick.
Every terminal state is compared with a separately constructed simulation driven
by the actual applied trace: metadata, stable sample IDs, positions, velocities,
fields, infection and per-cell stock. Terminal resume, pumping and attempted input
must leave the stopped boundary unchanged and reject admission.

`results.json` contains all three outcomes, settings, checkpoints every 60 ticks
and at terminal, and the complete actual trace with request tick, applied tick,
sequence and field values. The writer flushes and checks persistence. Exports use
the production software painter on the completed snapshot: 1280×720 BMPs at tick
60 when reached and at each terminal. Tick 60 may precede application of an edit
requested at 60; that distinction is intentional. Root records executable/source
hashes, build configuration, capture dimensions and conversion/inspection evidence.
A failed run or partial export is not accepted evidence.

## Interpretation and next receiving

Compare terminal ticks, recovered biomass, applied command counts and trajectories;
do not infer broad robustness from three perturbations. Each route overwrites one
slot; this is not multi-field planning. The fixed world, fully automated input,
fixed radius/strength and software captures limit transfer to physical interaction.

[Human protocol](../../playtests/reclamation-phase7.md) prepares observation of
mission comprehension, committed input and route planning. Actual participants,
physical devices, accessibility and balance remain P05-F01/P04-F02 receiving work.
Current colors do not encode faction relationships. Three-plus faction identity,
relations and command authorization remain separate future consumers under
[faction groundwork](../../decisions/faction-extensibility.md).

## Observed results

The frozen 30/0, 120/0 and 60/60 cases won at ticks 297, 271 and 449 with 10, 3
and 7 applied edits respectively. All reclaimed1780 with full-state terminal replay
and conservation. No quota/rule tuning or globally best cadence follows. The
late-start tick60 capture has no field until first application at61. Original
local raw outputs are preserved pending recovery; hosted GPU CI retains a fresh
explicit Release study/capture artifact rather than claiming those files recovered.
