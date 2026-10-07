# P8-F: straight capsule flow receiving

Status: resumed self-review complete; native validation pending architect CPU, 2026-10-04.
Worktree `Crucible-phase8`, branch `phase8-playable-flow`; baseline
`50f61d89bb1324d2f3bdaa09e1b16bcf34fec6d9` (PR19 merge).
The [frozen command/force](../../decisions/phase8-flow.md) is authoritative.
`cpp-write` and its four shared references were loaded before authoring.

## Consumed change

FieldEdit appends `set_flow` and the owned endpoint without changing old aggregate
positions or enum ordinals. Validation requires finite distinct endpoints and
finite nonnegative radius/strength. Radial/removal commands continue ignoring end.
FieldSet replaces one fixed slot atomically, retaining flow kind/end; copied radial
and removal observations canonicalize end to zero. Sampling projects onto the
bounded segment in double, uses capsule linear falloff along the segment direction,
and accumulates mixed fields in stable slot order before existing finite saturation.
Zero radius/strength contributes zero; no epsilon, new allocation or storage growth.

Production callers remain Simulation::TryApplyFieldEdit/Simulation::tick and
Swarm::Steering::TryCompute through the existing FieldSet boundary. The desktop
worker consumes the frozen command through existing bounded ingress. Trace and
snapshot containers already copy the full owned command; no routing protocol or
runtime production implementation changes are needed.

Full-state comparisons in main, phase2/phase3, mission examples/sensitivity,
ReferenceMissionRoute, ReclamationMission and HeadlessSession now include both
endpoint coordinates. The phase3 text dump includes endpoints. GPU/mission JSON
exports identify set_flow distinctly and include end_x/end_y. The existing CLI SVG
export draws a clipped capsule and directed centerline for active flow; no field is
misrepresented as removal. Legacy mission rules and radial schedules are unchanged.

## Authored evidence, not execution claims

Existing registered fixtures were extended, with no new executable:

- Contracts pin preserved enum ordinals, finite/distinct endpoint validation,
  zero values, owned copies and ignored radial/remove endpoints.
- Fields independently pin horizontal center/boundary/end caps, reversal,
  vertical/diagonal direction, opposing/mixed overlap, inactive contributions,
  nonfinite input, fixed capacity, rejection immutability, retained observations,
  insufficient-copy nonmutation, canonical copies, extreme subtraction/saturation
  and the smallest distinct float segment. Sampling remains allocation-free by
  inspection of the existing fixed-slot loop and stack-only math.
- Ingress pins atomic invalid/full batch rejection, sequence preservation, FIFO
  erase and immunity to both source endpoints being mutated after admission.
- HeadlessSession pins paused admission, completed/retained snapshots, replace/
  erase, independent production acceleration and full live/replay comparisons of
  metadata, sample IDs/positions/velocities, fields/endpoints, infection, stocks,
  biomass ledger and exact applied commands. Both steering and legacy movement
  paths receive flow; invalid replay endpoints reject before any mutation.
- The independent all-pairs steering oracle uses normalized longitudinal travel
  instead of production's squared-length projection. Mixed/strong/extreme flows
  receive existing Euclidean acceleration and speed caps.

No builds or native tests were run by this worker. `git diff --check` found no
whitespace errors at handoff. No staging, commits, pushes or CPU reservations.
Architect must receive Debug/Release/sanitizer and unchanged legacy mission outcomes.
The architect has received the manifest request and added Crucible::Presentation
to the existing crucible_runtime_session_tests link list for ScenarioSnapshot receiving.
The resumed L0–L3 self-review found no blocking issue: the production Simulation
and Steering consumers use the extended fixed-slot boundary, sampling has only
stack intermediates, and all full-state comparison/export sites retain endpoint
coordinates. SVG/JSON changes have been source-audited; actual flow captures and
native execution remain architect receiving gates. No authored fixture is an
execution or human comprehension claim.

The resumed receiving audit added off-axis rounded end-cap samples, diagonal
falloff and the smallest diagonal segment; axis-only cases cannot conceal these
projection errors. Changed public header contracts now document parameters and
result behavior. The desktop worker received the ready contract and will add
replay of an event-generated trace. Current CLI/GPU diagnostics retain their
radial reference scenarios: flow export branches have source audit, while executed
flow captures come from the production SDL painter.

## Limits and reuse

Input gestures/drawing/pixel/event fixtures belong to P8-P and reciprocal review.
Actual implementation captures and visual inspection belong to architect receiving;
no image or human acceptance is claimed here. Existing hardware prerequisites and
physical/iOS/full-frame gates remain independent. R07/R08 remain local: this is a
specific gameplay consumer, not a generic flow framework, new pin or upstream claim.
