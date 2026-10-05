# Phase 11: forge the sub0 production backbone

Status: planned, 2026-10-05. Dispatch only after Phase10 exact-head merge. Architect
owns the integration contract and records the actual merged baseline at dispatch.
This phase promotes Pub/Pipeline from stack examples to the live structural
backbone and strengthens receiving of the already production-consumed ECS. It is implementation work,
not another indefinite "adopt when needed" note. Use ECS/Pub v2; evolve neutral APIs
upstream where receiving exposes missing guarantees.

## Shipped outcome

One production path carries owned field/fuse/shatter intent through Sub0Pub v2 into
the existing bounded admission boundary. A startup-built Sub0Pipeline graph runs
completed boundaries, publishes the owned frame and evaluates mission/summary with
explicit dependencies. Sub0ECS v2 remains the authoritative world queried/mutated
only by its exclusive coordinator. CLI route and desktop input consume this path.

The direct path is the controlled receiving comparator while promotion is underway;
it is not a second rules implementation. Both call the same Simulation and queue.
Make the integrated path the normal production choice after parity/lifetime/build
receiving passes; retain direct execution only as a narrow test/benchmark baseline.
No mandatory threading, generic service locator, new umbrella framework or copied
broker/executor/ECS. Build dependencies follow actual consuming targets/options.

| Boundary | Library responsibility | Crucible responsibility |
|---|---|---|
| Typed intent delivery | Pub owns registration, typed routing, callback/disconnect lifetime and delivery policy | Concrete command payload, owned-copy adapter, all-or-nothing batch admission, accepted/full/closed receipts; callbacks never mutate ECS |
| Tick graph | Pipeline owns task dependency/dispatch/completion/failure/join contracts | Cutoff/trace capacity, actual boundary → owned observation → mission/summary jobs; declared exclusive world and scratch ownership |
| State store | ECS owns query/storage/identity/capacity guarantees | Fixed game identities/activity, biomass/faction policy, gather/order and full replay schema |
| Observation | Log may consume bounded diagnostics independently | HUD/mission values remain owned; dropping diagnostics cannot change simulation |

## Same team and staged packages

| Owner | Concrete package | Gate |
|---|---|---|
| Architect / contracts + receiving | Freeze typed intent/admission receipts and coordinator graph inputs/outputs; production wiring, direct comparator, docs/build/pins and merge | Queue/cutoff/trace semantics unchanged; complete scenario/terminal/restart equality and exact-head CI |
| Delivery worker | Inspect latest Pub v2, implement one owned sink consumed by desktop and scripted route; improve neutral upstream adapter/example where appropriate | Explicit most-derived subscribe/disconnect, joined producers, full/closed/batch/capacity and source-mutation receiving; delivery is not application |
| Execution/state worker | Inspect Pipeline and ECS v2; startup-owned sequential graph around the existing boundary and observation, explicit data access and teardown | Actual ECS structural scenario, graph failure/dispatch/completion callback/join fixtures, retained frames, truthful allocation and identity guarantees |

Reassess paths at dispatch and keep architect plus two workers. Review upstream
PRs and pin promotion separately from game receiving. No worker mutates fetched
CPM checkouts or runs shared builds. Upstream work lives in its project with its
own instructions/package tests; Crucible receives the reviewed full commit.

1. **Source/API shootout:** compare current pinned APIs with current ECS/Pub v2 and
   Pipeline main; read changes, not just tags. Build the minimum production receiving
   example with two actual intent sources and an owned sequential graph. Freeze
   adapter placement and API requirements; no performance promise from a checksum.
2. **Contract evolution:** implement/review necessary upstream changes with neutral
   fixtures. Existing APIs suffice where they already meet the receiving contract.
   Require failures and teardown to be representable without throwing from callback
   paths or leaving borrowed inputs alive in orphan work.
3. **Production receiving:** desktop + CLI route use the integrated path. Exercise
   field, fuse, shatter, normal refusal, pause, late admission/cutoff, full queue/trace,
   terminal and restart. Compare every sample/activity/field endpoint/stock/infection/
   member/generation/ledger/hold/outcome with direct execution.
4. **Promotion:** upstream package tests and consumer packaging pass; promote full
   pins atomically with adapters, supported Debug/Release/ASan+UBSan and exact-head CI.
   Remove unused intermediate adapters/examples. Keep failure/teardown fixtures.

