# Experience design: clear command, earned progression, safe return

Proposal dated2026-10-09, read against source baseline `f70ce7d` and
[current game intent](../../game-design.md). None of the new menu, campaign,
persistence, skills or Quantum behavior here is an implemented claim.

## Current reality and intended experience

Current source contains a bounded planar arena, silver-wedge concept identity,
spatial field tools, fixed-tick command ingress/replay, finite reclamation ledger,
and a quota/deadline challenge plus optional secure-relay fusion/hold/shatter.
Phase13 P13-01 provides readable HUD/tool controls and fullscreen behavior. Its
sound and participant gates remain open. Phase14 adds a received-source candidate
for complete-frame capture and bounded row execution; the major native performance
and quality gates are not closed by this design package.

The optional relay reference uses2048 fixed identities,1780 reclaimed by tick900,
a120-consecutive-completed-tick hold,64 invested on fusion and48 returned/16 lost
on shatter. Those are existing reference rules, not values retuned by this art.
Growth/contact attrition, terrain mining/bridges, new factions, networking,
campaign persistence and a cinematic renderer/audio route remain future work.

The desired player journey is calm entry → understandable objective → visible
spatial command → inspectable consequence → a concentration/redeployment decision
→ an explained result → a meaningful next mission. A vast flowing swarm matters
because it makes those decisions feel like shaping living material. It is not a
reason to hide failed edits, unreadable fronts or lost resources under effects.

| Journey stage | Player's question | Proposed response and failure behavior |
|---|---|---|
| First launch | What is this, and can I enter quickly? | Skippable nanite cinematic, accessible settings entry, honest loading failure. Subsequent launches open menu. |
| Main menu | What can I do now? | Continue (with exact resume scope), New Run, Missions, Options, Quit. Continue disabled with a short reason when no compatible progress exists. |
| Mission choice | What will I learn and what wins? | Short objective, unlocked/locked/completed state by text+shape, preview consistent with actual mission, Begin/Back. No decorative locked content masquerading as a feature. |
| Briefing | What should I do first? | Objective, failure condition, available tools and one actionable tip; start paused or wait for Begin, with no deadline ticking under the briefing. |
| Active play | Did my input work? | Editable preview, admission receipt, committed field/structure state, clear refusal reason. Current resource and objective values come from the same completed state. |
| Pause/options | Can I plan or leave safely? | Explicit pause state, resume/restart/menu actions, unchanged objective clock while paused. Cancel uncommitted gestures before leaving. |
| Result | Why did this end? | Latched win/loss, quota/hold/deadline evidence, final ledger, Retry/Next/Menu. A terminal screen never changes the terminal outcome. |
| Return later | What survived closing? | Verified mission-boundary progress and compatible next mission. In-progress attempts restart from their declared start until exact restoration is implemented and received. |

No simulation runs behind menu/briefing/results. Transition input has an explicit
owner: Escape cancels a live gesture before opening pause; an intro-skip key is
consumed; Begin is latched once. A submitted command remains distinct from committed
state, including when paused, a queue is full or a domain action is refused.

## Menu, mission and HUD language

Use the [menu concept](assets/menu-missions-v1.png) for hierarchy and material,
not as an exact pixel layout. Keep conventional buttons, a clear focus ring and
readable text on quiet backing. Continue includes a short description such as
“Next mission: Secure the relay” or “Restart current attempt from its start.”
Do not imply a mid-tick checkpoint when only mission-boundary progress is saved.
New Run must explain that it creates a separate attempt; overwriting existing
progress requires an explicit user choice, not a side effect of selecting a card.

Mission cards use a name, objective, available tools, prerequisites and a concise
failure condition. Locked cards name the prerequisite instead of showing only a
padlock. The same objective wording appears in briefing, HUD and results. During
active play, preserve the battlefield's visual dominance: compact ledger strip,
one objective card, tool strip, selected-field inspector and camera/minimap context.
Advanced timings/workers/storage remain in an optional diagnostic view.

Results should show what the player can act on. A deadline loss can say “Reclaimed
X of the required quota before the deadline”; a hold failure identifies the broken
consecutive interval and the last refusal if relevant. Show final mobile, reserve,
anchored and lost material distinctly. Do not score loss as harvested material or
introduce an invented biomass reward. Retry resets the attempt through Runtime's
normal new-run boundary. Next is enabled only by a successfully recorded eligible
completion or a visible unsaved-result policy, never by a UI animation finishing.

## Input and accessibility

