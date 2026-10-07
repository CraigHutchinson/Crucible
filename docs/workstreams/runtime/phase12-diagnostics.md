# Phase 12 bounded runtime diagnostics

`RuntimeDiagnostics` is a session-owned optional sink for command admission
refusals, completed structural refusals and one terminal mission summary per run.
The session controls once-per-run emission after publishing the completed frame;
the sink ignores accepted/applied/active dispositions. Enabling the sink changes
no gameplay, command admission, trace or replay outcome.

## Pinned API evidence

The inspected cached dependency at `build/sanitize/_deps/sub0log-src` has exact
HEAD `85498bb7b735f8c7e43653955be582ee903c653f`, matching the Crucible pin.
`Logger::createInMemory`, `Logger::ScopedBind`, `Logger::stats`,
`SegmentReader::open`, `Decoder::decodeAll` and `Decoder::format` are actual APIs
at that revision. No changes to the sibling Sub0Log repository were made.

The pinned `Segment::createInMemory` requires aligned caller-owned storage, zeros
it once, uses a compact 192-byte header, and requires room for at least one
chunk. Its `segmentBytes_` option is ignored for memory segments. Crucible owns
an `alignas(64)` fixed 65536-byte array with 4096-byte chunks: fifteen chunks,
with a bounded unused tail. Logger destruction precedes storage destruction.
Startup allocates the opaque owner once; refusal/summary methods allocate no
storage, format no text and open no files. The pinned emit source uses fixed
encoded numeric payloads and its existing TLS writer cache. No hard timing
bound is claimed; library/platform TLS and atomics qualifications still apply.

Each emission binds the final-address logger only within its stack scope and
restores the previous binding. This binding is process-global in the pinned
library; coordinator-only use is required, with no overlapping binding from
other threads. Interleaved sequential sinks are supported. Exhaustion drops
records and increments Sub0Log's producer counter; the segment never rotates,
blocks for space or grows. Site definitions consume space too, so no fixed
number of gameplay events is promised.

## Schema and shutdown consumer

All messages use subsystem 1 and literal `schema=1 event=N` site metadata.
Values are encoded numeric copies, not views into caller storage. Enum ordinals
are tied to the versioned Crucible contracts; changing enum order requires a
schema review.

| Event | Fields in argument order |
|---|---|
| 1 admission refusal | run, completed tick, admission status, action, requested generation, field kind, slot, center x/y, radius, strength, endpoint x/y |
| 2 structural refusal | run, completed tick, structural result, action, requested generation |
| 3 terminal summary | run, completed tick, outcome, reclaimed, target, deadline |

`GetImage` borrows storage until sink destruction. Stop emissions before copying
or decoding; a reader and its decoder borrow that same image. A copied image
can outlive the sink when decoded by a fresh reader/decoder. `WriteDecoded`
uses the pinned readers to export the committed prefix plus validity, producer
drop/truncation and decoder damage counters. It is an allocating shutdown path.
Output/allocation exceptions belong to the export caller; diagnostic failures
must not affect production outcomes. In-memory data disappears on a hard kill.

## Receiving fixtures

The standalone runtime fixture checks typed decoder fields after caller mutation,
sequential interleaved bindings, previous-binding restoration, ignored successful
dispositions, bounded exhaustion/drop counts, readable committed prefixes,
export health/output failure, source-copy survival after teardown, and fresh
logger-generation/site behavior after reconstruction. Integrator receiving owns
build/test execution and the integration test for exactly one summary per run.
No performance or hard-kill persistence claim accompanies these fixtures.
