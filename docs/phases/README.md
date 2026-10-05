# Iterative phase workflow

Crucible progresses through reviewed increments merged to main. The game design
sets intent; the architecture sets technical boundaries; the package backlog lists
capabilities. A phase selects a small coherent subset of that backlog. A folder or
package is not a permanent agent assignment.

| Phase | Increment | Record |
|---|---|---|
| 1 | Headless foundations, bounded commands/replay, spatial/fields/Blight and game intent | [Review](../sprint-reviews/phase-01.md), [handoff](../workstreams/integration/wave1-validation.md), [PR 1](https://github.com/CraigHutchinson/Crucible/pull/1) |
| 2 | Bounded local steering, clock/observability, owned state inspection and actual-state SVG | [Review](../sprint-reviews/phase-02.md), [delivery](../workstreams/integration/phase2-validation.md), [plan](phase2.md) |
| 3 | Finite reclamation; Crucible hex receiving proof after upstream H2 delivery | [Work packages](phase3.md), [review](../sprint-reviews/phase-03.md), [delivery](../workstreams/integration/phase3-validation.md); complete, [PR 7](https://github.com/CraigHutchinson/Crucible/pull/7) at `21f3786` |
| 4 | Live owned-frame inspection, field input and concrete batch adapter | [Plan](phase4.md), [review](../sprint-reviews/phase-04.md), [delivery](../workstreams/integration/phase4-validation.md); complete, [PR 9](https://github.com/CraigHutchinson/Crucible/pull/9) at `09ae74f` |
| Interim | Owned instance-packet rendering spike | [Report](../../spikes/rendering/README.md); [PR 11](https://github.com/CraigHutchinson/Crucible/pull/11) at `0bc9731` |
| 5 | Playable reclamation quota/deadline and instancing design | [Plan](phase5.md), [review](../sprint-reviews/phase-05.md); complete, [PR 12](https://github.com/CraigHutchinson/Crucible/pull/12) at `57c1da6` |
| 6 | Real instancing foundation and mission receiving examples | [Plan](phase6.md), [review](../sprint-reviews/phase-06.md); optional foundation delivered, PR15 at `7317a03c8bca6848b17a352f0a192fb0d8e0e375`; fault/device gates open |
| 7 | Controlled GPU fault/retirement receiving and mission timing investigation | [Plan](phase7.md), [review](../sprint-reviews/phase-07.md); merged/recovered through PR19 at `50f61d8` |
| 8 | Playable straight flow, primitive cues and resource/faction concepts | [Plan](phase8.md), [review](../sprint-reviews/phase-08.md); recovered/merged PR21 at `a95c511` |
| 9 | Structural/arena rules comparison and production relay-density investigation | [Plan](phase9.md), [review](../sprint-reviews/phase-09.md); investigation delivered, [PR22 publication receipt](https://github.com/CraigHutchinson/Crucible/pull/22) |
| 10 | Playable fixed-identity relay lattice, loss/protection/hold and owned replay | [Plan](phase10.md), [review](../sprint-reviews/phase-10.md); merged PR23 at `262c4eb` |
| 11 | Forge ECS/Pub v2 and Pipeline into the production backbone; upstream API evolution | [Plan](phase11.md), [review](../sprint-reviews/phase-11.md); local receiving complete, [PR24 receipt](https://github.com/CraigHutchinson/Crucible/pull/24) |

| 12 | Runtime budgets and opt-in bounded diagnostics | [Plan](phase12.md), [review](../sprint-reviews/phase-12.md); receiving complete, PR25 publication |
| 13 | Consumer presentation/input/sound reconciled with measured foundations | [Proposal](phase13.md); not dispatched |

The [sprint-review archive](../sprint-reviews/README.md) tracks findings, follow-ups
and delegation lessons for every completed increment.

Phase 1 merged to main at `ab708a7` after exact-head Linux/Windows Debug/Release and
sanitizer CI passed. Agent worktrees remain retained with their prior artifacts;
they are not active phase 2 claims.

## Phase start: review the division of work

### Standing two-way design loop

Every phase develops the intended consumer and its technical foundations together.
This is standing user direction from 2026-10-05, recorded in AGENTS.md. A library
backlog alone cannot choose the next product increment, and a visual concept alone
cannot establish backend feasibility or rules.

1. **Top-down:** choose one observable player task and a bounded end-consumer
   concept/spike. Explore graphics, interaction/UI and sound together where they
   affect that task. State desired fidelity, feedback, latency, cadence and failure
   behavior. Name what actual rendering/input/audio and participant evidence can
   answer; distinguish generated concepts and synthetic probes.
2. **Bottom-up:** inventory the actual consumed sub0/runtime capabilities. Receive
   storage bounds, ordering, lifetime, performance and failure behavior. Feed these
   verified limits forward into the consumer's design rather than treating the
   existing implementation as the final experience by default.
3. **Reconcile:** map each demonstrated consumer requirement to an existing
   capability, a bounded local interface or a justified upstream requirement.
   Record trade-offs, evidence and disposition. Feed product-neutral reproductions
   to the owned library; keep game rules and UX policy local. No speculative APIs.
4. **Meet in a vertical slice:** freeze only the contracts the slice consumes,
   implement and play/render/listen to it, then receive full behavior/lifecycle and
   review quality. Update both the consumer concept and foundation from findings.

At start, name both directions and the intended reconciliation; at close, report
what each taught us, which requirements were met or revised and the next useful
consumer-led slice. These are design directions, not permanent worker assignments.
Balance their scope within the existing quota and serial measurement rules.

Read the latest [sprint review](../sprint-reviews/README.md), carry unresolved follow-up
IDs into the new plan and create its in-progress review from the template.
Start from current main and inspect the completed increment, open findings, product
intent and remaining quota. Record one player-facing or foundation outcome and its
acceptance. Reconsider the active streams before dispatch:

- Retain a stream when it has an independent deliverable, clear owner and real caller.
- Consolidate closely coupled small changes when their coordination costs exceed
  their independent value; separate folders can share one phase owner.
- Split a stream only around stable contracts and disjoint ownership, not to fill slots.
- Defer a stream when it lacks a consumer or an unmet prerequisite blocks meaningful work.
- Identify shared surfaces, contract gates, integration order and a bounded stretch.
- Review the [reuse catalog](../reuse/README.md): retain domain policy locally,
  reuse or improve existing sub0 libraries, and propose extraction where a consumed
  product-neutral boundary is demonstrated. Record upstream evidence and disposition.

The phase plan records the resulting ownership table and reasons for every change
from the preceding phase. Revise it when evidence changes scope. Keep durable module
folders; do not rename/move modules merely because agent assignments changed.

## Quota and execution

Prefer a small number of complete increments to many concurrent partial packages.
Default to the architect plus at most two implementation agents for phase 2. Each
gets a bounded task, exact paths, base, prerequisite contract and stop condition.
Existing GPT-6.1 Sol low assignments can be reused when workers are dispatched;
planning alone does not launch them or start implementation.

Architect owns shared contracts/wiring and review. Workers notify relevant peers
and architect once a contract changes or a gate/handoff is ready; record decisions
in docs instead of repeated status polling. Use one consolidated review per coherent
handoff and one combined validation pass after integration. A failure or new change
justifies targeted additional checks; a green unchanged tree does not need repeated
full builds. Serialize builds and reserve an uncontended host for measurements.

Before long acceptance runs, publish a reviewed source checkpoint to the sprint
branch and record its exact SHA; a draft PR can expose incomplete status. A local
commit alone does not survive every execution-service failure. Preserve completed
receipts and captures remotely. On transport loss, keep unknown runs unconfirmed,
use hosted exact-head CI, and reconcile retained local trees when access returns.
Recover source only from exact authored records and verify immutable blob identity;
never regenerate a completed package to guess what was lost.

Spend remaining quota on correcting/reviewing the core increment before stretching.
Never skip correctness or lifecycle gates to obtain an attractive demo. Visual
evidence is part of completion wherever viable, as specified below. Bound captures
to the changed behavior; avoid adding a graphics stack solely for evidence.

## Visual examples at each iteration

At phase start, name the observable behavior and planned capture in each package.
At completion, capture and visually inspect representative examples of the actual
implementation wherever viable: screenshots, deterministic frame exports, a short
interaction recording, or a plot/diagram derived from actual output for headless
changes. Choose the smallest set that explains the increment and its relevant
states; use before/after pairs when the change is visual. A code-only change with
no useful observable visual may record an explicit reason instead.

Store captures with the Git-backed sprint evidence (normally
`docs/concepts/exports/phaseN-*`), link them from the sprint review and record:
source revision/dirty state, reproduction command or capture script, scenario/seed,
tick and applied commands, backend/platform/toolchain, dimensions/DPI as relevant,
and what was visually checked. Keep raw output or replay identifiers behind plots.
Distinguish software exports, actual GPU/device captures and generated concepts.
Do not infer device acceptance, human playtesting or performance from an image.
If capture is blocked, record the blocker, owner and receiving gate; it remains
open rather than being silently dropped. For a failed required backend/device gate,
a software substitute does not complete that package. Avoid sensitive/private SDK
content in public captures.

## Phase close: review, merge and preserve evidence

Complete real consumer wiring, appropriate fixtures, cpp-review and supported
Debug/Release/sanitizer checks for substantive C++. Record actual results, limits,
commits and unmet package gates. For documentation-only changes, validate references
and consistency; avoid unrelated rebuilds locally.

Push the phase branch, open/update a reviewable PR, verify exact-head CI and resolve
findings before merge. Merge the completed increment to main and confirm the local
checkout matches origin/main. Record the PR and merge commit in the handoff. Do not
leave a completed phase on an unmerged branch as the next phase's assumed baseline.

Retain worktrees with live work or useful artifacts. Mark old claims complete;
refresh or create phase worktrees from the merged baseline only after inspecting
status. Delete only branches/worktrees proven unused and safe to remove. Closing a
phase does not authorize deleting someone else's work or artifacts.

Complete `docs/sprint-reviews/phase-NN.md` using the [review template](../sprint-reviews/template.md),
link it from the phase plan/index and active work log, and record the merge baseline.
Include delivered scope, findings/resolutions, evidence and limits, stable follow-up
IDs with owner/gate, reuse feedback, next split and cleanup/artifact disposition.

End with a short retrospective: what was delivered, what was consolidated/deferred,
what review found, which uncertainty remains and how that changes the next division.
The next phase begins by reassessing these facts rather than repeating the old team map.

Current increment is [Phase12](phase12.md); [Phase13](phase13.md) shapes a balanced consumer slice. Shader portability and second-backend
work retain their capability gates in the hardware backlog; they are not
prerequisites for this SDL-free rules investigation.
