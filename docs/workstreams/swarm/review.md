# Swarm cpp-review handoff

Review scope: supplied base `a4e83f0` plus uncommitted swarm provider/manifests/tests.
The dispatch supplied the base and prohibited Git writes; no fetch or PR operation
was attempted. cpp-write and all four shared references were loaded before authoring;
cpp-review's stranger-question and L0–L3 checks were applied read-only afterward.

1. **SHOULD / L0, integration gate:** Steering has no production caller in this
   worker worktree. The execution ADR names Simulation's optional steering path
   and assigns its wiring to root. Keep this as a review-ready handoff; do not
   describe it as delivered until the architect's caller and full-state tests land.

No MUST findings identified in the bounded provider. L1: one concrete scratch-owning
type, canonical paths, no new abstraction or dependency cycle. Spatial/Fields are
private link dependencies and only forward-declared in the header. L2: frozen API,
nodiscard failure, checked capacity/order/finite/bounds/mapping, documented grid
precondition, exclusive access/borrow expiration and transactional alias-safe output.
The frozen bool plus destination-span API is intentional for caller-owned bounded
storage. L3: corresponding header first, range algorithms and scoped query borrows,
startup-only resize, no raw ownership/global state or hot-path allocation.
No new public observer requires a constexpr decision; TryCompute depends on mutable
Grid queries and runtime FieldSet sampling and is not constexpr.

Rule fixtures and runtime behavior remain subject to root's requested executable
validation. An independent integrator review should verify the actual Simulation
gather/commit ordering and caller precondition. Product-rule suitability is owned
by the frozen project ADR; this review does not claim playtesting or concurrency.
