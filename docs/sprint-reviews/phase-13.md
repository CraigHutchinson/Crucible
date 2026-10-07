# Phase 13 sprint review: readable player controls and bounded sound

Status: in progress, 2026-10-07. Root accountable. P13-01 is implemented and under
receiving; P13-02 sound remains undispatched. This is not Phase13 closure.

## Intent, baseline and scope

Dispatch clean main `5b84ccd` (PR28), following PR26/27 pin refresh. The
[coherence review](../decisions/phase13-coherence.md) reconciles actual structure,
SDL, diagnostics and ownership with the [plan](../phases/phase13.md). Top-down:
read objective/commands, steer, understand receipts and operate the relay.
Bottom-up: unchanged owned snapshots, camera, SDL renderer and sequential runtime.
Their received slice is a readable HUD, shared targets and reversible fullscreen.
Sound has no audio-enabled build/device acceptance yet and retains P13-02.

## Work delivered and division

Root consolidates presentation/input and shared desktop wiring. Added larger body
text, two toolbar rows, visible fuse/shatter controls, bounded two-row feedback,
`--fullscreen`, F11 and Escape-to-window. Disabled structure controls do not submit
legacy commands. Fixed camera/world geometry and existing game rules are retained.
AGENTS.md explicitly adopts the user-selected sub0 authoring/review profile.
No new workers, dependency pins, runtime APIs, framework or simulation concurrency.

## Findings and resolutions

| Finding | Resolution/evidence | Remaining gate |
|---|---|---|
| Phase13 baseline predates pin/ownership updates | Dispatch from PR28 and reconcile current catalog/roadmap/design | Pins retained, no new upstream requirement |
| Tiny text and cramped single-row toolbar | 1280x864 canvas, 20px body font cells and shared two-row rectangles | Native DPI and participant readability |
| Structural actions only discoverable by keys | Visible F FUSE / X SHATTER; production event receiving while paused | Human comprehension/balance |
| Admission can be mistaken for application | Queued preview and feedback retained until completed trace; full replay fixtures | Sound cadence stays separate |
| Audio build is explicitly disabled | P13-02 remains undispatched with lifecycle/device prerequisite | Actual sound/listening |
| Historical tests hardcode old display geometry | Resize/replay fixtures and all production-painter exporters updated | Hosted platform receiving |
| Existing C++ names differ from sub0 | New declarations use camelCase; existing API/file migrations remain bounded follow-up | No repository-wide style compliance claim |

## Verification and useful artifacts

Windows MSVC19.51, CMake4.2.3, Ninja1.13.2, Python3.14.5. Development canary passed.
Fresh build trees use checksum-verified CPM0.42.1 and read-only cached sources whose
Git heads match all six full pins. No shared dependency cache edits. Release full
42/42 passes (36.13s) with diagnostics enabled. Full Debug42/42 passed
(203.62s); final changed desktop/painter receiving4/4 passed after the resize
review fixes. Hosted platform/sanitizer acceptance pending.
The receiving runner is `scripts/run_tests.py` with the repository preset plus
`--test-dir` / `--prerequisite-build-dir` for the isolated tree.

Actual production SDL software output at1026x607 and2048x1280 reports13/25 occupied
body-text rows respectively. Small capture inspected: all controls fit, FLOW
selection is visible, queued feedback wraps, and world geometry remains separate.
Files/provenance reside in `build/phase13-evidence`; hosted Linux Release captures
the same receiver as the `readable-desktop-controls` artifact with source/tree state.
These are software output and dummy-window lifecycle fixtures, not physical monitor,
hardware input-to-photon, operator fullscreen or participant evidence. No new
timing/allocation/performance result is claimed.

[Sub0 code-quality review](../workstreams/presentation/phase13-review.md) records
resolved findings and existing style deviations. After automated receiving, use
the [personal native/participant checklist](../playtests/phase13-readable-controls.md).

## Two-way reconciliation and reuse

Extra vertical space and larger primitive text solve the bounded output problem
without changing world/GPU geometry. This trades screen area for legibility; native
receiving may revise that choice. Fullscreen stays opt-in until physical startup,
toggle/focus/geometry and usability evidence justifies promotion. HUD/bindings stay
local; no neutral library gap needs extraction or upstream API growth.

## Follow-ups and next balanced slice

P12-F01 has portable presentation/input receiving here; native DPI/fullscreen and
participant gates remain open. P12-F02 maps to P13-02: enable the consumed SDL audio
profile, freeze owned clips/background/cue coalescing/mute/pause/restart/failure,
receive asynchronous shutdown and actual output, then gather separate listening
feedback. Carry all Phase12 deferred growth/scale/world/faction/concurrency/iOS and
physical GPU/human gates with their existing owners. P13-01 publication/CI is pending;
Phase13 remains in progress until its separately received scope is reconciled.
