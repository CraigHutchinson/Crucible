# Phase14 production frame provider handoff

Status: source preparation only. No build, device execution, capture inspection,
timing or native handoff acceptance is claimed by this handoff. Root receives the
combined production caller before publication.

## Painter policy and accounting

`ScenePainter` retains exact individual rendering by default. Its existing call
shape remains valid; capacities still allocate once at startup. Invisible cell and
sample quads are compacted out of the submitted batches. HUD population remains
the authoritative mobile count rather than the visible geometry count. Existing
2K pixel fixtures are retained unchanged and must pass during root receiving.

The explicit `ViewPolicy::densityOverview` candidate adds startup-owned 4-logical-
pixel screen bins. Below four logical pixels per world unit, visible mobile samples
are accumulated by screen bin and represented by a two-pixel mark at their mean
projected position. Logarithmic intensity distinguishes concentration. Detail zoom
returns to individual markers. This is a presentation prototype awaiting actual
overview/front inspection; it is neither simulation LOD nor received visual quality.
Simulation, query order, positions, resource state and exact picking stay unchanged.

`getDrawStatistics()` copies counters from the last successful whole drawing call;
failed drawing preserves the previous counters. The closed invariant is:

```
authoritativeMobile = individualSamples + aggregatedSamples + hiddenSamples
aggregateMarks <= aggregatedSamples
```

Root adds the policy to `DesktopApp::StartupSettings`, passes it to the painter and
copies statistics through the production frame observations. Small scenarios keep
the exact policy. Counts distinguish aggregated identities from aggregate marks.
The source fixture covers exact fit, independently derived zoom visibility,
overview/detail accounting, rejection retention and snapshot immutability.

## Concrete completion observer

`FrameCompletionObserver` accepts the borrowed main-thread SDL renderer and positive
startup pending capacity. On Windows D3D11 it owns COM device/context references and
one event query per slot; other backends/platforms are explicitly unsupported.
Its public header contains no Windows headers. The renderer outlives the observer.

Root records a copied positive run/frame identity and completed tick after the
production world/HUD presentation call. `recordFrame` flushes SDL before native
interop, inserts the native event marker and flushes it once. `pollOldest` reports
pending, completion or latched failure and copies the original run/frame/tick.
Only completion retires a query slot. Full and invalid recording preserve slots;
the observer allocates no storage per record/poll. The native query can complete
before the first poll; tests do not assume a delayed physical device.

Native device-property replacement latches `deviceChanged`; no new work is accepted
even if the old property reappears. `tryDrain` joins retained markers against their
original owned context. It cannot impose a hard wall-clock driver bound. Native
failure reports false; destruction logs the failure before releasing query/context
references. The observer owns no submitted frame buffers. Root owns renderer/frame
resources and their separate native shutdown policy.

Root destroys the observer before its renderer, receives drain before replacement
and records unsupported/error/backpressure cases honestly. GPU-command completion
does not prove successful presentation, scanout or physical input latency. In this
SDL pin, `SDL_RenderPresent` hides backend present failure, so root must receive its
native handoff policy independently. No query receipt closes that missing gate.

Microsoft's event-query semantics require S_OK and TRUE to establish command
completion. Flush sends work asynchronously; it is not itself completion:
[event query](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_query),
[GetData](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-getdata),
[Flush](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-flush).

## Checked native handoff candidate

`CheckedPresentation` receives only an actual direct3d11 renderer name plus its
public device/swapchain properties. Original COM references are owned at startup;
other backends/platforms are unsupported. It replaces the ordinary presentation
call for an explicitly selected receiver and never double-presents.

The coordinator checks identity/device state, checks `SDL_FlushRenderer`, calls
native Present1 with fixed sync interval one/flags zero, then flushes SDL again to
invalidate cached native state. The pinned D3D11 invalidation clears the cached
render target, shader, blend/rasterizer and dirty viewport/clip state, so the next
SDL drawing call rebinds after a native flip. No private renderer fields are read
or changed. Existing small desktop runs retain ordinary SDL presentation.

