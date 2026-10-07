# Phase12 personal review: automated behavior first

This is a usability/presentation review, not the first correctness test. Before
inviting a session, the agent checks the exact build's automated gates and supplies
these instructions. Computer control is reserved mainly for diagnosing a reported
failure; do not silently take over the participant's run.

## Ready build and coverage

Current local Release binary was built from `1eef613`; later commits change docs
only. Release42/42 and affected Debug14/14 passed; all9 final-head CI jobs passed
and PR25 merged at `c4ae9c0`. Automation already covers:

| Review behavior | Automated receiving |
|---|---|
| Pause/restart, FLOW keyboard/drag/admission/application/cancellation | `desktop_events`, `field_tools`, `runtime_inspector` |
| Fuse/shatter queued/application/refusal/restart | `structural_desktop_events`, `runtime_structural` |
| Mission closure and direct/integrated state/trace parity | `runtime_backbone_parity`, mission/route fixtures |
| Optional diagnostic cadence/exhaustion/restart | `runtime_diagnostics`, `runtime_diagnostics_parity` |

SDL event/software fixtures prove application behavior; they do not prove your
display readability or physical input mapping. Native operator evidence is
[recorded separately](../workstreams/integration/phase12-evidence/native-receiving.md).
Fullscreen, sound and human accessibility/balance are not accepted features yet.

## Launch and review steps

From PowerShell in `D:\Craig\GitHub\Crucible`:

```powershell
& .\build\phase12-release\src\desktop\crucible_desktop.exe --diagnostics
```

1. Press Space promptly: the run starts immediately. Expect PAUSED and a stable
   tick number. If it already reached LOST, press R then Space promptly.
2. Select4 FLOW. Drag horizontally across the middle of the world and release.
   Expect a dashed field preview and queued/waiting text; the paused tick remains
   unchanged. Note whether tool selection and pending state are understandable.
3. Press Space to resume, then Space to pause again after a moment. Expect the
   field outline to become solid and feedback to say applied at completed boundary.
   Note whether the transition is clear without explanation.
4. Press R. Expect a fresh active world, cleared prior field/queue/result and reset
   tick progression. Press Space again to pause for inspection.
5. Compare normal and maximized window sizes using the Windows maximize control.
   Inspect objective, numeric HUD, all eight toolbar labels and feedback. Record
   which text is comfortable to read and which controls are hard to distinguish.
   This compares window sizes; borderless fullscreen is proposed for Phase13.
6. Close normally. With diagnostics enabled, its decoded transcript is printed to
   the launch console. You need not parse that transcript to understand the UI.

Optional structural review, with the same automated prerequisites:

```powershell
& .\build\phase12-release\src\desktop\crucible_desktop.exe --structural --diagnostics
```

Pause promptly and press X on the empty relay. Expect a queued structure action
while paused; resume to see the explicit empty-relay refusal. Press R to reset.
F means fuse in this mode; use the FIT VIEW toolbar for fitting the camera.
Do not infer a winning route from these checks; the deterministic route is a
separate automated gate and player balance remains open.

## Report a finding

Record step, expected/actual result, visible tick/pause/state, window size/display
scale and whether assistance was needed. A screenshot is optional. If behavior
fails, stop that sequence; the agent reproduces it with automation, diagnoses the
specific native/input issue if necessary, and provides a corrected build before
repeating personal review. Subjective readability/confusion is valid review
feedback even when functional tests pass. No audio review is requested yet.
