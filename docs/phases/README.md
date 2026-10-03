# Iterative phase workflow

Crucible progresses through reviewed increments merged to main. The game design
sets intent; the architecture sets technical boundaries; the package backlog lists
capabilities. A phase selects a small coherent subset of that backlog. A folder or
package is not a permanent agent assignment.

| Phase | Increment | Record |
|---|---|---|
| 1 | Headless foundations, bounded commands/replay, spatial/fields/Blight and game intent | [Review](../sprint-reviews/phase-01.md), [handoff](../workstreams/integration/wave1-validation.md), [PR 1](https://github.com/CraigHutchinson/Crucible/pull/1) |
| 2 | Bounded local steering, clock/observability, owned state inspection and actual-state SVG | [Review](../sprint-reviews/phase-02.md), [delivery](../workstreams/integration/phase2-validation.md), [plan](phase2.md) |
| 3 | Finite reclamation; Crucible hex receiving proof after upstream H2 delivery | [Work packages](phase3.md), [review in progress](../sprint-reviews/phase-03.md); architecture preparation only |

The [sprint-review archive](../sprint-reviews/README.md) tracks findings, follow-ups
and delegation lessons for every completed increment.

Phase 1 merged to main at `ab708a7` after exact-head Linux/Windows Debug/Release and
sanitizer CI passed. Agent worktrees remain retained with their prior artifacts;
they are not active phase 2 claims.

## Phase start: review the division of work

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

Spend remaining quota on correcting/reviewing the core increment before stretching.
Never skip correctness or lifecycle gates to obtain an attractive demo. Choose one
small visual artifact when it consumes the validated state without forcing a new
graphics stack. Carry unfinished stretch explicitly to the next phase.

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
