# Phase 12 sprint review: runtime budgets and bounded diagnostics

Status: in progress, 2026-10-05. Architect accountable.

## Intent, baseline and scope

Baseline PR24 merge `835c781`; [plan](../phases/phase12.md). Receive controlled
production backbone costs and opt-in bounded Sub0Log outcomes, then use this
dedicated Windows machine for actual native rendering/input evidence and Phase13
scope. Publication, exact-head CI and merge remain pending until gates pass.

## Work delivered and delegation

Measurement worker owns actual production benchmark/capture script and exact-source
Pipeline allocation audit. Diagnostics worker owns fixed logger/decoder/lifetime
provider. Architect owns optional dependency/runtime wiring, omission/full-state
receiving, desktop consumer and all serial CPU/GPU runs. Game rules consolidate;
concurrency/growth/other platform backends defer. Existing artifacts are retained.

## Findings and resolutions

| Finding | Resolution/evidence | Limit |
|---|---|---|
| Untimed Pipeline resets stop_source per node/run | Measure actual warmed allocation counts before disposition | No zero-allocation claim |
| Logger active binding is process-global | Scoped restoration for each emission; coordinator-only consumer | No concurrent logging claim |
| Default production build must omit Log | Build option OFF; runtime request independent and explicit | BUILD_TESTING still receives stack Log |
| Dedicated hardware was previously unreceived | Windows C++23 canary passes; physical Intel/RTX5070 Vulkan inventory | Actual shader/input receiving pending |

## Verification and useful artifacts

Pending final build/source receiving; receipts retained under `build/phase12`.
Windows MSVC19.51, CMake4.2.3, Ninja1.13.2, Python3.14.5. No completed suite,
measurement, participant test or new physical shader acceptance is claimed yet.

## Retrospective and reuse

R04 becomes an optional runtime diagnostic consumer; R02/R03 remain production
backbone. Frozen schema/cadence/capacity bounds prevent diagnostic policy from
entering core simulation. Existing pins retained pending measured disposition.

## Follow-ups

Carry P11-F01 measured allocations and P11-F02 bounded diagnostics through final
receiving. P11-F03 concurrency, P09-F01/P05-F01/P04-F02 human tuning/input,
P01-F03/F04 growth/scale, P06-F01 factions/combat, P05-F02/P06-F02 physical renderer,
P04-F01/F03 iOS, P02-F02 observation and P02-F03/P03-F01/HX-07 world gates remain
open unless closed by named actual evidence. Phase13 plan follows results.

## Closure

Active claims remain; source checkpoint, publication/CI/merge receipts pending.
No historical worktree, branch, build or sibling-project cleanup.
