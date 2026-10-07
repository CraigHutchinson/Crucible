# Fields implementation

Own explicit source registration in CMakeLists.txt. See [workstream brief](../../docs/workstreams/fields/README.md).

Implemented bounded radial fields and straight FLOW capsule currents, consumed
by Simulation/Steering. Owned FieldEdit values and startup-sized slots preserve
validation, overflow and replay boundaries.
