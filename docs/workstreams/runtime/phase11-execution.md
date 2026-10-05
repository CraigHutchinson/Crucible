# Phase 11: sequential committed-boundary graph

`BoundaryPipeline` builds three dependent Sub0Pipeline jobs once: the existing
`HeadlessSession::TryStep`, an owned scenario capture, then mission/summary
completion and publication. The production inspector and CLI consume this graph;
the direct inspector remains the receiving comparator and calls the same rules.
The graph owns its stable context and two startup-sized snapshots. It borrows the
session and simulation exclusively, runs untimed on the coordinator, and dispatches
no threads. Declared dependency edges consume the ownership order; the jobs have
no independent ECS access that could safely run concurrently.

A paused, closed or trace-full boundary returns its original concrete result and
cancels downstream graph jobs. Capture and completion are not called. Trace-full
does not drain ingress. Successful completion reads the staged frame; only after
it succeeds is the published pointer exchanged. Terminal completion closes ingress
inside the graph, before catch-up can attempt another boundary. Exceptions close
ingress, preserve the previous published frame and propagate their original cause.
Unexpected graph-result failure latches application failure. A committed simulation
tick cannot be rolled back when later observation fails; retained frame tick and
completed simulation tick may therefore differ in a failed run. No retry mutates
that run again. Inspector mission output is staged and committed only after the
matching frame has successfully published.

The published snapshot is owned. A caller's borrow expires on the next successful
boundary or destruction; pointer alternation is not a general persistent history
API. Consumers needing longer retention copy into their own snapshot. Startup
rejects an undersized initial capture before the graph becomes usable. Teardown
occurs on the quiescent coordinator, destroying graph jobs before their context.
Untimed inline jobs and their synchronous completion callbacks return before
`TryStep` returns or unwinds; no orphan work retains the ECS, queue or frames.

## Exact-source audit

Pipeline pin `f6f54c623908649e8daac3613545062cf08b3822` and discovery main
`1c50051fe4a5d4d06766d53624542b0025573be1` were inspected from separate source
checkouts. Their implementation and public execution header are unchanged;
the discovery diff adds examples and README entries. `run_inline` uses a stack
inline executor, `runImpl` resets cancellation/dependency state for repeat runs,
and failed required jobs skip successors. The run guard clears its running flag
on exceptions. There are no timeouts, external cancellation, observers or injected
executors in this receiving path; broader dispatch/join contracts remain upstream
fixtures, not a claim of arbitrary executor safety from our single-thread tests.

Snapshot buffers and topology allocate at startup, but graph execution is **not
allocation-free**: source inspection shows per-node fresh `std::stop_source` and
first-use validation/skip scratch growth. No allocation count or steady-state
bounded-execution promise is established here. A future bounded-runtime gate must
measure the actual three-job workload and receive reusable cancellation/traversal
storage upstream rather than copy an executor into Crucible.

ECS v2 pin `8391f81fd74a016564b4711b074eb286d3c5e14b` and discovery v2
`60285914ee8925f0ce20ac5426511604fb0c6529` were inspected. The world source diff
removes MSVC flatten attribution; identity/storage behavior used here is unchanged.
`each` directly borrows partition columns during its callback; create/migration/
destroy may invalidate component addresses. No query borrow crosses our tick or
capture. Population is fixed and private after construction; Crucible checks its
24-bit identity bound and never destroys/recycles these entities. Structural
activity is application state, not ECS entity destruction.

The upstream allocator masks indices to 24 bits, wraps its 8-bit version after
256 releases and checks liveness by slot/version without world provenance. World
`reserve` reserves records only, not every partition, allocator or side pool;
`create` is not a transactional rollback contract across allocation failures.
Growth, recycling, cross-world identity, complete reserve and fault-injected
rollback remain explicit upstream prerequisites before introducing those features.
They do not justify a replacement ECS or broader guarantees in this fixed-world
increment. No dependency pin promotion is needed for the consumed contract.

## Receiving and independent review

`tests/runtime/BoundaryPipeline.cpp` checks 63 repeated live structural boundaries
against the direct coordinator, including insufficient-mass refusal, field-driven
gather, fuse, stale-generation refusal, shatter and exact full-state equality.
It also checks suppressed capture/completion for pause/close/trace exhaustion,
successful reuse after a cancelled paused epoch,
same-boundary terminal closure, retained publication after an original callback
exception, permanent fail-stop and rejected startup capacity.

The execution worker did not run shared builds. The integrator reports Release's
35 registered tests passed; Debug/sanitizer/CI and final exact-head evidence belong
in the sprint review. This graph changes execution ownership, not visible rules;
phase-level actual-state exports provide the visual receiving evidence.

Independent source review covered `IntentDelivery`, inspector/clock wiring, CLI
scenario receiving and `BackboneParity`. No blocking correctness or lifetime issue
was found in the supported coordinator-only path. Pub copies admission before
return, domains cannot cross-route, most-derived sinks disconnect before teardown,
inspector graph and delivery die before borrowed session/world, and mission/frame
publication precedes clock terminal closure. The parity suite includes mission win,
structural win, refusal, full queue, trace-full, pause/resume, restart and terminal
catch-up. Concurrency, injected dispatch failure and physical-device behavior are
separate receiving gates; these tests do not establish them.
