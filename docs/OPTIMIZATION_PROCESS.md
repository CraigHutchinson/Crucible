# Optimization process: measure, explain, refine, receive

Standing workflow, 2026-10-09. Applies to Crucible performance work and to
product-neutral improvements made in its Sub0 dependencies. Start an
[iteration record](optimization/iteration-template.md) before changing a hot path.
Use the [benchmark receivers](benchmarking.md) rather than a new timing loop.

The user authorizes iteration and optimization across **all Sub0 libraries**.
Choose the repository from the measured bottleneck and named consumer. Check that
repository's AGENTS.md, active work log, toolchain, tests and optimization process;
this permission does not replace its numerical, compatibility or ownership gates.
Claim exact paths and CPU/GPU time. Parallel source work is encouraged; builds,
profiles and qualifying measurements share one reserved hardware owner. Preserve
unrelated edits, processes, branches and artifact-bearing worktrees.

## 1. Freeze the question and control

Name the actual receiving caller and the observable improvement: whole tick,
complete frame, command latency, bytes moved, allocation or startup cost. Describe
the hypothesis, its falsifier, workload, capacities and required correctness before
implementing it. A faster isolated helper is not automatically a faster game.

Record full source/tree SHAs, dependency pins, executable hash, compiler executable
and version, actual compile/link commands, optimization/ISA/FP flags, symbols,
OS/device/driver, worker/partition axes and diagnostic settings. Build baseline and
candidate once using the same toolchain/settings. Prefer same-source one-worker
versus multi-worker for scheduling comparisons; a PR29 control answers a different
question. Rebuild embedded source provenance after any source commit, even a merge.

Use representative scales and a small regression case. Crucible's current core
is 100K/400x250 and 150K/500x300 evolving worlds plus the 2K mission. Keep the dense
64x32 stress case separate. Freeze FLOW/gather/repel, camera and completed-state
quality criteria. Never substitute terminal/static frames for evolving workload.

## 2. Diagnose from the system down to instructions

| Layer | Question | Required evidence before selecting a change |
|---|---|---|
| O0 system | Compute, bandwidth, I/O, pacing or another wait? | Whole consumer timeline; CPU/GPU/IO scopes, load, power/thermal condition and pacing/debt. |
| O1 subsystem | Which component dominates? | Inclusive/exclusive costs and share of the actual boundary; worker critical path and joins. |
| O2 data flow | Are useful bytes moved once in the right layout? | Reads/writes/copies, working set, cache/NUMA behavior, allocation, alignment and ownership. |
| O3 call/algorithm | Is repeated work avoidable without changing semantics? | Invocation counts, redundant sorts/rebuilds/searches, validation frequency and complete-query behavior. |
| O4 code generation | Does the compiler produce the intended loop? | Vectorization successes/misses and reasons, linked disassembly, tail/fallback paths and ISA dispatch. |

Do not descend past an unexplained higher-level bottleneck. Re-profile after a
large improvement (approximately 20% or a changed dominant component), after a
dependency/backend change, and before using an old profile to reject another lever.
Record a new map rather than applying percentages to an obsolete baseline.

## 3. VTune collection is an evidence gate

Receive the installed version and collection help first. On Windows the locally
received executable is `C:/Program Files (x86)/Intel/oneAPI/vtune/2026.4/bin64/vtune.exe`;
2026-10-09 metadata probes returned version 2026.4.0/build632893 and Hotspots help
with exit0. Logs are retained locally in `build/optimization-workflow-20261009/`.
That receipt proves CLI metadata availability, not hardware counters or a profile.

Use an optimized build with usable symbols and the same arithmetic/ISA policy as
the control. Save compile commands, symbol paths and hashes; check source/function
attribution in the resulting report. Do not profile Debug and describe its checked
STL/orchestration overhead as production cost. If separate profiling flags alter
the binary, preserve that identity and use an uninstrumented matched build for
qualifying timing. Do not treat stripped or unresolved stacks as source evidence.

The following commands illustrate collection/reporting with an already-built
named application. Replace placeholders with an exact received executable and
its real bounded workload arguments; they are not a runnable Crucible preset.

```text
vtune -version
vtune -help collect hotspots
vtune -collect hotspots -knob sampling-mode=sw -duration=30 -result-dir <new-result-dir> -- <application> <workload-arguments>
vtune -report summary -result-dir <result-dir>
vtune -report hotspots -result-dir <result-dir> -format csv -report-output <new-report.csv>
```