Existing desktop references include1 Attract,2 Repel,3 Erase,4 Flow, Tab slot,
Space pause, R restart, F fit or fuse in the structural mode, X shatter, F11
fullscreen and mouse pan/zoom. The F ambiguity is a known design constraint:
proposed rebinding should split Fit and Fuse into distinct labeled actions and
audit every caller before changing bindings. Do not silently change those controls
in a design asset. The generated Space/Enter menu hint applies to proposed menu
navigation only, not the current executable's gameplay semantics.

Proposed input receives keyboard focus traversal and activation, mouse equivalent
actions, visible remappable bindings and conflict detection. Escape backs out one
level; it cancels a preview before offering to leave. Introduce no gamepad promise
until focus/aim/tool interactions have a concrete receiver. A single-player pause
provides time to think; accessibility options must not require a faster gesture
than ordinary play. Route radius/strength values can be edited without wheel-only
input. Do not consume camera gestures when over UI.

Expose UI scale, text size, high-contrast backing, color-independent shape cues,
cursor size, tooltip delay, reduced motion, intro playback, bloom/DOF reduction,
screen shake off (no shake planned), separate volume controls/master mute, focus
loss behavior and reset-to-defaults. Use arrows/inward/outward glyphs for fields,
interlocked rigid form for anchored mass and contiguous crust for Blight; color
is reinforcement. Reserve selectable contrast palettes without tying faction kind
or resource ownership to tint. Never use ambient sound as the only denial cue.

Receive smaller windows, high DPI, fullscreen, resized and ultrawide layouts with
actual buttons and labels, not a scaled screenshot. Text must remain readable at
the supported minimum, avoid clipped objective words and offer keyboard access to
all controls. Screen-reader compatibility is a later explicit platform gate:
large visual labels alone do not establish it. Record unfamiliar-player task
completion, confusion and control recovery separately from automated event tests.

## Staged mission progression

The proposed small curriculum grows decision complexity without immediately growing
simulation complexity. Balance values are frozen independently before a new recipe
starts; concept art numbers and apparent densities are not balance evidence.

| Stage | Teaching purpose | Rule source and proposed gate |
|---|---|---|
| 01 First Current | Observe a route preview, apply a field, cancel a mistaken gesture and inspect visible movement; learn that FLOW routes while Attract gathers. | Onboarding wrapper over existing bounded tools; any completion predicate needs replayed committed receipt/state fixtures and a human comprehension pass. Not yet a shipped mission. |
| 02 Reclaim the Front | Keep mobile mass working while achieving a finite quota before a tick deadline. | Reuse the existing challenge recipe without changing resource math. Freeze descriptor/seed/rules identity; receive menu launch and explained win/loss. |
| 03 Secure the Relay | Gather sufficient mass, invest64 in a lattice, hold while mobile mass reclaims, understand48/16 shatter consequences. | Reuse the current optional relay reference; existing objective/ledger rules remain authoritative. Receive progression into this recipe and terminal outcome. |
| Later: Divide the Flow | Allocate routes between competing fronts/objectives. | Requires a named multi-objective consumer and independently frozen goals; not inferred from multiple visual structures. |
| Later: Pressure/terrain/Quantum | Add one new decision axis after the core journey is playable. | Separate resource/replay/input and fidelity receiving; no omnibus economy or terrain system. |

First sprint implementation should receive **Reclaim the Front → Secure the Relay**
with current rules, mission-boundary completion persistence and correct continue.
First Current can initially be an optional briefing/help exercise without granting
unreceived campaign completion. It becomes an unlocked curriculum mission only
after its concrete predicate is reviewed. Replayable mission descriptors own their
stable ID, recipe/rules version, initial state and available-tool policy; they are
Crucible game data, not new library interfaces.

Mission progression records completion and available next missions. It does not
transfer hidden material between separately initialized arenas. Restarting or
failing does not consume permanent player progress, and replaying a completed
mission cannot repeatedly grant an unlock. A profile reset has an explicit
confirmation and recoverable backup; a new attempt preserves the existing profile
until its own completion transition commits.

## Power-ups and skills: gain agency before gain arithmetic

Prefer earned command literacy and optional tool assists first. Suggested staged
skills are **Flow mastery** (route-edit practice and clearer preview), **Gather
discipline** (eligibility forecast for the current lattice rule) and **Relay timing**
(clear consecutive-hold warning). These can become help/visualization unlocks that
do not alter physics or resource totals. If a visualization forecasts future state,
label it as a prediction with its horizon and assumptions, not committed fact.

