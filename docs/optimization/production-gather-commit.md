# Production gather/commit optimization

Status: promote A+B; complete scale and mission cohorts pass the median gates. Final delivery follows passing checks. Owner: Integration. Date: 2026-10-12.

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

Actual baseline diagnostics: GCC 13.3, Release O3 C++23, portable x86-64,
CPU affinity 0–3, 100K/400×250, 30 warmup+60 advancing diagnostic ticks, gather route.
Stage means (ms), collected separately from qualification:

| Workers | Boundary | Gather | Index | Propose | Commit | Resources | Rebuild |
|---|---:|---:|---:|---:|---:|---:|---:|
| 1 | 121.299 | 2.664 | 4.144 | 97.180 | 5.583 | 4.149 | 4.567 |
| 4 | 63.023 | 2.496 | 3.664 | 41.170 | 5.409 | 3.756 | 4.065 |

These diagnose an 8.6% serial commit share in the four-worker boundary, not a
speedup. Proposal work remains dominant. VTune is unavailable (`ENOENT`); both
version/help failed receipts are retained. No cache/PMU claims. Existing production
stage clocks provide narrower source attribution. GCC vectorization remarks and
linked control/candidate/combined disassembly were captured separately from timing.
They preserve the distinction between scalar identity lookup/control flow and
ordinary component moves; no SIMD arithmetic or no-alias contract was added.
The combined linked artifact retains introsort fallback calls. These are code-shape
receipts, not hardware-counter measurements or proof of whole-consumer benefit.

Pass A retains sorting and replaces binary-search commit with a startup-reserved
ID→compact-row map, filled alongside existing spatial staging. Private population
IDs remain 1..count; mobile subsets may have holes. Workers do not mutate membership.
Gather validates ID range/uniqueness and all proposal IDs are validated after join
before any world write. No stale entry is consumed: every mobile ID is remapped in
that epoch. Storage cost is sizeof(size_t)×population (1.2 MB at 150K on this host);
no steady allocation is added. Arithmetic/reduction order is unchanged.

Independent oracle tests receive empty/single/odd worlds, non-prefix anchored
holes, a fully anchored epoch, two fuse/shatter cycles and permanent lost identities
at 1/2/4 workers. The oracle calls public all-row steering from a pre-tick snapshot,
bypassing Simulation gather/commit. Worker parity alone would share the changed map.

The qualifying campaign selects integrated four-worker whole pump/capture at
2048, 100K, 150K, 120 warmup+300 evolving ticks, gather route, five alternating pairs.
Small regressions above 5% block blanket promotion; improvements must exceed paired
noise and be reported individually, including any overlap. One-worker and direct
routes receive correctness and diagnostic comparisons; native acceptance remains
separate. Complete reference/structural mission pairs remain regression controls.


## Pass B and combinations

B checks whether IDs are already sorted before the existing gather, spatial
rebuild and snapshot sorts. It retains the original sort on an inversion, so it
requires neither arbitrary ECS-order assumptions nor dense mobile IDs. Spatial
finite/capacity checks, duplicate rejection, clamping and cell preparation remain
in the same order; rejected input preserves committed query state. Query-result
sorting and ordered neighbor accumulation are unchanged. B adds no scratch memory.
The spatial fixture now receives sorted valid input and sorted duplicate rejection
alongside reversed input, checking occupancy and independent brute-force queries.

The four source arms are:

| Arm | Measured local source | Change from control |
|---|---|---|
| Control | `b45b145b4db52039c0ae12dce2265fe182ea6cd9` | Production base plus benchmark receiver only |
| A | `822652fe0a2d5da441040a3a7c78d63675d9a304` | Checked compact-row commit |
| B | `63c11e1769634164ed1bc0c882c1e17bde564738` | Three guarded identity sorts |
| A+B | `d5c902012ca83b410d76b1426012661fd586e45f` | Both mechanisms |

