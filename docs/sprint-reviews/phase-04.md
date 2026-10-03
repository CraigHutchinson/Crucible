# Phase 4 sprint review: interactive inspection and control

Status: complete and merged, 2026-10-03. Base647f536. Accountable reviewer: architect.
[Plan](../phases/phase4.md), [rendering/data decision](../decisions/phase4-rendering.md),
[combined evidence](../workstreams/integration/phase4-validation.md).

## Delivered and reviewed

Portable camera and discrete field commands are consolidated in P4-01; startup-batched
SDL drawing is P4-02; architect integrates owned run/frame/event/window lifecycle.
Existing gameplay/ECS/resource/spatial modules are retained. No virtual renderer or
concurrent exchange is needed by this consumer. Field placement, feedback, pan/zoom,
pause/restart and finite-resource display have actual production callers.

Peer review found and resolved stale application feedback clearing previews,
overlapping suspension reasons, cosmetic controls discarding elapsed time and
lost-focus drags. Native resize fixtures explicitly wait for asynchronous completion.
Apple hosted tests also exposed FMA cancellation at fitted camera corners.
Canonical endpoint interpolation fixes the defect while preserving strict exterior
rejection; root reproduced the failure/fix locally and the provider peer-reviewed it.
Reciprocal provider and root lifetime/numerical reviews found no remaining blockers.
See combined evidence for independent oracles, builds, actual-state image and limits.

## Retrospective and next division

The consumed ECS boundary is contiguous owned world-space data. Concrete painters
can be selected by the build; introduce interfaces only when actual implementations
establish operations that vary. SDL supplies a first public desktop adapter without
making platform types part of Simulation/Runtime. Keep the current pure camera,
batch drawing and coordinator ownership boundaries; avoid a permanent agent per
module. Use the next two-worker increment for a consumed mission/interaction rule
and its playable presentation acceptance, consolidated where dependency requires.
iOS packaging/touch/device work is a separate toolchain gate; do not infer support
from SDL capability or desktop fixtures. GPU uploads require measured workload and
explicit retirement ownership before new abstraction/exchange work.

## Follow-ups

| Stable ID | Action / state | Accountable role / gate |
|---|---|---|
| P01-F04 | First input/display implemented; complete playable mission still partial | Presentation/game architect; mission goal/progression and real interaction playtest |
| P04-F01 | Open: public iOS package/touch/device acceptance | Platform architect; Apple build/signing, logical touch, background/foreground and physical-device proof |
| P04-F03 | Open: Apple redistributable bundle/runtime validation | Platform architect; package/sign runtime dependencies, validate native Apple toolchain before distribution |
| P04-F02 | Open: complete native workload/device measurements and final typography | Presentation/validation; input-to-present measurements, readable/DPI/device playtests before renderer scale claims |
| P01-F03 | Finite ledger delivered; full W6 structural growth/fusion remains partial | Resource architect; capacity/identity/commit audit |
| P01-F05 | Deferred: structural/concurrent/measured scale | Integration/scheduling; capacity/generation/join and complete workload |
| P02-F02 | Deferred: concurrent frame exchange | Presentation; actual reader/upload and retirement consumer |
| P02-F03 | Deferred: height/mining/bridges/sphere | Game/geometry; conservation/traversability/metric gates |
| P03-F01 | Open: quantify compatibility/candidate/storage costs | Spatial/validation; fallback frequency and tail work |
| HX-07 | Receiving pack retained; no upstream source edit required | Geometry owner; oracle pack before hierarchy decisions |

## Reuse and disposition

Keep Camera2D, FieldTool and InspectorSession local to the fixed interactive contract.
Reuse SDL3 pinned at829a65d769d935c4852f8159e964312c0957260a and retain the verified
H2 pin/oracle. No upstream defect or measured optimization was found. Worker branches,
worktrees and build/visual artifacts are retained; no unrelated session was cleaned up.
## Publication closure

[PR 9](https://github.com/CraigHutchinson/Crucible/pull/9) merged at
`09ae74f9f2a1a05c95bb5e1d4196256880cb55a7` from exact head `250aecaa949adc8971acbda629701dfa6f012dec` after
[CI](https://github.com/CraigHutchinson/Crucible/actions/runs/37110577249) passed all eight jobs: Linux/Windows/macOS Debug/Release
23/23 each, ordinary Linux headless Release21/21 and normal Linux sanitizer23/23.
Both Linux native X11 event/resize smokes passed. macOS used Clang21.1.8 with
matching libc++/libunwind; Windows used MSVC19.51.36260.0. The final published tree
matched reviewed local tree `a1d8ce30e74067d549e9aa18beb52849943aec22`.

Local main was verified clean and equal to origin/main at that merge. This
small documentation closure records the known merge without changing source.
Start the next increment from merged main; all worker write/CPU claims are complete.
