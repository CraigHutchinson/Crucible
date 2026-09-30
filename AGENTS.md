# Crucible repository guidance

The first milestone is validated integration of the sub0 stack. Resolve dependency,
toolchain, API, lifetime and build failures before adding gameplay systems.

Use clean C++23, RAII, explicit ownership and value semantics. Keep rendering and
platform APIs outside simulation code. Declare system data access and DAG dependencies;
do not assume the ECS or pub/sub broker is safe for arbitrary concurrent mutation.
Drain bounded input commands at tick boundaries. Structural changes must occur after
workers finish. Define overflow, teardown and borrowed-state lifetime behavior.

Use target-scoped CMake requirements and CPMAddPackage in cmake/Dependencies.cmake.
Pin full dependency commits in cmake/DependencyPins.cmake; Sub0Pub and Sub0ECS must
come from v2. Disable dependency tests, examples, tools and benchmarks in consumers.
Use CMakePresets.json for local work and CI. Do not disable checksum or TLS verification.

Follow CONTRIBUTING.md and docs/benchmarking.md. Keep performance measurements advisory.
Add meaningful behavior and lifetime tests for integration changes; run Debug, Release,
and ASan/UBSan on supported platforms. Document what actually passed and limitations.
Do not claim 100K+ entities at 60 FPS from an isolated integration loop.
