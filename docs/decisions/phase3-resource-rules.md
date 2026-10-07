# Phase 3 reference resource rules

Date: 2026-10-03. Architect-selected rules for [phase 3](../phases/phase3.md), not
playtested balance. Actual consumed value types and independent fixture review precede
implementation dispatch. Infection is Boolean and never implicitly a material quantity.

## Units, startup and conservation

One biomass quantum is an integer accounting unit. Checked uint64 totals and cell
stocks; validate all sums/products before construction or publication. Resource-enabled
startup supplies finite stock per cell, reserve and positive mass per fixed sample.
Initial mobile mass has an external startup source included in initial_total.
Reference scenario: four stock quanta per cell (including clear cells), one mobile
quantum per sample, zero reserve, global action limit 64. Fixture values may differ.

Mobile mass stays constant; reserve cannot spawn entities this phase. Track initial
total, remaining substrate, mobile mass and reserve. Harvested cumulative material is
a diagnostic, not an extra conserved bucket. At every completed tick:

`initial_total = sum(remaining_cell_stock) + mobile_mass + reserve`.

No loss/structure/growth transition or unused public placeholder is enabled. Later
attrition/fusion/shatter must extend the equation and capacity rules explicitly.
Legacy/unconfigured scenarios retain current behavior.

## Spread, work and arbitration

Cardinal nonwrapping spread derives from immutable prior infection, one edge per tick.
Spread changes infection only; it creates/restores no stock. Each unique sample can
request one action on the rectangular cell containing its post-move position. Clamp
to the closed physical world; interior boundaries use floor, maximum edges the final
cell. These contacts do not depend on the proposed hex swarm bins.
Finite out-of-world positions clamp; nonfinite positions reject. Resource proposals
use unique nonzero SampleIds from the coordinator's fixed active population (currently
1 through N). Zero or an ID absent from that active snapshot is invalid. An ID is
never an implicit array offset. This does not narrow generic Spatial's ID contract.

Sort proposals by row-major cell then ascending SampleId. Each ID acts at most once;
global successful actions are at most min(population, configured limit). Limit zero
is valid and disables reclamation. This deterministic priority has no fairness
guarantee; any later fairness cursor becomes replayed state under a revised rule.

- Clear cell: no action, no budget cost.
- Infected positive stock: remove exactly one quantum and credit exactly one reserve
  quantum; retain infection if stock remains, clear when stock reaches zero.
- Infected zero stock: spend one action, clear infection and credit nothing.

Later proposals see earlier proposed changes. After clearing a cell, they do nothing
and spend no budget. Newly spread infection can be processed that tick. A depleted
cell may be reinfected in a later spread tick: maintenance costs work but creates
no biomass. Depletion grants no permanent immunity. Spawning/attrition are later work.

## Hand-calculated oracle

Unless noted, two fixed IDs have one mobile quantum each, initial reserve zero,
global limit two; infection below is the prepared state after spread.

| Input | Remaining / reserve / actions | Infection and invariant |
|---|---|---|
| Stock 3 infected, IDs 2 then 1 | 1 / 2 / 2 | Infected; initial 5 = 1 + 2 + 2 |
| Stock 1 infected, IDs 2 then 1 | 0 / 1 / 1 | Clear; initial 3 = 0 + 2 + 1; second ID does nothing |
| Stock 0 infected, IDs 2 then 1 | 0 / 0 / 1 | Clear; initial 2 = 0 + 2 + 0 |
| Previous zero-stock cell reinfected next tick | 0 / unchanged / 1 | Clear again, no new mass |
| Stocks [2,2] infected; ID 2 cell 0, ID 1 cell 1; limit 1 | [1,2] / 1 / 1 | Both infected; cell priority precedes ID; initial 6 = 3 + 2 + 1 |
| Stock 3 clear, both IDs present | 3 / 0 / 0 | Clear; initial 5 = 3 + 2 + 0 |
| Stock 3 infected, limit 0 | 3 / 0 / 0 | Infected; unchanged material |

Spread fixture: prior infection [1,0,0] on 3x1, stocks [0,2,2]. Prepare [1,1,0]
without changing stocks. No samples: commit that infection and preserve the ledger.
One action at cell 1 leaves stock [0,1,2], credits one reserve and retains [1,1,0].
Check multi-tick clearing/reinfection from prior committed infection, without in-place
cascades. Check exact interior/final-edge contact and shuffled-input equivalence.

## Ownership, capacity and failure

Simulation owns committed values and coordinator publication; A owns domain calculation
and startup-sized pending scratch. Inputs remain immutable through prepare. Proposals
are bounded by population, cell outputs by validated cells; no per-tick vector growth.
Store no ECS reference or borrowed query span in proposals. Validate IDs, finite
positions, capacities and ledger arithmetic before mutation. Invalid/duplicate ID,
capacity exhaustion, inconsistent ledger or overflow rejects the complete transition.
Counter overflow also rejects before publication. Destinations and prior state remain
unchanged; no partial clearance, stock debit or reserve credit.

Prepared spread/consume/commit publishes infection and material/ledger together. Existing
Blight::Step commits immediately; implement an actually consumed staged path for resource
ticks while retaining legacy Step. Owned inspection includes every stock and conserved
bucket for replay. This does not promise full-tick ECS rollback: unexpected failure
stops runtime and suppresses a successful completed-tick publication.
