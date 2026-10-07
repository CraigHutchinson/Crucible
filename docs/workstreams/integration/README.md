# Integration workstream — W0 and composition ownership

## Hierarchy and acceleration boundary

Own frozen ECS gathering, exact-pin capability audit, publication/structural commit and cross-module wiring. Appoint the navigation research owner before HN spikes; coordinate semantics with their defining modules rather than absorbing them.

See the [application responsibility map](../hierarchy-boundaries.md) for defining owners,
read/write sets, lifetimes, bounded progress and cross-project handoffs.

Current groundwork evidence: [2026-10-01 validation](validation.md).
Current implementation evidence: [first collaborative wave](wave1-validation.md).

Own root CMake, cmake/, presets, CI, AGENTS.md, CONTRIBUTING.md, src/main.cpp,
src/simulation.cpp, include/crucible/simulation.hpp, tests/integration/ and central docs.

Current phase 3 evidence: [finite reclamation and hex receiving](phase3-validation.md).

Core owns ECS and consumes Contracts/Swarm/Spatial/Fields/Blight/Interactions; Runtime is the main caller.
Stack libraries link explicitly to the stack test instead of leaking through Core.
Integration tests register in tests/integration/CMakeLists.txt; domain tests are local.

First task: audit exact pinned identity/capacity/callback/executor guarantees and
validate Debug/Release plus supported sanitizers. Existing groundwork checks do not
complete W0's full audit. Upstream/pin changes require consumer evidence.

Coordinate W1 contracts; merge prerequisites before wiring. Own queries, scheduling
integration and coordinator commit. Audit allocation/exception behavior before W6
promises transactional structural mutation. Review all shared-surface consumers and
lifetime/access declarations, run combined tests, preserve the 150K/60-tick workload,
and record platform limits. Use [session guide](../README.md) and
[work breakdown](../../work-breakdown.md); claim files/CPU before work.
