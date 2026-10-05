# Phase 12 proposal: receive runtime budgets and bounded diagnostics

Status: proposed; dispatch only after Phase11 accepted merge. Read its review and
reassess the workstream split before implementation.

## Outcome and scope

Establish controlled production overhead/allocation budgets for the sequential
Pub/Pipeline backbone, then receive one bounded Sub0Log diagnostic consumer for
command refusal and completed mission summaries. Logging remains opt-in and cannot
change admission, replay, mass, terminal state or retained frame publication.

Architect owns the workload, budgets, direct comparator, package/pin promotion and
consumer acceptance. Retain two workers only if independent packages remain:
execution worker owns exact-source allocation/dispatch measurement and a minimal
neutral upstream Pipeline receiving fixture; diagnostics worker owns bounded backing,
drop accounting, decoding and teardown. Consolidate if measurement finds no needed
API work. No worker mutates CPM checkouts.

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

Stretch: improve one neutral upstream example with independently useful lifecycle
receiving. Defer parallel ECS mutation, dynamic growth/recycling, world streaming,
GPU/device/iOS and human balance; their existing follow-up gates remain open.
