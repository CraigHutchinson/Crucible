# Local faction arena: bounded Phase 9 alternative

Status: proposal for the [Phase 9 shootout](../phases/phase9-investigation.md),
2026-10-05. Not selected, implemented or playtested. Generated faction/deathmatch
art supplies a readability target, not rules. This proposal intentionally shows
the extra rules needed to make that target playable instead of merely recoloring it.

## Smallest complete loop

One flat rectangular arena, three hostile factions and no terrain, networking,
structures or campaign. A and B own mobile nanites; C owns stationary Blight cells.
The player controls A; B and C use startup-frozen command schedules. A second
scenario selects C as the player without changing simulation rules. Direct a
current toward finite shared stock, contest the other factions, and preserve
enough living mass to be the last faction standing. A 600-completed-tick deadline
bounds passive games. Compare receiving work with the structural candidate on
the same 64×32, cell-size-1 world and 2,048 organism quanta: A/B start with 768
nanites each, C with 512 occupied cells; stock4 per cell and C reserve1. Small
rule/command oracles use an 8×8 or smaller fixture instead of that workload.
Freeze these constants only if the architect selects this
alternative; they are fixture values, not balance evidence.

The three factions each have four owned field slots, unrelated to faction count.
Use existing flow/radial force math for nanites. Every organism quantum is one
active nanite or one owned Blight cell; reserve is integer biomass. Stock is
unowned substrate, including clear cells. Harvesting transfers stock into the
harvester's reserve. Ownership never transfers between factions in this slice.

## Identity and authority

Faction IDs A/B/C, organism kind, controller ID and palette remain separate values.
The local controller mapping is fixed at startup; selecting a view does not grant
authority. Relations are a bounded explicit 3×3 table: self allied, all distinct
pairs hostile. No live alliance edits or neutral-faction behavior in this increment.
The bounded representation must permit a later fourth faction without a two-side
Boolean. Nanite identity uses existing stable SampleId values; inactive samples
retain identity rather than being removed/recreated in ECS. Blight identity is
row-major cell plus activation generation, checked before reuse. Epoch changes
on restart so retained commands cannot target a new match accidentally.

A concrete local command envelope contains epoch, controller, faction, strictly
increasing controller sequence and owned FieldEdit. Boundary admission checks
authority, epoch, sequence and faction slot. Unauthorized, stale, duplicate,
out-of-range and overflowed commands reject without changing fields or resources;
trace the reason. Successful admission is distinct from completed application.
Controller schedules enter through the same ingress as player edits. No transport,
prediction, generic command bus or multiplayer framework is introduced.

## Completed-tick rules

1. Drain the bounded cutoff, validate envelopes and apply accepted faction edits
   in admission order. Commands have no resource cost. Snapshot the living actors.
2. Move active nanites with their own fields and same-faction swarm neighbours.
   Opponent neighbours do not contribute cohesion/alignment. Blight stays on its
   owned cell. Gather immutable post-move actor contacts on rectangular cells.
3. Harvest at most one quantum per active actor on positive stock. For each
   row-major cell, visit factions in cyclic order beginning at
   `(completed_tick_before_step % faction_count)` and actors in stable identity
   order. Later actors see stock already debited. This cursor is derived from the
   replayed tick, not hidden mutable fairness state. Depleted stock yields zero;
   harvesting does not require infection and never replenishes stock.
4. For each cell that had actors from hostile factions in the contact gather,
   every actor in that cell loses its one organism quantum simultaneously.
   Credit cumulative explicit loss, deactivate those nanites and clear those
   Blight cells. No actor is protected by earlier harvest priority. A one-cell
   three-way collision can eliminate all three factions. This severe contact
   rule is deliberately inspectable; it is not proposed combat balance.
5. Each surviving Blight faction can create at most one adjacent cardinal cell
   per tick, transferring one reserve quantum to one new organism quantum.
   Candidate empty cells must have a positive projection on an owned flow's
   local direction. Attract/repel contribute their existing sampled vector;
   zero vector means no growth. Choose greatest projection, then source cell and
   target cell row-major order. Read post-attrition occupancy; nanite-occupied
   targets are invalid. Resolve factions using the same cyclic priority; a
   claimed cell rejects later growth without debit. No same-tick growth cascade.
   Grid capacity bounds Blight storage; insufficient reserve/no valid candidate
   changes nothing. Existing Boolean infection-only spread is disabled here.
6. Eliminate any faction with no active organism, forfeiting its remaining
   reserve into explicit loss. Reserve alone cannot revive a faction. At most
   one survivor wins; none draws. Otherwise the deadline compares
   `active organism mass + reserve`; equal greatest scores draw between those
   factions. Elimination precedes deadline scoring. Terminal outcome latches;
   pause does not advance ticks; restart restores stock, population, schedule,
   epoch, fields and ledger together.

Nanites cannot spend reserve on reinforcement in this slice; reserve is a
deadline score and Blight growth fuel. This organism asymmetry must be explained
in the HUD and playtest, not hidden behind identical colors. The Blight player
uses currents to choose a growth front, not to move existing cells. A field
overlay must mark growth-eligible cells separately from mobile trajectories.

