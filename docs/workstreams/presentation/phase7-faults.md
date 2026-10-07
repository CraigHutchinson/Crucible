# Phase 7 GPU fault receiving

The Linux fixture links the unchanged production `Crucible::GpuOffscreen` archive
and intercepts its public SDL calls with ELF `--wrap` symbols. A stack-scoped
controller owns only bounded tracking records and borrows real SDL handles; SDL
retains ownership rules for all resources, commands and fences. Pinned SDL's internal
`*_REAL` names keep these receiver hooks separate from driver-internal operations.
Each case verifies its intended hook, real resource/command accounting and outcome.
No production constructor, renderer variant or packet/shader ABI changes.

## Seam decision

| Candidate | Ownership and cost | Decision |
|---|---|---|
| Linux link wrappers | Fixture-only symbols/controller; same production archive, real executing device and resources; requires ELF wrapping | Selected for this Linux evidence package |
| Per-instance private call table and friend test access | Portable seam but changes production construction and call dispatch solely to host these faults | Defer until a real non-Linux consumer needs controlled faults |
| Separately compiled renderer with test branches | Test behavior can diverge from the shipped implementation | Rejected |

The controller counts successful real creations/releases, live handles, mappings,
command cancellation, acquired fences and wait barriers. Startup allocation ordinal
1 through 18 covers both shaders, pipeline, quad/index/startup-transfer and all
three slots' instance/upload/download/output allocations. Ordinals after startup
submission require the real startup wait. Additional cases fail startup map,
command acquire, copy-pass begin and pre-submit admission. Frame cases fail acquire,
upload map, upload-copy begin, render-pass begin, download-copy begin and pre-submit
admission. Readback-map rejection checks the completed receipt's destination and
both latched APIs. Every returned failure must hit its hook; no failure receipt
publishes an ID/issuer or completed output.

SDL upload/download commands themselves return void. This package injects the
receiving copy-pass failure, not an invented return-value failure for those calls.
Submit rejection cancels the real unsubmitted command before returning null. It
never performs real submission or discards an acquired fence. Thus these are controlled
API admission failures, not device loss after queued submission.

## Actual three-slot sequence and export

Three real submissions retain ticks 0, 1 and 2 with distinct red/green/blue cell
colors. The controller deliberately withholds fence observations. The fourth
attempt must return busy without acquiring, mapping, copying, rendering or submitting.
Pending polls preserve a sentinel destination. A real device idle wait completes
work independently while the receiver still sees hidden fences. Only the middle
receipt is exposed and read; reuse must select that slot, issue the next ID/tick
and expire its predecessor. The receiver's blocking drain then makes the remaining
original outputs readable; independent central pixel tests distinguish all three
retained outputs and the yellow replacement. All real handles/fences balance at exit.

Optional evidence command (root records exact paths/environment/provenance):

```sh
crucible_gpu_fault_test vertex.spv fragment.spv --export NEW_DIRECTORY
```

It writes exactly `retained-0.bmp`, `retained-1.bmp`, `retained-2.bmp` and
`events.csv`. Rows record actual event order, receipt ID/tick/slot and observed
status; they contain no invented timestamps or duration. Busy receipts retain
API-default zero metadata, which is not a submitted frame. BMPs are completed
readbacks; capture/export is outside the verified receiving calls. Physical GPU
completion may be immediate; this proves controlled retirement visibility and
bounded ownership, not hardware saturation or GPU speed.

## Isolated teardown protocol

`gpu_fault_process` starts separate children for failed drain and hung wait, using
the same executable/archive. Both first queue real work. The failed wait returns
false and remains false for destruction. A custom terminate handler accepts only
the triggered wait, unchanged release count and good handle bookkeeping, emits
`FAULT_TERMINATE_NO_RELEASE` and exits 86. The earlier explicit drain emits
`FAULT_DRAIN_FALSE_NO_RELEASE`; arbitrary crashes/nonzero exits are failures.
Any intercepted release after the failed barrier emits `FAULT_UNSAFE_RELEASE` and
exits 87, which the parent rejects.

The hung-wait hook emits/flushed `FAULT_HANG_WAIT_ENTERED` and blocks deliberately.
The Python parent requires that marker and its 15-second timeout, kills the child's
process group, reaps it and checks SIGKILL. Failed-drain startup/termination has a
30-second bound; cleanup/reap has five seconds. Each CTest test has a 120-second
outer limit. Forced child exit intentionally bypasses normal GPU destruction and
process-exit leak checking; the normal fault/retirement fixture retains leak
checking. These children test host failure policy and isolation, not a real hung
or lost driver, and cannot establish safe in-process recovery after device loss.

## Acceptance status and limits

Authoring and self-review are complete; architect owns builds, actual Release,
Debug/sanitizer receiving, exact-head hosted CI and visual inspection. At author
handoff no new fixture execution is claimed. No source-level production bug has
been exposed yet. Ordinary packet/slot tests remain SDL-free; these tests exist
only with the optional receiver on Linux. X11/Xvfb and actual Vulkan/software ICD
requirements remain the Phase 6 environment; dependency pins are unchanged.

Open evidence remains physical device loss, driver hang, hardware delayed
saturation, Metal/D3D12 receiving and public physical-device acceptance. The private
fixture symbols are not a reusable backend interface or upstream extraction.
