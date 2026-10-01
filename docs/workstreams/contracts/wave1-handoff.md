# Wave 1 contracts handoff

Accepted architect gate: finite rectangular GridConfig, SampleId owned identity,
and FieldEdit set/remove values only. Existing Position/Velocity remain unchanged.

- `GridConfig.hpp`: `GridConfig {size_t columns, rows; float cell_size;}` and
  `TryValidate() const noexcept -> optional<GridExtent>` with derived cells/width/
  height. Rejects empty axes, nonpositive/nonfinite cell size, size_t product
  overflow and extents exceeding finite float range. Domain is the closed
  rectangle from zero to width/height; modules clamp positions without wraparound.
  Concrete module/scenario constructors own storage budgets and max_size checks.
- `SampleId.hpp`: uint64 value with total value ordering. Coordinator assigns
  application IDs once for wave 1 (no entity recycling). Spatial samples retain
  IDs independent of bins/row ordering. This does not strengthen ECS generations.
- `FieldEdit.hpp`: kind set/remove, bounded slot, owned center/radius/strength.
  `IsValid(field_capacity)` validates slot for both kinds and all set numerics;
  radius may be zero, signed strength admits attractors and repulsors. Remove
  ignores unused numeric payload. Set upserts; removal is idempotent. Fields owns
  sampling semantics; runtime admits owned values and applies them at boundaries.

Named combined production consumers: root optional ScenarioConfig/Simulation grid
validation; spatial index geometry and stable gathered sample IDs; fields slot
updates; runtime command admission/replay payloads. Root must wire these callers
before declaring completion. No snapshot/event/structure or generic limits APIs.

Source audit: integration/wave1-pinned-audit.md (relative sibling docs folder).

## Requested validation (not run by this worker)

Contracts test target `crucible_contracts_test`, CTest `contracts_values`, label
`contracts`. Tests check derived geometry, zero/negative/nonfinite values, size_t
cell multiplication overflow, float extent overflow, copied command independence,
negative radius/nonfinite strength/invalid enum, slot bounds including zero capacity,
ignored removal numerics, and sample ordering. Root owns Debug/Release and supported
ASan/UBSan build reservations and combined production/lifetime acceptance evidence.
No timing, FPS, allocation or thread-safety acceptance claim is made.

Self-review loaded cpp-write/cpp-review references. L0 callers are agreed combined
integration work, pending root wiring; L1 standard-only owned contracts and no new
module cycles; L2 nodiscard/noexcept failure signals and explicit bounds semantics;
L3 no allocating contract validation or borrowed payloads. No MUST findings remain
in this bounded diff. Build/run evidence remains pending, not inferred from review.
