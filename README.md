# Crucible

Macro-RTS / swarm simulator powered by the sub0 ecosystem. Direct a nanite swarm
with attractors, repulsors and painted flow fields, consume a cellular Blight,
and fuse density into macro-structures.

**Status: integration groundwork.** A headless ECS workload and a complete stack
round-trip test are implemented. Gameplay and rendering are planned. The design
target is 100,000–150,000 entities at 60 FPS; this is not a measured game result.

## Build

Requires CMake 3.25+, Ninja, Git and a C++23 toolchain (GCC 13+ or current MSVC).

```sh
cmake --preset debug
cmake --build --preset debug --parallel 4
ctest --preset debug
./build/debug/crucible
```

On Windows use a VS developer prompt and build/debug/crucible.exe.

Dependencies use one verified CPM 0.42.1 bootstrap, namespaced targets and full
commit pins in [cmake/DependencyPins.cmake](cmake/DependencyPins.cmake):
Sub0ECS **v2**, Sub0Pub **v2**, Sub0Pipeline main, Sub0Log main. Dependency developer
tools are disabled. Set CPM_SOURCE_CACHE for a reusable dependency source cache.

See [CONTRIBUTING.md](CONTRIBUTING.md) for Debug, Release and sanitizer workflows,
[benchmarking](docs/benchmarking.md) for reproducible evidence capture, and
[architecture](docs/architecture.md) for the design and integration boundaries.
Functional CI runs on Linux and Windows; benchmark CI is manual and advisory.

Source layout: include/crucible and src for simulation, tests for behavior and
stack integration, benchmarks for opt-in timing, cmake for dependencies, scripts
for evidence capture. Dependency licenses remain with their projects; Crucible's
own license has not yet been selected.
