# Phase 6 sprint review: instancing and mission examples

Status: implementation delivered, receiving/validation in progress, 2026-10-03.
Accountable reviewer: architect. Combined tests, hosted CI and publication pending.

## Intent, baseline and scope

Dispatch `44701d6ff0a2ad94acf312f07f7f0ce2ab17ae83`; [plan](../phases/phase6.md).
Deliver optional executing instancing/readback with owned numeric/upload contracts,
a shared mission route, replay-backed strategy observations and viable actual visuals.
No physical GPU/platform, human tuning, faction mechanics or multiplayer claim.

## Work delivered and delegation

P6-R consolidated normalized packet, concrete Vulkan/SPIR-V offscreen renderer,
shader layout and three-slot upload/readback/fence ownership under one worker.
The diagnostic executes two indexed instance draws from owned completed snapshots;
the current SDL desktop remains the production interactive path. Arbitrary opaque
colors are presentation values, not faction identity. Shader/API execution on a
software Vulkan device does not establish physical-device performance or promotion.

P6-M delivered the pure owned reference-route provider and independent boundary,
schedule-partition and full-state replay fixtures. Architect-owned CLI and export
callers consume the same provider. Four frozen strategies retain the existing
quota/deadline and resource rules. P6-I owns shared wiring, builds, provenance,
captures and publication. Workers authored and reviewed without competing CPU runs.

## Findings and resolutions

| Finding | Impact | Resolution / evidence | Limit |
|---|---|---|---|
| Raw physical float halves underflow for valid tiny cells | Candidate layout narrowed production geometry support | Normalize physical coordinates in double before float packing; nonbinary/denorm/maxfloat and fit/zoom/pan fixtures | CPU arithmetic bound and GPU masked pixel agreement are separate evidence |
| Bounded slot count mistaken for bounded latency | Live saturation could wait indefinitely | Query fences and return busy; drain is a separate blocking teardown/diagnostic barrier | Actual delayed three-slot saturation timing is not established by drained readbacks |
| Receipts could outlive or belong to another renderer | Address reuse could expose unrelated readback | Weak issuer identity plus slot submission ID/tick; executing foreign/expired/short readback fixtures | Device-failure receipts are reviewed, not fault-injected |
| Request tick confused with application tick | Exports could depict accepted input as committed | Independent requests 0/60 apply at 1/61; shared CLI/export provider and completed snapshots | No human comprehension claim |
| Buffered evidence streams checked before flush | Late write failures could escape persistence checks | Explicit flush before final stream checks in mission/GPU evidence callers | Filesystem/device failure injection remains separate |
| Malformed shader preflight mistaken for partial GPU creation cleanup | Failure acceptance would be overstated | Empty shader fixture rejects before GPU creation; partial-creation RAII/drain reviewed | P06-F02 remains open for actual fault receiving |

## Verification and useful artifacts

Architect reports the optional executing GPU readback fixture passed under same-shell
Xvfb (0.35 seconds, a test duration rather than performance evidence). The receiver
used Mesa 25.2.8 llvmpipe/LLVM 20.1.2, a Vulkan 1.4.318 software device; shaders were
compiled with glslang 15.1.0. It covers
arbitrary palette interiors, retained packets, receipt lifecycle, software-oracle
comparisons and initial/evolved 2K frames. Live diagnostic initial/evolved/palette
captures completed. [GPU frame records](../workstreams/integration/phase6-evidence/gpu-frames.json)
and [shader/device provenance](../workstreams/integration/phase6-evidence/gpu-metadata.json)
retain source/SPIR-V hashes and same-snapshot pairing. Architect visually inspected
[initial](../concepts/exports/phase6-gpu-initial.png),
[evolved tick 60](../concepts/exports/phase6-gpu-evolved.png),
[synthetic palette](../concepts/exports/phase6-gpu-synthetic-palette.png), and the
[same-input software/GPU pair](../concepts/exports/phase6-gpu-software-pair.png).
The [software frame](../concepts/exports/phase6-gpu-initial-software.png) is the
production painter counterpart, not a generated concept. GPU output contains the
world pass only; field/HUD overlays remain with the production painter.

The local combined gpu-release suite passed 31/31 in 42.89 seconds before final
exporter trace/ledger/flush refinements. Local Debug passed 29 cases; the four-strategy
fixture reached its original 180-second timeout, then passed its targeted rerun in
241.10 seconds after increasing the bounded limit to 900 seconds. No criterion or
simulation rule changed. The local sanitizer's first ten cases passed; the execution
service disconnected while its strategy fixture was running. Remaining local results
are unconfirmed and are not counted as passes.

