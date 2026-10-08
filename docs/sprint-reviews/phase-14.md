# Phase14 sprint review: interactive large-swarm performance

Status: in progress, execution authorized2026-10-08; planning finalized2026-10-08.
Accountable architect/reviewer: root. This record follows the review template;
execution fields stay pending rather than fabricating completion evidence.

## Intent, baseline and scope

[Plan](../phases/phase14.md), [contracts/review](../decisions/phase14-scale-contracts.md).
User chose full-frame performance plus bounded parallel simulation. Top-down:
read/control a continuous large swarm through FLOW/gather/repel and camera changes.
Bottom-up: receive immutable queries, bounded dispatch/joins and complete native
frame costs. Target both100K and150K evolving populations on a qualified reference
configuration; preserve the2K relay regression. Planning main `9e7fed990c404e95ab25e699090996aeec048040`
is PR29's merge; all9 checks succeeded at49f702a in run37619200451.

## Work delivered and delegation

Planning checkpoint `c801123` precedes user-authorized execution. A received
immutable Spatial queries and the shared Swarm row kernel at `e6b242d` (integrated
as `526ed66`), with seven focused tests passing in both Debug and Release. B
received view-only density/counts and bounded D3D11 completion observation at
`5d2238f` (integrated as `58d6b8a`), then explicit checked D3D11 presentation at
`469bc6f` (integrated as `94bf512`). These are provider checkpoints, not major
milestone acceptance.

Root promoted Sub0Pub to verified merged main `d566c71` and migrated sink teardown
to `unsubscribe` at `3f85c82`; fresh Release43/43 passed. Shared startup settings
receive continuously evolving100K/400x250 and150K/500x300 worlds through Runtime
and desktop at `4e80534`. Restart preserves geometry/policy and changes run identity.
`93c1d9a` wires the immutable whole-input row seam into actual Simulation staging;
eight affected Release checks pass, including full-state direct/integrated scale
commands, pause, resource conservation and restart. `75e2868` consumes checked
native presentation in Windows scale mode; the explicit native fixtures passed
at the later `fd8075e` checkpoint as recorded below.

The upstream Pipeline prerequisite is merged and received at exact pin `f730c4e`.
A now owns the bounded Scheduling provider; B owns the production capture harness
and independently reviews the provider and shared coordinator wiring.
Root retains Contracts/Simulation/Runtime/desktop, shared inventories/pins/CI,
central records, independent integration review, serial CPU/GPU receiving and
publication. Parallel consumption is being received in Simulation; the default
remains one worker until matched performance gates support promotion. No physical
frame-budget or useful-parallelism result is claimed.

## Findings and resolutions

P14-R01-R08 are recorded with actual source evidence in the decision. Shared scratch,
allocating dispatch, coordinator graph ownership, fixed-world density, terminal-frame
FPS and offscreen-versus-live presentation require explicit receiving gates. Design
resolves decomposition; executor/API/backend/metric freeze remains G0 work.
The old architecture pin labels and P13-01 publication status are reconciled.
Independent draft review also found a circular G0/backend prerequisite, missing
all-stream supplement, weak route influence, ambiguous completion/paused latency,
discretionary sample length and implicit worker FP/dependency direction. Root
split G0a/G0b, supplied W0-W10 ownership plus affected briefs, and tightened those
criteria before the final plan review. The pipeline choice is a prerequisite
investigation; no implementation consumer is declared ready by prose alone.
Final correction preserved structural command ordering before movement and allowed
G0a capture probes to inform G0b. Useful front motion requires field attribution,
not initial drift. Consumer reviewer closed four findings and supplied that last
clarification; foundation reviewer confirmed its six original corrections and
supplied the two remaining wording fixes before a usage-limit interruption. Root
applied/checked all three final clarifications; no further independent pass is claimed.

Execution source review found that this SDL pin ignores queued-flush failure in
`SDL_RenderPresent`, hides backend present failure, and treats D3D11 busy/occluded
results as success. Clearing SDL errors cannot establish a handoff. The selected
bounded receiving path uses public SDL interop to flush, call native Present1 once,
classify its actual result and invalidate SDL state; default2K presentation stays
SDL. GPU completion remains a separate observer receipt. Actual native lifecycle
receiving must establish this path before G0b/G3/G4 adoption.

## Verification and useful artifacts

Planning evidence: clean git status/worktree inventory, source inspection, exact
Pipeline cache-head match, live GitHub PR29 merge/all9-job receipt and read-only CPU/
GPU model inventory. Documentation references/consistency and independent plan
review establish the planning baseline. Combined provider Release receiving at
`45acf15` passed47 of48 cases; the workflow fixture alone failed because the shell
used a sandbox TEMP directory. Re-running that fixture with owned TEMP passed1/1.
The later production row checkpoint passed its eight affected checks. Combined
final-head unfiltered and supported platform/sanitizer/race acceptance remain open.

The initial native D3D11 completion fixture passed capacity/refusal/correlation,
drain/new-run and synthetic device-property replacement receiving against the
original completion-only source. It is not physical device loss, checked native
presentation success, scanout, visual quality or performance evidence. The checked
revision then passed its explicit native query/handoff fixture. Production desktop
receiving at `fd8075e` passed actual D3D11 world/HUD handoff, completion drain,
resize/fullscreen while paused, restart identity and resumed evolution, and close
at both100K and150K. Renderer reported direct3d11, pixel density1.00, display scale1.25.
The small software lifecycle fixture first exposed event handling resetting the
clock baseline; processing those events before the test's elapsed interval corrected
the fixture, then passed. This is lifecycle behavior, not a timed sample.

