# Development workflow

Requirements: CMake 3.25+, Ninja, Git, Python 3.10+ and a C++23 toolchain with
std::expected (GCC 13+ or current MSVC). Use a VS developer prompt on Windows.
Clang requires a C++ standard library implementing std::expected.

From the repository root:

```sh
cmake --preset debug
cmake --build --preset debug --parallel 4
ctest --preset debug
cmake --preset release
cmake --build --preset release --parallel 4
ctest --preset release
cmake --preset sanitize
cmake --build --preset sanitize --parallel 4
ctest --preset sanitize
```

The sanitizer preset requires GCC/Clang. Run it separately from timing work.
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
