# Phase 8 proposal: portable shader receiving and field inspection

Status: **PROPOSED — NOT DISPATCHED**, 2026-10-03. This proposal does not dispatch
implementation, dependency changes or a platform winner. Set the baseline from
verified merged main at dispatch, after the [Phase 7 review](../sprint-reviews/phase-07.md).

## Outcome and prerequisites

Propose one optional second-backend shader receiver using the existing owned
instancing contract, alongside a small player-facing field-inspection improvement.
This is not default-GPU promotion, four-platform packaging or faction gameplay.
Consume [Phase 7 scope](phase7.md), [GPU rules](../decisions/phase6-gpu.md),
[instancing architecture](../decisions/render-instancing.md),
[game intent](../game-design.md) and [faction groundwork](../decisions/faction-extensibility.md).

Phase 7 controlled failures and deliberately hidden queries exercise the concrete
Vulkan receiver; they do not prove physical device loss, a real hung driver or
hardware latency. Local Debug passed 34/34; the local sanitizer namespace blocks
LSan /proc inspection; hosted leak-enabled acceptance passed at Phase 7 head8b3eec.
The frozen timing cases won at ticks 297, 271 and 449 with independent full-state
replay. These automated results do not establish human robustness. Hosted receipts and fresh durable evidence are recorded in the Phase7 publication.
P07-F01 original evidence recovery and local baseline verification remain necessary
before beginning this phase.

## Division and bounded packages

Keep architect plus at most two workers. Consolidate shader tooling, concrete
backend receiving and its lifetime evidence under one rendering owner; split
mission/presentation only at the existing owned snapshot boundary. Do not create
four platform teams before a common shader and lifetime receiver is consumed.

| Package | Proposed exclusive paths | Result / dependencies |
|---|---|---|
| P8-R rendering worker | `include/crucible/presentation/gpu/`, `src/presentation/gpu/`, `tests/presentation/gpu/`, `spikes/rendering/phase8-shaders/`, `docs/workstreams/presentation/phase8-portability.md` | Two bounded shader-production prototypes and one selected concrete second-backend receiver; depends on dispatch-time tool/device availability and frozen shader artifact contract |
| P8-M mission/presentation worker | `src/presentation/desktop/ScenePainter.cpp`, `tests/presentation/desktop/ScenePainterTests.cpp`, `docs/workstreams/presentation/phase8-field-inspection.md`, `docs/playtests/reclamation-phase8.md` | Direction cues for committed attract/repel fields and truthful preview/selection inspection, consumed by the existing desktop; depends on architect-frozen visual fixtures and unchanged mission rules |
| P8-I architect | Shared contracts, desktop callers, CMake/presets/CI/pins, capture/provenance scripts, central docs and all build CPU | Availability audit, freeze, serial integration, exact-head acceptance and publication |
| Simulation/ECS/factions/platform packaging | Retain/defer | No structural mutation, faction components, network authority, concurrent exchange or signed mobile packaging in this increment |

Paths are reservations for dispatch, not current claims. Recheck actual file names
and adjacent caller ownership before assigning work. Workers submit shared-manifest
patch requests; no shared staging or competing build runs.

## P8-R: shader-production shootout and second receiver

Before authoring, inspect pinned SDL capabilities and available public toolchains.
Compare build-time translation into versioned backend artifacts against a pinned
runtime shadercross library. Use exactly the current vertex/fragment pair, same
32-byte instance records, camera uniforms, two world ranges, opaque encoded color
and 1280×720 output. Neither prototype may introduce a generic renderer interface,
entity-owned graphics resources or live ECS queries.

| Question | Bounded measurement / fixture | Stop / decision gate |
|---|---|---|
| Build-time versus runtime production | At most two prototypes; record source/compiler/output hashes, required tools/dependencies, reproducible clean build and runtime compilation failures | Select only an approach that generates the needed artifact and is consumed by the chosen receiver. Prefer fewer required runtime capabilities when both pass; report limitations rather than inventing a winner if either cannot be exercised |
| Which second backend can be received now? | Audit macOS Metal and Windows D3D12 runner/device capabilities before dispatch; choose one runnable target, retaining Linux Vulkan as reference | Freeze one target with real shader/device execution available. Compilation-only evidence is labelled compilation-only; absent tools/devices block that receiver, not a passing substitute |
| ABI/color/geometry parity | Reuse nonbinary geometry, fit/zoom/pan, initial/evolved 2K and synthetic palette packets; independently mask declared one-pixel edges | Preserve CPU .01-logical-pixel arithmetic and actual interior ±1-byte criteria. No post-result tolerance widening, faction claim or screenshot-only acceptance |
| Upload and teardown portability | Three startup-bounded slots, receipt identity/reuse/foreign/expired/short-output fixtures, pending destination preservation and actual drain | A submitted receipt is not presentation. No slot overwrite, retained caller borrow or release before completion. Linux wrapper faults remain Linux-specific; define the second receiver's honest failure evidence without pretending ELF wrappers are portable |

Freeze the smallest consumed shader-format/entry-point artifact contract with
actual generated outputs before changing the concrete receiver. Root approves any
new pin/tool dependency from official sources and wires target-scoped requirements.
Shader error handling must fail before publishing a frame and retain bounded
resource cleanup. Keep the current SDL interactive renderer available and default.
Offscreen parity does not establish swapchain, full HUD, resize/present or latency.