User-mode Hotspots is the first collection where supported. Hardware sampling is
a separate capability probe; request `sampling-mode=hw` only after checking the
installed help, processor/event support and access. Threading addresses worker
waits/synchronization; Memory Access or Microarchitecture Exploration is selected
only for a measured memory/pipeline question and received capability. Check each
analysis's installed knobs rather than copying event names across CPU generations.
Intel documents these collection modes and their different scopes in
[the collection reference](https://www.intel.com/content/www/us/en/docs/vtune-profiler/user-guide/2025-4/collect.html).

Retain command, result directory, stdout/stderr, exit code, sample coverage,
finalization status and exported reports. Set a collection duration **and an
external collection/finalization deadline**. Duration alone does not bound a
stalled finalizer. An owned supervisor records the exact launched process tree;
on timeout request a supported stop for that result, then terminate only those
owned processes if needed. Never kill collectors by name or delete failed results.
Do not launch a second collector while an earlier owned one is unresolved.

A valid profile requires usable nonempty samples for the intended running region
and resolved attribution adequate for the claim. A version banner, empty CSV or
partially created directory is not a pass. If blocked, mark **no profile**, preserve
the failure and use production stage timings, compiler remarks and disassembly for
narrower attribution. Do not invent cache/counter conclusions. The earlier stalled
Phase14 Pipeline collection remains a no-profile receipt; retry only with a changed
capability/workload diagnosis. Profiling overhead never qualifies a speedup.

## 4. Vectorization and generated-code checks

Capture both successful and missed transformations for the actual hot translation
units, with source lines and the compiler version. Use a separate owned diagnostic
build, preserving production optimization/FP/ISA flags. Append reporting options;
do not replace the cached build flags or globally enable fast math to silence misses.

| Compiler | Diagnostic options to verify against the installed version |
|---|---|
| MSVC | `/Qvec-report:2` reports vectorized and missed loops/reason codes; retain build stderr/stdout. `/FAs` provides an assembly/source listing; keep object-specific listing paths. |
| Clang | `-Rpass=loop-vectorize -Rpass-missed=loop-vectorize -Rpass-analysis=loop-vectorize`; inspect SLP separately with the appropriate pass expression. `-fsave-optimization-record` retains structured remarks. |
| GCC | `-fopt-info-vec-optimized -fopt-info-vec-missed`; retain stderr or one per-translation-unit report file. Multiple report filenames can conflict or overwrite one another. |

These reporting options are described by
[Microsoft](https://learn.microsoft.com/en-us/cpp/build/reference/qvec-report-auto-vectorizer-reporting-level?view=msvc-170),
[Clang](https://clang.llvm.org/docs/UsersManual.html#options-to-emit-optimization-reports)
and [GCC](https://gcc.gnu.org/onlinedocs/gcc/Developer-Options.html).
MSVC's assembly listing is documented in
[the listing reference](https://learn.microsoft.com/en-us/cpp/build/reference/fa-fa-listing-file?view=msvc-170).

For each important miss classify dependence/alias uncertainty, noncontiguous
access, alignment, calls, control flow, reduction ordering or cost-model choice.
Address the actual reason; a SIMD pragma is not proof that a profitable loop exists.
Inspect the final linked hot function, not only a source listing that predates LTO.
Record actual SIMD width, scalar epilogue, bounds/runtime alias checks, gathers,
spills, calls, divisions, branches and code size. ISA availability and frequency
effects need measured evidence; wider vectors are not an automatic promotion.
PGO/LTO are separate controlled arms with training-workload provenance.

Crucible requires ordered neighbor accumulation and bitwise 1/2/N state/replay.
Vectorizing independent rows may preserve that contract; reassociating one row's
neighbor reduction may not. Receive empty/single/odd/tail counts, sparse IDs,
coincident samples, full-input rejection and in-place compatibility through
independent references. Compiler remarks alone do not prove correctness or benefit.

## 5. Aliasing, lifetime, alignment and numerical proof

Before adding a no-alias promise, enumerate every caller and write down input,
output and scratch byte ranges, mutation permissions, borrow expiry and overlap
policy. `const`, distinct variable names and disjoint row numbers do not prove
disjoint underlying storage. Crucible's legacy in-place steering remains supported;
do not add a blanket restriction to that public path. A private disjoint kernel
must be reached only after a received validation/copy/dispatch boundary.

Compiler-specific restricted pointers are contracts, not hints that repair an
overlapping call. Keep extensions private and verify their exact compiler semantics:
[MSVC](https://learn.microsoft.com/en-us/cpp/cpp/extension-restrict?view=msvc-170),
[G++](https://gcc.gnu.org/onlinedocs/gcc/Restricted-Pointers.html).
For a boundary that rejects overlap, test exact alias, partial overlap, adjacent
nonoverlap, empty spans, overflow-safe range calculations and rejection without
partial commit. If overlap is supported, test its correct output instead. Never
exercise a prohibited overlap inside a restricted implementation as a supposed test.

Review C++ object lifetime/type access separately from pointer overlap. Prefer
legal value/byte-copy or `bit_cast` operations where applicable; a reinterpret cast
is not lifetime or aliasing proof. Do not disable strict aliasing globally to hide
an unreviewed type-punning defect. Sanitizers passing does not prove an alias contract.

Verify actual allocated addresses, element stride and partition offsets before
assuming alignment. `alignas` on a wrapper does not establish every inner slice's
alignment. Measure AoS/SoA conversion costs, cache-line crossings, false sharing at
partition boundaries, padding footprint, cache misses and bytes per useful row.
Pre-size and reuse hot scratch. Count first/steady/failure/recovery allocations;
qualify ordinary/aligned/C-runtime/OS/GPU interception rather than claiming universal
zero from one counter. Separate checked-Debug orchestration from the strict kernel.

Freeze rounding, denormal controls, FMA/contraction, signed-zero/nonfinite policy
and operation order. No silent `/fp:fast`, `-ffast-math`, approximate distance or
capped neighbors. Any semantic change needs an explicit rules decision and a new
independent oracle before optimization claims. Worker FP installation/restoration,
throw/rejection/join and unsupported fallbacks have distinct receiving gates.
Report limitations honestly; restore source review is not direct native observation.

## 6. Iteration and qualifying measurement

Make one attributable change, then receive correctness and bounds before timing.
Run affected checks while iterating; run complete combined suites and independent
`cpp-review` before promotion. A filtered pass does not cover omitted consumers.
Correctness, ownership/lifetime and resource conservation are hard gates at every pass.

Take **at least three meaningful refinement passes** before parking a mechanism for
negative performance. Each pass names a changed implementation and its evidence;
repeating the same binary three times is not three passes. A break-even or improving
pass justifies further work. Do not preserve unsafe behavior merely to reach three.
Retain safe measured alternatives behind an existing consumed selection where
practical; otherwise preserve exact source in a named branch/commit and document
the restart condition. Do not add unused public switches or ship broken dead code.
Failed attempts and findings are banked, never erased to manufacture a clean result.

Measure combinations: control, A, B and A+B with identical workload/settings,
including useful parked variants. Keep prerequisite ordering and explain omitted
arms. An isolated result before another optimization does not describe that pair.
Sub-noise results are inconclusive, retained and eligible for combination testing.
Estimate noise from repeated local controls; do not import Sub0Llm's threshold.

For promotion use at least five independent alternating A/B process pairs, with
cooling and declared load/power/thermal conditions. Keep raw slower arms, failures
and timeouts. Distinguish cache-hot/cold, profiled/unprofiled and diagnostic/ordinary
work. Preserve source/binary/settings for every arm; do not rebuild mid-campaign.
Predeclare scope, sample minima, exclusions and statistics; never select the
fastest run, discard an outlier or compare unrelated stage percentiles.

For Phase14 use `scripts/capture_phase14_frames.py` and its classifier. The
[receiver guide](workstreams/presentation/phase14-frame.md) documents exact CLI
arguments, same-source worker and PR29 control cohorts, current-only stage probes,
timeouts, raw schema and actual backend/storage/fallback/memory receipts. `--probe`
and `--profile-stages` are diagnostic only; they never receive acceptance percentiles.
Normal acceptance retains120 warmup ticks, at least1800 advancing frames **and**
30 seconds,300 accepted running-visible mutations per arm and the plan's five pairs.
Nonzero parallel fallback deltas, missing observations or unsupported GPU completion
cannot qualify. Whole-frame/pacing/input/route-quality gates stay independent.

## 7. Upstream integration and release

Trace the bottleneck to its defining owner. Feed a product-neutral Sub0 issue with
a minimal receiving reproduction, exact pin, proposed contract and before/after
evidence. Keep game rules, title choreography and mission policy in Crucible.
Do not copy an executor/cache/math implementation merely because upstream needs
refinement. Follow the library's own full/default/variant/consumer gates, then
independently review and merge its PR. Pin the actual merged commit in Crucible and
receive its real caller; a sibling checkout's HEAD is not a dependency receipt.

Merge only reviewed safe delivery with appropriate Debug/Release, supported
ASan/UBSan/race and required exact-head hosted checks. Record PR/source/tree/merge
identity and verify local/origin main. Library or source delivery does not close
physical performance targets. PR30 merged at `c2ca771` with all ten exact-head jobs
passing at `1f8d019`; Phase14 G3-G5 remain open. The user explicitly requested that
delivered baseline merge while iterative optimization continues.

Each record ends with the disposition, uncertainty, next falsifiable pass and
owner. Capture actual changed behavior where applicable. Separate generated
concepts, automated fixtures, native execution and participant observations.
The [optimization index](optimization/README.md) carries the current queue and
gate ledger; no backlog row by itself dispatches implementation.
