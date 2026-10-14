# Native production receiving: R0 and M3 R1

2026-10-13. These are actual Vulkan composed-frame readbacks from
`crucible_game`, not generated concepts. The selected device was NVIDIA GeForce
RTX5070 Laptop GPU, Vulkan driver617.14/API1.4; the full reported device and
installed-driver identities remain in the logs and [hardware receipt](r0/hardware.json).
Readback is not proof of physical scanout, participant usability or performance.

![Native menu with real themed controls](r0/menu.png)

![Native advancing mission snapshot and HUD](r0/world.png)

![Native original wordmark fixture](r0/hero.png)

The [receiving manifest](r0/receiving.json) retains commands, commit/worktree
identity, executable/material/font hashes, PNG/log hashes and each frame/surface
receipt. Seven finite processes received macro, assembly, hero, menu, options,
missions and advancing mission state. The world image receives frame64 at actual
completed tick247; skipped graphics submissions do not imply a one-to-one mapping
between render frame IDs and simulation ticks. No rate is inferred from this run.

Independent reviews fixed early capture/resize reservation, callback retirement
during suspension, elapsed-time loss on graphics skips, native fullscreen
confirmation, oversized resize refusal and intro replay selecting retained world
state. First native inspection found inverted PNG rows and material descriptors
changed after `beginFrame`; pinned source established top-down rows and the
descriptor commit order. Fresh captures are upright and none contains the two
observed sampler/descriptor warning messages. This is not a Vulkan validation-layer
or arbitrary device-loss acceptance claim. Failed/initial captures remain in the
owned build tree.

## Reproduce and review

On Windows x64 with a received MSVC Release `/MD` toolchain:

```powershell
./scripts/production/qualify_filament.ps1 -Stage all -Jobs 3
cmake --preset production-release
cmake --build --preset production-release
python scripts/run_tests.py --preset production-release -L "production|application"
python scripts/production/receive_native.py --output build/native-review-new
./build/production-release/src/game/crucible_game.exe --profile build/player-progress.txt
```

An explicitly selected different Visual Studio/toolset must be qualified with
`-VsInstallationPath` and `-ExpectedToolset`; importer matches the actual compiler
and source/archive hashes. For an existing qualification pass its source/release
directories using `CRUCIBLE_FILAMENT_SOURCE_DIR`/`CRUCIBLE_FILAMENT_BUILD_DIR`.
Native receiving output must be a new directory. The two fonts and original
licenses are copied beside the executable. Unsupported graphics capability fails
explicitly; the concept SDL painter is not a fallback.

Keyboard/left-pointer navigation consumes actual ImGui controls. During play,
1/2/3 select attract/repel/erase;4 selects a dragged flow;Tab changes field slot.
Escape or Pause suspends the mission. F/X admit fuse/shatter only during Secure
Relay; admission and completed structural results are separate. The profile stores
mission-boundary continuation and preferences, not a live checkpoint. Focus loss
pauses; regaining focus does not automatically resume.

## Visual findings and next consumer

R0 establishes one real window, scene/UI composition and mission migration seam.
The old wedge macro and sparse wordmark are visibly below the concept target.
Metallic planes lack a useful reflected environment, the industrial floor remains
a simple procedural grid, and the gameplay overview lacks distinct material cues.
Menu focus is visible but its composition still needs refinement against the art.
Do not label these images production fidelity or final introduction acceptance.

PR46/47's updated swarm/camera and nanite proxy are the next consumed source.
Their oblique orthographic camera, presentation/choreography clocks, rigid core,
speed-stretched shell and morphed docking footprints require an explicit bounded
geometry/camera receiver. Retain reduced-motion and immediate skip throughout.
New single-unit articulated rig/crystal concepts remain separately gated. Apple,
Linux production graphics, native DX12, audio and human review remain open.

## M3 R1: latest model in the actual Scaffold, 2026-10-14

All ten serial native Vulkan readbacks passed and were visually inspected: initial,
immediate menu preparation, macro, assembly, settled title, menu, options, missions,
advancing mission completion and reduced-motion startup. The [clean-source manifest](r1/receiving.json)
records checkpoint0102ed5, an empty tracked diff, executable/studio/material/font
hashes, per-image identities, actual clocks, body profile and emitted detail counts.
The selected RTX5070 Laptop GPU is recorded in each native log; this remains
readback evidence rather than physical scanout or performance.

![Actual M3 nanites in the operated macro shot](r1/macro.png)

![Actual assembled M3 title](r1/hero.png)

![Actual M3 menu background and themed controls](r1/menu.png)

The macro emits1,048,218 corners, with30 Articulated/Bevel units among38 visible
units. The settled title emits586,752 corners for all1536 units:1408 silhouette,
38 frame-flat and90 frame-chamfer. Four variants across six cold rungs retain
3,322,172 bytes; no cold numerical slivers were omitted. Source height is0.898146,
span1.1 and lean0. Opaque housing omissions are explicitly counted. The renderer's
external staging cap is288 MiB, not a total process/GPU-memory assertion.

[Release62/62](r1/release-tests.log) and [Debug6/6](r1/debug-tests.log) include model,
normal/winding, global bounds, zero ordinary hot allocations and unchanged
source contact/clearance/settling gates at portrait and desktop sizes. Independent
C++ review found no blocking issues. The old27-case proxy oracle remains a test
fixture; it does not qualify M3.

The silver macro geometry and wide title now remain visible after correcting the
material's UV intensity flip and receiving a convolved studio environment. These
are integration progress, not final cinematic fidelity. Visible rung differences
still need native fade/calibration, the menu panel obscures the left title letters,
and the skip panel dominates the intro composition. The mission overview is
readable as a bounded state receiver but needs stronger gameplay material cues.
Next pair composition and native transition refinement with complete-frame
profiling under the [optimization process](../../../OPTIMIZATION_PROCESS.md).
Precise GPU articulation, glass, audio, participant acceptance and additional
production platforms remain open.
