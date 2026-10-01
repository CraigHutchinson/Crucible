# Phase 3 proposal: give steering a gameplay consequence

Status: proposed, not dispatched. Read the [phase 2 review](../sprint-reviews/phase-02.md)
and carry its open IDs into this plan at dispatch; create phase-03.md from the review
template when work starts. Reassess against merged phase-2 main and remaining
quota before execution. Phase 2 already supplies movement, bounded clock, exact replay
and owned snapshots; keeping the same stream split would create artificial tasks.

## Outcome and design gate

The next useful increment is a small headless scenario in which nanites arriving at
Blight consume a bounded quantity and change an observable ledger. Before code,
architect freezes the proposed game-design choices: whether infection and mineable
material are distinct, source of initial nanite mass, consumption/growth conversion,
per-tick budget, contested-cell ordering and conservation/loss accounting. Document
units and examples, then make fixtures the independent oracle. Do not invent unlimited
mass or equate a byte of infection with a material quantity by convenience.

## Reassessed ownership

| Proposed stream | Consumed increment | Ownership / prerequisites |
|---|---|---|
| A: resource interactions + Blight | One bounded consume/grow rule and ledger with stable ID reduction | interactions/blight code and tests; architect approves actual shared values after product gate |
| B: reusable hex geometry | Sub0HexGrid H2 finite regions and complete radius candidates, standalone strict consumers | Separate repository; checked scalar H1 is baseline; no mandatory Crucible migration this phase |
| C: architect integration + scenario inspection | Compose gameplay consequence, expose only used ledger values, replay/visual evidence | Shared Simulation/contracts, main/presentation, combined gates; two bounded workers at most |

Consolidate steering maintenance with integration unless defects require independent
ownership. Runtime remains unchanged absent a concrete failure. Reuse the owned copy;
no exchange pool or background rendering is needed for a headless gameplay increment.
Hex adoption is optional and only follows H2 brute-force completeness, selected layout
and boundary semantics, and integrated full-state replay. Keep Blight adjacency an
explicit game-design choice. If quota only covers one implementation worker, prioritize
resource consequence and retain hex H2 as the next upstream increment.

## Gates and deferrals

Acceptance: independent conservation examples, empty/depleted/contested/border cells,
finite bounded values, no per-tick growth, tiny and 2,048-sample integrated fixtures,
full-state replay including the ledger, review and supported Debug/Release/sanitizers.
Keep a true input/window loop as a later candidate after a backend ADR; SVG is current
inspection, not a substitute for eventual playtesting.

Height, mined depressions, permanent fused bridges and eventual spherical subdivision
remain in the terrain ADR. The first resource ledger should allow future conservation
reasoning without adding speculative terrain or sphere code. Relay mission, combat,
lattice/shatter, population destruction and performance claims remain separate gates.

At phase start update the reuse catalog with receiving consumers, verify upstream
HEADs/pins, and revise this split from evidence. No permanent four-agent team is implied.
