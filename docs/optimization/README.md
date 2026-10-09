# Optimization work and receiving ledger

Follow the [standing process](../OPTIMIZATION_PROCESS.md). Copy the
[iteration template](iteration-template.md) into an owned evidence directory for
each mechanism. Keep raw runs in a new artifact directory; publish the source,
compact record and necessary evidence/provenance without overwriting earlier work.
Do not put unbounded profiler databases into ordinary Git history.

## Current baseline and next investigations

PR30 merged on2026-10-09 as `c2ca771a9fd6746294e4f23fa5460f8a5bf05375`.
Its head `1f8d0191d7f65d402f14fb4ea0987ce4210c50d2` passed all ten jobs in
[run37917590945](https://github.com/CraigHutchinson/Crucible/actions/runs/37917590945).
The [sprint review](../sprint-reviews/phase-14.md) preserves actual source, local
receiving, diagnostic samples and limits. Merge receives delivered code and design;
it does not establish the major100K/150K complete-frame target.

| Mechanism | State / owner | Evidence and next falsifier |
|---|---|---|
| Immutable complete queries and bounded rows | Delivered / Spatial,Swarm,Scheduling,Runtime | Bitwise/replay/lifetime/FP/allocation receiving. Native fallback-free short samples; original worker FP state between jobs still lacks direct observation. |
| Exact dense-ID lookup | Delivered / Swarm | Four advancing rounds/dense-sparse oracle and Debug/Release checks. Short attribution only; qualify whole-consumer effect and combinations before a speedup claim. |
| Guard redundant sorts; dense commit lookup | Audited proposal / architect,Spatial | Preserve arbitrary order, duplicates, sparse holes and all float assignments. Compare sorted/unsorted input and full state/replay; no dispatch or speedup inferred. |
| Spatial rebuild reuse | Proposed / architect,Spatial | Needs valid geometry/membership epoch, invalidation before mutation/failure and FP-environment matching. Same-object FP transitions and post-failure queries are missing receiving. Do not remove rebuilds from a geometry-only assumption. |
| Candidate-cell iterator/code generation | Proposed / Sub0HexGrid owner | Profile divisions/mapping in an actual Swarm query before selecting an upstream change; preserve complete enumeration and region bounds. |
| Prepared validation/data layout/SIMD | Proposed / Swarm,architect | Actual call/byte accounting and compiler remarks first. Preserve complete input validation, operation order, in-place compatibility, borrow lifetime and bounds. |

Latest short eight-worker boundary means were28.90ms at100K and42.01ms at150K,
with approximately16.00/23.61ms outside proposals. These are unpaired diagnostic
means, not frame percentiles, qualified speedups or a measured serial lower bound.
They motivate both coordinator and kernel investigations.

## Acceptance remains unchanged

| Gate | Required observation |
|---|---|
| G1/G2 correctness and bounds | Bitwise1/2/N state/replay; joined failures/FP/storage/lifetime; no new kernel/query allocations. |
| G3 whole frame | Both target scales/routes: complete native frame service p95≤16.67ms and p99≤20ms, supported GPU completion and handoff. |
| G4 pacing/input | Paced60Hz, ≥59 presentations/sec, ≤1% missed/stale frames, no tick debt;300 accepted visible mutations/arm and p99≤50ms. Full cadence details remain in the phase plan. |
| G5 useful parallelism | Same-source whole-tick p95 ≥10% improvement at one target, neither >5% worse; matched2K overhead≤10%. |
| G6/G7 lifecycle/release | Actual supported native behavior, reviewed full/platform/sanitizer/race/omission receiving and exact-head publication/merge. Participant and unavailable device paths retain separate limits. |

The [phase plan](../phases/phase14.md#workload-and-success-gates) defines the authoritative
gate wording and measurement contract. No optimization record may weaken it by
substituting a smaller population, shorter sample or different timing scope.