Source checkpoints b11a377, c9a8704 and 0cd1299 preserve progress. Fifty reviewed
file blobs were uploaded before the disconnect; the agents recovered the remaining
five test files exactly from their authored reads/patches. No source was regenerated.
The public repository and push permission were verified before recovery publication.
Hosted exact-head CI must validate all nine jobs and the final exporter; its uploaded
GPU artifact supplies fresh executable/source/shader/device/trace provenance.
Local tree reconciliation remains pending execution-service recovery.

Portable packet fixtures check a frozen 0.01-logical-pixel CPU float/FMA arithmetic
bound. Executing GPU comparisons separately permit one byte per interior channel,
with predeclared analytic one-pixel edge masks and minimum unmasked coverage.
Deterministic slot fixtures check delayed/busy/retired/reuse bookkeeping; readback
diagnostics drain between frames and do not prove actual three-slot saturation.
Device/submission/map failures, partial GPU creation after resource allocation,
failed drain and hung drivers were not injected. These remain P06-F02 receiving
gates rather than passing fault tests.

The shared route fixture passed. [Strategy results](../workstreams/integration/phase6-evidence/mission-strategies.json)
and [provenance](../workstreams/integration/phase6-evidence/mission-metadata.json)
retain full-state independent terminal replay for all four cases:

| Frozen strategy | Outcome | Completed tick | Reclaimed | Applied edits |
|---|---|---:|---:|---:|
| Passive | LOST | 900 | 1,774 | 0 |
| Shared sweep | WON | 267 | 1,780 | 5 |
| Stationary attractor | LOST | 900 | 1,774 | 1 |
| Repel/reposition/erase | LOST | 900 | 1,774 | 3 |

Actual production-painter [ACTIVE tick 60](../concepts/exports/phase6-sweep-active.png),
[WON tick 267](../concepts/exports/phase6-sweep-won.png), and
[LOST tick 900](../concepts/exports/phase6-passive-lost.png) were captured and visually
inspected by the architect. The [strategy comparison](../concepts/exports/phase6-strategy-comparison.png)
uses retained checkpoints. These show executable behavior, not a human playtest,
balanced difficulty or broad strategy robustness. Remaining combined, native,
hosted GPU and exact-head CI results are architect-owned and pending.

## Retrospective and reuse

Consolidating numeric representation and GPU lifetime under one renderer owner kept
the shader/upload contract coherent. The independent mission owner provided a small
receiving pack without changing game rules; architect retained shared callers and
serialized CPU. Self and reciprocal review found no code-level numeric/ownership
blocker, while identifying the unexecuted fault paths explicitly.

Next consolidate lifetime fault receiving before splitting public-platform work.
Keep deterministic slot tests and executing readbacks as complementary evidence;
neither substitutes for device-loss, partial-creation or hang receiving. Existing
SDL3/H2 are reused; R07/R08 remain local. No upstream utility extraction or faction,
structural, terrain or concurrent simulation implementation was dispatched.

## Follow-ups

| Stable ID | State / next action | Owner / receiving gate |
|---|---|---|
| P06-F02 | Open: actual device/submission/map failure, partial GPU creation, failed drain/hang and delayed saturation receiving | Rendering/integration; consumed fault/retirement evidence before default-backend promotion; consolidate first |
| P05-F02 | Partial: real optional instancing/readback delivered | Rendering; physical public devices, full frame workload and promotion gates |
| P05-F01 | Open: mission comprehension/difficulty/diverse strategies | Game/presentation; human playtest before tuning; four replay-backed cases retained |
| P06-F01 | Groundwork retained; no faction/network implementation | Game/contracts; three-plus faction/control/resource/command consumer |
| P04-F01/F03 | Deferred: iOS packaging/touch and Apple distribution | Platform; signed package/toolchain and physical lifecycle/device proof |
| P04-F02 | Open: native workloads/typography | Presentation; physical input-to-present, DPI/readability acceptance |
| P02-F02 | Deferred: concurrent exchange | Rendering/runtime; actual delayed reader/upload retirement consumer |
| P01-F03/F04/F05 | Deferred: fusion/relay/parallel scale | Game/integration; frozen conservation/identity/protection and joined workload |
| P02-F03, P03-F01, HX-07 | Retained: terrain direction, spatial cost and geometry oracles | Game/spatial; explicit traversal requirement or measured workload |

## Closure

Pending reviewed publication, exact-head CI, merge and verified baseline.