Linux/Windows/macOS remain public desktop targets. iOS is a public future target:
record whether the selected Apple artifacts can compile for its public SDK, but
separate that result from application packaging, signing, touch, lifecycle and
physical-device receiving. Keep non-public platform information outside repository
source, docs and captures. No claim of cross-platform execution follows from one
backend's output.

## P8-M: field inspection and human receiving preparation

Current production fields use colored rings with selection thickness and a dashed
preview. The game intent asks for inward/outward cues; current code does not draw
those glyphs. Propose one bounded improvement: add shape-based direction cues that
make committed attract/repel signs inspectable without relying on color alone.
Freeze the placement/count, selected-field distinction and preview semantics with
independent examples before drawing code. Radius/magnitude, four slots, commands,
mission 1780/900, startup scenario and route remain unchanged.

Receive actual production-painter attract, repel, selected and preview cases at fit
and zoom, with clipping/outside-field, zero-strength/radius and empty-slot fixtures.
Show before/after captures from the same owned snapshot; check direction and
visibility independently rather than mirroring the draw loop. Rejected or queued
input must never look like committed state. Do not add an unused render abstraction,
field editor or picking API merely for the cues.

Consume [prepared human protocol](../playtests/reclamation-phase7.md) and the final
Phase 7 timing observations. Prepare a short comparative task for the direction
cues, selected-slot replacement and queued-versus-applied feedback. Actual human
sessions require consenting participants and recorded build/device/confounds;
moderator observations and automated fixtures are separate evidence. If no humans
participate, deliver preparation and visual receiving while keeping P05-F01 open.
Do not tune quota/cadence from the automated shootout or claim its outcome is human
strategy robustness. A distinct confusion found during sessions merits its own
bounded receiving change, not an unlimited usability redesign.

## Architecture and faction continuity

Simulation remains the exclusive ECS owner. Completed-boundary extraction produces
contiguous owned snapshots/packets; presentation consumes those values. Camera
changes redraw without changing world identity; shader instance index is not a
SampleId. Two draw ranges identify cells/markers, not opposing factions. Avoid
Renderable vtables, per-entity GPU resources, direct renderer ECS queries and a
generic engine/backend framework with one implementation.

P06-F01 remains groundwork. Organism kind, faction identity, controller identity,
infection state and resource ownership are separate decisions. More than two colors
remain a synthetic palette, not implemented factions. A future actual faction
consumer must freeze bounded stable IDs, neutral/allied/hostile relations, ownership
transitions, authorized commands and global/per-owner conservation with full replay.
Do not bind faction count to four field slots, shader ranges or color signs. This
proposal does not add those APIs or mechanics merely to reserve them.

## Scheduling, acceptance and retained gates

1. Architect closes Phase 7 and audits tools/devices; freeze one second backend,
   shader-production candidates and field-cue oracle before workers implement.
2. Workers operate independently on disjoint paths. Rendering prototype decisions
   precede production receiver changes; field cues consume existing snapshots.
3. Architect reserves serial CPU, receives each coherent handoff once, integrates
   shared callers and performs targeted checks followed by combined acceptance.
   Publish a reviewed source checkpoint before expensive device/CI runs.
4. Preserve headless and existing replay/resource/fault checks. Require supported
   Debug/Release and ASan/UBSan with leak checking, exact-head hosted CI, actual
   selected-backend execution and provenance-backed inspected captures. An unavailable
   required device gate remains open; a software image cannot silently replace it.
5. Accept only the optional receiver that passed its declared gates. Default
   interactive GPU selection requires a later complete-frame/window/input/present,
   physical-device and failure-policy decision. Review/merge this bounded increment
   and verify main before another dispatch.

| Follow-up | Disposition / next receiving gate |
|---|---|
| P06-F02 | Retain controlled fault evidence; physical device loss, real driver hang and hardware delayed saturation remain open |
| P05-F02 | Partial instancing foundation; second backend only where executed. Physical devices, complete world/HUD workload and 2K/100K/150K performance remain open |
| P05-F01 / P04-F02 | Direction cues and prepared comparative protocol; actual comprehension/accessibility, physical DPI/input-to-present and balance remain open |
| P04-F01/F03 | Defer iOS packaging/touch and Apple distribution pending toolchain/signing/device/lifecycle consumer |
| P06-F01 | Retain faction/material/control groundwork; no faction/network implementation |
| P02-F02 / P01-F03/F04/F05 | Defer concurrent exchange, fusion/relay and parallel scale until real delayed consumer or frozen structural/resource workload |
| P02-F03 / P03-F01 / HX-07 | Retain terrain/spatial/geometry investigation gates; no topology, traversal or pin change |
| P07-F01 | Integration must reconcile retained local trees and recover original capture bytes/hashes after environment_offline; no source regeneration or unknown-run green claims |

Reuse disposition: keep R07/R08 local, retain pinned SDL/H2/sub0 foundations and
prefer a consumed SDL shader-production capability over a new homegrown translator.
Only an exact-version reproducible defect merits upstream feedback. No residency
consumer justifies R09 or a storage dependency; no second reusable consumer justifies
extracting a graphics/runtime framework. Reassess consolidation versus platform
split after the actual shader/device and human evidence, without automatic dispatch.
