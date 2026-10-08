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
