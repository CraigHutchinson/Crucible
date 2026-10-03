# Phase 4 combined receiving evidence

Base `647f536b0a37b48f08b1b58b0c791e2ec850bf4b`; 2026-10-03.
[Plan](../../phases/phase4.md), [decision](../../decisions/phase4-rendering.md),
[review](../../sprint-reviews/phase-04.md).

## Delivered boundary

P4-01 Camera2D/FieldTool local handoff f0c0129, integrated cd7aea1; P4-02
ScenePainter local handoff e16c8d6, integrated 9aa9d6f. These are retained local
source-history IDs, not public commit links. Architect owns InspectorSession,
SDL callback/window/event lifetime, shared toolbar, tests/wiring/platform CI.
Both workers completed reciprocal cpp-review; architect reviewed integration and
ran all builds. Maximum architect plus two workers; no worker heavy-CPU runs.

Production `crucible_desktop` owns one reference finite-reclamation scenario,
sequential retained frame and concrete batch painter. Native/software fixtures
consume that painter. Pointer and keyboard actions use the same event router;
window coordinates pass through SDL's logical conversion before Camera2D.
Restart constructs a replacement before closing the old ingress and destroying
its borrowers; no old queued edit or replay trace survives. SDL callbacks stop
on errors, guard off-main-thread input before owner access and reclaim the owner
before SDL teardown. There are no producer threads or retained ECS views.

## Executed local evidence

Linux GCC13.3, CMake4.4.3, Ninja1.13.2; exact pinned SDL3 source3.4.18.
`cmake --preset debug`, build and unfiltered CTest: 21/21 pass, no SDL dependency.
`desktop-debug`: unfiltered 23/23 pass; after lifecycle/input refinements, focused
software/event/runtime/camera/tool gates pass. `desktop-release`: 23/23 pass.
`desktop-sanitize`: all 23 tests pass across the unfiltered run plus a focused rerun
of three executables whose workspace execute permissions needed restoring. Local
`ASAN_OPTIONS=detect_leaks=0` is required because this managed runtime prevents
LeakSanitizer process inspection; no sanitizer code/preset suppression was added.
Hosted CI uses normal ASan/UBSan/LeakSanitizer settings.
Legacy release executable: 150000 entities, 60 ticks, checksum225000; existing
phase2 full-state/reclamation/H2 oracle and allocation fixtures remain included.

Local SDL configuration used scratch-extracted X11 development packages and
explicit CMake prefix/pkg-config paths because privileged apt installation is
unavailable. No project fallback flags were added to conceal SDK failures.
Native X11 window/event/resize smoke passed under Xvfb with the software renderer,
including actual asynchronous resize synchronization. It uses the production
DesktopApp event/draw path. Dummy video event fixture is independent of the native
smoke. Neither is a physical GPU/device playtest.

Independent camera examples cover affine mapping, nonbinary boundaries/exterior
rejection, finite arithmetic and unchanged invalid updates. Runtime tests verify
paused queued admission, full queue rejection, completed trace/frame publication,
fresh restart, trace-full blocked run, close and independently reconstructed
full sample/infection/stock replay. Event tests exercise signed fields, toolbar
pause/restart, camera/wheel, lost-focus drag, overlapping background/minimized
reasons, deliberate pause preservation, resized letterbox rejection and mapping.
Software fixtures read independently chosen pixels for stock/infection palette,
nanite overlay, clipping, committed versus preview rings, capacity rejection before
draw, retained-frame immutability and finite extreme/zero-radius fields.

The [actual frame](../../concepts/exports/phase4-live-frame.png) was exported from
`crucible_scene_painter_test output.bmp`, converted losslessly to PNG and visually
inspected. Tick60, 2048 samples, two applied fields and one uncommitted preview;
ledger10240 = 6439 stock + 2048 mobile + 1753 reserve. This is simulation data,
not generated concept art. No throughput, GPU or game-balance claim follows.

## Review refinements and limits

Peer review caught applied feedback erasing fresh previews; confirmation now consumes
its admission once. Independent background/minimized reasons prevent premature
resume. Cosmetic tool/slot/fit changes preserve elapsed clock time; pause/restart
reset the baseline. Focus loss cancels a drag whose release might be lost.
Shared button geometry prevents hit-test/paint drift. SDL resize is asynchronous:
native fixtures wait for completion before coordinate assertions.

Hosted CI gates Linux/Windows/macOS Debug/Release desktop packages, ordinary Linux
headless Release and Linux ASan/UBSan, with native Linux Xvfb smoke. Exact-head
workflow results and the merge baseline are recorded in the completed phase review.
iOS packaging/signing/touch and physical-device lifecycle/graphics remain open.
No new backend abstraction, GPU shaders/retirement, concurrent frame exchange,
structural growth/fusion, mission outcome or hierarchy was introduced.

The first hosted macOS run exposed a pre-existing stack toolchain requirement:
Apple SDK libc++ lacks Pipeline's `std::stop_token`/`std::jthread`. The macOS presets
now select Homebrew LLVM21 and its matching headers/libc++/libunwind together,
using the [vendor's documented runtime and availability selection](https://formulae.brew.sh/formula/llvm@21).
No Pipeline API shim or reduced tests were added. This developer toolchain does not
establish Apple bundle redistribution/signing; that remains a packaging gate.
