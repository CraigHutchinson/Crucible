# Faction and multiplayer extensibility groundwork

2026-10-03. User-requested design note; no faction or multiplayer system is
implemented by this decision. Current single-player nanite/Blight behavior, resource
rules, commands, replay and visual palette remain the verified baseline.

## Keep organism type separate from allegiance

Support a future world with more than two factions. Both nanite and Blight variants
may belong to different factions and may potentially be player-controlled. Do not
encode nanite=player and Blight=enemy as a universal ownership rule. Material/entity
kind describes behavior; faction describes allegiance/control. Infection state,
resource ownership and player/controller identity are separate decisions. A player
may eventually control multiple groups, and a faction may be AI-controlled.

At the first actual faction consumer, specify a bounded stable faction identifier
and explicit neutral/unowned representation, capacity, lifecycle and deterministic
ordering. Avoid a fixed two-value faction enum, bool enemy flag or XOR opponent rule.
Define relations independently (same/allied/neutral/hostile); do not infer hostility
from different colors. More than two factions does not require every pair to fight.
These future types are not added to Core merely to reserve a concept.

## Relevant decision points

| Decision point | Groundwork requirement | Future consumed gate |
|---|---|---|
| Phase 6 render packet/shader | Color is presentation data per instance, not a hardcoded two-faction branch. Two world ranges distinguish cells/markers, not armies. Preserve ability to draw many variants without backend-specific entity types | P6 shader/readback fixture with three marker and three cell colors; opaque RGB/alpha=1 only, labeled synthetic palette, not playable factions |
| First faction ECS/component/query change | Keep entity kind and faction identity independent; include both in owned observation where needed. Define relation lookup and ownership changes at completed boundaries | Three-or-more factions including both organism kinds, neutral/allied/hostile fixtures and stable-ID replay |
| First owned-resource/fusion mechanic | Decide whether substrate is shared/unowned and reserves/mobile/structure mass are faction-owned; define transfer/capture/attrition before mutation | Global conservation plus per-owner accounting, contested interactions, capacity/atomic failure and ownership-transition replay |
| First controllable Blight variant | Specify commands, spreading/reclamation interactions and victory rules rather than recolor existing behavior and call it playable | Concrete player input and independently checked multi-faction outcomes |
| First multiplayer/remote-input consumer | Define authoritative ownership/admission, tick/sequence ordering and deterministic rejection; distinguish sender/controller from faction | Unauthorized commands, duplicate/out-of-order/delayed delivery, disconnect/rejoin and full-state/replay agreement |
| First faction-facing UI | Palette maps identity to presentation; readable labels/shapes supplement color, and spectator/selected-player view is explicit | Three-plus faction visuals, color-accessibility and human comprehension checks |

Rendering consumes owned values and does not query ECS or decide alliances. A
palette can map faction identity to color upstream of the concrete painter without
putting faction rules into GPU layouts. Add explicit instance identity only when
picking/inspection needs it. Preserve camera, upload capacity and fence lifetime
contracts regardless of faction count. Do not equate extra colors with extra draw
calls or demonstrated gameplay. Public examples must label exploratory palette
fixtures accurately.

## Multiplayer direction and limits

The current single coordinator, completed-boundary commands, stable IDs and replay
are useful foundations. They do not prove network determinism or multiplayer support.
Choose authority/lockstep/rollback and numeric determinism policy around an actual
network consumer and measured latency needs. Do not introduce transport, prediction,
replication, concurrent mutation or a generic multiplayer framework in Phase 6.

At that gate, command envelopes must identify authorized control and deterministic
ordering; resource/entity ownership changes are authoritative completed-boundary
operations. Specify versioned snapshot/replay compatibility when adding faction
identity. Validate malicious/stale input as well as ordinary delayed messages.
Faction capacities and relation storage must be bounded; avoid coupling faction
count to the current four field slots or the two renderer world passes.

## Sprint disposition

Carry P06-F01: faction/material/control separation and multiplayer groundwork.
Phase 6 reviews packet/color and capacity assumptions against this note; it does
not implement faction components, per-faction ledgers, new playable species or a
network protocol. Before structural allocation or multiplayer packages dispatch,
freeze the respective rules above with concrete callers and fixtures. See the
[Phase 6 plan](../phases/phase6.md), [game intent](../game-design.md) and
[instancing decision](render-instancing.md).
