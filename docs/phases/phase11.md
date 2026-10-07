# Phase 11: production sub0 backbone

Status: implemented; final receiving in progress, 2026-10-05. Baseline is
Phase10 [PR23](https://github.com/CraigHutchinson/Crucible/pull/23) merge
`262c4eba4211779042f17bf5a2da03d333c35a91`, after nine exact-head jobs.
[Review](../sprint-reviews/phase-11.md) records acceptance and remaining limits.

## One consumed increment

Desktop input, scripted mission routes and the scenario exporter use synchronous
Sub0Pub v2 delivery into the existing bounded admission queue. A startup-built
Sub0Pipeline graph executes boundary commit → owned frame capture → mission
evaluation/publication. Sub0ECS v2 remains the authoritative world under one
exclusive coordinator. Game rules, queue cutoff and replay schema have one source.

The integrated path is the default. `InspectorSession::ExecutionPath::direct` is
a narrow receiving/measurement comparator using the same simulation and mission
calculation. Trace replay is deliberately direct: it receives recorded application,
not Pub delivery. No mandatory threads, service locator or umbrella framework.

## Dispatch and ownership

| Owner | Package | Division decision |
|---|---|---|
| Architect | Clock/Inspector/CLI wiring, contracts, full-state parity, builds/docs/publication | Consolidate unchanged simulation/presentation rules and their receiving |
| Delivery worker | IntentDelivery source/header, receiving fixtures and API handoff | Split at stable ingress boundary; no ECS mutation |
| Execution/state worker | BoundaryPipeline source/header, failure/lifetime fixtures and ECS/API audit | Split at stable session/frame boundary; sequential execution only |

Workers own disjoint files; architect owns shared manifests and all serial build/test
CPU. Defer concurrent producers/executors, Log without a bounded consumer, dynamic
ECS growth, terrain, factions and physical/human receiving. Retain prior artifacts.

## Received contracts

| Boundary | Library owns | Crucible owns |
|---|---|---|
| Intent | Pub scoped routing, explicit registration/disconnection and synchronous delivery | One private domain/sink per run; queue copies whole batch; receipt owns run/request IDs and status/sequence range |
| Execution | Pipeline dependency order, untimed caller-thread dispatch and run result | Commit → staging capture → mission staging → frame publication; concrete boundary status and fail-stop |
| State | ECS component/query storage and identity | Fixed retained game identities, activity/resource rules, exclusive access and full-state comparison |
| Observation | Optional future Log storage/decoding | Owned HUD/mission values; diagnostic loss must never alter rules |

Pub's typed envelope borrows its batch only during synchronous publish. The sink
immediately performs the existing owned queue copy; no span survives the call.
This refinement avoids redundant intermediate buffers and preserves oversized/invalid
validation precedence and rejection counters. Delivery/admission/application remain
distinct. Requests are coordinator-only; a broker lock does not make adapter
receipts concurrent. Callers quiesce before teardown; sink disconnects while ingress
and domain remain alive. Registration failure blocks startup; identifiers never wrap.

Graph jobs run inline without timers or orphan work. Paused/closed/trace-full skips
capture and evaluation, preserves pending commands and returns the real status.
Two startup-sized frames retain the last good publication if a later stage fails.
Unexpected failure closes admission and propagates or reports application_failed;
committed simulation work is not rolled back. Mission progress stages from the
new owned frame and commits after graph success. Terminal closure happens before
another clock catch-up boundary; evaluation is never duplicated.

## Source/API decision

Pinned Pub `2cd3daf` broker/config match latest v2 discovery `b1166d9`. Pinned
Pipeline `f6f54c6` supplies repeatable `run_inline`; latest `1c50051` adds examples/docs
without changing execution. Pinned ECS `8391f81` and latest v2 `6028591` share the
capacity/generation limitations relevant here. Full pins remain in
[DependencyPins](../../cmake/DependencyPins.cmake); no upstream edit or pin promotion
is necessary for this sequential consumer. See the worker
[delivery](../workstreams/runtime/phase11-delivery.md) and
[execution](../workstreams/runtime/phase11-execution.md) audits.

Pipeline run bookkeeping may allocate. No allocation-free or speedup promise;
[Phase12](phase12.md) selects measured budgets before upstream optimization. ECS
growth/recycling still needs checked capacity, world provenance, generation
exhaustion and failure/rollback fixtures in ECS itself. Keep domain policy local;
no reusable-library extraction without an independent consumer.

## Acceptance and stop gates

- Actual Pub field/fuse/shatter source-copy, correlated receipts, live-domain
  isolation, restart/disconnect, full/closed/invalid/batch and exact counter parity.
- Actual graph structural transitions, repeated epochs, skipped jobs/backpressure,
  terminal closure and exception/retained-frame fixtures.
- Every completed state, command/result trace, mission and clock summary matches
  direct execution through quota win, relay win, pause/full/refusal/restart and
  catch-up terminal loss. Independent arithmetic/query oracles remain.
- Supported Debug/Release and normal ASan/UBSan; BUILD_TESTING=OFF includes real
  Pub/Pipeline callers and omits Log; nine exact-head hosted jobs before guarded merge.
- Actual scenario export and software structural receiving where viable; routing
  itself changes no visual design. No image implies physical/human acceptance.

Unresolved upstream requirements are named with exact source, fixture and owner;
never hide an unsafe contract behind a DAG. A blocked required gate retains the
working baseline and the specific blocker rather than declaring completion.
