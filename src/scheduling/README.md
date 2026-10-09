# Scheduling implementation

`row_partitions.cpp` implements the synchronous bounded row graph consumed by
Simulation. Its owned pool uses the pinned `Sub0Pipeline::Priority` target.
Scheduling reads resolved Contracts values and has no Core or Runtime dependency.

See the [workstream brief](../../docs/workstreams/scheduling/README.md) and
[provider contract/receiving](../../docs/workstreams/scheduling/phase14-row-partitions.md).
