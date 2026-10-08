# Scheduling interface

`row_partitions.hpp` defines the startup-owned row callable, disjoint range value
and synchronous typed-result provider for Simulation. Runtime alone resolves
startup execution policy. No asynchronous output or executor borrow is exposed.

See the [provider contract/receiving](../../../docs/workstreams/scheduling/phase14-row-partitions.md).
