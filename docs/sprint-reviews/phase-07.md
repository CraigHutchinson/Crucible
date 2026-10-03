# Phase 7 review: controlled GPU failures and frozen mission timing

Status: source implementation reviewed and locally received; final hosted acceptance
and publication pending, 2026-10-03. Architect accountable.
Dispatch baseline 0767a3033a84417c19bde9a4ff46dca0ce118f99.
[Plan](../phases/phase7.md), [receiving](../workstreams/integration/phase7-validation.md),
[next proposal](../phases/phase8.md).

## Delivered increment and ownership

P7-R consolidated resource/failure/retirement receiving under one renderer owner.
Linux test-only linker wrappers consume the unchanged production GpuOffscreen
archive and real SDL/Vulkan calls. Eighteen creation ordinals, startup/frame/map/
readback failures, sticky error state and owned handle accounting are received.
Three actual submissions test busy/pending immutability, selective completion,
slot reuse/old-receipt expiry, drain and distinct retained colors. Failed/hung
drains run in isolated children with exact markers, expected exit/timeout and reap.

P7-M remains independent. Short timing fixtures run in default CI; one explicit
Release shootout consumes the same canonical route and unchanged 1780/900/2048
rules. Full terminal states replay independently. The human protocol is preparation,
not participant, accessibility or balance acceptance.

P7-I owns wiring, CI, serial local CPU and publication. Workers self-reviewed and
reciprocally reviewed; root corrected handle/ownership scope, CLI/export refusal,
bounds and subprocess marker findings before receiving. Explicit REQUIRED_FILES
native prerequisites extend narrow execute-bit repair, verified with a deliberate
process-only permission-loss probe and five permission fixtures.
No production renderer/API/shader, numerical rule or dependency pin changed.

## Findings and learning

| Finding | Resolution | Limit |
|---|---|---|
| Test-specific renderer compilation would diverge from production | Choose linker wrappers over a duplicate renderer and an unconsumed call-table API | Linux ELF fixture scope only |
| Three slots alone do not prove executing retirement | Real submissions with deliberately withheld queries and selective completion | Controlled visibility, not physical GPU latency |
| Python test hides its native subprocess | Explicit CTest REQUIRED_FILES and owned-native preflight | Never chmod arbitrary arguments, external tools or source files |
| Recovered Xvfb missing its compiled helper | Restore available helper, receive unchanged actual GPU cases | Software Vulkan device only |
| Local LSan cannot inspect /proc task processes | Preserve fatal logs and require hosted leak-enabled normal/GPU checks | Local sanitizer is blocked, not green |
| Startup has no seed API | Bound investigation to three timing perturbations | No initial-world robustness claim |
| Fewer edits finish earlier in this fixed world | Retain outcomes with unchanged quota and full replay | No winning cadence or balance tuning |
| Execution disconnected during final evidence documentation | Source checkpoint already pushed; continue hosted receiving and preserve original local files | Local final review/reconciliation and original capture publication remain P07-F01 |

## Observed acceptance and visuals

Full local GPU Debug passed 34/34. Release passed 31 non-GPU cases and its three
environment-blocked GPU cases subsequently passed after the helper correction.
SDL-free Release passed 28/28. The full GPU sanitizer built but 32/34 cases report
the same LSan /proc fatal error; leak checks remain enabled. Exact-head hosted nine
jobs, including normal sanitizer and three actual GPU cases in Debug/sanitizer,
remain required before merge.

| Timing case | Outcome / tick | Reclaimed | Applied edits |
|---|---|---:|---:|
| cadence30-start0 | WON / 297 | 1780 | 10 |
| cadence120-start0 | WON / 271 | 1780 | 3 |
| cadence60-start60 | WON / 449 | 1780 | 7 |

All three pass conservation, full-state replay and stopped admission. Root inspected
six actual painter frames, three retained GPU color frames, the checkpoint plot and
receipt-order table before disconnect. Logical state captures are 1280×720. The
delayed-start tick-60 image correctly has no field before application at tick61.

Original local PNGs/raw metadata/results/logs were copied into Git-backed evidence
paths but final review/publication is unconfirmed after environment_offline.
The workflow now retains a fresh Release study and GPU captures in the same hosted
90-day artifact; it does not imply original files were recovered. Artifact receipts
and hosted acceptance will be recorded at closure. Forced-exit/killed child fixtures
bypass exit-time LSan; normal paths keep leak checks. Images do not prove physical
device loss, performance or human comprehension.

## Retrospective and next structure

One rendering/lifetime owner avoided dividing a common failure contract.
Short default fixtures and one Release investigation avoid repeated full timing
studies on every desktop runner. Publish reviewed source checkpoints before long
execution; preserve known passes and unconfirmed runs without rebuilding blindly.
The workflow avoids duplicate checkpoint-branch/PR matrices and cancels obsolete
heads for the same PR. Remote receiving is a recovery path, not permission bypass.

R07/R08 remain local; SDL/H2/sub0 pins are unchanged. No exact-version upstream
defect or second reusable consumer justifies extraction. The Phase 8 proposal
keeps shader production/one second receiver consolidated, compares build-time and
runtime candidates before selecting, and independently receives shape-based field
inspection cues. Audit actual public device/tool availability before backend split.

## Retained follow-ups

| ID | Status / accountable owner / gate |
|---|---|
| P06-F02 | Partial after controlled receiving; rendering retains actual device loss, driver hangs and physical delayed saturation before default promotion |
| P05-F02 | Partial; rendering/integration requires physical public devices, second backend and complete world/HUD 2K/100K/150K measurements |
| P05-F01 | Open; mission owner observes unaided human comprehension, controls and balance on recorded devices/builds |
| P04-F01/F03 | Open; platform owner receives iOS packaging/touch/signing/lifecycle and physical devices |
| P04-F02 | Open; presentation receives DPI/accessibility/input-to-present; proposed field cues are bounded work |
| P06-F01 | Groundwork; contracts/simulation freezes organism/faction/controller/resource identities, relations, transitions, commands and replay at first consumer |
| P02-F02 | Open; runtime/scheduling needs actual delayed/concurrent consumer before exchange/leases |
| P01-F03/F04/F05 | Retain; simulation/scheduling freezes structural resource, fusion/relay and parallel-scale workload before implementation |
| P02-F03 / P03-F01 / HX-07 | Retain; spatial/world owner receives declared terrain/topology/traversal and exact geometry gates before pin changes |
| P07-F01 | Open; integration recovers original local evidence bytes/hashes and reconciles retained source trees after environment_offline; local baseline verification remains required before dispatch |

## Publication

[PR17](https://github.com/CraigHutchinson/Crucible/pull/17) receives this increment.
Remote checkpoints d188c6e/9ec371b/0e48748/9a99293/1712c89 matched the committed local
trees; original commits are retained by local checkpoint tags. Final documentation/
workflow publication uses the authorized connector while execution is offline.
It does not claim local reconciliation. Preserve all Phase 6/7 branches, builds,
raw captures, failed outputs and worker documents; no cleanup is dispatched.
Exact final-head CI/artifact receipt and actual merged baseline follow at closure.
