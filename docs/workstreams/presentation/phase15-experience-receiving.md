# Phase15 production frontend and cinematic receiving

Owner: consumer worker B, isolated `codex/production-frontend`, initial base
`8bdf587faf733efac07257306c2f3015987d61fb`. Style profile: sub0, declared in
AGENTS.md; overlay: none. Root owns the shared values, application/profile,
platform input/context and final receiving; A owns the framework renderer/upload
adapter. This brief records implementation and receiving separately.

## Experience and capability reconciliation

The player sees the approved silver/cyan machine identity, obtains immediately
usable controls, understands a selected mission and receives an explained result.
The controls are real themed ImGui widgets over the production scene. The baked
menu board remains concept art. Existing SDL drawing remains dev/test tooling.

Selected typography is [Rajdhani SemiBold and Source Sans 3 Regular](../../../assets/production/fonts/README.md),
with original licenses, pinned sources and exact hashes. Near-black industrial
panels, subdued silver rules, cyan focus accents and restrained amber progression
carry the concept's composition. Large, stable targets and readable body text take
priority over decorative motion. Red/magenta identifies failure rather than routine
selection. No debug/demo panel or automatic cursor movement belongs to the flow.

The Filament1.77.3 candidate supplies the material/depth/post-processing foundation
and matched ImGui1.92.5. Native framework receiving is pending independently;
source code, font assets and synthetic UI frames cannot establish rendered
material fidelity, actual input latency or GPU success.

## Choreography contract

`CinematicChoreography::evaluate(std::chrono::milliseconds elapsed,
float drawableAspect)` receives root's clock and actual drawable aspect. Time is
clamped to0..12000ms. Aspect must be finite in[1/32,32]. Storage is startup-owned,
bounded by4096 decorative instances, with stable count/order and a synchronous
pose borrow. No gameplay state or wall-clock input enters evaluation.

Letter placement is authored geometry spelling exact uppercase CRUCIBLE; it does
not sample or mask a generated raster. Purposeful source lanes converge into fixed
strokes and open counters. The sequence follows macro wake, lane response,
assembly, full readable hero hold and a calm menu composition. Camera fitting
must retain the entire wordmark in portrait and landscape. The settled12000ms
composition is also consumed by root's reduced-motion/skip policy.

| Authored elapsed time | Received producer behavior |
|---|---|
| 0..1800ms | Close macro camera; deterministic source lanes wake their cyan cores. |
| 1800..4200ms | Camera opens onto purposeful lanes; wedges begin letter-ordered assembly. |
| 3300..8132ms | Fixed authored strokes assemble; shortest-path unit-quaternion interpolation turns the wedges upright. |
| 6800..10000ms | Front-facing hero camera; the entire wordmark has settled by8132ms. |
| 10000..12000ms | Camera moves to the static menu composition: title beside landscape controls, above portrait controls. |
| 12000ms or later | Stable settled wordmark, camera and material intensities. |

Root owns elapsed time, suspend/skip/replay policy and frame sequencing. Decorative
airborne arcs introduce no gameplay unit. The provider never reads the clock;
the light/core curves are scene values, with actual material/post-processing and
audio receiving assigned separately. Initial sounds remain unimplemented.

Evaluation is repeatable for the same elapsed time/aspect on a receiving build;
this is decorative interpolation, not a cross-toolchain simulation replay claim.
Actual image comparison must inspect silhouette, core placement, continuity,
specular identity, readability and the transition against the approved concepts.

## Frontend contract and resource envelope

The frontend borrows root's ImGui context. Construction loads trusted bundled
fonts and applies the cold theme; draw requires that same current context and an
active root-created frame. View labels/spans expire after draw. Only IDs/focus
metadata persist. One optional typed intent carries the originating screen and
transition serial; root validates and applies it. There is no Runtime mutation,
event forwarding, NewFrame/Render call or profile write in this provider.

The root-supplied safe content rectangle contains all controls, with scrollable
overflow and focused-control visibility. Desktop/mobile use the same policy;
orientation changes affect layout without requiring a mouse hover. Initial
screens consume at most2 mission cards,12 actionable widgets and4096 displayed
characters/codepoints, including bounded dynamic labels. At most64 fixed
rectangle/line decorations are planned. No custom draw callbacks, freehand paths,
plots or large UI image texture is used in the initial live-scene path.

Finite base font sizes14/18/22/32 logical pixels receive the selected text scale
once. Framebuffer density is distinct from logical layout. Dynamic ImGui font
baking/upload/atlas growth requires real receiving across the supported scale/DPI
matrix. A derives and preflights the whole UI vertex/index/texture/upload envelope;
capacity failure must never silently remove later controls from a rendered frame.

