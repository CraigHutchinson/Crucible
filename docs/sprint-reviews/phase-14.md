# Phase14 sprint review: interactive large-swarm performance

Status: planning reviewed, finalized 2026-10-08; implementation undispatched.
Accountable architect/reviewer: root. This record follows the review template;
execution fields stay pending rather than fabricating completion evidence.

## Intent, baseline and scope

[Plan](../phases/phase14.md), [contracts/review](../decisions/phase14-scale-contracts.md).
User chose full-frame performance plus bounded parallel simulation. Top-down:
read/control a continuous large swarm through FLOW/gather/repel and camera changes.
Bottom-up: receive immutable queries, bounded dispatch/joins and complete native
frame costs. Target both100K and150K evolving populations on a qualified reference
configuration; preserve the2K relay regression. Planning main `9e7fed990c404e95ab25e699090996aeec048040`
is PR29's merge; all9 checks succeeded at49f702a in run37619200451.

## Work delivered and delegation

Planning only. Two read-only agents assessed consumer and foundation constraints;
root reconciled user-selected scope and authored the package/contracts/gates.
Proposed A consolidates Spatial/Swarm/Scheduling; B consolidates snapshots/rendering/
frame receiving. Root retains Contracts/Simulation/Runtime/desktop shared wiring,
dependency promotion, all serial hardware acceptance and publication. No C++,
worker implementation, pin promotion, heavy build or performance capture occurred.

## Findings and resolutions

P14-R01-R08 are recorded with actual source evidence in the decision. Shared scratch,
allocating dispatch, coordinator graph ownership, fixed-world density, terminal-frame
FPS and offscreen-versus-live presentation require explicit receiving gates. Design
resolves decomposition; executor/API/backend/metric freeze remains G0 work.
The old architecture pin labels and P13-01 publication status are reconciled.
Independent draft review also found a circular G0/backend prerequisite, missing
all-stream supplement, weak route influence, ambiguous completion/paused latency,
discretionary sample length and implicit worker FP/dependency direction. Root
split G0a/G0b, supplied W0-W10 ownership plus affected briefs, and tightened those
criteria before the final plan review. The pipeline choice is a prerequisite
investigation; no implementation consumer is declared ready by prose alone.
Final correction preserved structural command ordering before movement and allowed
G0a capture probes to inform G0b. Useful front motion requires field attribution,
not initial drift. Consumer reviewer closed four findings and supplied that last
clarification; foundation reviewer confirmed its six original corrections and
supplied the two remaining wording fixes before a usage-limit interruption. Root
applied/checked all three final clarifications; no further independent pass is claimed.

## Verification and useful artifacts

Planning evidence: clean git status/worktree inventory, source inspection, exact
Pipeline cache-head match, live GitHub PR29 merge/all9-job receipt and read-only CPU/
GPU model inventory. Documentation references/consistency and independent plan
review are the relevant checks. No new test counts, race pass, FPS, live device,
native readability, audio or participant result is claimed. Actual captures and
raw frame timelines are pending P14-03/04/05; diagrams here explain proposals only.

## Retrospective and reuse

The intended scale experience requires readable density and coherent input, not
population alone. Existing mutable query scratch and inline ownership determine
the first safe concurrency seam. R03 executor guarantees may need a product-neutral
Pipeline reproduction/upstream improvement; H2 math and domain policies remain in
their current homes. No unconsumed caches, hierarchy or generic renderer is proposed.

## Follow-ups

| Stable ID | Status / accountable owner | Phase14 receiving gate or preserved scope |
|---|---|---|
| P01-F05, HW-05 | Core / architect,A,B | G1-G7, joined compute and complete physical frame envelope at both target scales |
| P11-F03 | Partial core / Runtime,Scheduling | Executor subset G1/G2; concurrent Pub producer remains deferred |
| P12-F01,P04-F02,P01-F04 | Partial core / presentation,architect | G6 native input/scale readability; no inferred participant acceptance |
| P05-F02,P06-F02,HW-06 | Conditional subset / rendering,architect | Selected live adapter lifecycle only; second backend and unobserved physical faults remain open |
| P12-F02,P13-02 | Deferred / consumer audio owner | Actual bounded audio/device/listening; separate increment |
| P09-F01,P05-F01 | Open / product owner | Human strategy/comprehension/balance; native operator checks cannot close |
| P01-F03,P06-F01 | Deferred / domain owners | Growth/attrition/resource/faction consumers, numerical/identity/authority contracts |
| P02-F02 | Deferred / snapshots,Runtime | Actual delayed concurrent reader, lease/backpressure/join receiving |
| P02-F03,P03-F01,HX-07 | Deferred / Spatial,world owner | Terrain/traversal/numerical geometry gates |
| P04-F01,P04-F03 | Deferred / platform owner | iOS packaging/signing/touch/physical lifecycle |

## Closure

Planning completion and changed paths are recorded in ACTIVE_WORK_LOG. Implementation
head, branch/worktree assignments, source checkpoints, measurements, exact-head CI,
PR/merge/local-origin verification and retained-artifact disposition are pending.
All old worktrees/artifacts remain preserved. Major milestone completion requires
the plan's G1-G7; a code-only merge cannot substitute for unreceived physical budgets.
