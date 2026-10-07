# Phase 13 coherence review and P13-01 contract

Reviewed 2026-10-07 against clean main `5b84ccd`, after PR26/27 dependency
receiving and PR28 responsibility reconciliation. Accountable owner: root.

## Vision and actual capability

The intended experience remains directing flowing mass, reclaiming biomass,
concentrating 64 at a relay, fusing a holding lattice, and shattering into 48 with
16 lost. This is the existing fixed-identity single-player reference loop.
Terrain, factions, population growth, 100K/150K scale, concurrency and a live
Vulkan desktop remain future gates. No new game rule is required for readability.

Bottom-up: current ECS master `1b1114a`, Pipeline `bf2ecce`, Pub v2, H2 and optional
bounded Log are consumed by owned-frame sequential execution. Display/input use
SDL at the existing full pin. SDL_AUDIO is explicitly OFF; the optional offscreen
Vulkan receiver does not establish a live Vulkan HUD. Three allocations/96
requested bytes per boundary is historical Phase12 evidence, not a current timing
or whole-frame measurement. P13-01 changes no boundary path or dependency pin.

Review findings: Phase13's original dispatch baseline predates pin refresh; the
workstream catalog still calls structures/graphics deferred; the reuse roadmap
still calls delivered diagnostics proposed; game intent mixes proposals with
implemented fixed rules. These current-state descriptions are reconciled here
and in their owning records. Historical phase evidence is preserved.

## Selected consumer and trade-offs

P13-01: read objective/mass/relay state, select FLOW or radial input, distinguish
queued from applied, operate visible fuse/shatter, and return from fullscreen.
One owner consolidates presentation and shared desktop wiring. P13-02 sound stays
separate until audio-enabled build/device and bounded clip/lifecycle receiving.

Selected layout retains the established world viewport and camera geometry,
enlarges HUD text and adds vertical space for two toolbar rows and wrapped
feedback. A single tiny-text toolbar preserves more world area but fails the
received small-window problem. Dynamic reflow would require a wider camera/GPU
geometry change; fixed shared geometry is the smaller useful receiving contract.
Material art, alternative renderer and generalized UI/audio infrastructure defer.

## Frozen local contracts

- Logical canvas: 1280x864, letterboxed; existing world rectangle 24,96,1232,520.
  Minimum window request: 1024x607. Body text uses 20 logical pixels, giving
  14 physical pixels at a 1026x607 output; output scaling is separately received
  by software fixtures, while native DPI/comprehension remain separate gates.
- Two rows, six columns maximum, use the same rectangles and action enum for
  drawing and hit testing. Labels fit their targets. Fuse/shatter are visibly
  unavailable in the legacy scenario and do not submit commands there.
- The window remains the default until native startup/toggle/focus/restoration
  receiving supports fullscreen promotion. `--fullscreen`, F11 and a visible
  fullscreen/window button request borderless desktop fullscreen. Escape cancels
  gestures and returns to a window. Failure retains playable windowed output and
  visible feedback. Toggle preserves mission, pause, receipts, selected tool/slot
  and camera; it cancels only an unfinished gesture and resets wall-clock baseline.
- Receipt admission remains distinct from completed trace/application; no HUD
  or fullscreen action writes simulation state outside the existing ingress.
  Feedback wraps into two bounded rows; no heap scratch or retained text borrow.
- Existing renderer and world geometry remain unchanged. Software captures are
  actual production-painter output, not native monitor or participant evidence.

## Receiving and remaining gates

Receive shared target bounds/labels, enlarged real text pixels, queued/applied
FLOW, structural actions/refusals, restart, output resize/letterbox coordinate
conversion and fullscreen lifecycle through the production SDL adapter. Retain
actual software output at 1026x607 and 2048x1280 with provenance and inspect it.
Run Debug/Release and code-quality review. Native monitor/DPI/fullscreen default,
normal hosted sanitizer, participant comprehension and sound remain explicit gates.
P13-01 completion does not close all of Phase13 or infer current performance wins.

Reuse disposition: HUD geometry, bindings and scenario feedback stay in Crucible.
No demonstrated neutral library gap calls for a pin change, scheduler, cache or
new public upstream API. Carry P12-F01 into this package; P12-F02 into P13-02.
