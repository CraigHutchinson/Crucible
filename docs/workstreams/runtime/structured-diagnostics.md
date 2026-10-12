# Structured diagnostics receiving (Sub0Log issue14)

This bounded maintenance increment extends Phase12's optional sink. Top-down:
an agent needs typed refusal/mission values and evidence of lost observations.
Bottom-up: the existing coordinator owns a fixed64KiB image, and Sub0Log supplies
a cold formatter over borrowed decoded records. Reconcile through a quiescent
shutdown call, not a live logger hook. Runtime, Telemetry and integration are
consolidated under root; no gameplay or Phase15 renderer change is dispatched.
Reuse disposition R04: upstream owns JSON encoding/provenance; Crucible owns event
semantics and actual producer counters. No generic telemetry adapter is extracted.

## Usage and ownership

Build the existing desktop target with `CRUCIBLE_ENABLE_DIAGNOSTICS=ON`, then run:

```sh
./build/desktop-release/src/desktop/crucible_desktop --diagnostics-json > events.jsonl 2> health.log
```

The flag enables the bounded sink. Close the app normally to export. Stdout contains
only upstream schema1 message objects. Stderr also contains SDL diagnostics; select
the JSON object with `kind: "crucible_health"`. Existing `--diagnostics` keeps text
output. If both flags occur, JSON wins. Logging remains optional: if compiled out
or allocation fails, startup warns and no health receipt is produced. A missing
receipt is not success. Shutdown export errors are reported on stderr; they do not
change SDL's prior exit disposition, so consumers must inspect the health receipt.

`RuntimeDiagnostics::writeJson(output, health)` allocates on the cold path, retains
no views and flushes both destinations. One idle coordinator must stop emission for
the entire call. The image outlives the Merger and its decoded views. Stream/allocation
exceptions propagate to the caller; partial output may precede failure. The bool
result rejects invalid images, decoder damage and either failed destination. Counted
producer drops still permit a valid committed-prefix export. Diagnostic storage
remains65536 bytes; the two additional numeric arguments reduce retained capacity.
Decoder/JSON allocations are additional, bounded by that fixed input and schema,
not an exact RSS guarantee. No hard filesystem/output timeout is provided in-process.

## Numeric application schema1

Exporter schema1 and application schema1 are separate contracts. Every event starts
with unsigned64 arguments `[app_schema, event_id, run_id, completed_tick, status]`.
Consumers require original TypeCode9 and parse decimal strings as integers; do not
interpret message text, source lines or site IDs as event identity. This is the
first structured application schema: older binary images lacked the first two
arguments. They remain readable as generic logs but are not schema1 app events.
Run IDs are session-local. Record generation/offset identify the physical message.

| Event | Remaining arguments | Meaning |
| --- | --- | --- |
| 1 admission refusal | action, generation, field_kind, slot, x, y, radius, strength, end_x, end_y | AdmissionStatus: full1, closed2, invalid3, sequence_exhausted4 |
| 2 structural refusal | action, generation | StructuralCommandResult: disabled1, occupied2, insufficient_mass3, empty4, stale_generation5, generation_exhausted6 |
| 3 terminal mission | reclaimed, target, deadline | ReclamationMissionOutcome: won1, lost2 |

Actions are field0, fuse1 and shatter2; field kinds are set0, remove1 and
set_flow2. Field payloads on structural actions are unused diagnostic values.
Event1 arguments9–14 are original F32 (JSON numbers or the upstream string tags
`nan`, `inf`, `-inf` for nonfinite refused inputs); all others
are unsigned64. Accepted/applied/active outcomes are intentionally omitted. The
text rendering is unchanged. Unknown app versions/events require explicit caller
policy; never infer completeness from the absence of a record.

Health schema1 uses `kind: "crucible_health"`, `valid` and `output_ok` booleans,
and decimal-string `producer_dropped_records`, `producer_truncated_records`,
`records`, `unreadable_bytes`, `unwritten_bytes`, `undecodable_records`, and
`skipped_records`. Recovery counts are null for invalid input. Unwritten bytes are
unused capacity, not damage. Producer loss counts come from the owning live logger,
unlike offline CLI health where they are unknown. A complete receive requires a
valid/output-ok receipt, no decoder damage, and caller acceptance of loss counters.

## Acceptance and review

`runtime_diagnostics_json` runs a real InspectorSession through queue-full refusal,
shatter-empty refusal and deadline loss, then independently parses every stdout and
stderr line with Python's standard JSON parser. A second run exhausts the sink and
verifies positive losses with a recoverable prefix. Existing binary lifetime tests,
full-state diagnostics parity and stack round-trip remain gates. Output failures on
both streams are covered by the C++ receiver. `desktop_diagnostics_json` receives the actual desktop destructor using the
existing SDL dummy/software fixture; hosted desktop builds also compile CLI wiring;
headless receiving does not claim a physical interactive session.

Manual review after automated gates: launch the command above, let a mission finish,
close normally, parse every events.jsonl line and find event3 by numeric argument1.
Check status/target/deadline and the final health losses. Repeat with ordinary
`--diagnostics` to inspect text. No useful screenshot applies: behavior is a stream
contract, and raw parsed output is the actual-state evidence.

Initial source checkpoint15ab9b1 (tree6dba4ef) is published in Crucible PR36.
GCC13.3 Release passed53/53 tests, including stack round-trip, full-state parity,
source-copy lifetime and JSON parsing. Development preflight passed. Debug also passed53/53 locally. All10 hosted jobs passed at15ab9b1, including
Windows/Linux/macOS Debug/Release, normal sanitizer, race, headless and GPU.
The added desktop shutdown receiver and final full pin require fresh exact-head CI. Local normal LSan canary
fails because /proc/task is inaccessible under ptrace; no suppression was applied.
A fresh Debug configure could not fetch Sub0Log through the local network proxy
(HTTP403); local source override uses the exact published library tree. Hosted
builds receive the full pin without that override.

Observed Release fixture health: normal3 records,0 dropped,0 truncated,
0 unreadable/undecodable,64344 unwritten bytes; exhaustion463 recovered records,
9539 dropped,0 truncated,0 unreadable/undecodable,5336 unwritten bytes. Serialized
stdout was2882/530984 bytes in that run (timestamps/identifiers affect size).
These are finite-fixture observations, not RSS or throughput benchmarks. The retained
prefix cannot attest to a terminal event after exhaustion. The selected full pin is f5daae93010d255a112abc602e1ceb1be0d9fb7c, the reviewed
Sub0Log PR18 head. Downstream merge requires that revision to be merged upstream
and its downstream exact-head CI to pass. It need not pin the incidental merge
commit: the reviewed source revision remains reachable from upstream main. Prior Phase13–15 native, participant,
audio and renderer gates remain open and are outside this maintenance increment.