Later rule-changing powers should be sparse, mission-local and defined as integer
or bounded finite transitions at completed tick boundaries. Examples worth testing
are a short field-strength pulse or a temporary protection ability. Neither is
selected for the first journey sprint. Before adopting one, freeze eligibility,
duration, maximum concurrent effects, exact command/replay representation, refusal
behavior and balance against a no-power reference. A cooldown in ticks is not a
new conserved resource. It cannot mint nanites, erase lost mass, extend a deadline
unrecorded or depend on render frames. Avoid a broad tech tree, random loot or
run-ending permanent upgrades until the base loop has participant evidence.

At every completed resource-enabled tick preserve the current accounting identity:
**initial total = remaining substrate stock + mobile mass + reserve + anchored mass
+ recorded lost mass**. Reclaimed cumulative amount is a diagnostic, not an extra
bucket. New growth/attrition/permanent construction would need an explicit versioned
extension and hand-calculated failure/conservation oracles. The skill screen may
show names and prerequisites, but must not introduce a second spendable currency
merely because a UI card wants a purchase button.

## Candidate Quantum mechanics

“Quantum” is proposed game fiction and interface direction, not a claim of physical
quantum simulation. Explore one bounded consumer at a time. No new public API or
command variant is dispatched by this table.

| Candidate | Player value | Cost/risk and disposition |
|---|---|---|
| **Quantum Lens: two route hypotheses** | While paused, compare two bounded field arrangements as clearly labeled ghost previews; commit one ordinary field edit. | Preferred concept spike. Uses a copied completed state and fixed horizon; allocates scratch at entry, caps at2 branches, releases before resume. Requires full mission-state copying/replay parity and explicit prediction limitation; never a promise of current implementation. |
| Phase Pulse | Temporarily switch between two declared field intents at a recorded boundary. | More direct skill, but changes gameplay timing/field strength and needs a deterministic duration/limit plus balance. Defer until a concrete rule is frozen. |
| Entangled anchors | Redirect two separated gatherings or lattice-related influence through a linked pair. | Interesting spatial decision but broadens capacity, command ownership and eligibility. No teleporting free biomass; defer until independent ledger/traversal proof. |
| Temporal rewind | Undo a run segment and revisit the same state. | Largest save/state/input/UI burden; potential duplicated rewards and hidden information. Defer. Replay remains an observation/receiving tool, not a player rewind power. |

The preferred balanced extension after the journey slice is a **Quantum Lens
prototype**, with a proposed mission-local budget of two charges, separate from
biomass. Charges are ability uses, not material or a profile currency; the existing
64/48/16 ledger is untouched. Freeze this new attempt-state policy before code.
A successful accepted comparison consumes one charge exactly once; invalid,
canceled-before-acceptance or failed comparison consumes none and publishes no
partial forecast. A coordinator-owned transaction stages the comparison and debit
together at a named paused completed-boundary decision point, recording completed
tick and unique request identity. Repeated input cannot duplicate a debit. If that
boundary cannot be received through Runtime's ownership contract, the first spike
remains a free static preview and does not claim a charged ability. Charges reset
only at new-mission initialization and join future attempt save/replay semantics.

Use existing small reference mechanics: pause, make at most two copied
candidate field plans, run a fixed completed-tick horizon off the live state, show
reclamation/relay forecasts with uncertainty text, then either cancel or submit one
existing command plan. No branch commits live material, no arbitrary branching
search, no background contender during Phase14 benchmarking. A startup-sized budget
and explicit Cancel/unsupported path precede a production proposal. If a copied
full state cannot be received safely, begin with static field-shape previews and
do not present them as simulated futures. Exact branch/horizon limits and copy
costs require actual receiving; this is later stretch, not first-sprint core.
Proposed initial receiving is limited to2048 identities and a120-completed-tick
forecast horizon, never the unqualified150K workload. A no-charge refusal keeps
live state, biomass ledger, charge count and editable intent unchanged.

## Persistence and safe continuation policy

The first core save is a **mission-boundary profile**, not a raw dump of ECS memory.
Propose a small version1 document with format/version, stable mission IDs and recipe
versions, committed completion records, unlocked mission IDs, selected next mission,
input/accessibility preferences and a generation/transaction identity. Bound file
size and record counts before parsing; validate IDs, ranges and mutually consistent
records before mutating any live profile. Diagnostic timing and worker counts are
not game state. A future playthrough save has its own rule/version contract.

