# Initial integration validation

Validated on 2026-09-30 with GCC 14.2.0, CMake 4.4.3 and Ninja 1.13.2,
Linux x86_64, AMD EPYC 9V74 (three logical CPUs visible to the container).

- Fresh Debug, Release, benchmark and sanitizer configurations succeeded.
- Both simulation behavior and stack round-trip tests passed in Debug and Release.
- ASan/UBSan passed with ASAN_OPTIONS=detect_leaks=0. LeakSanitizer cannot operate
  under this sandbox's ptrace supervision; leak checking remains enabled in CI.
- The headless executable processed 150,000 entities for 60 ticks and produced
  the expected checksum of 225,000.
- Five independent Release benchmark processes were captured from clean source
  commit 1a29217482d2956c8d8e1b57684877ccd71c13c6. Raw evidence is in
  [benchmarks/initial](benchmarks/initial).

| Entities | Median of process medians (µs) | Process median range (µs) |
|---|---:|---:|
| 100,000 | 20.169 | 19.849–26.809 |
| 150,000 | 28.933 | 28.5875–39.859 |

These numbers measure single-threaded ECS position integration. Background load
and power settings were uncontrolled; compilation was occurring during capture.
Use these as harness evidence, not a controlled performance comparison or full
game FPS result. No baseline/current regression comparison exists yet.

Integration fixes: selected Pub and ECS v2 APIs and official consumer targets;
disabled dependency development targets; corrected CPM 0.42.1's stale bootstrap
hash by checking the published GitHub release asset digest; instrumented Pipeline
and its headless executor as well as Crucible in sanitizer builds.

The pinned Pipeline source emits a GCC missing-field-initializers warning for
DispatchContext::skipMutex. It compiles and its round trip passes; no source patch
or warning suppression was applied. Threaded handoff, parallel executor use and
full game workloads remain outside this initial validation. Windows is configured
in CI and has not been run in this Linux workspace.
