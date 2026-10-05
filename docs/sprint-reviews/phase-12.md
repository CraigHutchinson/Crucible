# Phase 12 sprint review: runtime budgets and bounded diagnostics

Status: receiving complete, 2026-10-05; final documentation-head CI/merge pending.
Architect accountable. [PR25](https://github.com/CraigHutchinson/Crucible/pull/25).

## Intent, baseline and scope

Baseline PR24 merge `835c781`; [plan](../phases/phase12.md). Production overhead
and bounded Sub0Log outcomes are received. Dedicated Windows rendering/input
provides consumer feedback for [Phase13](../phases/phase13.md), not human balance
or sound acceptance. Global and project guidance now require top-down consumer
concepts and bottom-up verified foundations to meet in a received vertical slice.

## Delivered work and division

Measurement worker delivered actual production benchmark/capture and exact-source
Pipeline allocation audit. Diagnostics worker delivered fixed backing, decoder,
lifetime fixtures and Windows display-candidate preflight. Architect received
optional CMake/runtime/desktop integration, full-state parity and all serial
build/test/measurement/graphics work. Game rules remain local; no upstream changes
or new pins were necessary. Implementation commits `9c3fdfe` and `1eef613`.
Independent cpp-review findings were resolved before receiving: direct test Log
dependency, optional startup allocation fallback, constructor documentation,
Release capture provenance and explicit one-summary-per-run cadence fixture.

## Findings, budgets and disposition

| Finding | Resolution/evidence | Limit |
|---|---|---|
| Pipeline resets three node stop_sources per run | Actual warmed integrated pump3 allocations/96 requested bytes; first6/124. Retain pinned graph and this baseline | No zero-allocation graph or speedup claim |
| Diagnostics must not alter simulation | Fixed64KiB session image; copied numeric refusals/one terminal summary per run; omission/exhaustion/restart parity | Decode is cold allocating work; bounded prefix/drop counts, no recycling/crash persistence |
| Logger active binding is process-global | Scoped restoration per emission; coordinator-only | No concurrent logger guarantee |
| Unused dependency must disappear | OFF by default; CI production graph verifies omission | BUILD_TESTING independently receives Log stack fixture |
| Native HUD has tiny text at captured scale | Actual queued/applied flow screenshots; Phase13 scalable presentation/hit geometry gates | Operator evidence only; no participant readability acceptance |
| Existing GPU receiver was locally unreceived | Actual physical Vulkan readback passed | Offscreen, not live desktop GPU/HUD presentation or named RTX selection |

[Production budgets and raw evidence](../workstreams/runtime/phase12-budgets.md)
cover five uncontended alternating independent-process pairs at64/2048 samples,
267/448-tick WON missions, startup/first/steady/admission and diagnostic exhaustion.
Admissions and diagnostic emissions allocate zero replaceable C++ objects.
Timings are advisory: probe/timer resolution, OS load and unmeasured thermal state
prevent throughput/default-promotion conclusions. Rendering is excluded.

## Verification and artifacts

At implementation source `1eef613c08d72535975aac9c7ce01c31f98efb9c`:

- Windows MSVC19.51/CMake4.2.3/Ninja1.13.2/Python3.14.5: initial full Debug41/41;
  final Debug rebuilt and affected runtime/mission/structural receiving14/14
  passed in217.67s.
- Full Release42/42 passed in33.89s, including actual Vulkan instancing/readback,
  palette variants, numeric oracle and receipt lifetime. Intel/RTX5070 physical
  inventory establishes availability; the test did not print selected device.
- [CI run95](https://github.com/CraigHutchinson/Crucible/actions/runs/37289449859):
  all9 jobs passed: Linux/Windows/macOS Debug/Release, normal LinuxASan/UBSan,
  default-OFF headless/production dependency omission, software Vulkan receiving.
  Windows sanitizer canary is separate, not a full local sanitizer/LSan claim.
- [Native interaction receipts](../workstreams/integration/phase12-evidence/native-receiving.md):
  actual terminal/restart/pause, queued/applied FLOW and Fit View; captures retained.
- [Executed decoder transcript](../workstreams/integration/phase12-evidence/diagnostic-transcript.txt):
  synthetic two-run terminal summary cadence, separately from native operator runs.

Local logs/preflight receipts remain under `build/phase12`; meaningful paired raw
provenance is Git-backed. Final documentation-head CI is the merge gate.

## Two-way reconciliation and retrospective

Bottom-up receiving establishes ordering, owned frame publication, bounded log
loss and exact allocation costs. Top-down native interaction exposes a readable
state headline but insufficiently large detailed HUD and edge-constrained tools.
The next slice prioritizes understandable command/status feedback and a bounded
sound prototype, weighing fidelity against verified ownership/timing/lifecycle.
Physical offscreen graphics do not justify switching the desktop backend yet.
Keep policy local; demonstrated neutral missing contracts require a minimal
receiving reproduction before upstream library evolution.

## Follow-ups and closure

P11-F01 measured allocations and P11-F02 bounded diagnostics close for this scope.
P12-F01 presentation/readability/input coherence goes to Phase13 architect;
gate: real display-scale screenshots and matching hit targets. P12-F02 sound
concept/fidelity goes to Phase13 consumer owner; gate: executing bounded audio
cue/background lifecycle and separate listening feedback. Phase13 is proposed,
not dispatched. P11-F03 concurrency, P09-F01/P05-F01/P04-F02 human tuning/input,
P01-F03/F04 growth/scale, P06-F01 factions/combat, P04-F01/F03 iOS,
P02-F02 observation and P02-F03/P03-F01/HX-07 world gates remain open.
P05-F02/P06-F02 physical offscreen receiving is now evidenced on this host;
live renderer promotion/second backend/device-fault scope retains its own gates.

Worker file and CPU claims release after handoff. Historical worktrees, branches,
builds and sibling work remain preserved. PR25 final-head CI/merge/local-main
receipt will be recorded at closure; do not use an unmerged phase as next baseline.
