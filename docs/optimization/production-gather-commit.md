# Production gather/commit optimization

Status: implementing and receiving pass A. Owner: Integration. Date: 2026-10-11.

## Frozen question and control

Control is Crucible `118ce62a75c8e6f1c325cc915cbc539c558a4c11` with committed
dependency pins. Candidate must retain those pins so consumer changes are
attributable. Actual caller is InspectorSession → HeadlessSession → Simulation;
timing includes complete pump/capture/publication. Existing reference and structural
missions remain regression workloads. Larger headless cases use the production
100K/400x250 and 150K/500x300 geometry, with an explicit small control.

Hypothesis: repeated gather sorting and per-mobile-row binary-search commit are
material coordinator costs. A checked per-tick identity mapping can remove those
costs without changing ordered steering, mobile membership or assignments. Reject
if whole-boundary savings disappear into repeated-control noise or correctness
changes. No native rendering/FPS claim follows from a headless measurement.

Before timing, require bitwise state/reference behavior, structural lifecycle,
joined worker/FP behavior and complete Debug/Release plus supported sanitizer gates.
At least five alternating independent process pairs, same toolchain/flags/pins and
identical benchmark source in each arm. Keep all slower/failed/time-out raw results;
report per-process medians, p95/p99 and ranges with exact source/binary hashes.
No timing exclusions except failed receipts, which invalidate qualification.
Stage-observed and instrumented profiles are separate diagnostic arms. Host power,
thermal state and external contention are uncontrolled and reported.

## Diagnosis and passes

Actual baseline diagnostics: GCC13.3, Release O3 C++23, portable x86-64,
CPU affinity0–3,100K/400x250,30 warmup+60 advancing diagnostic ticks, gather route.
Stage means (ms), collected separately from qualification:

| Workers | Boundary | Gather | Index | Propose | Commit | Resources | Rebuild |
|---|---:|---:|---:|---:|---:|---:|---:|
| 1 | 121.299 | 2.664 | 4.144 | 97.180 | 5.583 | 4.149 | 4.567 |
| 4 | 63.023 | 2.496 | 3.664 | 41.170 | 5.409 | 3.756 | 4.065 |

These diagnose an8.6% serial commit share in the four-worker boundary, not a
speedup. Proposal work remains dominant. VTune is unavailable (`ENOENT`); both
version/help failed receipts are retained. No cache/PMU claims. Existing production
stage clocks provide narrower source attribution; compiler/disassembly checks follow.

Pass A retains sorting and replaces binary-search commit with a startup-reserved
ID→compact-row map, filled alongside existing spatial staging. Private population
IDs remain1..count; mobile subsets may have holes. Workers do not mutate membership.
Gather validates ID range/uniqueness and all proposal IDs are validated after join
before any world write. No stale entry is consumed: every mobile ID is remapped in
that epoch. Storage cost is sizeof(size_t)×population (1.2MB at150K on this host);
no steady allocation is added. Arithmetic/reduction order is unchanged.

Independent oracle tests receive empty/single/odd worlds, non-prefix anchored
holes, a fully anchored epoch, two fuse/shatter cycles and permanent lost identities
at1/2/4 workers. The oracle calls public all-row steering from a pre-tick snapshot,
bypassing Simulation gather/commit. Worker parity alone would share the changed map.

The qualifying campaign selects integrated four-worker whole pump/capture at
2048,100K,150K,120 warmup+300 evolving ticks, gather route, five alternating pairs.
Small regressions above5% block blanket promotion; improvements must exceed paired
noise and be reported individually, including any overlap. One-worker and direct
routes receive correctness and diagnostic comparisons; native acceptance remains
separate. Complete reference/structural mission pairs remain regression controls.

