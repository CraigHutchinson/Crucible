# Validation workstream — W10

## Hierarchy and acceleration boundary

Own independent consumer oracles, shared comparison protocol and raw full-cycle evidence across motion, views and navigation. Distinguish headless export from actual GPU rendering, capture build/update/output/order costs, and serialize controlled runs; gameplay tolerances remain consumer-owned.

See the [application responsibility map](../hierarchy-boundaries.md) for defining owners,
read/write sets, lifetimes, bounded progress and cross-project handoffs.

Own benchmarks/, scripts/capture_benchmarks.py and new capture tools, plus evidence
in docs/benchmarks/ and this folder. Central validation.md and CI edits go through
integrator. benchmarks/CMakeLists.txt owns benchmark source registration.

Current crucible_bench measures only sequential ECS integration; preserve its label
and evidence. Full-tick work depends on W7/W8b; FPS also depends on W9. Prepare
fixtures earlier without claiming absent systems complete.

First task: define reproducible tiny/100K/150K complete scenarios. Record grid density,
fields, cellular rules, churn, capacities, workers and telemetry. Correctness precedes
timing. Reserve an uncontended host and follow alternating comparisons in
[benchmarking](../../benchmarking.md). Report tick p95/p99, command age/drops,
allocations/memory and reference results; frame timings include upload/presentation/
catch-up. Record missed targets and unsupported coverage. Follow
[session guide](../README.md) for claims/handoffs.
