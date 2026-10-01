# Runtime wave 1 handoff

Status: implementation and fixtures ready for architect integration; no build or
test execution by this worker. The architect owns serialized verification.

## Registered acceptance

`runtime_headless` remains unchanged and covers the old `run_ticks` workload.
`runtime_ingress` covers full and atomic batch rejection, invalid payloads, monotonic
sequences without rejection gaps, cutoff late arrival, wrap-around FIFO, insufficient
drain output, close with a full ring, copied producer values, joined concurrent
producer/coordinator lifetime and zero/overflow construction failures.

`runtime_session` covers paused admission, resume boundary consumption, set/remove
batches, exact owned trace and fresh-scenario checksum/cellular observation replay,
whole-input rejection before mutation, explicit replay/recording capacity failure,
close with an exhausted trace/full queue and trace overflow construction failure.

Requested commands in the combined tree: Debug and Release preset configure/build,
`ctest --preset debug -L runtime --no-tests=error`, both unfiltered suites, and the
supported ASan/UBSan preset suite. No throughput or race-sanitizer claim is made.

## Self-review

`cpp-write` and its four references were loaded before authoring; `cpp-review`
was applied to the owned source against the accepted wave 1 contract. Ring and
session enforce real ownership/capacity/lifecycle invariants, expose spans instead
of containers, mark failure/observer results nodiscard and allocate only during
construction. Session buffer arithmetic is checked before allocation. Mutex APIs
intentionally permit lock errors to propagate rather than promise noexcept.

No remaining owned-code MUST finding identified. L0 production wiring is an
architect integration gate: main's optional scenario must consume admission,
statistics, pause/resume, step, trace/replay and close; the session consumes ingress
cutoff/drain and Simulation hooks. The worker's isolated tree does not supply main
or shared Simulation implementation.

Unexpected Simulation application failure/tick exception is handled explicitly but
has no injected-fault fixture: valid admitted edits against the unchanged borrowed
scenario are expected to apply. There is no test-only callback or invalid lifetime
seam to force that failure. Threaded simulation, Pub delivery/unsubscription,
wall-clock catch-up and display snapshots remain outside this bounded increment.

## Architect integration result

The first increment is now wired and passes the combined Debug/Release and
ASan/UBSan suites. See [central wave 1 evidence](../integration/wave1-validation.md)
for actual commands, review findings and remaining package gates. The worker-only
execution status above records the original handoff.