## Independent conserved trace

At every completed tick, checked integer arithmetic proves:

`initial_total = remaining_shared_stock + Σ(active_mass + reserve) + lost`.

Harvested/work counters are diagnostics, never extra buckets. Startup fixture:
cell X stock 2, cell Y stock 1; A/B each have two nanites, C has two Blight cells
and reserve 1. Thus initial total is `3 + 2 + 2 + 2 + 1 = 10`.
Positions/contact cells below are independently specified fixture inputs; the
movement oracle separately verifies that the command schedule reaches them.

| Boundary | Contacts and actions | Stock | A mass/reserve | B mass/reserve | C mass/reserve | Lost | Check |
|---|---|---:|---:|---:|---:|---:|---|
| Startup | No work | 3 | 2/0 | 2/0 | 2/1 | 0 | 10 |
| Tick 1 | One A and one B meet at X; A wins its first harvest, B its second. One C harvests Y. A/B contact actors die. C grows once for 1 reserve | 0 | 1/1 | 1/1 | 3/1 | 2 | 0+1+1+1+1+3+1+2=10 |
| Tick 2 | Remaining A/B meet at depleted X, harvest nothing and both die. Their reserves are forfeited. C has no new growth candidate | 0 | 0/0 | 0/0 | 3/1 | 6 | 0+3+1+6=10; C wins |

Command trace: tick-1 cutoff accepts controller-A sequence 1 flow to X and
controller-B sequence 1 flow to X, plus controller-C sequence 1 growth flow from
Y. A command with controller-A/faction-C rejects unauthorized; C sequence 1
again rejects duplicate. Tick-2 cutoff accepts A/B sequence 2 targeting X;
C removes its growth field with sequence 2. Reverse producer arrival of different
controllers must not alter the replay of the recorded admission order. Different
accepted order within one faction deliberately remains a different command trace.

Independent tie fixture: one organism and one reserve for each faction, zero
stock, all three contact the same cell. Contact loss 3 plus reserve forfeiture 3
equals initial 6; all eliminate on the same completed boundary and draw. Deadline
fixture: survivors A/B have totals 3/3 and C has 2; at tick 600 outcome is an A/B
draw. A last survivor on tick 600 wins regardless of an eliminated opponent's
pre-forfeit score.

## Integration and acceptance cost

This cannot safely be a presentation-only extension. Production changes include
Simulation's active mask/kind/faction gather, FieldSet selection, per-faction
steering candidates, Blight ownership/growth, conserved loss ledgers, authorized
Runtime admission/trace, mission outcomes and owned StateCopy/ScenarioSnapshot.
Existing BiomassLedger explicitly promises constant mobile mass; existing
SampleState has no faction, FieldEdit no authority and Blight is Boolean. Extend
these at consumed arena boundaries while keeping quota/deadline replay as a
regression; do not make old constructors silently activate arena behavior.
Painter/GPU consume owned records rather than querying ECS or inferring allegiance
from organism kind. Restart must retire input selection and retained command epochs.
Pinned ECS population remains fixed; no transactional entity creation is assumed.

Acceptance fixtures must cover the numerical trace above, three-way elimination,
deadline ties/precedence, shuffled contact input, contested harvest rotation,
same-target Blight growth with no debit on rejection, reserve/stock overflow,
zero-strength fields, no growth cascade, inactive sample exclusion from spatial
queries, unauthorized/duplicate/stale envelopes, queue rejection feedback and
restart epoch isolation. Exact replay compares every active flag, faction/kind,
field, stock, Blight generation/owner, per-owner bucket, loss and outcome.
Both selected organism kinds need a headless schedule, desktop controls and a
human task sheet. A recolored scene cannot satisfy these gates.

The [primitive mock](phase9-arena-proposal.svg) uses A cyan circle badge,
B amber triangle badge, C violet diamond
badge; nanites are points/ribbons with their badge on field origins, Blight is
crosshatched filled cells with square organism silhouettes. The HUD names
controller and organism kind beside each badge, shows mass/reserve/loss and
highlights contested cells with a white crossed outline. Use those shapes in the
minimap and field labels; color alone is insufficient. These are proposed symbols,
not captures or asserted accessibility acceptance.

## Shootout recommendation

Select structural reclamation first. It consumes the current single swarm,
finite ledger and currents with one concentration/hold/redeployment decision.
Arena adds authority, per-owner accounting, contacts/attrition, active identity,
an owned controllable Blight growth policy and tied terminal outcomes together.
It also replaces infection-only resource semantics and makes reserve mean
different things to different organisms. Those are valid later game decisions,
but a much wider teaching and regression surface than one lattice/relay loop.

Retain this as evidence for P06-F01, not a generic framework backlog or promise
of multiplayer. If arena is chosen instead, freeze these rules and implement the
independent three-faction headless trace before UI work; stop if fixed identity,
global conservation or complete replay fails. No scale, human comprehension or
physical performance claim follows from this proposal. R07/R08 remain local and
no upstream pin change or extraction has a consumed need.