Only native S_OK is `handedOff`; busy and occluded are non-presenting cohorts.
Other native results, flush failure and changed device/swapchain identity latch an
explicit error. A copied optional signed HRESULT is absent when no native operation
supplied a result. Root propagates these outcomes separately from GPU completion.
The receiver owns handoff/pacing on this one backend, not scanout or a backend
recovery framework. Retaining original swapchain references can prevent automatic
replacement; identity/loss is fail-stop pending explicit receiving/reconstruction.

The explicit native fixture now exercises the checked handoff instead of the SDL
wrapper, then correlates its commands with the completion observer. Its property
replacement fixture checks both receivers; no physical loss/hang is induced.
Root subsequently received the concrete provider and full production desktop at
both scale settings: checked handoff/completion drain and native resize/fullscreen,
pause/restart/close were reported at its `fd8075e` fixture checkpoint. That receiving
does not establish route quality or performance. The capture consumer below remains
source-only at this handoff; its C++ build, classifier fixtures and actual capture
execution remain root-owned gates.

## Root receiving and shared wiring

The local manifest includes the observer in `Crucible::ScenePainter`. The normal
`scale_view_counts` fixture uses software output and proves unsupported observation,
not physical completion. After root reserves native display/device time, invoke:

```
crucible_scale_view_test --native-completion
```

That explicit mode requires a real D3D11 renderer and receives bounded capacity,
refused/duplicate identities, ordered receipt correlation, drain and new-run reuse.
A synthetic public device-property replacement checks refusal plus original-context
drain; it is not physical device removal/loss. The mode is not registered as an
ordinary headless test and never substitutes software for the required backend.
The host must provide an external process timeout; the internal polling deadline
cannot interrupt a driver call or license releasing submitted frame storage.

Root's scenario/desktop checkpoint supplies settings-based construction,
`getRenderer()`, `getFrameStatistics()` and session `getRunId()`. Follow-on capture
code must consume those production hooks, attach painter statistics, correlate
native completion with CPU intervals and keep output cold. No duplicate simulation
loop or offscreen readback benchmark is supplied here. Before major acceptance,
freeze observation granularity/overhead and the completion/presentation distinction
through G0b, then receive evolving normal routes at both required scales.

## Production capture consumer and bounded observation

`benchmarks/phase14_frame.cpp` constructs the actual `DesktopApp`, receives native
events, dispatches scripted keyboard/mouse gestures through `HandleEvent`, and calls
`Iterate`. There is no alternate measured simulation/render loop. Root includes the
owned `benchmarks/phase14_frame.cmake` from its benchmark inventory; the target is
`crucible_phase14_frame_bench`. It requires desktop+benchmark configuration and the
concrete checked D3D11 startup mode. Unsupported startup is a process failure with
stderr retained, never an SDL software substitution.

The first G0b arm is `joined-yield-zero`: after checked handoff, record a D3D11 event
query and poll/yield until it completes before beginning another frame. The frame
starts before native event service; marker/flush, query calls and all completion
waits are included. The receipt is an observation upper bound, not a GPU timestamp.
Poll call count and cumulative call time are retained. No overlapping throughput is
claimed. Internal deadlines still require original-context drain; an external
process watchdog is the bound on a blocked driver call. A timeout retains failure
artifacts and supplies no successful completion receipt.

Pacing uses a fixed 16,666,667ns schedule with `SDL_DelayPrecise` before each frame's
service interval. Whole overdue periods are counted/skipped rather than silently
adding pacer debt; lateness, zero/multiple-tick frames and ClockDriver discarded time
remain in raw rows. Native sync interval one is not assumed to mean 60Hz. The actual
window's display refresh, display scale/pixel density and output dimensions are
recorded. The D3D11 adapter vendor/device/LUID comes from the renderer's actual
device property through COM-owned DXGI observations, not a system device inventory.
The adapter name and optional UMD version are copied from that actual adapter;
`CheckInterfaceSupport(IDXGIDevice)` is used only for the version query, not to
infer D3D11 support ([Microsoft API contract](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiadapter-checkinterfacesupport)).
Missing driver version explicitly blocks acceptance eligibility.

All frame/input/trace/final-sample vectors are sized before the measured loop;
the query capacity is eight. Frame capacity defaults to 10,000, scripted mutations
to 300, trace capacity to 4096. Hitting the frame/time bound is explicitly
nonqualifying. Raw row storage is never silently overwritten or grown; dropped row
counts are zero because the receiver stops before exhausting it. A refused query
slot remains a `full` cohort, not a completed sample. Drawing/presenting still belongs
to the coordinator, and original-context draining precedes renderer destruction.
Cold JSON encoding, file output, oracle construction and images are excluded.

