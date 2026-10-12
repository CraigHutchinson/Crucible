# JSON export receiving, 2026-10-12

Consumer baseline: main 70b557b. Root/integrator owns the dependency pin and this
record; serial builds/tests, no workers. Upstream candidate is Sub0Log PR #19,
head 3c040bd, direct decimal append in the existing formatter. The caller remains
RuntimeDiagnostics::writeJson, invoked at quiescent runtime/desktop shutdown.

## Measured upstream mechanism

Sub0Log's existing format benchmark now measures JSON on its real decoded
four-argument fixture. Five alternating Release process pairs show 20–26% less
formatter time, median 1074 -> 847 ns. Raw epochs, immutable binary/header hashes,
compile/link commands, compiler remarks and linked-call excerpts are preserved
in upstream docs/optimization/json-append*. This is a cache-hot isolated formatter
result on a shared virtual host, not a game/shutdown throughput or RSS claim.
VTune is absent; there are no sampled hotspots or hardware-counter claims.

The upstream full Release suite passed 173/173; the old/new CLI produced identical
18064-byte JSON for the same eight-record edge fixture. Producer logging,
quiescence, owned output, schema, integer precision, invalid-byte preservation
and stream failure behavior remain unchanged.

## Consumer gates

Candidate local Release: 52/53 passed initially; runtime_tick_observations did not
start because a restored ELF lacked execute permission. The prescribed runner
repaired it and the isolated retry passed 1/1; combined coverage 53/53. This was an
artifact-mode failure, not a test assertion failure. Actual runtime JSON receiving
(normal and exhausted capacity) passed. Debug receiving is pending. Initial
development preflight lacked CMake/Ninja on PATH; using the existing tool bundle
passed all required checks. No prerequisite was suppressed.

Pinned upstream merge: 29ef3aa64e768e9af00127f6490d4fd664dd2b11.
All 18 upstream exact-head jobs passed in run 38250581890 before merge. A fresh
consumer dependency build including desktop JSON and full exact-head consumer CI
remain promotion gates; completion receipts will be posted on the consumer PR.
Normal local LeakSanitizer was unavailable under this container's ptrace limits
in the preceding feature campaign; hosted normal ASan/UBSan remains required.

## Next candidate: simdjson

User requested assessment of simdjson generation. Its public string_builder
supports JSON construction and SIMD-assisted escaping, with explicit UTF8
validation and shortest-round-trip float defaults:
https://simdjson.org/api/4.6.4/md_doc_2builder.html .

Track the controlled follow-up in Sub0Log issue #20:
https://github.com/CraigHutchinson/Sub0Log/issues/20 . Compare the merged formatter
against a pinned builder on actual 15/7/8-argument event shapes, long/escape-heavy
strings, invalid-byte fallback and batches; include owned output copy and compare
per-record creation with caller-owned reuse. Preserve exact schema and review
float/escaping spelling compatibility, output budgets, error conversion, binary
and dependency costs, C++23 and current platform matrix. No simdjson dependency
or unmeasured generation speedup is claimed in this promotion.
