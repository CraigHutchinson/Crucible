# Phase 9 candidate: one relay, one reversible lattice

2026-10-05. Bounded rules proposal, not implementation or a gameplay acceptance
claim. Receive the Phase 8 flow baseline first. Architect selects the candidate
before dependent coding; constants below are fixture choices, not tuned difficulty.

## Smallest consumed package and scope

Recommend a real mobile-mass transition with a single lattice fixed at one relay.
The player routes mobile nanites into the relay's radius, fuses 64 of them, holds
for 120 completed ticks, and can shatter to recover 48 of those same identities.
This creates the intended mobility-versus-holding decision without unit creation,
multiple structures, terrain, combat, growth or a building economy. Reuse the
64×32 arena, cell size 1, 2,048 startup nanites, four field slots, existing
steering, finite stock 4 per cell and work cap 64. Relay center is (48.5,16.5).
Initial mobile mass is 2,048, stock 8,192, reserve 0: total 10,240.

Current Phase 9 investigation receives actual tool concentration through the
[production density spike](../../spikes/structural/README.md). It performs no structural
transition. The following proposal freezes a subsequent implementation candidate.

The original cost32/refund24/loss8 candidate was evaluated and rejected as a
concentration gate: startup contains49 nearby samples, and the full Release study
kept passive density between34 and51 throughout all900 completed ticks (end38).
The predeclared cost64/96 comparison therefore selects cost64 with 75% shatter
recovery: refund48, loss16. Passive never reaches64; straight-flow's observed maximum
is54 (end19); radial reaches74 by tick60, 116 by120 and174 by240, with maximum312
and end311. All three actual final states replay through900. This establishes a
reproducible radial concentration opportunity, not human-tested difficulty or a
claim that straight flow alone unlocks fusion. One lattice leaves1,984 mobile
workers in the reference scenario. Density evidence selects this subsequent rule;
actual transitions, protection and relay outcomes remain unimplemented.

Smallest implementation acceptance is fuse → inspect frozen members and ledger →
shatter → redirect recovered members → full-state replay. Completing the relay
mission additionally requires protection/hold/outcome receiving and live primitive
commands. Ship the mission only when both sets pass. A headless transition fixture
alone does not establish the promised player loop.

## Exact transition rules

- Structural mode requires mass_per_sample=1. Retain every startup ECS entity and
  immutable SampleId; each identity is mobile, anchored or lost. Never delete,
  reuse or manufacture IDs. Lost identities remain owned records, not participants.
- One structure slot and one fixed relay. Fuse requests name the relay, not an
  arbitrary free-world placement. Eligibility uses the committed tick-start mobile
  positions: inclusive Euclidean distance ≤4, finite double squared-distance test.
  Select the lowest 64 eligible SampleIds. Fewer than 64 or an occupied slot rejects
  the entire request with no membership, ledger, hold or generation change.
- Fusion transfers 64 mobile quanta into structure mass. Members move to relay
  center with zero velocity; they stop steering, separation, field sampling,
  spatial participation and reclamation. No reserve is spent or credited.
- Each successful fusion increments a nonwrapping uint64 structure generation.
  A shatter must name the occupied generation. Return the lowest 48 member IDs to
  mobile at relay center with zero velocity; mark the highest 16 lost permanently.
  Transfer structure64 → mobile48 + cumulative loss16. Clear slot and reset hold.
  An empty, stale or exhausted generation request rejects without mutation.
- No automatic growth, maintenance debit or per-contact attrition. Attrition cost
  is explicitly zero in this increment; shatter is its only biomass loss. Do not
  imply that this closes the future Blight-combat rule gate.

Conservation becomes `initial_total = remaining_stock + mobile_mass + reserve +
structure_mass + lost_mass`. `harvested` and `work_actions` remain diagnostics,
never added to that equation. Structural mission reclamation progress is harvested,
not reserve−startup_reserve; reserve becomes spendable only in a later rule. Keep
legacy quota/deadline behavior unchanged for scenarios without structural mode.

## Completed-boundary order and protection

Drain the existing bounded queue cutoff; process all admitted field and structure
commands in admission order against the committed state, with earlier structural
commands visible to later commands at the same boundary. Fuse followed by matching
shatter is legal and incurs the stated loss; a second fuse sees updated mobile mass.
Then gather only mobile identities, compute existing steering/movement, rebuild
mobile query input, prepare cardinal spread, apply bounded reclamation sorted by
(cell, SampleId), clear protected infection in that same pending step, and publish
infection, stock, ledger and completed tick. Finally update hold and evaluate mission.

One occupied lattice protects cells whose centers are within inclusive radius2 of
relay center. Protection clears pending infection but never harvests/debits stock,
creates reserve or increments work_actions. Reclamation runs first, so ordinary
mobile harvest credit remains possible there on that tick. Spread still reads the
previous committed infection: newly fused infected cells can emit one last cardinal
wave outside protection on their fusion tick. Record and display this deliberate
simple rule; it is not an impenetrable collision wall or permanent reclaimed terrain.
After that commit protected cells cannot be spread sources while the lattice holds.
Shatter removes protection before that tick's spread; reinfection can occur immediately.

Hold is consecutive completed ticks with an occupied lattice: successful fusion
boundary counts as 1, shatter boundary counts as 0. Field edits, pause, admission
rejection and trace-capacity blockage alone advance neither movement nor hold.
Victory requires harvested≥1,780 and hold≥120. Otherwise defeat at completed tick900
or when `mobile_mass + (occupied ? 48 : 0) < 64`. Victory precedes defeat on the
same boundary. Terminal outcome latches and stops further ticks/admission; restart
constructs a fresh run including generations, members, loss, mission, queue and trace.