Warmup excludes at least 120 actual completed ticks and drains a checked native
marker before the measured origin. Unexpected operator field mutations during
warmup or native event service fail the frozen script; normal resize/focus/window
events are serviced and their resulting visible/paused/focused cohorts are retained.
Restart fails the measurement as a separate run cohort. A successful short probe
does not receive acceptance percentiles. Source SHA is mandatory and must equal the
configured executable's embedded SHA; configure refuses uncommitted compiled paths.
The wrapper checks Release configuration, source directory/tree/dirty compiled
paths, executable SHA256, cache, dependency pins and its own SHA256. Documentation
dirt does not masquerade as modified compiled code or invalidate that classification.

## Workload and cold field quality

Both population settings retain the agreed evolving grids (100K/400x250 and
150K/500x300), one-unit cells, four fields, radius16/magnitude4, no early mission
cutoff and density overview policy. Three named routes are consumed:

| Route | Mutation and measurement purpose |
|---|---|
| `none` | Native no-command comparator; evolving simulation remains active |
| `flow` | Sustained rightward FLOW from2% to98% world width; alternate49%/51% world height without reversing direction |
| `gather` | Same persistent FLOW slot0, repel slot1 at25% width, attract slot2 at85%; repeated updates through the real slot/tool/gesture controls |

One scripted mutation is attempted every six measured frames, until the startup
mutation bound. Acceptance and application remain distinct. Raw trace values retain
each actual command payload and applied tick. The input receipt follows the first
checked, GPU-completed frame carrying that applied edit; the earlier application
frame is recorded independently. Ignored/refused, paused/hidden and uncompleted
inputs stay visible and cannot help reach the 300-event qualifying count.

After the native loop/drain, an untimed `InspectorSession` with identical settings
advances to the first applied field's tick minus one. The complete production
`FieldSet::Sample` evaluates every authoritative copied sample there; nonzero
acceleration freezes the initial FLOW corridor ID cohort. This is actual first-field
influence, not nominal capsule area or a claim about all later route fields. The
same no-command session then reaches exactly the captured final completed tick.
Corresponding sample IDs are checked before comparing signed +X displacement.
Reference CPU time and cohort count are reported cold. Quality requires >=10%
initial nonzero field samples and cohort **signed mean** displacement >=5% world
width versus that matched reference. Maximum displacement and count individually
crossing the threshold are diagnostics; one outlier cannot establish front quality.
The separate native `none` arm complements this exact-age oracle and is not silently
treated as state-parity evidence at a different wall-clock tick.

Optional `--capture-dir` requests actual overview/detail BMPs after measurement.
The app is paused through its production control, the retained authoritative final
tick stays fixed, and root's one-shot hook reads the complete world/HUD before
native flip. Readback/save and a final native observer drain are timed/labeled cold.
These paused retained-frame images receive visual quality, not evolving-frame FPS,
participant comprehension or physical scanout.

## Schema and classification receiving

JSONL schema2 contains one `capture`, ordered `frame` and completed-boundary `tick`
rows, `input` rows, the exact `trace`, and one `quality`. Monotonic frame nanoseconds
share the measured steady origin; tick intervals are copied duration values.

| Row | Consequential fields |
|---|---|
| capture | source/tree, actual renderer/adapter/display, scenario/tool settings, capacities, duration/warmup, initial tick/observation cursor, retained observation count and drops, attribution flag, initial discarded time, drain/bounds/drops/failure, joined observation and60Hz pacing modes |
| frame | run/frame/tick, advanced ticks, begin/end, production service/pump/draw/present intervals, native status/signed HRESULT, marker status/interval, handoff/completion observations, poll calls/time, pace lateness/skips, visibility/focus/events, closed view counts, ingress/applied/discard counters |
| tick | run/completed tick and containing frame, complete boundary duration, applied-command count and optional copied simulation stage durations/counts |
| input | run/sequence/applied tick, event/admission/application observations, application frame versus completed-handoff frame, acceptance/running/visible state |
| trace | exact sequence/tick/action/result and field slot/kind/geometry/strength |
| quality | cold oracle/image CPU time, retained capture tick, influenced count/cohort, signed mean/max displacement and threshold-crossing count |

