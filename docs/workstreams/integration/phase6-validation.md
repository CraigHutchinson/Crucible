# Phase 6 receiving evidence

Dispatch44701d6, 2026-10-03; integration underway.
[Plan](../../phases/phase6.md), [review](../../sprint-reviews/phase-06.md),
[frozen GPU contract](../../decisions/phase6-gpu.md).

## Mission provider and examples

Checkpoint source b11a377. Production CLI and integration export share
GetReferenceMissionRouteEdit; independent route/boundary/schedule/replay test passed
Linux Release. Four reference strategies conserve biomass and independently replay
all terminal metadata, stable sample IDs/positions/velocities, fields, infection and
stock. Rules/population unchanged, no human tuning inferred.

| Strategy | Outcome | Completed tick | Reclaimed | Applied edits |
|---|---|---|---|---|
| Passive | LOST | 900 | 1774 | 0 |
| Swept attractor | WON | 267 | 1780 | 5 |
| Stationary attractor | LOST | 900 | 1774 | 1 |
| Repel/reposition/erase | LOST | 900 | 1774 | 3 |

[Raw checkpoints/traces](phase6-evidence/mission-strategies.json),
[capture provenance](phase6-evidence/mission-metadata.json).
[ACTIVE60](../../concepts/exports/phase6-sweep-active.png),
[WON267](../../concepts/exports/phase6-sweep-won.png),
[LOST900](../../concepts/exports/phase6-passive-lost.png) and
[comparison](../../concepts/exports/phase6-strategy-comparison.png) visually inspected:
header, outcome, quota/deadline, ledger, applied-field world and controls readable
at the prototype canvas. Last three strategies overlap near the quota; magnified
plot shows this six-quanta margin, not balanced difficulty.

Reproduce after building a desktop preset:
`python scripts/capture_mission_examples.py --executable build/desktop-release/tests/integration/crucible_mission_examples --output NEW_DIRECTORY`.
Pillow/Matplotlib are only artifact-generation dependencies. Capture executable
uses actual coordinator/painter; source commit and command metadata retained.

## GPU receiver

Pinned SDL3 reused; local shader compiler and software Vulkan device execute.
Actual indexed instance submission/readback passed on llvmpipe. Portable arithmetic
checks .01 logical pixel; executing software-oracle comparison permits one byte
per interior channel under frozen analytic one-pixel edge masks. Retained/foreign/
expired receipts and camera-only redraw passed. Deterministic slot fixtures cover
busy/delayed reuse; actual fault/partial-creation/hang and three-inflight saturation
receiving remain P06-F02.

[Raw GPU frames](phase6-evidence/gpu-frames.json), [local provenance](phase6-evidence/gpu-metadata.json),
[matched initial world pair](../../concepts/exports/phase6-gpu-software-pair.png),
[evolved tick60](../../concepts/exports/phase6-gpu-evolved.png) and
[synthetic palette](../../concepts/exports/phase6-gpu-synthetic-palette.png) were captured
and visually inspected. The pair uses one completed tick0 snapshot and fitted camera;
GPU world-only output omits software HUD/fields. Synthetic colors do not implement factions.

Reproduce under an available X11 Vulkan loader (for example xvfb-run):
`python scripts/capture_gpu_examples.py --executable build/gpu-release/crucible_gpu_receiver --vertex build/gpu-release/gpu-shaders/world.vert.spv --fragment build/gpu-release/gpu-shaders/world.frag.spv --output NEW_DIRECTORY`.
The stdlib script retains executable/shader hashes, committed source tree/dirty status,
compiler, device inventory, pins and actual applied trace. Hosted evidence uses an
output directory outside the checkout. Original local images predate the final
exporter metadata refinements; hosted exact-head captures provide current provenance.
Physical public-platform receivers and performance remain open.

## Combined validation/publication

Local GPU Release31/31 passed42.89sec. Local Debug29 cases passed plus the strategy
rerun1/1 at241.10sec after the original180sec timeout; its bound is now900sec.
Local sanitizer's first10 cases passed, then execution transport disconnected;
completion and subsequent headless runs are unconfirmed. No interrupted result is
counted as a pass. Exact authored source recovery preserves the reviewed packages;
hosted final-head nine-job acceptance and GPU artifact remain the merge gates.
Use the permission-repair test entrypoint; preserve source/build/evidence checkpoints.
