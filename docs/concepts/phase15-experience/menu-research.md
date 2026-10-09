# Menu research: cinematic identity, immediate control

2026-10-09 · proposed design/receiving follow-up to the approved concept direction.
No artwork regeneration, code, build, native capture or participant session is
supplied by this note.

## Source observations — bounded anecdotal summary

The user-selected [r/truegaming menu-design discussion](https://www.reddit.com/r/truegaming/comments/tlvpi/im_a_designer_looking_for_some_inspiration_what/)
was read directly. The archived page labels the discussion15 years old. It is
self-selected anecdotal inspiration, not representative or current gamer research.
Commenters favor responsive controls, readable/scalable text and contrast, shallow
navigation, shortcuts for repeated tasks, and prompts that follow remapped keys.
Some object to automatic mouse movement/locking and animation waits. Others
appreciate integrated atmosphere and memorable menu sound. Important disagreements
remain: radial menus appeal to some controller users but receive mouse-use
criticism; escaping all nesting quickly is attractive to some and disruptive to
others. DiRT's cinematic menus receive aesthetic praise and complaints about slow
or cumbersome navigation. Fable3's spatial sanctuary is described as attractive
and convenient by one participant and cumbersome by others. Comments also raise
resume/quit ordering and confirmation concerns. Claims about cursor technology and
latency in the thread are not technical measurements or proof of a specific cause.

## Crucible decisions — our proposed policy and reasoning

The decisions below are application design choices to test, not conclusions
established by the discussion. Preserve the high-class twelve-second nanite intro,
coherent silver/cyan/amber materials and calm main-menu atmosphere. Put the scenic
animation behind stable usable controls; it must not own the player's input cadence.
Use the existing [director's brief](director-brief.md) for cinematic timing and
[journey design](experience-design.md) for mission/save semantics.

| Requirement | Crucible decision and observable consequence |
|---|---|
| M01 immediate ready-state control | Once a menu's visible controls and layout are ready, process fresh navigation, activation and cancel intents immediately. No transition timer, camera travel or logo pose delays them. Give a visible selected/pressed/refused cue. |
| M02 stable targets | Buttons remain in stable screen-space positions while background nanites/camera animate. Keyboard focus does not move due to a background beat. Decorative fading never makes an invisible control accidentally clickable; the first interactive menu presents all active controls legibly. |
| M03 honest transition routing | The input that skips the intro is consumed once by that transition; it cannot also activate a mission. A new input after menu readiness is handled normally. Pair pointer press/release with their original target/state, latch one activation, account for invalid repeat events and refuse stale targets explicitly. Never flush all input just because an animation runs. |
| M04 first versus repeat launch | First launch offers Skip and accessibility immediately; returning launches go straight to the ready menu with Continue focused only when compatible progress exists. Replay Intro is optional. A first-ready menu accepts input even if decorative reveal continues behind it. |
| M05 pointer autonomy | PC menu entry does not warp the pointer, lock it to a list, confine it to a button or alter sensitivity/acceleration to create atmosphere. Investigate a native cursor path with actual receiving; do not assume it fixes measured latency. Existing gameplay gesture capture, if required, is scoped to that gesture and released before menu interaction. |
| M06 shallow frequent paths | Continue and mission Retry are direct actions. Options uses visible categories and returns to the last category/focused control. Proposed common options require at most two selections from Menu/Pause. Offer repeated mission/tool-plan shortcuts only when real consumed behavior exists; add no speculative loadout system. |
| M07 explicit Back/Escape | Escape closes the current transient editor/dialog or returns one screen level, preserving valid changes and focus. At the root Pause screen it resumes if safe. Back uses the same rule and a consistent visual location. Explicit Resume is available rather than treating repeated Escape as Quit. Dirty-edit dialogs expose Apply/Discard/Cancel with a safe default. |
| M08 protect the attempt | Pause initially focuses Resume, never Quit. Leaving an unsaved attempt/result requires one clear consequence confirmation with Cancel/Stay as default. Opening/closing Options does not restart, quit or silently resume gameplay. Avoid repeated confirmations when no progress is at risk. |
| M09 prompts from actual bindings | Rebinding updates menu hints, tooltips, tutorials, accessible labels and relevant HUD actions together. Conflict/refusal is visible; keep a keyboard path to recovery/reset. The static board's Space/Enter text is illustrative, not an implementation binding contract. |
| M10 readable over changing art | Use an opaque/quiet text backing, scalable text/layout, focus/pressed shapes plus color, and reduced-motion/static background. Receive contrast with the brightest/darkest scene beats, supported sizes and DPI. Rich background texture must not become text noise. |
| M11 atmospheric but optional audio | Keep the proposed core/assembly/menu tonal identity. Navigation/denial cues have simultaneous visual feedback; mute or missing audio never delays input. Limit repetitive cue volume and avoid playing queued hover sounds after focus returns. |

The chosen flat PC list preserves the approved mockup's hierarchy. Radial or spatial
menu variants require their own device/task evidence before adoption; neither is
declared universally good or bad. This first slice remains mouse/keyboard. Back
and Resume are separate explicit intentions so an unfamiliar player can recover
without learning a hidden shortcut or leaving the attempt accidentally.

Opening Options from active play pauses at a coherent Runtime boundary, cancels
unsubmitted gestures and preserves admitted-command semantics. Deadline/hold ticks
and wall-clock debt do not advance under Options. Returning to the origin Pause
screen stays paused until Resume; returning from main-menu Options returns to Menu.
If an in-game direct-options shortcut is later consumed, record its origin and
return policy explicitly. No new simulation pause semantics are implemented here.

## Proposed native response budget — not a thread finding

For warm, resource-ready menu actions on the named reference device, propose
**event dequeue → first correctly drawn cue with checked native handoff and
observed GPU completion: p95≤33.33ms, p99≤50ms**. Propose event dequeue → ready
destination-screen response p95≤50ms, p99≤100ms for ordinary resource-ready
navigation. Root freezes the budget and precise observer capability before native
collection; these are application acceptance hypotheses, not measured results.

Count every accepted ready-state action, refusal, duplicate activation, stale target
and unhandled event. Separate focus movement, activate, cancel, screen navigation
and intro Skip cohorts; collect at least300 accepted events per measured cohort
under declared warm/cold/render/settings conditions and retain failures/outliers.
Report nearest-rank distributions, event/frame/run identity, service/dequeue and
handoff/completion scopes, queue dwell where clock conversion is valid, and source/
device/driver/toolchain provenance. No retries or hidden animation exclusions.

Loading a recipe, cold asset decode and disk persistence have separate honest
loading/save states and timing; they cannot silently join the warm-action cohort.
Background animation should not add control lockout even when a frame misses its
budget. If a ready-state action is refused for a domain/state reason, preserve its
intent and report the reason; a slow decorative transition is not such a reason.
Without a supported completion observer, report service/native-handoff timing with
its narrower scope rather than claim this GPU-complete budget passed.

These clocks do not establish physical cursor latency, scanout or input-to-photon.
Those need an independent instrumented receiver. Phase14 currently leaves actual
full-frame performance gates open, and no interactive menu is received by this
concept package. Active-world timing or a generated screen supplies no menu latency
proof. Menu measurements require their own executable route and an uncontended
reservation; they do not run alongside the root's Phase14 campaign.

## Receiving protocol — proposed, not performed

**Source/fixture:** expose only consumed Desktop event/view contracts. Feed fresh
activation/cancel events during each transition phase; verify one result per intent,
stable target/focus, no invisible activation and no blanket input discard. Exercise
Skip key hold/release, click-through prevention, stale presses after resize,
repeated Begin, focus loss, dirty rebind, disabled Continue and unsaved Quit.
Snapshot mission tick/ledger before and after Options; assert unchanged semantic
state/debt until explicit Resume. Existing gameplay Escape/fullscreen/Fit/Fuse
bindings require a caller audit before any proposed menu mapping is integrated.

**Playable/native:** first launch, repeated launch and Replay Intro each receive
keyboard and pointer paths. Test navigation while decorative motion is active,
then static/reduced-motion/muted variants. Measure the two response cohorts with
the proposed scope above. Resize/DPI/fullscreen and brightest-background cases
retain target alignment and text readability. Inspect each failed receipt rather
than inferring responsiveness from a fluid background video.

**Participant task study:** propose a small formative cohort of6–8 players with
mixed RTS familiarity and keyboard/pointer habits, testing ordinary and enlarged
text. It is usability iteration, not a representative gamer survey. Give tasks
without describing the desired navigation path:

1. Cold launch, skip when desired and start the offered mission; explain Continue's
   disabled/enabled reason and what it will restore.
2. Relaunch with compatible mission-boundary progress and reach Continue without
   replaying the intro. Repeat the same menu path several times to expose friction.
3. Pause, open Options, remap a currently consumed action, identify the updated
   prompt, return and deliberately Resume. Deadline/hold state must not advance.
4. Change text size and mute/background motion, find the same control again and
   explain selection versus activation versus refusal from visible cues.
5. Back out of nested settings, cancel a mistaken edit and avoid an accidental Quit;
   distinguish Resume from Return to Menu and the unsaved consequence.
6. Retry a lost mission and select the next eligible mission after a won result;
   explain exactly what progress has and has not been saved.

Record task completion, elapsed task time, path/actions, wrong turns, accidental
activation/quit, need for assistance and participant comments about repeat-use
friction, atmosphere and legibility. Treat an accidental destructive action or
unrecoverable focus/control loss as a design defect to fix and re-receive. Report
sample/context and divergent opinions; do not turn6–8 observations into population
percentages or technical latency proof. Actual participant acceptance remains open.

## Asset and plan amendments

The three approved PNGs and their hashes remain unchanged. Their controls illustrate
final static composition; they do not imply moving hitboxes, animation-gated input,
automatic cursor capture, implemented Continue or current keybind semantics.
Add M01–M11 plus native/participant evidence to E02/E03 and J4/J6 receiving. Keep
the full director's artistic ambition while independently receiving the fast
repeat-use route. All new policies remain Crucible-local; this research dispatches
no generic Sub0 input, menu, animation or cursor API.