Enum integers map to their defining C++ enums: native handedOff0/busy1/occluded2/
unsupported3/deviceChanged4/deviceError5; marker recorded0/full1/invalid2/
unsupported3/deviceChanged4/deviceError5; clock running0/paused1/closed2/blocked3/
invalid_elapsed4. Missing optional native/marker status is -1; HRESULT is null when
no native operation supplied one. No timestamp is invented for uncompleted work.

`scripts/capture_phase14_frames.py --summarize raw.jsonl` checks identity, population
accounting and input/frame correlation before classification. It uses nearest-rank
percentiles only with >=1800 **advancing** frames **and** >=30s, >=120 warmup ticks, complete
running/visible checked handoffs and >=300 completed accepted scripted inputs for
active routes. Collection continues through idle redraws until the advancing-frame
minimum; G3 uses advancing frames, while all raw handoffs remain in G4 cadence and
stale-frame reporting. Tick deltas must equal advanced ticks from the captured
initial tick. Globally ordered frame intervals, handoff/marker/completion timing,
input application-frame correlation and nonnegative counters are validated. Every
advancing tick must have exactly one immutable observation in the same run and
containing frame. Each tick's command count must match the actual applied trace;
the sum of boundary durations cannot exceed its containing pump duration. Missing
or dropped tick observations block all acceptance percentiles; duplicate, mixed-run
or wrongly correlated observations are rejected. Whole-tick p95/p99/max use actual
complete boundary rows, including command application and snapshot publication,
rather than dividing a multi-tick pump interval by its advanced count. A
changed native output size is a separate nonqualifying workload cohort.
Full-frame p95<=16.67ms/p99<=20ms and actual handoff cadence/input
targets are independent from eligibility and quality. Repeats and discarded time
use conservative raw-count gates; they do not assert every repeat's cause. Maxima
and over-budget counts remain alongside percentiles.

The wrapper requires five independently alternating baseline/current process pairs
per selected workload, uncontended/background-power-thermal attestation, separate
fresh output directories and an external process deadline. No retry removes a
failure or slower outlier. Probe mode uses three frames/no warmup and explicitly
cannot receive acceptance. All per-process summaries and raw data are retained;
pair receipt eligibility does not itself close G1/G2/G5/platform/participant gates.
Seventeen independent synthetic classifier fixtures are registered as
`phase14_capture_classification`. They exercise minima, correlation/accounting,
occlusion/full slots, clock discard, mean-cohort quality, idle-frame dilution,
corrupt tick/backward timing, missing driver, a four-tick pump, incomplete/duplicate/
mixed-run boundaries, applied-command and bounded-prefix accounting, and explicit
stage attribution exclusion. All17 passed with
`python tests/presentation/desktop/test_phase14_capture.py -v` on2026-10-08;
the exact output is retained locally in
`build/phase14-boundary-receipts/classifier.txt`. This supplies schema coverage;
the updated C++ harness and native receipts still require architect receiving.

The harness preallocates both Runtime observations and its cold-copy destination
to `4 * maxFrames + warmupTicks + 4` at startup. Warmup freezes the initial tick and
observation cursor; the immutable prefix is copied once after measurement/drain.
Default `--profile-stages false` enables matching outer boundary clocks in both
comparison arms and leaves simulation stage clocks disabled. The wrapper's
`--profile-stages` flag instead runs a current-only attribution arm, refuses baseline
arguments and passes `true` explicitly. Its raw optional stage intervals and counts
are diagnostic evidence and never qualify G3/G4/G5. The sequential control receives
the same Runtime outer clocks while retaining its prior Simulation/Spatial/Swarm
bytes; it rejects stage attribution explicitly. The architect owns that separate
receiving change and its actual source-parity receipt.

Root handoff: include the owned manifest, commit the compiled source checkpoint,
reconfigure so the embedded SHA/tree matches, receive classifier plus targeted C++
build/tests, then reserve a native `--probe` process before any full paired study.
If a budget fails, retain the exact receipt and use the planned three refinement
passes; no source-only optimization or smoke result changes Phase14 acceptance.