## Contracts frozen before dispatch

- Each run has a distinct Pub domain/route and exactly one command-admission sink.
  Two live inspectors cannot cross-deliver intent. Restart constructs/registers a
  replacement without exposing old callbacks; quiesce publishers and disconnect
  the old most-derived sink while its ingress/storage remains alive. Receive
  registration/table-full failure before declaring the run usable.
- An owned admission receipt correlates a run/request ID with the whole batch,
  accepted sequence range or normal full/closed/invalid refusal. Delivery success
  is distinct from queue admission and later application result. Exactly one sink
  resolves the receipt; no borrowed callback payload or unbounded reply queue.
- Build/run the graph per attempted completed boundary inside clock catch-up.
  Boundary status paused/closed/trace_full skips capture and mission evaluation;
  unexpected failure preserves the last published good frame and fail-stops.
  Trace-full must not drain input. No partially completed frame is published.
- Order is boundary commit → owned state capture → mission/summary. Mission reads
  that completed frame, publishes its matching progress/outcome and closes input
  before the clock can run another catch-up boundary. Current direct evaluation
  is before capture but reads the same quiescent committed values; differential
  receiving proves equivalent outcomes and tick/admission stop semantics. Do not
  keep both stop callbacks or evaluate a completed boundary twice.
- A graph owns stable startup context and returns a concrete boundary status;
  paused/backpressure are normal results, never false task-success advancing the
  clock. Teardown joins dispatched bodies and completion callbacks before releasing
  borrowed ECS/ingress/frame state. No timed/orphan work in the first production path.

## Upstream requirements to resolve rather than work around

- **Pub:** bounded typed ingress receiving and explicit admission receipts; registration
  failure, concurrent disconnect and producer ownership. Decide whether the neutral
  sink fits an opt-in Pub extension, a Pipeline adapter, or should remain concrete
  application glue after inspecting both APIs. Cross-project coupling stays opt-in.
- **Pipeline:** repeatable startup-built graph, visible run/body/dispatch failure,
  completion-callback-inclusive wait and guaranteed owned teardown. Measure steady
  boundary allocations before promising bounded execution. If current run storage
  allocates, evolve reusable traversal/executor storage with independent fixtures.
  State access declarations must be consumed by execution/review; cosmetic tags do
  not establish scheduling safety. Timed work cannot escape capture lifetime.
- **ECS v2:** confirm query/view lifetime, capacity/index limits, generation reuse and
  reserve/create behavior against the selected exact source. Fixed Phase10 identity
  retention remains sufficient; growth/destruction promises wait for checked capacity,
  provenance/liveness and fault-injected rollback receiving in ECS itself.

Parallelism is a bounded stretch after sequential parity: only a genuinely independent
read-only geometry/observation or disjoint scratch partition, with joined mutation,
race/lifetime tests and measured end-to-end benefit. Do not dispatch ECS mutation
from Pub callbacks or hide unsafe storage access behind a DAG.

## Evidence and stop criteria

Receive actual live structural frames and route/replay through the integrated path,
full-state differential tests, teardown/failure stress, no-allocation counts where
promised, and a controlled direct-versus-integrated tick/build-cost comparison.
Measure Pub delivery/admission separately from graph dispatch and full tick/frame
work. Label tests that traverse live Pub versus direct trace replay; replay alone
does not prove delivery isolation or teardown. Report overhead honestly; integration benefit is the consumed ownership/composition
contract, not an assumed speedup. Verify licensing/package options before new pins.

A discovered upstream gap is a named requirement with a minimal fixture and owner;
complete its receiving fix before promoting the corresponding path. If a boundary
cannot be safely received within the sprint, retain the working baseline and report
that specific blocker rather than claim the backbone shipped. Human comprehension,
physical GPU/iOS and world streaming remain separate gates.

## Source discovery checkpoint

GitHub branch metadata checked 2026-10-05: ECS v2
`60285914ee8925f0ce20ac5426511604fb0c6529`; Pub v2
`b1166d908dbbd35eb56ca617bf1ee51d72e6e21f`; Pipeline main
`1c50051fe4a5d4d06766d53624542b0025573be1`. These are discovery revisions,
not audited/promoted pins or a claim that latest code satisfies the contracts.
Refresh and inspect them at dispatch. Existing Phase10 pins remain unchanged.
