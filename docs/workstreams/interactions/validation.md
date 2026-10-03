# Reclamation worker handoff

Base: planning `71c14c0`; shared contracts received as `7f0e75c`. Owned changes are
interactions/Blight headers, sources, tests and documentation only. Root composition,
state-copy contracts, presentation and integration acceptance remain architect-owned.

Authored fixtures independently specify exact stock, infection, reserve, mobile mass,
initial total and cumulative work for partial/depleted/zero-stock work, natural spread,
reinfection maintenance, absent/disabled population, shuffled inputs, cell-first global
priority, clamped contact, interior/final edges and adjacent representable positions.
Failure fixtures cover duplicate/zero/out-of-domain IDs, insufficient/excess sample
count, nonfinite positions, mismatched geometry and an outstanding Blight lease.
Startup fixtures cover invalid geometry/mass, checked product/sum overflow and an
accepted harvest at the maximum conservation total.

An isolated executable intercepts ordinary new/new[] over 128 accepted domain steps
and one rejected input after startup. It observes the actual provider/Blight calls;
it does not claim full ECS workload allocation freedom, aligned allocation coverage
or interception of direct C allocation. No steady-state vectors resize or grow.

The work_actions exhaustion branch is reviewed but not dynamically reached: the private
counter begins at zero, and reaching uint64 maximum would require infeasible ticks.
No test-only setter or unused public arithmetic helper was added. Runtime arithmetic
rejects before committed infection/material/ledger publication; pending lease destruction
releases its scratch on failure.

cpp-write/cpp-review SKILL.md and the four shared references were loaded; repository
C++23, standard-library and .hpp conventions take precedence over generic overlays.
Self-review finds no unresolved MUST: the value contract and actual Simulation caller
are supplied in the architect integration; bounded ownership justifies concrete stateful
classes; no cyclic module dependency or arbitrary committed clearing API exists.
GetLedger/GetStocks/GetConfig use constexpr for their simple value/borrow observers.
The long TryStep coordinates validation, deterministic reduction and publication in
one bounded operation; splitting it merely to meet a line-count heuristic would obscure
its atomicity. Gameplay balance remains the architect's product review responsibility.

`git diff --check` passed. No compile, CTest, sanitizer or timing run was performed by
this worker: the architect reserves CPU and runs combined executable gates. This is a
provider handoff, not delivery evidence until those calls and gates pass.
