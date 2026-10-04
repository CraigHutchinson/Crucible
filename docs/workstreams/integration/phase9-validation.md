# Phase 9 receiving: concept coherence and actual relay density

Status: in progress, 2026-10-05. Dispatch baseline PR21 merge
`a95c5111991f441a451df144fdf443d2379a0939`. Fresh source checkout matches main;
Git LFS fetched the two concept boards and Phase8 flow captures. No prior session
workspace or artifact was altered.

## Source and review

Structural and arena workers supplied independent conserved traces and a production
consumer density experiment. Arena worker independently reviewed structural/spike
identity, command, query and replay boundaries. Architect reviewed source and central
docs; identified startup49 versus threshold32 and replaced speculative exchange
implementation with a gated future consumer. No production gameplay code/pin changed.

`cpp-write`/`cpp-review` were referenced by repository guidance but are unavailable
in the current catalog and searched skill/workspace directories. Existing C++23
patterns, independent source review and actual supported acceptance are retained.

## Frozen experiment

Linux x86_64, GCC13.3.0, installed CMake/Ninja; production default ScenarioOptions,
64×32 unit cells, 2,048 samples, four fields. One command before first boundary:
radial center(48.5,16.5)/radius8/strength4 or FLOW(24.5,16.5)→(48.5,16.5)/radius8/strength4;
passive has none. No objective latch, no modeled structural transition.

Relay query is inclusive radius4 at (48.5,16.5); threshold32 stays fixed.
Every tick checks owned-snapshot brute force against production spatial queries and
conservation. Checkpoints0/60/120/240/480/900 retain all six ledger fields, count,
delta and consecutive eligible durations. Each strategy independently replays its
actual field trace through900 and compares complete state. A tiny hand-count fixture
receives exact radius-boundary inclusion before the runs.

## Local receiving

Pending combined results. Raw JSONL and a production-derived SVG will be linked here
with source hashes/reproduction. Debug/Release and normal ASan/UBSan are required;
unsupported local capability failures remain explicit and hosted CI must receive
normal leak checking. No disabled sanitizer options or inferred physical/human results.

## Hosted receiving and publication

Pending exact reviewed-head nine-job CI, PR and merge/main reconciliation.
Recovery PR21 is a publication receipt, not a new test receipt. Phase9 hosted suites
include retained Phase8 behavior and the opt-in spike on desktop Debug/Release,
headless and normal ASan/UBSan. Software Vulkan receiving remains the existing scope.
