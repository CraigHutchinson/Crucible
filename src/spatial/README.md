# Spatial implementation

Own explicit source registration in CMakeLists.txt. See [workstream brief](../../docs/workstreams/spatial/README.md).

Implemented H2-backed stable-ID binning and complete radius queries, consumed
by Simulation and steering. Crucible owns storage/IDs and exact filtering;
private compatibility fallbacks preserve unsupported numerical domains.
