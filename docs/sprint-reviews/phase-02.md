# Phase 2 sprint review: steerable and inspectable simulation

Status: complete. Date: 2026-10-01.

| Record | Evidence |
|---|---|
| Dispatch baseline | `65bb8c4fbea4ee8ebca74e8c14f2b7e9114c118e` |
| Reviewed head | `971cfc56e463cfe778e9b876ecf456f485ee1d12` |
| Main merge | `cdc603472f32a5d8a3204a30e4e6f7e84a0a195f`, [PR 4](https://github.com/CraigHutchinson/Crucible/pull/4) |
| Detailed implementation evidence | [Phase 2 validation](../workstreams/integration/phase2-validation.md) |
| Intent / contract gate | [Phase plan](../phases/phase2.md), [frozen ownership and rule](../decisions/phase2-execution.md) |

## Intent and delivered work

Make the sequential scenario steerable and inspectable while preserving exact tick
replay and bounded startup storage. Two Sol 6.1 low workers owned swarm and runtime;
the architect owned contracts, Simulation composition, snapshots, integration and
review. Runtime and observation were consolidated; presentation stayed with the
architect until a consumed state contract existed.

Delivered immutable-input, stable-ID separation plus radial force with acceleration/
speed caps; a bounded 60-Hz ClockDriver with lifecycle, overflow, catch-up and discard
policies; and owned snapshots containing every sample, field slot and Blight cell.
Main consumes the clock/summary/snapshot. The integration oracle compares full state
under different elapsed schedules and exact replay. The visual stretch delivered an
[actual-state SVG and PNG](../concepts/phase2-state.md), not a playable window.

## Findings and dispositions

| Finding / decision | Disposition | Learning |
|---|---|---|
| Implicit snapshot moves left source metadata engaged after moving storage | Fixed: stable snapshot ownership with copy/move disabled | Review moved-from invariants alongside borrow/capacity rules |
| Swarm source/test used a nonexistent timing header | Fixed in integration to consume existing timing.hpp | A static handoff is review-ready, not executable proof |
| GCC reported missing optional steering initializers | Fixed with an explicit empty member default | Preserve prior aggregate callers when extending consumed settings |
| Sub0HexGrid H1 lacked bounded regions/complete radius candidates | Deferred adoption; rectangular bins retained, no new pin or duplicate backend | Reuse is gated by the actual query consumer and independent completeness oracle |
| Full-state inspection did not require concurrent exchange or telemetry adapters | Kept minimal sequential copy and clock-owned summary | Consolidate coupled work instead of filling every reserved stream |

No outstanding MUST finding remained under the fixed-population, single-coordinator
contract. Copy failure preserves all destinations; snapshot failure preserves its
previous frame. Unexpected tick failure stops runtime, but does not roll back field
edits already applied at that boundary. Exact tick-start grid consistency remains a
composition precondition, verified by the production caller.

## Validation and limitations

MSVC Debug/Release and GCC 15 ASan/UBSan each passed **13/13** unfiltered tests.
Exact reviewed-head push and PR CI passed Linux/Windows Debug/Release and sanitizer
jobs. The legacy checksum remained **225000**. Full-state checkpoints and replay
agree at populations 0/8/2048 over 60 ticks, including pauses and different elapsed
schedules. The independent steering oracle covers dense/coincident/border/invalid,
ordering, aliasing, finite bounds and unchanged-on-failure cases.

SVG reproduction, XML element counts, failure exits and rendered appearance were
checked; tick 20 shows 2,048 samples and 800 infected cells. No new renderer dependency
or install was needed. No cross-compiler bitwise identity, allocator interception,
threaded simulation, performance result, resource ledger or mission behavior is claimed.

## Retrospective and reusable-library disposition

Two implementation workers plus the architect supplied a complete increment with
small shared contracts and one composition owner. Independent review found a real
ownership defect; executable gates found an integration defect. Keep both gates.
Core verification preceded visual stretch. Retain domain steering policy and bins in
Crucible. Keep Sub0HexGrid standalone until H2 is complete; no upstream code or pins
changed. Reuse decisions remain in the [catalog](../reuse/README.md).

For the next phase consolidate steering/runtime maintenance with integration. Propose
resource interactions/Blight as the main implementation stream and optional standalone
HexGrid H2 as the second. Freeze the material ledger first. If quota supports only one
worker, prioritize the gameplay consequence. Read the [phase 3 proposal](../phases/phase3.md)
and reassess this split before dispatch; phase 3 is not started.

## Follow-ups and carry-forward

| ID | Follow-up | Status / receiving increment | Accountable role / acceptance gate |
|---|---|---|---|
| P01-F01 | Separation, clock and consumed summary | Closed by this phase | Swarm/runtime; 13/13 suite and actual main consumers |
| P01-F02 | Sequential owned inspection/full-state replay | Closed by this phase for sequential scope | Architect; retained-frame/capacity and exact state fixtures |
| P01-F03 | Resource/consumption ledger | Open; phase 3 priority | Architect + interactions/Blight; units, conversion, contested ordering and conservation oracle before code |
| P01-F04 | Real input/rendering backend | Deferred; W9 | Architect/presentation; backend ADR, playable input and lifetime evidence; SVG does not close it |
| P01-F05 | Structural/concurrent execution and measured scale | Deferred; W7/W10 | Scheduling/integration; generation/lifetime/failure gates and controlled real-workload measurements |
| P02-F01 | Sub0HexGrid H2 finite regions and complete radius candidates | Open; optional standalone phase 3 stream | Hex geometry owner; checked domain, brute-force candidates and strict package consumer before Crucible adoption |
| P02-F02 | Concurrent snapshot exchange / reader and upload retirement | Deferred; W8b | Architect/presentation; real concurrent reader first, then lease/close/supersession gates |
| P02-F03 | Planar height/mined depressions/permanent bridges; later sphere | Deferred; future terrain increment | Game design/geometry; material accounting and sourced metric/adjacency contract; [terrain ADR](../decisions/terrain-and-world-extension.md) |

## Closure and cleanup

Main was verified equal to origin at `cdc6034` after merge. Branch cleanup was initially
held by automatic approval review, then explicitly authorized by the user. Completed
branch references were removed; worker commits are remotely archived and all six
worktrees/artifacts retained. See [cleanup and recovery](README.md#branch-cleanup-2026-10-01).
The next sprint must read this review and record its own before closure.
