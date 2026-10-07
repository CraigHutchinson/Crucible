# Owned sub0 library roadmap

Crucible is both a game and a receiving consumer for libraries we maintain. Current
pins are the reproducible baseline, not a ceiling on architecture. When a missing
neutral capability blocks a useful increment, evolve its owning library with a
small requirement, independent fixture and consumer receipt. Keep biomass, missions,
factions and device presentation policy in Crucible.

## Architecture direction and accountable handoffs

| Project / boundary | Current receiving | Requirement to carry upstream | Trigger and acceptance |
|---|---|---|---|
| Sub0ECS / storage and identity | Fixed startup worlds/queries; Phase10 preserves identities and filters participation | Delivered at master `1b1114a`: bounded handle count (create terminates at `Entity::kMaxEntities`), reserve covers the entity table, documented 256-reuse stale-handle horizon. Still open: a recoverable capacity result, explicit generation exhaustion and transactional create/rollback guarantees | Before population growth/destruction: minimal capacity/fault-injection/forged-handle repro in ECS; exact-pin Crucible growth/replay receiving. Do not silently adopt allocation promises |
| Sub0HexGrid / geometry | H2 layout/complete candidates plus exact distance fallback | Stable finite-region numerical/rounding contracts; future height/traversal metric support belongs in geometry research | Spatial owner carries HX-07/extreme/boundary fixtures upstream. Terrain starts from sourced adjacency/metric rules; entity bins and stock remain local |
| Sub0Pub / typed delivery | Production scoped synchronous typed admission; queue owns accepted batches | Documented concurrent subscribe/disconnect lifetime; optional bounded owned sink/batch handoff if independently useful | Runtime owner supplies a real second producer, shutdown/join and source-mutation receiver. Prefer opt-in companion to duplicating a broker; preserve command cutoff/sequence and normal rejection semantics |
| Sub0Pipeline / dependency execution | Production startup-built inline boundary/capture/mission graph | Owned graph receiving, completion callback/join guarantees, failure propagation; bounded executor/cancellation support from measured workloads | Scheduling owner first receives declared production dependencies and full-state parity with direct execution. Parallel partitions follow race/lifetime evidence and end-to-end benefit, not core-count assumptions |
| Sub0Log / compact diagnostics | Phase12 optional startup-owned64KiB RuntimeDiagnostics; refusal and terminal records | Existing API received without upstream changes; keep record policy local | Decode/exhaustion/restart/full-state parity pass; coordinator-only binding, counted drops and cold output bounds documented |
| Sub0MemPage / bounded backing | No pin/caller | Explicit page ownership, allocation failure, resident budget and retirement | Evaluate exact source when measured working-set/allocation needs exceed fixed scenario storage; receive budgets and lifetime before integration |
| Sub0TieredCache / residency | No pin/caller | Eviction/residency ownership, tier transitions, deterministic failure and observation | Streaming/world scale with a named cache caller and controlled workload; no speculative cache in the present 2,048-cell arena |

Architect owns requirement triage; the affected workstream owns the neutral fixture
and library handoff. Library maintainers review API/packaging. Integration owns pin
promotion and all consumer acceptance. The same people may fill these roles, but
upstream library correctness and game receiving are distinct receipts.

## Increment sequence

1. **Phase10:** consume existing ECS/H2 for structural state, sparse queries and
   loss/protection; keep one bounded command path and owned read model. Record
   any neutral defect with exact source/repro, no library-wide fork of game rules.
2. **Phase11 backbone integration:** [forge the production backbone](../phases/phase11.md)
   with Pub v2 owned intent delivery, a real Pipeline sequential graph and ECS v2
   world receiving. Evolve APIs upstream when receiving exposes a gap, with fixtures and exact pins; make
   the integrated path normal only after direct-path parity and lifetime acceptance.
3. **Diagnostics increment:** receive Sub0Log's already available bounded
   memory API on actual command outcomes/summary records. Specify cadence, backing
   capacity, loss counters, decoding and shutdown before implementation. Enable the
   dependency only for its consuming target/option.
4. **Further input/execution increments:** extend the Phase11 production
   Pub→bounded sink→Pipeline consumer with neutral upstream examples and independently
   received producer/executor capabilities.
   Decide adapter ownership from actual API dependencies and independent consumers;
   no umbrella project or mandatory Pub/Pipeline coupling by default.
5. **Growth/world increment:** improve ECS and geometry guarantees before dynamic
   population/terrain; evaluate paging/cache only after recorded residency demand.

The diagnostics increment is a proposed next package, not a promise to block the
playable loop on logging. Reassess ordering at each sprint close with current need.

## Requirement and promotion record

Every upstream handoff contains: originating source SHA/phase; concrete consumer;
neutral requirement and smallest failing/receiving fixture; storage/lifetime/thread
ownership; capacity/overflow/failure behavior; alternatives; destination project;
maintainer disposition; upstream issue/PR/commit; proposed full pin; local and hosted
consumer acceptance. A source finding is not a current-HEAD defect without a repro.
Use repository-owned requirement documents first; open cross-project issues/PRs when
the receiver and exact-version evidence are ready. Do not alter dependency checkouts.

Pin promotion keeps one implementation source, target-scoped options and independent
package/consumer tests. A borrowed buffer, dispatch failure, drop or stale identity
must have a received contract before gameplay depends on it. Existing standard
containers/RAII remain appropriate where no sub0 capability is missing.

The [extraction review](phase9-extraction-review.md) identifies bounded handoff,
finite viewport geometry and capability tooling as possible base libraries. Prefer
improving existing sub0 projects where the neutral boundary fits. A new package
requires a named independent consumer, license decision, minimal API and evidence
that removed duplication exceeds packaging/versioning cost.
