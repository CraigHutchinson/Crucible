# Sprint reviews and retrospectives

One review per completed phase, named `phase-NN.md`. Plans remain in `docs/phases`;
stream handoffs retain detailed implementation evidence. This folder records what
was delivered, what review found, what we learned and what the next iteration must
consider. Read the latest review before planning or delegating another sprint.

| Phase | Outcome / state | Review | Merged baseline |
|---|---|---|---|
| 1 | Headless foundations and game intent; complete | [Phase 1](phase-01.md) | `ab708a7`, PR 1 |
| 2 | Steering, bounded clock, owned inspection and actual-state visual; complete | [Phase 2](phase-02.md) | `cdc6034`, PR 4 |
| 3 | Finite reclamation and consumed H2 receiving proof; complete | [Phase 3 review](phase-03.md), [plan](../phases/phase3.md) | `21f3786`, PR 7 |
| 4 | Interactive inspection/control; complete | [Phase 4 review](phase-04.md), [plan](../phases/phase4.md) | `09ae74f`, PR 9 |
| 5 | Reclamation challenge, instancing design and resilient validation; complete | [Phase 5 review](phase-05.md), [plan](../phases/phase5.md) | `57c1da6`, PR 12 |

## Review procedure

Copy [the template](template.md) when a phase starts and mark it **in progress**.
Update it at coherent handoffs and finish it before merging the phase PR. Link exact
implementation/evidence commits, test results and their limits. After merge, record
the actual merge baseline; a small documentation follow-up may supply that SHA.
Do not call a proposed phase delivered or invent execution/measurement evidence.

Give follow-ups stable IDs, a status, an accountable role, a gate and a receiving
phase/backlog item. Carry unresolved IDs forward explicitly instead of silently
resetting or duplicating them. Close them with evidence. Old reviews remain historical
snapshots; the latest review explains which earlier follow-ups were resolved.

Record retain/consolidate/split/defer decisions and reusable-library disposition.
Keep the work breakdown and reuse catalog authoritative for capability scope; reviews
explain the decisions and learning. Link this review from the phase plan, phase index
and active work log. Review branch/worktree cleanup after each merged phase.

## Branch cleanup: 2026-10-01

User authorized branch cleanup after phase 2. Nine completed local branches and three
remote branches were removed: `workstream-groundwork`, `phase2-plan`,
`phase2-steer-inspect`, four `wave1-*` branches and two `phase2-*` worker branches.
The three remote branches were the groundwork/plan/phase delivery branches; worker
branches were local. Main and every affected worktree were clean before cleanup.
Worker handoffs were integrated through reviewed cherry-picks, so some worker tips
were not main ancestors. Their exact commits are preserved by archive tags on origin;
all six worktrees remain detached at those commits with files/build artifacts intact.

| Retained worktree | Archive tag | Commit |
|---|---|---|
| `.worktrees/blight` | `archive/sprints/phase-01/blight` | `a5784ac` |
| `.worktrees/integration` | `archive/sprints/phase-01/integration` | `65a4fbd` |
| `.worktrees/runtime` | `archive/sprints/phase-01/runtime` | `4739a22` |
| `.worktrees/spatial` | `archive/sprints/phase-01/spatial` | `81e13dc` |
| `.worktrees/phase2-runtime` | `archive/sprints/phase-02/runtime` | `ad269ce` |
| `.worktrees/phase2-swarm` | `archive/sprints/phase-02/swarm` | `18dfe8e` |

Inspect historical work with `git show archive/sprints/phase-02/runtime`. Fetch tags
in a new clone if needed. Start new implementation work from current merged main,
not an archived handoff. This cleanup removed branch references, not worktree folders,
artifacts, commits or any sibling-project branch. The review-docs delivery branch
is temporary and is removed after its PR merges.
