# Fields handoff and validation

Changed owned files: include/crucible/fields/FieldSet.hpp, src/fields/FieldSet.cpp,
src/fields/CMakeLists.txt, tests/fields/FieldSet.cpp, tests/fields/CMakeLists.txt,
and this stream README/design/validation. Base 978190c; worker made no Git mutations.

The architect-owned Simulation optional scenario is the planned production caller:
construct fixed FieldSet once, check each boundary TryApplyEdit outcome, sample owned
Acceleration and integrate acceleration times tick duration. Runtime admits owned
FieldEdit values. Two consumed kinds are set(upsert) and remove(idempotent); no unused
add/clear/paint interface was introduced. Full-capacity replacement is valid; an
outside slot returns slot_out_of_range without mutation. Invalid in-range payloads
return invalid without mutation. Shared FieldEdit::IsValid(capacity) owns validation.

Fixtures register fields_sampling with CTest label fields. Analytic cases pin attraction,
repulsion, linear axial/diagonal falloff, range boundary, cancellation, zero radius,
coincident centers, upsert/remove/idempotence, ignored remove geometry, exhausted/zero
capacity, invalid enum and nonfinite inputs. Maximal strengths check finite saturation.

No compilation or tests were run by this worker; root owns CPU reservations. Request
Debug and Release fields_sampling, then supported ASan/UBSan and unfiltered combined
suites. No performance claim or instrumented allocation count is available. Source
review confirms edit/sample paths neither resize nor allocate. Prototype radial rule
is locally chosen (design.md), not an external algorithm port; painted flow is deferred.

cpp-review self-review: L0 production wiring pending architect; L1 FieldSet owns bounded
slot invariant and uses no interface hierarchy; L2 acceleration units, sample invalidity,
capacity and rejection semantics documented; L3 nodiscard/noexcept/span-free owned values,
matching class filenames and double finite accumulation checked. No open MUST/SHOULD
findings in worker surface; review real callers and admission/application handling.