Every arm retains the same benchmark source and dependency pins. Some host builds
reuse clean CPM source checkouts at those exact pins; caches and resolved dependency
revisions must accompany the results. Published Git Data API commits may differ in
metadata from these local commits; tree identity is the source-equivalence check.
Build and collector commands are in [benchmarking](../benchmarking.md#production-headless-pump-and-snapshot).

## Initial A timing evidence

Five alternating control/A process pairs, four workers, 120 warmup plus 300 measured
advancing ticks and the gather route. Values are median of process medians, with
complete observed process-median ranges in milliseconds:

| Population / geometry | Control | A | Median reduction |
|---|---:|---:|---:|
| 100K / 400×250 | 62.785 [60.403–64.048] | 59.868 [59.175–62.420] | 4.65% |
| 150K / 500×300 | 90.824 [87.891–94.684] | 87.330 [84.736–88.038] | 3.85% |

The ranges overlap. This isolated mapping pass is encouraging but does not establish
a robust production gain beyond host variation. All emitted semantic state/trace
checks passed, with zero measured coordinator ordinary/aligned C++ new calls; this
is not universal allocation interception. The old 2048 cohort is incomplete because
two raw process receipts are missing. Its summary cannot close the regression gate.
Fresh complete combination cohorts are received below. The old combined 2048
cohort has three empty raw receipts and is also unqualified. Complete fresh 2048
and mission cohorts supersede the incomplete attempts.

The [Sub0ECS production receipt](https://github.com/CraigHutchinson/Sub0ECS/blob/master/bench/results/reference/work-mode-crucible-production/README.md)
is the companion cross-project evidence record; it preserves raw captures and
source reconstruction patches. The earlier library-version comparison remains a
separate inconclusive experiment and is not relabeled as this consumer change.

## Correctness and delivery evidence

Independent C++ review received both mechanisms. The combined source passed all
50 local Release CTests. B-only passed 49 initially, with the remaining executable
permission issue repaired and that test passing on rerun. All ten hosted checks
passed on combined head `3754092943e5386fb8774a8f41011c4989de8fcc`
([CI run](https://github.com/CraigHutchinson/Crucible/actions/runs/38212724350)).
These checks establish the
reported correctness/build scope, not physical performance acceptance.

Scale and complete-mission median gates now pass. Final delivery follows passing
checks and preserves the archived source/artifact identities. Native G3/G4
frame/pacing/input and G5 useful
parallelism remain governed by their existing Phase14 criteria; headless results
cannot substitute for those observations.


## Received 100K combination cohort

All 20 process receipts are complete and match semantic state, trace and work counts:
five alternating rounds of control, A, B and A+B. GCC 13.3 O3, four workers, 120 warmup
plus 300 measured advancing ticks, gather route, same fixed dependency pins.

| Arm | Median [observed process-median range], ms |
|---|---:|
| Control | 63.603 [62.822–65.792] |
| A: checked mapping | 60.787 [60.098–61.579] |
| B: sort guards | 59.041 [57.924–59.788] |
| A+B | 56.493 [55.279–56.874] |

The combined 100K headless median improves 11.18%; all five paired comparisons improve
and the observed process-median ranges do not overlap. Median process p95 improves
70.331→64.478 ms, while median process p99 rises 74.365→75.687 ms. This is a measured
median improvement with mixed tails on the declared host, not universal latency
improvement or native FPS acceptance. The companion receipt's
`combined-100000-received/` directory holds the complete raw cohort and summary.


## Received 150K control/combined cohort

All ten raw receipts are complete for five alternating control/A+B pairs, with
unchanged four-worker GCC 13.3 O3 settings, 120 warmup and 300 measured advancing
gather-route ticks. Control median pump/capture is 94.563 ms [91.512–98.268] and
combined is 82.051 ms [81.425–84.195], a **13.23% median reduction**. All five pairs
improve and process-median ranges do not overlap. Median process p95 improves
104.561→93.287 ms and p99 improves 126.453→100.356 ms. These are independent process
statistics, not pooled percentile estimates or native frame measurements.

This receives the combined effect at 150K; full four-arm comparisons cover 2048
and 100K. The 100K mixed p99 result remains part of the record. Companion
raw evidence is in `combined-150000-received/`.

## Final small-world and complete-mission receiving

The fresh 2048 four-arm cohort (`final-2048/summary.json` in the companion receipt) retains all 20 raw
process receipts. The complete-mission cohort (`final-missions/summary.json`)
retains all ten control/A+B receipts. State, traces and completed work match.
Values are medians across five independent processes, in milliseconds:

| Workload | Control | A+B | Paired median wins |
|---|---:|---:|---:|
| 2048 production scale pump/capture | 2.2548 | 2.0553 | 5/5 |
| Complete reference mission | 924.0456 | 866.6965 | 5/5 |
| Complete structural mission | 2063.6036 | 1922.8286 | 5/5 |

The fresh 2048 A and B medians are 2.1660 and 2.1478 ms, respectively. The
small-workload median gate passes. Tail results are mixed:
legacy ordinary 64-row median process p99 rises 0.253→0.543 ms, and ordinary
2048-row p99 rises 4.049→4.361 ms. Together with the mixed 100K p99, these prevent
any blanket tail-latency claim.

**Disposition: promote A+B** for the measured production headless median gains,
with unchanged dependency pins and public APIs. Final delivery follows passing
checks. Native G3/G4 frame, pacing and input-latency acceptance and G5 useful
parallelism remain open; this evidence does not establish FPS or handwritten-game
equivalence.

## Integration with concurrent main

Before publication, main advanced to `7c6c461` through PR36/PR37 (structured
diagnostics and a Sub0Log JSON-export update). Those changes are retained. The
measured arms keep their original fixed pins and binaries; final source receiving
covers the merged application separately. The benchmark runs with diagnostics
disabled, and neither Simulation nor Grid changed in those upstream commits.
Do not relabel this evidence as a new comparison of Sub0Log revisions.
