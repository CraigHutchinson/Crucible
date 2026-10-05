# Phase 10: one playable relay lattice

Status: implementation/local receiving delivered; hosted publication pending, 2026-10-05. Baseline PR22 merge
`fc9a4689992e0f803faf07d4315ddb1bf8d78ce1`. Architect accountable.

Implement the frozen [64→48+16 contract](../decisions/phase9-structural-proposal.md):
fixed mobile/anchored/lost identities, one relay, typed fuse/shatter commands,
protection, consecutive hold, conserved biomass and complete replay. Ship primitive
live controls and honest rejection feedback with the structural mission; legacy
modes remain supported. No growth, combat, factions, terrain or networking.

| Owner | Retain / package | Receiving |
|---|---|---|
| Architect | Contracts/runtime/mission, central docs/build/CI, sub0 requirements, evidence and merge | Bounded union queue, normal rejection trace/replay, terminal/restart, legacy regressions and exact-head acceptance |
| structural_rules | Simulation + interactions; fixed participation, membership/loss, protection and hold | Independent hand-count transitions, sparse query/reclamation, radius/order/stale rejection, no tick allocation |
| arena_rules | Presentation/desktop; mobile filtering, lattice/relay cues, F/X actions and owned equality | Live events/pixels, actual retained frames, GPU packet parity and rejection feedback |
| Hardware/platform | Defer physical/iOS sessions | Existing HW-01..HW-07 gates unchanged |

All buffers remain startup-owned; no parallel ECS mutation or new pin. Architect
owns serial builds. Shared contracts are frozen before dependent edits; workers
cross-review boundaries after integration. cpp-write/cpp-review skills remain
unavailable; repository patterns, independent review and actual acceptance apply.

Receive full fuse→hold→shatter→redirect and a structural win/replay, plus normal
rejections, insufficient mass, same-boundary ordering and terminal closure. Capture
and inspect actual structural frames with source/executable provenance. Run supported
Debug/Release and hosted normal ASan/UBSan, all nine final-head jobs before merge.
Human comprehension/tuning and physical performance remain separate gates.

## Sub0 evolution

Use the [owned-library roadmap](../reuse/sub0-roadmap.md) at dispatch/close. The
structural increment receives ECS/H2 through real population/query consumers and
keeps game rules local. Next integrations carry minimal upstream requirements,
fixtures and exact-pin consumer acceptance. No adapter is added only to fill a
reserved target, and no useful upstream requirement is discarded because a pin
currently lacks the capability.

[Receiving](../workstreams/integration/phase10-validation.md) and
[review](../sprint-reviews/phase-10.md) record actual results/limits. Next dispatch
is [Phase11](phase11.md), the coherent ECS/Pub v2/Pipeline backbone increment.
