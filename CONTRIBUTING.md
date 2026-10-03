# Development workflow

For parallel sessions, start with [the workstream map](docs/workstreams/README.md).
Each stream owns local source/test manifests; shared wiring goes through the integrator.

Requirements: CMake 3.25+, Ninja, Git, Python 3.10+ and a C++23 toolchain with
std::expected (GCC 13+ or current MSVC). Use a VS developer prompt on Windows.
Clang requires a C++ standard library implementing std::expected.

From the repository root:

```sh
cmake --preset debug
cmake --build --preset debug --parallel 4
python scripts/run_tests.py --preset debug
cmake --preset release
cmake --build --preset release --parallel 4
python scripts/run_tests.py --preset release
cmake --preset sanitize
cmake --build --preset sanitize --parallel 4
python scripts/run_tests.py --preset sanitize
```

The sanitizer preset requires GCC/Clang. Run it separately from timing work.
Use `scripts/run_tests.py` for local and CI execution. It asks CTest for the selected
commands, repairs lost execute bits on owned ELF/Mach-O test artifacts under `build/`,
reports repairs, then runs the requested suite once. It preserves read/write modes,
ignores source/scripts/symlinks/external tools and leaves Windows modes unchanged.
Pass ordinary CTest filters after `--preset`; `--ctest path` selects a tool outside
PATH. A real test failure propagates without automatic retry. After a test that
failed to launch, use the runner with `--rerun-failed` instead of repeating the suite.
Use clang-format with .clang-format. Avoid global compiler flags and hidden fetches.
Only benchmark tools are opt-in; correctness tests stay enabled.

Dependency integration changes must configure from a fresh build tree and run the
stack round-trip test. Change full SHA pins deliberately, record the source branch,
and verify Pub and ECS v2 APIs. CPM_SOURCE_CACHE can be set in the environment;
CPM_Sub0ECS_SOURCE and equivalent package overrides support local checkouts.
The CPM bootstrap is SHA256-verified and dependencies are not installed system-wide.

Review gates: explain behavior, validate ownership and data access, report tests
actually run, document remaining gaps. For hot-path changes capture baseline/current
Release results on the same machine as described in docs/benchmarking.md.

When execution is interrupted, retain the branch, build directories and evidence.
Record the commit, completed checks, tool paths and next action in
`docs/ACTIVE_WORK_LOG.md`; resume from that checkpoint. Run the test wrapper before
rebuilding unchanged code. If only a tool permission or launch problem blocked a
case, repair that specific owned artifact/tool and rerun the failed selection; do
not repeat successful checks unless code or configuration changed.

Sprint completion includes visually inspected examples wherever viable. Plan the
capture with the package, preserve reproduction/provenance and link it from the
review; see [visual evidence criteria](docs/phases/README.md#visual-examples-at-each-iteration).

Before long validation, publish a coherently reviewed source checkpoint to the sprint
branch and record its remote SHA (draft PR optional). Preserve acceptance outputs
with that checkpoint and label pending/partial results. A local commit does not
protect against loss of the execution service. Use hosted exact-head CI if local
transport fails; do not infer a pass or rerun successful checks. Reconcile local
branches/build artifacts after recovery. Exact authored-file recovery must preserve
blob identity; do not recreate code from an approximate description.