## Independent oracle and command trace

Small arithmetic fixture: 16 cells, stock4 each, 80 startup IDs, reserve0. Place
IDs1..64 exactly inside relay radius and IDs65..80 outside. Disable reclamation
for transition boundaries, then enable exactly three infected contacts with stock.
Total is 64+80=144. This arithmetic fixture disables mission evaluation: the final
fuse leaves only48 recoverable mass and would immediately lose under the proposed
minimum64 viable-mass mission rule. Geometry fixtures separately verify membership/
protection (inclusive protection radius **2 world units**, eligibility radius4).

| Boundary command/action | Stock | Mobile | Reserve | Structure | Lost | Result |
|---|---:|---:|---:|---:|---:|---|
| Startup | 64 | 80 | 0 | 0 | 0 | Total144 |
| sequence1 Fuse; tick1 | 64 | 16 | 0 | 64 | 0 | Members1..64; generation1; hold1 |
| sequence2 Fuse; tick2 | 64 | 16 | 0 | 64 | 0 | Occupied rejection; hold2 |
| sequence3 Shatter generation0; tick3 | 64 | 16 | 0 | 64 | 0 | Stale rejection; hold3 |
| sequence4 Shatter generation1; tick4 | 64 | 64 | 0 | 0 | 16 | IDs1..48 survive; IDs49..64 lost; hold0 |
| Three successful harvest contacts; tick5 | 61 | 64 | 3 | 0 | 16 | harvested3; work_actions3 |
| sequence5 Fuse; tick6 with only63 in radius | 61 | 64 | 3 | 0 | 16 | Insufficient-local-mass rejection |
| sequence6 Fuse; tick7 with all64 in radius | 61 | 0 | 3 | 64 | 16 | Generation2; total144; hold1 |

The rejected command itself must leave all rule state unchanged. Its enclosing
successful tick can still move nanites, harvest, spread and advance hold; compare
rejection with a no-command tick, not an earlier complete frame. Separate fixtures
cover exact distance4, 63/64 membership, same-boundary fuse/shatter ordering, generation
exhaustion, victory on deadline, starvation, and protection's one-final-wave rule.

## Actual integration surfaces and ownership

`Simulation::ScenarioState` owns fixed identity activity/member data and startup
scratch. Existing `Spatial::Grid::TryRebuild` and `Steering::TryCompute` already
accept ≤capacity prefixes and sparse sorted IDs; reuse them without a new engine.
`Reclamation::TryStep` currently requires exactly startup population and sorts its
whole proposal vector. It needs active-prefix validation/sort and the extended
conservation buckets; retain original ID-domain checks and duplicate rejection.
Protection uses existing `blight::Grid::PreparedStep::TryClear` before its commit;
no alternative spread scheduler or hex-topology selection is needed.

`CommandIngress`/`HeadlessSession` currently carry only FieldEdit and treat failed
application as fatal. Extend their concrete owned command payload/trace once for
field/fuse/shatter, with explicit successful or normal rule-rejection results.
Insufficient mass/stale target must not fail or close the session. Keep admission
sequence, cutoff, bounded storage, trace-full-before-drain and fatal invariant-failure
semantics. Do not encode structure actions as fake field slots or add another queue.

Owned state copies retain every original sample's ID, position, velocity and activity,
plus structure generation/position/members, extended ledger, hold/outcome and all
replay configuration. Drawing consumes that copy and renders only mobile samples
plus a primitive lattice/relay, eligibility64, refund48/loss16 and rejection reason.
Replay owns complete payloads, order, boundary ticks and application results; fresh
matching configuration must reproduce every sample, field endpoint, infection,
stock, member, generation, ledger and terminal value. Retained frames survive later
shatter/restart; no ECS/spatial/member spans escape capture.

Reuse disposition: R01 avoids ECS structural allocation; R05/R06 reuse current
complete-radius geometry; R07 extends a consumed owned input/read model locally;
R08 retains game rules locally. No pin change, abstract building system, renderer,
upstream extraction, networking or faction framework is justified.

## Feasibility and receiving gates

| Package | New rules/surfaces | Decision quality and limitation |
|---|---|---|
| True one-relay lattice, recommended | Identity activity, ledger transfer/loss, one slot generation, typed actions/rejections, protection, hold/outcome, owned presentation | Field tools control concentration; anchoring removes workers; shatter makes a visible cost. Touches central simulation/replay, but avoids ECS mutation and unused growth APIs |
| Reserve-funded anchor, smaller alternative | Reserve→structure64, refund48/loss16, nearby64 density gate; no activity filtering | Lower implementation cost; gathered nanites remain mobile, so this does not demonstrate the intended mobility tradeoff. Call it an anchor experiment, not completed fusion |
| Full structural design, deferred | Multiple placement/structures, attrition, growth, obstacles, terrain and tuning | Additional independent rules; not needed to demonstrate this one decision |

Do not choose from imagined FPS or human playtest responses. Compare the independent
arena proposal at this same bounded arena/population; its author owns the distinct
identity/authority/contention/tie evidence. Structural acceptance needs conserved
transition oracles, variable mobile membership regressions, no debit on rejection,
complete replay, retained-snapshot/restart tests, legacy quota win/loss fixtures,
primitive live commands and inspected actual fuse/shatter/relay frames. Finish with
Debug/Release/supported ASan+UBSan and exact-head hosted CI. Human tasks must separately
ask a player to gather, explain the lost workers, shatter, redirect and win; automated
receiving cannot close comprehension, tuning, physical performance or hardware gates.
