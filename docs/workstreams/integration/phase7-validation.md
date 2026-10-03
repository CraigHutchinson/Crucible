# Phase 7 receiving evidence

Dispatch `0767a303`, 2026-10-03; [plan](../../phases/phase7.md),
[review](../../sprint-reviews/phase-07.md). Implementation received locally; hosted exact-head acceptance and durable capture publication pending.

## Source and review

Remote checkpoints: plan `d188c6e`, mission `9ec371b`, renderer `0e48748`, native
prerequisite repair `9a99293`. Each GitHub tree was compared to the local committed
tree before publication/reconciliation. Local original commits are retained under
`phase7-local-*-checkpoint` tags; no source was regenerated. Local Git lacks push
credentials; the authorized connector publishes exact blobs/trees, then fetch and
tree comparison reconcile the branch without discarding worker files.

Renderer and mission owners self-reviewed and reciprocally reviewed. Root reviewed
ownership, isolated error markers, CLI/export refusal, shader bounds and explicit
wrapper handle scope. Findings were corrected before receiving. No production
renderer, runtime API, shader, simulation rule or dependency pin changes.

## Executing renderer and workflow receiving

Linux linker wrappers consume the unchanged production GpuOffscreen archive.
Eighteen allocation failure ordinals plus four startup, six frame and one readback
failure cases check the actual requested hook and resource/map/command/fence
accounting. Submit rejection cancels before real submission, never hides a real
fence. Upload/download themselves return void; copy-pass begin failure is tested.
The three-slot sequence executes three colored tick 0/1/2 frames while withholding
fence observations, verifies busy/pending immutability, exposes only the middle
fence, reuses that slot, expires its old receipt, drains and verifies distinct
retained interiors plus the replacement. This is controlled retirement visibility,
not hardware saturation or a claim that the GPU was physically slow.

Failed/hung drains run in isolated children. The parent accepts exact markers and
exit 86 for no-release termination, or a reached hang hook and timed-out SIGKILL
with reap. Sanitizer diagnostics invalidate those expected outcomes. Forced exit
and killed children bypass exit-time leak checking; normal fault paths retain it.
See [detailed renderer scope](../presentation/phase7-faults.md).

The recovered local Xvfb lacked its hardcoded xkbcomp helper. Existing SDK tooling
supplied the missing helper; unchanged GPU tests then launched. No failing required
backend was replaced with a software painter. Original Release receiving passed
31 non-GPU cases; the three failed GPU launch cases subsequently passed after this
environment correction. Workflow prerequisites changed afterward and were received
again with five permission fixtures and a deliberate process-only execute-bit
loss probe. `REQUIRED_FILES` registers the child; arbitrary arguments, external
tools and source files remain outside chmod scope.

Full GPU Debug passed 34/34 (310.232 seconds including receiving). GPU sanitizer
configured and built, but the execution namespace prevents LeakSanitizer from
opening /proc/<process>/task; even small existing numeric/simulation fixtures
report the same fatal environment error. This is blocked acceptance, not green.
Leak checks remain enabled, and hosted normal/GPU sanitizer checks remain mandatory.
SDL-free Release passed 28/28 (10.639 seconds). Full local GPU sanitizer returned
CTest exit 8: 32/34 cases report the same LSan /proc fatal error. No ASan error
report or UBSan runtime error was observed, but this is blocked acceptance,
not a passing sanitizer suite. Hosted exact-head counts remain pending.
All runs use scripts/run_tests.py; the sanitizer retains ASan/UBSan, leak checking,
scoped -O1, assertions and direct X11 linkage only in gpu-sanitize.

## Mission timing investigation

Existing four reference strategy checks remain unchanged. The short default
schedule fixture independently verifies requests/application boundaries with
small coordinator runs. The explicit Release study freezes three perturbations:
cadence30/start0, cadence120/start0, cadence60/start60, all consuming the same
canonical route positions and unchanged 1780/900/2048 settings. Every terminal
state requires independent full-state replay, conservation and stopped admission.
Explicit Release study and export completed successfully:

| Frozen case | Outcome / terminal tick | Reclaimed | Applied edits |
|---|---|---:|---:|
| cadence30-start0 | WON / 297 | 1780 | 10 |
| cadence120-start0 | WON / 271 | 1780 | 3 |
| cadence60-start60 | WON / 449 | 1780 | 7 |

All three cases pass conservation, independent full-state terminal replay and
stopped admission. The slowest cadence finishing earliest on this fixed world
does not establish optimal timing, difficulty or scenario robustness.

The [human protocol](../../playtests/reclamation-phase7.md) is preparation only;
it does not close human comprehension, physical input or balance acceptance.

## Artifact reproduction

After the corresponding Release build:

```sh
python scripts/capture_phase7_examples.py mission --executable build/gpu-release/tests/integration/crucible_mission_sensitivity --output NEW_MISSION_DIRECTORY
xvfb-run -a python scripts/capture_phase7_examples.py gpu --executable build/gpu-release/tests/presentation/gpu/crucible_gpu_fault_test --vertex build/gpu-release/gpu-shaders/world.vert.spv --fragment build/gpu-release/gpu-shaders/world.frag.spv --output NEW_GPU_DIRECTORY
```

Pillow/Matplotlib are capture dependencies only. Script hashes executable, relevant
source/shaders, raw outputs and converted images; records source commit/tree/dirty
state, OS, build cache, capture tool versions and GPU shader/device inventory.
Mission plots connect actual checkpoints; the GPU event table uses actual receipt
order without fabricated timing. Outputs live outside checkout during capture.
Local capture source was 1712c89a3be156eadd824c4008b3472c71ebf9a5,
tree 106e12febc58714ba81a0c05242b469a231ac803. Mission metadata reported clean;
GPU metadata reported dirty because only the proposed Phase 8 document had been
added. Receiving sources/shaders were unchanged and capture input hashes recorded.

Root visually inspected six production-painter states (tick 60 and terminal for
each case), the full/magnified checkpoint plot, three retained GPU colors and
the receipt-order table. Mission HUD/state labels and field positions were readable.
The delayed-start tick-60 state has no committed field: its first edit applies at
61. Actual GPU outputs show distinct red/green/blue interiors with unchanged
background. Event order records busy, pending, selective completion, reuse, expiry
and drain without inventing GPU time. The first mission attempt forced X11
against the fixture's dummy hint and exited before the study; only the successful
dummy-driver study counts. GPU exports use Vulkan llvmpipe/Xvfb.

After copying PNGs, raw results/events, metadata and logs into local Git-backed
evidence paths, the execution service disconnected (409 environment_offline).
Final on-disk review/hash verification and publication are therefore unconfirmed.
Do not guess or regenerate those original bytes. Preserve the local files and
recover/compare them when execution returns (P07-F01).

To avoid losing all capture evidence with that workspace, the reviewed workflow
now performs one explicit three-case Release study in the GPU job, alongside the
actual retirement export, and retains both in gpu-readback-evidence for 90 days.
This does not multiply the study across the six desktop jobs or reduce any CTest
checks. Hosted captures are fresh evidence, not claimed copies of the local files.
Exact artifact identity, run/source SHA and downloadable receipt will be recorded
at closure. A future recovered original capture may coexist with these receipts.

## Retained gates

P06-F02 controlled receiving can become partial; actual hardware device loss,
driver hangs and delayed saturation remain open. P05-F02 physical public receivers
and whole-frame 2K/100K/150K measurements remain open. P05-F01 human playtesting,
P04-F01/F02/F03, P06-F01 faction/control/resource authority and the other stable
backlog gates carry forward explicitly. No performance or default-GPU promotion.
