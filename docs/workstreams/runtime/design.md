# First bounded runtime increment

The optional headless scenario is the production consumer. The existing
`run_ticks(Simulation&, count)` remains the compatibility path.

Admission owns copied field edits in a ring allocated once at construction. A
mutex serializes admission, cutoff capture, bounded draining and close. Producers
must finish before the owner destroys the ingress; the ingress retains no caller
buffers or callbacks. Reject newest when full. A batch either receives consecutive
sequences in full or changes nothing. Sequence zero denotes no accepted command.

At a tick boundary the coordinator captures the latest admitted sequence and
drains only its prefix. Commands arriving after capture remain queued. Close is
stored separately from ring occupancy and rejects new input even when full.
Pause suppresses boundaries and simulation ticks while admission remains bounded;
resume applies queued edits at the next boundary.

The coordinator reserves trace storage before removing a captured prefix or
applying it. A trace-capacity failure leaves the simulation and queue unchanged.
Each applied record owns the edit, admission sequence and tick. Replay validates
the complete bounded input before applying any edit, then uses recorded boundaries
and sequence order; it does not depend on wall time or a Pub callback.

The accepted shared command is `FieldEditKind::{set,remove}` with a bounded slot,
center, radius and signed strength. Set upserts; remove is idempotent and ignores
unused numerical payload. `FieldEdit::IsValid(field_capacity)` is checked before
admission and before replay. The architect supplies Simulation's
`TryApplyFieldEdit` and `GetFieldCapacity` hooks. No Pub adapter or threaded
executor is part of this increment.

`HeadlessSession` borrows Simulation exclusively and owns ingress, boundary scratch,
trace, completed tick count and pause/failure state. Session methods belong to one
coordinator; only ingress methods may run concurrently. The matching Simulation
must start fresh for replay, and producers must be quiescent during replay input
validation. Replay closes live admission before advancing recorded boundaries.

Trace exhaustion leaves the whole captured prefix queued and performs no field
edits or tick. An unexpected field rejection closes ingress and permanently stops
the session; previous edits at that failed boundary are not rolled back. A throwing
tick also closes/stops before propagating. Neither failure records a successful
boundary. The completed trace ends at the last successful tick.

## Behavior fixtures prepared for the contract handoff

- Full ring rejects newest without consuming a sequence; space can be reused.
- A batch larger than free space rejects entirely; accepted sequences are adjacent.
- A late arrival after cutoff survives that drain and appears at the next boundary.
- Close works with a full ring, is idempotent and rejects admission.
- Pause retains queued edits and tick identity; resume applies them exactly once.
- Exact replay reproduces field state and simulation observations at the same ticks.
- Invalid capacity arithmetic fails construction before allocation; session checks
  combined ring, boundary and trace bytes before constructing any buffers.
- Producer values can be changed/destroyed after admission; consumer reads copies.
- A producer thread is joined before ingress destruction; no callback borrows exist.

## Verification ownership

The architect runs serialized Debug, Release and supported ASan/UBSan builds and
the unfiltered combined suite. This worker registers runtime fixtures but does
not run builds or measurements. Concurrency coverage is limited to the ingress
mutex and explicit joined lifetime fixtures, not a threaded simulation scheduler.