The `Frontend(ImGuiContext&, fontDirectory)` constructor performs cold font/theme
setup outside a frame. `draw(const FrontendView&)` requires root's current active
frame. It validates every borrowed label/card and actual safe content/density
before theme or widget mutation. Mission cards emit `SelectMission`; options emit
only their own preference variant. Other controls emit `NavigationAction`, including
explicit pause, save retry/unsaved continuation and protected quit confirmation.
Root remains the legality/session/profile owner.

Each new screen/transition receives the first enabled default control with a
visible keyboard cursor; ordinary preference application on Options keeps focus.
Keyboard navigation scrolls a newly focused target into view. Steady frames retain
deliberate wheel scrolling. The compact playing HUD leaves non-HUD pointer space
available to root's gameplay routing. No transition animation gates control input;
root's transition identity and release policy prevent old activations applying to
the next screen.

## Receiving ledger

Font originals/licenses downloaded and hashed on2026-10-09. No framework build,
native image, device execution or performance receipt is claimed by that work.
The four owned C++ files and two behavior fixtures are authored against the
root-reviewed copied header checkpoint. Independent cpp-review of the four files
found no consequential MUST; its same-screen focus SHOULD is addressed with
transition identity while ordinary options updates retain focus. Source review
also caught transparent gameplay-window input capture: playing uses a compact
HUD, leaving the remainder of the viewport to application gameplay routing.
Heavy CPU/GPU runs require root's resource handoff.

The [original-font study](../../../assets/production/fonts/README.md#visually-inspected-typography-study)
was inspected separately. It is a reproducible static specimen, not an ImGui or
native game capture; approved concept assets are unchanged. Actual changed menu
and title behavior must be captured after the root's production caller and
renderer are received.

Required behavior fixtures cover deterministic cue evaluation, stable authored
count/order, portrait/landscape camera fit, finite unit quaternions, time clamp,
invalid aspect refusal and borrowed-storage reuse. UI fixtures exercise real
ImGui keyboard/pointer controls, correlated intents, disabled reasons, options,
results/save failure and safe-area/rotation layouts with actual font assets.

Received on2026-10-09: exact Filament source
`d852e34cd5629a6f3851d613dcd17aa0ffd025a2`, its ImGui1.92.5 Release/MD archive,
MSVC19.51.36246.0 (toolset14.51.36231 x64), C++23, `/W4 /WX /permissive- /fp:strict`.
An isolated ignored fixture project imports that archive rather than compiling a
second core. The root-approved correctness CPU reservation used at most3jobs.
Final `run_tests.py` receiving: **2/2 PASS**,0.21s CTest suite time, no device/window.

```powershell
python scripts/run_tests.py --preset release --prerequisite-build-dir build/consumer-fixtures-release --test-dir build/consumer-fixtures-release -V --output-on-failure
```

Exact successful compiler/test outputs remain in this owned checkout's ignored
`build/consumer-fixtures-build6.log` and `build/consumer-fixtures-tests5.log`.
Earlier configure/build/test failures remain alongside them. Initial sandbox
compiler probing failed C1902; Embedded debug information plus the authorized
scoped execution received the actual toolchain. A missing stroke-table brace was
fixed. The actual keyboard fixture then exposed hidden navigation-cursor Enter
refusal; the provider now makes its default cursor visible. A test initially
misclassified valid background viewport clipping; its final independent check
receives indexed triangles after clip intersection, covering actual visible extents.

- Choreography:1251 instances; exact time clamps/repeated-build fields and stable
  storage; independent CRUCIBLE letter/counter geometry and look-at projection;
  all cue boundaries plus dense samples at aspects1/32,.5,2/3,1,1.5,2,32; complete
  camera/pose/light bounds; zero **ordinary** allocation calls on the first and
  repeated valid evaluations. This is not whole-process allocator coverage.
- Frontend: all ten screens, real event-queue keyboard/Tab/pointer controls,
  every typed preference/navigation family, disabled mission/Continue behavior,
  safe rectangles including inset coordinates,320×480/480×320/1280×864,
  text scales1/1.25/1.5 and framebuffer densities.5/1/2/4; maximum admitted labels,
  invalid UTF-8/zero serial/duplicate IDs/nonfinite geometry refused. The observed
  maximum6828 vertices belongs to this fixture corpus, not a universal bound.
  CPU acknowledgement of font texture requests proves no GPU upload.

Independent source cpp-review and self-review use sub0/none. Root receives the
actual executable/CMake/pins/common headers and performs combined Debug/sanitizer
and native rendering gates. Font study/hash receipts and these synthetic frames
cannot stand in for final cinematic fidelity or human usability.

Root's native receiving then closes scene+UI captures, focus/skip without
click-through, text scale/DPI, cold startup/resize, uploads/callback retirement and
the menu→mission→result→safe Continue journey. Audio, physical mobile execution,
participant comprehension and Phase14 G3-G5 remain distinct open gates until their
own actual evidence exists.
