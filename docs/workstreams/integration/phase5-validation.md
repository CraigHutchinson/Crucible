# Phase 5 receiving evidence

Base `0bc9731`, 2026-10-03. [Plan](../../phases/phase5.md),
[review](../../sprint-reviews/phase-05.md), [rules](../../decisions/phase5-reclamation.md).
Complete: [PR 12](https://github.com/CraigHutchinson/Crucible/pull/12), merge
`57c1da65c2e6e74a6f9b7f76f596fb8d36bf590e`.

## Actual consumers and checks

Desktop opts into the reference mission and passes owned progress beside the owned
snapshot to ScenePainter. Headless `crucible --mission` and `--mission --route` use
the same coordinator and independently replay terminal state before reporting.
The route moves slot-0 radius 8 / strength 4 attractor every 60 completed ticks across
x=8/24/40/56 at y=8 then y=24, repeating. Receiving experiments revised quota 6,144 to
1,780. [Raw outcomes and metadata](phase5-evidence/metadata.json) retain commands,
scope and pre-tuning results; these are outcome proofs, not timings or human playtests.

`runtime_mission` checks independent five tick-one contacts and thirteen further
tick-two contacts, quota/deadline precedence, first-terminal catch-up stop, schedule
equivalence/full-state replay, owned observation, pause/blocked/closed admission,
restart and invalid settings. Clock separately tests stop observation, carried-time
discard and exception latching. No-mission fixtures remain intact.

Painter compares actual software text to expected labels and bounded bar pixels
for ACTIVE/WON/LOST; world/ledger regions stay identical. Preflight rejects invalid
views before clearing. Actual desktop event fixtures instantiate configured wins/
losses, deny terminal clicks, refuse pause/foreground revival and restart fresh.

## Local verification

Linux x86_64 managed host, GCC 13.3.0/CMake 4.4.3/Ninja 1.13.2. Desktop trees opt into
the prior rendering spike, whose world raster/lifetime oracle still passes.

| Configuration | Result |
|---|---|
| desktop-release | Unfiltered 26/26 passed after workflow addition |
| desktop-debug | Unfiltered 25/25 passed |
| release, headless | Unfiltered 24/24 passed; both CLI outcomes/full-state replay; no SDL build directory |
| desktop-sanitize | All 25 covered: 24 passed unfiltered; phase3 could not launch due generated executable permission; restored mode and targeted rerun passed |

Local ASan/UBSan uses `ASAN_OPTIONS=detect_leaks=0` because managed leak inspection
is blocked; hosted sanitizer stays normal. No changed-source warning, dependency/pin
update or unsupported device claim. Hosted head `94d2e269c5c6620d7a21acdd6b3629222ff3bdde` passed all eight jobs: Linux/Windows/macOS
Debug/Release and normal Linux sanitizer 26/26 each, headless 25/25 with SDL absent,
both native X11 smokes. [Run](https://github.com/CraigHutchinson/Crucible/actions/runs/37123756682).
Published tree matched reviewed local tree and merged main was verified clean.

## Permission-loss prevention

`scripts/run_tests.py` inventories selected CTest commands before execution and repairs
missing execute bits on owned ELF/Mach-O artifacts under `build/`. It ignores source,
external commands, direct symlinks and missing artifacts; real failures propagate
without retries. CI, README and contributor commands use this entry point. Four
permission fixtures pass; deliberately stripping execute permission from Release
`runtime_mission` produced one reported repair and a passing test. The new workflow
CTest also passes in headless Release (25 cases now configured). Debug/sanitizer
C++ evidence above predates this Python-only addition; hosted suites receive it.

## Visual artifact

[Actual mission frame](../../concepts/exports/phase5-mission-frame.png): tick 60,
2,048 samples, applied attract(24,16)/repel(48,16),radius 8 / strength ±4; 1,753/1,780,
840 ticks left ; ledger 10,240=6,439+2,048+1,753. Production coordinator/painter output,
distinct from generated art and the winning swept route. Title, progress, instructions,
world clipping and controls checked visually; prototype typography remains a gate.

```sh
cmake --preset desktop-release
cmake --build --preset desktop-release --parallel 4
build/desktop-release/tests/integration/crucible_desktop_event_test --export-mission output.bmp
```

Convert BMP to PNG without changing content. Ordinary release stays SDL-free.

## Review and limits

Providers performed C++ self/reciprocal review; architect reviewed callback capture/
teardown, capacity, stopped failures and final CLI/event/export wiring. Final peer
review found no unresolved actionable findings. Closed-input feedback was fixed;
initial quota revised using receiving evidence. Restart allocation failure has no
injected fixture; construction-before-retirement reviewed directly.

Long routes are Linux Release outcome evidence. Six-quanta margin over passive
recovery does not establish robust difficulty. No human playtest, native latency,
GPU instancing/shader/upload/fence/device or iOS package ran locally. Promotion must
pass [instancing gates](../../decisions/render-instancing.md).
