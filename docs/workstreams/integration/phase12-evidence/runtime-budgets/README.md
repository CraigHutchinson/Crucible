# Phase12 Windows runtime budget capture

Five serial alternating integrated/direct process pairs from
`bench-results/phase12-windows-pairs`, captured 5 October 2026. The workload,
accepted allocation budgets and timing limits are documented in
[phase12-budgets.md](../../../runtime/phase12-budgets.md).

`summary.json` contains process aggregate observations; `pair-*.jsonl` and
`pair-*.stderr` retain each raw invocation. `invocations.json` records ordering,
commands, return codes and process wall times. All ten return codes are zero.
`cpuinfo.json`, `CMakeCache.txt`, `compiler-0.cmake`, `DependencyPins.cmake` and
`metadata.json` identify the host/build/source. The benchmark and capture script
are retained as the exact measurement inputs, with a clean empty `source.diff`.

Only `metadata.json` binary/build paths were changed to workspace-relative paths.
Its exact source SHA and binary SHA256 are unchanged. Every other source capture
file is preserved byte for byte, including invocation and toolchain paths.
`sha256.json` lists the preserved file hashes; it excludes itself and this README.

Accept 3 calls / 96 requested bytes per warmed integrated tick, 6 / 124 on its
first pump, zero measured admission allocation, and zero measured diagnostic
refusal-emission allocation through exhaustion. Retain the existing full Pipeline
pin. Timing remains advisory and establishes neither a speedup nor a game FPS.
