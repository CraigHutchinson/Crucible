# Phase 5 sprint review: reclamation challenge

Status: implementation reviewed and locally validated; publication pending,
2026-10-03. Accountable reviewer: architect.

## Intent, baseline and scope

Base `0bc9731`; [plan](../phases/phase5.md), [rules](../decisions/phase5-reclamation.md),
[instancing design](../decisions/render-instancing.md),
[combined evidence](../workstreams/integration/phase5-validation.md).
Deliver a reference quota/deadline loop over finite reclamation and carry the owned
packet experiment into future rendering. Desktop uses target 1,780 / deadline 900;
mission is optional for existing inspector consumers. This is the challenge increment,
not Secure the Relay/fusion/protection or measured GPU scale.

## Work delivered and delegation

Runtime objective/stop consolidated under resource_package; concrete painter/HUD
under hex_adoption_package; architect owns contract, application, docs and validation.
Gameplay foundations retained; GPU/platform work remains a separate consumed gate.
Workers performed self and reciprocal review without competing builds. Runtime
evaluates each successful completed tick; quota wins before deadline, closes admission
and stops catch-up immediately. Progress/frame share a tick. Pause/blocked boundaries
do not advance mission, resume cannot revive it, and restart constructs a fresh run
before retiring old storage. Desktop and headless CLI are production callers.

## Findings and resolutions

| Finding | Impact | Resolution / evidence | Limit |
|---|---|---|---|
| Outcome evaluated only at end of pump | Catch-up could overshoot | Startup-owned completed-boundary stop callback; early-stop/exact-deadline fixtures | Failure stops; no boundary rollback |
| Objective only available through full state copy | Would sort/copy world every tick | Owned `GetBiomassLedger`; publish snapshot only after pump | Coordinator exclusivity remains |
| Terminal message hid denied edit feedback | Rejected input lacked explanation | Keep closed admission feedback and clear preview; peer/event review | Human comprehension untested |
| Startup stock did not prove quota reachable | Provisional 6,144 exceeded tested recovery | Final 1,780: passive loss and live-tool scripted win replay exactly | Six-quanta margin is initial tuning |
| Compact data mistaken for GPU instancing | False performance/lifetime inference | Static-quad/instance-rate proposal and shader/precision/upload/fence gates | No GPU consumer ran |
| Managed host lost one generated executable permission | Sanitizer test did not launch | Selected-artifact preflight adopted in local commands and CI; deliberate permission-loss probe and four fixtures pass | External tools remain outside repair scope |

## Verification and useful artifacts

Linux GCC 13.3/CMake 4.4.3: desktop Debug/Release 25/25 each; headless Release 24/24,
including passive/route CLI and SDL absence. ASan/UBSan covered all 25:24 launched
and passed unfiltered, permission-blocked phase 3 passed after mode restoration.
Local leak scanning is disabled because managed process inspection is blocked;
hosted sanitizer is normal. Exact-head platform acceptance pending publication.

[Raw outcomes](../workstreams/integration/phase5-evidence/metadata.json): passive LOST
at 900 / reclaimed 1,774; swept attractor WON at 267 / reclaimed 1,780 with five applied edits.
Both states independently replay metadata, stable IDs, positions/velocities, fields,
infection, stock and ledger. Fixtures independently count five tick-one contacts and
thirteen further tick-two contacts, check clock stop/carry/exception, mission pause/
blocked/terminal/restart, software text/bar pixels and actual native-adapter events.

[Actual frame](../concepts/exports/phase5-mission-frame.png) is visually checked
production coordinator/painter software output at tick 60, 1,753/1,780 and 840 ticks
left; 10,240=6,439+2,048+1,753. Two fixed fields differ from the winning CLI route.
No human playtest, device/font acceptance or full-frame performance is inferred.

Release additionally passes 26/26 after adding the workflow fixture; headless workflow
fixture passes separately. Permission loss is repaired before suite execution, so
a restored artifact does not waste a full run or require rebuilding unchanged code.

## Retrospective and reuse

R07/R08 stay local; SDL3/H2 reuse unchanged. No upstream source change dispatched.
Two owners were useful around the frozen value contract; root retained CPU/wiring.
Keep passive and scripted receiving paths before selecting future thresholds. Keep
objective policy independent from world geometry and extraction. Next consolidate
R1 real instancing and R2 numerical/upload contract under a renderer owner, then
split R3 public-platform receiving around a consumed shader/retirement protocol.
Mission playtesting/tuning can remain a bounded product stream. No interface hierarchy
is warranted before actual backends establish the shared protocol.

## Follow-ups

| Stable ID | Action / state | Accountable role / receiving gate |
|---|---|---|
| P05-F01 | Open: mission comprehension/difficulty and diverse strategies | Game/presentation; human playtest and replay-backed spawn/frontier/threshold changes |
| P05-F02 | Open: actual instanced backend | Rendering R1/R2; shader/layout/color/numeric fixtures, bounded upload/fence retirement and complete device/frame measurements |
| P01-F04 | Partial: goal/outcome/restart delivered; relay still open | Game architect; structure/protected duration and human acceptance |
| P01-F03 | Open: structural growth/fusion ledger | Resources; capacity/identity/atomic commit/conservation |
| P01-F05 | Deferred: concurrent/structural scale | Integration; joined ownership and complete workload |
| P04-F01 | Open: iOS package/touch/device | Platform; signed receiver and physical lifecycle/input proof |
| P04-F03 | Open: Apple distributable runtime | Platform; signed redistribution/toolchain validation |
| P04-F02 | Open: native workloads/typography | Presentation; input-to-present measurement and readable/DPI playtests |
| P02-F02 | Deferred: concurrent exchange | Rendering; actual reader/upload retirement consumer |
| P02-F03 | Deferred: height/mining/bridges/sphere | Game/geometry; conservation/traversability/metric |
| P03-F01 | Open: spatial compatibility/storage/tail costs | Spatial; measured fallback frequency/workload |
| HX-07 | Receiving evidence retained | Geometry; oracle pack before hierarchy changes |

## Closure

Pending exact-head CI, merge and verified baseline. Worker write/review claims are
complete; architect owns publication. Prior worktrees, spike sources and receiving/
strategy/visual artifacts retained; no unrelated cleanup.