Fresh combined Debug passed48/48 at `93c1d9a`; all six affected checked desktop
fixtures then passed in Debug and Release after the lifecycle correction. Actual
captures/raw timelines, physical G3-G6 quality/budgets, audio and participant
results are pending; diagrams here explain proposals only.

Hosted [run37806301603](https://github.com/CraigHutchinson/Crucible/actions/runs/37806301603)
passed all ten jobs at `b3f73ba19bbc98d726f268edc8a4795c7b0a7508`: Windows,
Linux and macOS Debug/Release, headless dependency omission, ASan/UBSan, software
Vulkan receiving and the new actual ThreadSanitizer query/partition/runtime
receiver. This precedes native capture-harness integration and parallel wiring;
neither later source nor concurrent worker execution inherits that receipt.

The production native frame harness was received as source at `ad48618`, with
shared benchmark registration at `c309aba`. It retains per-frame/command identity,
checked handoff and joined GPU observation, actual adapter/driver metadata,
explicit60Hz pacing, cold exact-tick route attribution and pre-present overview/
detail images. Classifier, executable and physical probe receiving remain pending.
The observed sequential control `72dbb51` uses the same frontend instrumentation
with the PR29 simulation/query/steering sources unchanged; its startup scaffold
permits the target scale, so it is not described as a stock PR29 executable.

Upstream [Sub0Pipeline PR36](https://github.com/CraigHutchinson/Sub0Pipeline/pull/36)
merged as `f730c4ec2973a449c45fbf9a74595414b9bf30e1`. It isolates bounded submission,
rejection/join and completion-target destruction repairs. Corrected head `b20428b`
passed all nine full jobs in [run37848858562](https://github.com/CraigHutchinson/Sub0Pipeline/actions/runs/37848858562),
including actual Qt/Zephyr adapters and sanitizer/race receiving. The preceding
head's macOS fixture failed because moved-from std::function targets need not
be empty; explicit exchange corrected the fixture without weakening lifetime checks.
Local Debug9/9 and Release9/9 preserve Core165 cases/10298 assertions and DSL35/292.
The consumed Crucible pin then passed full Release51/51 (34.24s) and Debug51/51
(221.78s), before new Scheduling consumption.

Five alternating upstream pairs and allocation/disassembly audits are retained.
The full ordered harness's validate1000 median was +32.15%; five isolated pairs
were -2.43% with overlapping ranges, and existing validate assembly was identical.
Neither result supports a performance promotion. First graph storage increased
by one allocation/40 bytes; Scoped ownership packets allocate per dispatch, so
the inner consumer uses the bounded Priority adapter directly. Stalled VTune
collection produced no profile; only owned collector processes were stopped.

Schema3 native diagnostic probes at `73b4f2e` receive six camera-cycle frames for
each scale and none/FLOW/gather route, with zero observation drops and closed tick
identities. The none-route mean boundary was94.147ms at100K and145.959ms at150K;
the unchanged proposal kernel accounted for81.965ms and126.486ms respectively.
These short attribution runs are not thermally qualified acceptance samples and
establish no percentile,60Hz, input-budget or mature-front claim.

Scheduling source review initially missed plain Pipeline body exception handling.
Release throwing-callback receiving reproduced process fail-fast; containment
inside the row job now restores FP through unwinding before returning failure.
Keep this failure and the qualified MSVC Debug orchestration allocation evidence
with the provider handoff. Integrated numerical, storage and final-head gates remain open.

## Retrospective and reuse

The intended scale experience requires readable density and coherent input, not
population alone. Existing mutable query scratch and inline ownership determine
the first safe concurrency seam. R03 executor guarantees may need a product-neutral
Pipeline reproduction/upstream improvement; H2 math and domain policies remain in
their current homes. No unconsumed caches, hierarchy or generic renderer is proposed.

## Follow-ups

| Stable ID | Status / accountable owner | Phase14 receiving gate or preserved scope |
|---|---|---|
| P01-F05, HW-05 | Core / architect,A,B | G1-G7, joined compute and complete physical frame envelope at both target scales |
| P11-F03 | Partial core / Runtime,Scheduling | Executor subset G1/G2; concurrent Pub producer remains deferred |
| P12-F01,P04-F02,P01-F04 | Partial core / presentation,architect | G6 native input/scale readability; no inferred participant acceptance |
| P05-F02,P06-F02,HW-06 | Conditional subset / rendering,architect | Selected live adapter lifecycle only; second backend and unobserved physical faults remain open |
| P12-F02,P13-02 | Deferred / consumer audio owner | Actual bounded audio/device/listening; separate increment |
| P09-F01,P05-F01 | Open / product owner | Human strategy/comprehension/balance; native operator checks cannot close |
| P01-F03,P06-F01 | Deferred / domain owners | Growth/attrition/resource/faction consumers, numerical/identity/authority contracts |
| P02-F02 | Deferred / snapshots,Runtime | Actual delayed concurrent reader, lease/backpressure/join receiving |
| P02-F03,P03-F01,HX-07 | Deferred / Spatial,world owner | Terrain/traversal/numerical geometry gates |
| P04-F01,P04-F03 | Deferred / platform owner | iOS packaging/signing/touch/physical lifecycle |

## Closure

Planning and execution claims are recorded in ACTIVE_WORK_LOG. Draft
[PR30](https://github.com/CraigHutchinson/Crucible/pull/30) tracks the source work;
source checkpoints and isolated worker trees are retained. Measurements, final-head
CI, major acceptance and PR/merge/local-origin verification remain pending.
All old worktrees/artifacts remain preserved. Major milestone completion requires
the plan's G1-G7; a code-only merge cannot substitute for unreceived physical budgets.
