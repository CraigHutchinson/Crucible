# Phase 8 receiving: straight flows and readable primitive tools

Status: original local receiving complete; recovered source and LFS images merged
in [PR 21](https://github.com/CraigHutchinson/Crucible/pull/21) at
`a95c5111991f441a451df144fdf443d2379a0939`. Recovery did not rerun acceptance.
Historical baseline
`50f61d89bb1324d2f3bdaa09e1b16bcf34fec6d9` (merged PR19). Implementation source
checkpoint `10cbd46` in isolated branch `phase8-isolated-receiving`.

The original `Crucible-phase8` worktree received unexplained concurrent edits from
an earlier session. New workers stopped writes; architect preserved the complete
tree at `828ecc4` and received it in `Crucible-phase8-receiving`. Original worktree
and edits remain intact. An isolated index initialization mistake was preserved
under `phase8-isolated-index-recovery` and corrected without changing source files;
the actual four-file preview fix is `83c420b`. No destructive source cleanup ran.

## Reviewed production paths

P8-F extends FieldEdit/FieldSet through the existing Simulation and Steering
callers. Owned observations and all full-state replay comparisons include the
endpoint. Independent analytical and all-pairs fixtures cover force, caps, overlap,
tiny/extreme finite geometry, rejected edits and retained snapshots. CLI/GPU JSON
and SVG branches were source-audited; those diagnostic scenarios remain radial,
so executed flow export acceptance is limited to the actual SDL painter below.

P8-P consumes SDL events, bounded admission and owned completed snapshots. Fixtures
receive drag/release/cancel, pause, clipping/letterbox, overflow, replacement,
terminal denial, restart and exact replay of actual routed event traces. Review
found stale accepted-preview retirement when a later refused command replaced
feedback; a separate admission sequence now retires that preview at confirmation.
The overflow-boundary-Escape regression receives the fix.

Reciprocal C++ reviews found no remaining behavior blocker. SceneUi documentation
was reattached to its type and toolbar index/result contracts documented. A first
Release selection-width pixel assertion failed on fractional raster-edge rounding;
the corrected independent column-coverage oracle tests visible thicker selection
without depending on one excluded edge pixel. Original failed output is retained.

## Local capability and tests

Linux GCC13.3/C++23, CMake4.4.3, Ninja1.13.2; retained full dependency pins and CPM
source cache. Development compile/runtime preflight passed. Native X11 SDK and
Xvfb are absent, so the local SDL build explicitly uses its console/software
profile (`SDL_UNIX_CONSOLE_BUILD=ON`, X11/Wayland off). This profile does not close
native-window or GPU receiving gates and is not a committed public preset change.

Normal sanitizer preflight fails before build on LeakSanitizer `/proc` access.
No leak disabling, fixture skip or substitute pass is claimed. Native X11,
Linux/Windows/macOS Debug/Release, normal sanitizers and optional executing GPU
regression remain hosted exact-head acceptance gates before merge.

First desktop Release run: 30/31 tests passed; selection-width oracle failed as
described above. Corrected Release scene fixture passed 1/1 without rerunning the
unchanged successful cases. Combined desktop Debug passed 31/31 in 293.63 seconds.
Run configured tests through `scripts/run_tests.py`. Headless receiving is recorded
in the reduced [test receipt](phase8-evidence/tests.json).

## Visual receiving

`python scripts/capture_flow_examples.py --executable
build/desktop-release/tests/presentation/desktop/crucible_scene_painter_test
--output <new-directory>` captures production software fit/zoom frames. The
fixture retains owned tick60 state with 2,048 samples, applied flow/attract/repel
and a white dashed uncommitted flow. Metadata records commit/tree/dirty state,
source/executable/PNG hashes, commands, dimensions and software classification.
Both exports were captured and inspected: [fit](../../concepts/exports/phase8-flow-fit.png)
and [zoom](../../concepts/exports/phase8-flow-zoom.png). Selected solid corridor and
rightward chevrons, shape-based radial cues and white dashed preview are distinct;
zoomed geometry stays clipped to the arena while the toolbar/HUD remain visible.
[Capture receipt](phase8-evidence/captures.json) preserves dimensions and hashes.
Only documentation was dirty during capture; painter/fixture match 10cbd46 exactly.
The generated resource and four-faction/deathmatch boards are separate art targets.

## Recovery publication

The original session's automatic approval review rejected publication. Later the
user explicitly authorized archive recovery and as-is merge; PR21 published the
recovered source and LFS payloads at `a95c5111991f441a451df144fdf443d2379a0939`.
That receipt closes the publication blocker without rewriting the historical
local validation above or inferring a new exact-head acceptance run. Current
behavior is rechecked in Phase9's hosted acceptance.

See [next investigation](../../phases/phase9.md) and the
[human preparation](../../playtests/reclamation-phase8.md). No physical device,
human comprehension, faction combat, networking or full-frame speed is established.
