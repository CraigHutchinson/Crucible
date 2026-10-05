# Phase 12: runtime budgets and bounded diagnostics

Status: dispatched 2026-10-05 from Phase11 PR24 merge `835c781`, on
`phase12-runtime-diagnostics`. [Review](../sprint-reviews/phase-12.md).
Dedicated Windows machine; user permits native graphics/rendering and interactive
playtesting. Capability must still be received against the actual build.

## Outcome and scope

Establish controlled production overhead/allocation budgets for the sequential
Pub/Pipeline backbone, then receive one bounded Sub0Log diagnostic consumer for
command refusal and completed mission summaries. Logging remains opt-in and cannot
change admission, replay, mass, terminal state or retained frame publication.

Architect owns the workload, budgets, direct comparator, package/pin promotion,
native graphics receiving, integration and acceptance. Retain two bounded workers:
execution worker owns exact-source allocation/dispatch measurement and a minimal
neutral upstream Pipeline receiving fixture; diagnostics worker owns bounded backing,
drop accounting, decoding and teardown. Consolidate if measurement finds no needed
API work. No worker mutates CPM checkouts or runs contended builds/measurements.
Simulation/game policy and desktop integration consolidate under architect;
concurrency, world growth and a second renderer defer until consumed gates exist.

## Frozen consumed contracts

- RuntimeDiagnostics owns one aligned 64KiB image for the session lifetime,
  fifteen 4KiB chunks and a compact header. No growth/recycling. Exhaustion retains
  its committed prefix and counts dropped records; decoding/output is cold work.
- `CRUCIBLE_ENABLE_DIAGNOSTICS=OFF` preserves production dependency omission;
  enabled builds still omit runtime logging unless explicitly requested.
- InspectorSession records refused admission, completed structural refusal and
  exactly one terminal mission summary/run. Records copy numeric values only;
  restart preserves log/counters and changes run ID. No logging in Simulation.
- Bindings restore the previous logger before each call returns. Single coordinator
  only: upstream process-global active binding is not a concurrent sink guarantee.
- Measurement uses actual integrated/direct production paths at64/2048, startup,
  first/steady pump, isolated admission and complete267/448-tick mission routes.
  Replaceable C++ new counts are requested bytes, not all OS/C heap allocations.
- Visual evidence: decoded actual diagnostic transcript, native interaction states,
  actual existing Vulkan shader readback where preflight passes. Automated operator
  observations remain distinct from participant comprehension/balance evidence.

1. Measure five uncontended paired production runs, startup and steady-state
   allocations, Pub delivery/admission and complete tick/capture/mission separately.
   Record source/pins/toolchain/hardware/raw data; correctness is a gate, timing advisory.
2. Select a measured budget and disposition. If reusable Pipeline run storage is
   needed, evolve it upstream with failure/repeat-run/package fixtures, then receive
   the exact full pin. Do not add a local copied scheduler or silently bypass graph work.
3. Inspect current Log bounded-memory API. Freeze capacity, record schema/cadence,
   counted drops, source-copy and shutdown before wiring actual outcomes. Require
   decoder parity and exhaustion fixtures; omission of diagnostics preserves every
   production state/result. No new dependency on the core simulation.
4. Repeat full-state receiving, Debug/Release/normal sanitizer and exact-head CI,
   capture a useful actual diagnostic view, review and merge.

Stretch: existing physical Vulkan readback and a bounded native interaction session
on the dedicated machine. Shape Phase13 around actual input/readability/renderer
findings. Defer parallel ECS mutation, growth/recycling, streaming, device fault
injection, iOS and participant balance; their existing follow-up gates remain open.
