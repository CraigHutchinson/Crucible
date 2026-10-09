# Coordinated Sub0 integration batch

Status: reviewed proposal; implementation and performance acceptance pending.
Owner: Integration. Date: 2026-10-09. User authorization covers coordinated changes
across the owned repositories; it does not require separate permission for each
library. Each repository still receives its own exact source and correctness gates.

## Consumer target

Improve the evolving simulation's whole tick/frame while making effective use of
Sub0ECS storage, Sub0Pipeline scheduling and Sub0Pub typed composition. Preserve
the production mission and the existing Phase14 acceptance contract. The main
control is `9f1716410cd223ad65237ded1779a7c3b4f39d54`; do not silently substitute
PR33's production journey branch for that control.

The detailed source audit and ordered backlog live in Sub0ECS at
[`docs/optimization/crucible.md`](https://github.com/CraigHutchinson/Sub0ECS/blob/codex/crucible-optimization-receiving/docs/optimization/crucible.md).
The branch is a review candidate; it is not a merged dependency receipt.

## Architectural decision for the first experiment

Start with an adapter using the existing ECS `parallelFor(items, callback)` seam
and a borrowed Pipeline executor. Keep a coordinator-owned execution budget;
flatten dependent stages instead of introducing another nested worker pool.
Preserve disjoint row/scratch lanes and complete joining before mutation or
publication. `ScopedExecutor` limits which submissions a wait accounts for but
does not guarantee progress if every pool worker blocks awaiting child jobs.

The current consumer already uses Pipeline RowPartitions for proposal work and
sequential ECS iteration for gather/commit. There are not two active compute pools
to eliminate today. First measure ECS cost and the surrounding sorts, grid work,
per-row commit searches and copies. A generic adapter is useful API integration;
its existence alone is not evidence that it improves this workload.

Use Pub's existing `Wiring`/`Sink` for a fixed-endpoint alternate arm and retain
`Domain<T>` where registration and concurrent quiescence are required. The current
IntentDelivery is one fixed sink with coordinator-only calls. Preserve its run /
request IDs, admission result and exception transport. An asynchronous handoff
must own its command payload: returning from publish does not extend the borrowed
`IntentBatch.commands` span until a later Pipeline job finishes.

Keep one owner for each contract. ECS Read/Write metadata is not currently a
const-enforced proof, Pub receive capability is not a data-hazard declaration, and
Pipeline completion does not equal Pub disconnect. Map these deliberately; do not
create a common status enum, wire ID or policy base merely for naming consistency.

## Batch and dependency order

| Package | Repository / owner | Acceptance before the next package |
|---|---|---|
| Source reconciliation | ECS / Integration | PR15 style conversion is still off master; integrate its existing commits with API rename migration and library gates |
| Trustworthy collection | ECS + profiling host / Measurement | Failed/empty reports fail closed; actual VTune samples, symbol attribution and owned timeout/finalizer supervision received |
| Bounded row adapter | ECS + Pipeline / Scheduling | Single/empty/tail/many-partition cases; unique scratch lanes; accepted/rejected submissions; throw/join/destructor and no nested-pool deadlock receiving |
| Consumed gather/commit arm | Crucible / Simulation | Counted disjoint gather, stable ID/order mapping, failure-atomic staging, FP/bitwise 1/2/N and allocation/lifetime tests |
| Typed delivery arm | Pub + Crucible / Runtime | Fixed wiring vs existing domain, independent sessions, teardown/reentrancy, receipts and bounded payload ownership |
| Combined qualification | All / Integration | Control, adapter, delivery and combined arms on frozen pins; five alternating process pairs; unchanged G1-G7 |

The first bridge stays optional: ECS's standalone C++20 target must not acquire an
unconditional C++23/Pipeline dependency. A common helper is extracted only after
at least two real callers need the same semantics. Pipeline issue27's lean-task
spike is an input, not an accepted replacement for every custom executor.

Pub main is newer than this control's pin (`504d772...` versus `d566c71...`): PR36
changes configuration. Receive that migration as its own arm before combining it
with routing changes. Do not bulk-pin moving heads. Commit candidates in library
PRs, validate the coordinated consumer against explicit candidate SHAs, then merge
reviewed libraries, pin their actual merge SHAs, rebuild provenance and revalidate
the combined consumer. No release merge from a sibling checkout's uncommitted HEAD.

## Work and evidence ledger

This session owns the isolated `codex/sub0-integration-campaign` checkout's
optimization documentation only. Runtime, Simulation, scheduling implementations,
contracts and pins are not edited. PR33 and prior worktrees/branches are preserved.
No heavy CPU reservation is needed for this documentation pass.

The upstream profiler failure tests passed locally using a fake collector. They
establish report handling, not installed VTune capability or a profile. This host
has no VTune, perf or CMake on PATH, so no C++ integration, hardware profile,
compiler report, native frame or speedup is claimed. The cpp-write/cpp-review
skills are not exposed in the current catalog; no substantive C++ change is made
under this documentation batch.

Next accountable package is actual-consumer attribution on the profiling host,
with [the iteration template](iteration-template.md), followed by the bounded
adapter pass if the attributed cost supports it. Retain source/raw failures for
all passes and at least three changed implementations before parking a mechanism
for negative performance. No proposed row above counts as a completed pass.