Completion persistence is idempotent: a terminal result identity may be committed
once, and retrying a failed disk write cannot duplicate unlocks. Stage the candidate
profile, write a bounded temporary file in the same destination directory, flush
as required by the supported platform, then atomically replace only after complete
validation/write success. Retain a last-known-good backup. The first concrete
storage consumer must receive the actual operating-system replacement/failure
behavior; a proposal saying “atomic” is not evidence of durability under power loss.
No transition claims saved success until its received write result confirms it.

If saving fails, preserve the current playable state/result and prior valid save.
Show “Progress not saved” with Retry and Return to Menu; keep the unsaved completion
in memory visibly for that session. Do not silently remove a win, overwrite a backup
or pretend Continue will restore it after exit. If the user chooses Quit, state the
unsaved consequence. A rejected/corrupt/future-version file is kept intact, with an
explanation and a choice to use a verified compatible backup or start separately.
Do not partially load a newer file or auto-reset it under the same filename.

Compatibility is explicit: unsupported format major version is rejected; only a
tested migration may translate old progress; unknown optional minor-version fields
can be ignored only when the version contract marks them nonsemantic. A mission
rules/seed/descriptor mismatch prevents restoring that attempt and explains the
reason. Cosmetic preference changes can migrate independently of progression.
Never deserialize pointers, executors, audio/video handles or borrowed snapshots.

Continue after a completed mission selects the compatible next unlocked recipe;
after an abandoned active attempt it clearly restarts that attempt from its
declared start. Exact mid-mission continue is deferred until an owned checkpoint
contains every deterministic semantic value: completed tick, scenario/rules/seed,
identity/activity, positions/velocities, fields, infection/stock/ledger, structure/
hold/mission status and relevant runtime command/replay cutoff state. Reconstruct
scratch, spatial indices and platform resources cold. Receive equality to the
uninterrupted run and bounded replay from an independently validated checkpoint.
Simulation's unexpected full-tick failure currently stops without claiming full
rollback; that failed intermediate state is never published as a resume checkpoint.

The missing mid-mission consumer is an application-owned **mission restore
transaction**: validate the complete payload in cold owned storage, construct a
replacement Runtime/Simulation with fresh process-local run identity, rebuild
derived state, then publish/swap only after semantic equality checks. This names
responsibility, not a new C++ API or dispatched module. Saves retain a stable
logical attempt identity; the replacement never reuses an old Runtime run ID or
lets stale input/frame receipts address it. Failure keeps the active run and
previous compatible save untouched.

Future checkpoint schemas need a simulation/rules fingerprint, dependency-relevant
algorithm version, descriptor/seed and semantic FP mode (rounding and denormal/
control policy where it affects results). Do not serialize opaque native fenv
blobs or assume equal dimensions imply equal computation. Reject unsupported or
mismatched semantics unless a reviewed migration proves parity. Bound foreign-data
population/cells/fields/commands/payload lengths and every sum/product before
allocation; verify IDs, finite values, ledger and mission consistency before
construction. Malformed data is a refusal, not a partially replaced run.

First mid-mission checkpoint proposal requires a paused **completed boundary with
no unresolved admitted commands**. Stop admissions, apply/drain eligible already-
admitted commands under normal boundaries, then copy one coherent semantic state;
if quiescence cannot be achieved, refuse and preserve the prior save. The save
operation must not secretly advance a paused mission to clear its queue; offer
an explicit resume-and-pause action or refuse that checkpoint instead. Unsubmitted
gestures/hover intent are canceled and never saved. Any later policy persisting
pending commands must version their cutoff, sequence and application semantics.
Restored attempts start paused with a fresh wall-clock origin and zero accumulated
elapsed debt; time spent closed/loading/menu is not ticks, hold time or deadline
consumption. Physical/GPU receipts from the old run are not resumed-rendering proof.

## Consumer boundaries and unresolved choices

Desktop owns intro/menu/options/focus and visual/audio lifetime. Runtime owns new
run, pause/restart, command boundaries and coherent completed-state observations.
Simulation/domain modules retain math/resource/state ownership. Mission descriptors,
progression, skill policy, save schema and outcome-to-profile transitions remain
Crucible-local application policy. Do not add campaign, Quantum or cinematic APIs
to Sub0 libraries before a named real consumer demonstrates a neutral requirement.

Bottom-up evidence constrains the top-down view: gameplay is currently planar,
mission state copying is not yet a persistence format, audio is unreceived, and
the scale target is not native performance closure. Top-down prototypes expose
actual focus/skip/save/receipt needs rather than inventing a framework. The next
[receiving plan](receiving-plan.md) deliberately meets those directions in a small
end-to-end loop, then reviews whether fidelity and later skills improve it.
