# Phase 9 sprint review: compare rules before structural implementation

Status: reviewed local investigation complete; final hosted acceptance and merge pending,
2026-10-05. Architect accountable.
Dispatch baseline PR21 merge `a95c5111991f441a451df144fdf443d2379a0939`.
See [plan](../phases/phase9.md), [selection](../decisions/phase9-selection.md)
and [receiving](../workstreams/integration/phase9-validation.md).

## Intent and delivered scope

Integrate resource/faction concepts into project entry points, reconcile stale phase
status and replace speculative architecture defaults with current ownership and gated
future changes. Execute the next planned rules comparison using two workers and an
actual production-state relay-density spike. This is an investigation increment;
no fusion, combat, faction authority or new playable objective is claimed.

Structural owner supplies an independent mobile32→structure32→mobile24/loss8
ledger and a production Simulation/HeadlessSession/Snapshot caller. Arena owner
supplies three-faction authority, contested harvest, Blight control and ties, then
independently reviews structural/spike boundaries. Architect owns shared docs,
CMake/CI, serial CPU, artifact review and publication. Gesture/painter is consolidated;
hardware-dependent receivers remain deferred.

## Findings and resolutions

| Finding | Resolution / evidence | Remaining limit |
|---|---|---|
| Phase8 was merged but entry docs called it blocked | PR21 merge independently verified; original evidence stays historical | Recovery did not rerun acceptance |
| Architecture described a three-buffer exchange as a default | Current sequential owned frame documented; leases require an actual delayed reader | Threaded observation remains open |
| Structural activity changes affect fixed-population reclamation | Freeze active-prefix/ledger/input/snapshot receiving in next contract | No structural code delivered here |
| Candidate relay has49 startup samples above cost32 | Observe all ticks: passive max51, radial312, FLOW54; select64/refund48/loss16 reference | Full structural loop and human tuning remain open |
| Small structural oracle would starve at tick7 | Label transition trace mission-disabled; starvation/terminal rules remain separate fixtures | No mission inference from arithmetic table |

## Verification and useful artifacts

Debug and Release passed30/30 each; final extrema reporting received targeted1/1
reruns in both. Full Release900-tick study passed conservation, independent query
parity at all2,703 strategy/tick states and three complete terminal replays. Normal
local LSan canary is blocked by `/proc` access; no checks were disabled. Required
hosted sanitizer/platform acceptance remains pending. Changed Markdown links and
whitespace pass; incomplete JSONL is rejected by the plot tool.

Both concept boards and the primitive arena mock were visually inspected. The
[actual density SVG](../investigations/phase9/relay-density.svg) is a static plot
of production counts, with [raw output](../investigations/phase9/relay-density.jsonl)
and [source/output provenance](../investigations/phase9/receipt.json). Its original32
eligibility duration is explicitly not structure hold. Detailed commands, observations
and limits are in [receiving](../workstreams/integration/phase9-validation.md).

## Retrospective and reuse

Independent proposals exposed the fixed-population and startup-density constraints
before API expansion. A production consumer keeps the experiment from becoming a
parallel rules engine. R05/R06 reuse the current query; R07/R08 stay local, with no
pin change or extraction. Select a contract before splitting implementation next.

## Follow-ups

| ID | Action / receiving gate | Owner / status |
|---|---|---|
| P09-F01 | Establish a meaningful gather/hold/redeploy decision and consume the selected structural contract | Architect/simulation; open |
| P06-F01 | Arena authority, ownership, controllable Blight and tie fixtures before any arena implementation | Contracts; specified, implementation deferred |
| P01-F03/F04/F05 | Structural participation/loss/relay and full-scale receiving | Simulation; open |
| P05-F01 / P04-F02 | Human FLOW and future structural task comprehension/DPI | Presentation; open |
| P05-F02 / P06-F02 | Physical full-frame/backend/failure receiving HW-02..HW-05 | Rendering; hardware gated |
| P04-F01/F03 | iOS signing/touch/device/lifecycle HW-06 | Platform; hardware gated |
| P02-F02 | Delayed concurrent observer before leases/exchange | Runtime; deferred |
| P02-F03 / P03-F01 / HX-07 | Terrain/traversal/exact geometry before pin changes | Spatial/world; deferred |
| P08-F01 | Recovered publication received; current retained behavior rechecked locally and in final hosted CI | Architect; hosted acceptance pending |

## Closure

Pending reviewed head, exact-head CI, PR merge and local/remote reconciliation.
All claims will close against actual receipts; no old worktree/artifact cleanup is requested.
