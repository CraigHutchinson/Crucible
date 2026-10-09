# Director's brief: a collective becomes CRUCIBLE

Proposal, 2026-10-09. Target twelve seconds, acceptable authored range ten to fifteen
seconds. The visual ambition is a cinematic production title, not a generic cloud
of points writing text. A single recognizable silver machine becomes coordinated
living material, establishes the game's material identity, then leaves room to play.
The selected [hero](assets/intro-hero-v1.png) and
[storyboard](assets/intro-storyboard-v1.png) are generated reference stills only.

## Dramatic structure and continuity

The opening is quiet enough to reveal tiny mechanisms. Coherence grows before
spectacle: a core wakes, others respond, lanes form, mechanical pieces align, the
title resolves. The final emotional beat is confidence and agency rather than
an explosion. No threatening narration or external lore is needed to understand it.

| Time | Shot and action | Camera, light and sound | Exit proof |
|---|---|---|---|
| 0.0–1.8s / 01 | One silver asymmetric wedge rests on dark ceramic. Its recessed cyan core wakes once; a narrow seam catches light. | Macro three-quarter view, shallow DOF on the core and front bevel. Soft white grazing key. Very quiet mechanical contact, then a short low pulse. | An identifiable machine, not a spark, is visible before collective motion. |
| 1.8–4.2s / 02 | Neighboring wedges answer in a traveling sequence and converge in two or three lanes. Units follow continuous paths and maintain purposeful spacing. | Smooth pullback along the substrate, no roll or handheld shake. Cyan glints travel with the units. A widening granular layer plus a restrained tonal rise. | Same wedge silhouette and core placement throughout; no random burst or teleport cuts. |
| 4.2–6.8s / 03 | Lanes branch into letter strokes; pieces rotate and interlock. The negative spaces are planned in advance, especially C, R, B and E. | Camera rises toward a nearly frontal baseline. Focus depth increases; cool broad key reveals brushed silver. Sparse tactile clicks follow authored locking beats. | Title geometry is a fixed authored mask; the readable silhouette emerges before the final hold. |
| 6.8–8.4s / 04 | Last units settle; exact uppercase CRUCIBLE becomes fully readable. Cyan internal light gives a single low-amplitude traveling accent. | Near-front view, no distracting perspective compression. One amber reflection nods to fused lattice material. Soft bass arrival with a short decay, no full-screen flash. | All eight letters sharp, open counters, stable baseline, intentional spacing. |
| 8.4–10.0s / 05 | Hero hold: silver surface, cyan cores, a few grounded foreground units. Surplus units form a quiet ribbon toward the future menu backdrop. | Tiny dolly drift or a still frame; title never swims. Controlled specular sweep only. Tone resolves, transient layer decays. | Wordmark readable in grayscale, at small window size and with bloom disabled. |
| 10.0–12.0s / 06 | Camera opens to the same industrial plate. Title moves to the menu anchor; the background ribbon settles and controls appear. | Gentle high-oblique rise, title and UI use stable screen-space anchors at handoff. Ambient bed crossfades if available. | Focus goes to Continue if compatible progress exists, otherwise New Run; no hidden input delay. |

The menu background is a presentation scene, not a live mission simulating in the
background. It must not consume gameplay ticks, reclaim biomass or mutate a saved
run. The title formation is artistic representation of cooperation, not a claim
that gameplay has a fusion recipe spelling letters. The canonical wordmark is
**CRUCIBLE**, with no slogan; an emblem can be a later independently reviewed asset.

## Camera and composition

Use one physical stage and a coherent path. Macro lens language establishes scale,
then widens as the collective comes into view. Avoid a series of unrelated dramatic
cuts. The hero remains nearly frontal enough for instant reading; use perspective
for foreground depth and specular surfaces, not to distort typography.

The proposed virtual lens progression is macro/85mm-equivalent to 40–50mm-equivalent
hero and a high-oblique tactical view. These are artistic starting points, not
engine camera API requirements or measured settings. Keep the title inside a central
safe region when adapting 16:9 to other aspect ratios. At 4:3 or an ultrawide window,
letter visibility takes priority over preserving every decorative edge of the stage.
Never stretch the wordmark. UI composition receives responsive layout separately.

DOF is concentrated in shot01 and nearest foreground units. The hero letters and
all menu text stay sharp. No roll, screen shake, whip pan or rapidly changing focus.
No camera motion may continue after reduced-motion mode selects a still menu.

## Material and light specification

The silver wedge is an engineered low asymmetrical triangular body, with distinct
beveled planes, a recessed protected cyan core and dark mechanical seams. It is
neither a flying fighter nor a humanoid. The macro version may show brushing and
micro-scratches; tactical LODs preserve silhouette and core location. Letter bodies
and foreground units must share the same material, scale logic and construction.

Proposed PBR authoring ranges are references to refine in the selected renderer:
silver metalness near1 with roughness roughly0.25–0.45; graphite ceramic metalness0
with roughness roughly0.6–0.9; recessed core emission kept below clipping through
the chosen exposure. No new shader/renderer feature is dispatched by these numbers.
Validate actual color management and SDR appearance before publishing final assets.
HDR is an optional future receiving mode, not inferred from bright concept PNGs.

Use a broad cool-white grazing key to read bevels, a narrow cyan edge contribution
motivated by cores and a faint amber reflection associated with lattice material.
Deep blacks retain object separation. Speculars need smooth controlled shapes,
not glitter across every wedge. Bloom should reveal small core emission without
connecting unrelated objects or dissolving title counters. Blight red/magenta
appears only in the final environment and menu preview, not as a competing title tint.

The generated storyboard contains raised relief and taller amber structure forms.
Production menu art should lower those forms and keep the inhabited gameplay patch
flat. Decorative seams must never imply an impassable channel or functional bridge.
Contact shadows sell nanite grounding; restrained surface reflections echo material.
No fog, massive sparks, chromatic aberration or lens flare should obscure the logo.

## Sound direction and receiving boundary

Begin with near-silence, a tactile core pulse and small mechanical contacts. Build
one quiet granular layer with a tonal rise as lanes coordinate. The title settles
with one restrained low arrival and a clean resolved note. A low ambient bed can
continue into the menu. No per-nanite sound instances, procedural audio framework,
spoken exposition or licensed track is required for the prototype.

P13-02 audio is still undispatched in the source design baseline. Sound cues here
are proposed direction, not implemented playback or listening evidence. A first
concrete Desktop audio consumer should preload owned clips, use one bounded ambient
layer and a small bounded transient set, drain/stop before source destruction and
have an immediate silent failure path. Missing audio never blocks Skip or Begin.
Exact mixer/device limits belong to that received consumer, not this concept board.

Music, effects and interface volumes should be independent, with master mute
available before the sequence. On Skip, fade current cues promptly without a new
arrival sound. Focus loss pauses or silences according to the tested application
policy; it must not blast the resolved note on return. If any narrative cue is
introduced later, provide text and subtitle timing independently of playback.
Human listening at low volume/headphones and ordinary speakers is a separate gate.

## Skip, replay, failure and reduced motion

On a first launch, make Skip visible from the first presented frame. Escape, Enter,
Space or clicking Skip should reach an interactive menu immediately; initiating
input is consumed so it cannot also select New Run. Holding a key or repeated
events may not repeatedly enter states or start a mission. Later launches go to
the menu by default; Replay Intro lives in Options. Persisting that preference is
best effort and cannot make the application unlaunchable.

Reduced motion selects the hero still and a short opacity transition or immediate
menu handoff, with no lens drift, assembly movement, parallax or looping background
motion. Also expose Intro Playback (first launch / always / off) independently.
The first-run accessibility entry must be reachable without sitting through the
sequence. An audio mute must remain effective through replay and skip.

Loading actual application resources is not disguised as cinematic progress.
If construction is incomplete when Skip is pressed, show a truthful bounded loading
state with failure/cancel path, not an inert menu. Device/video/asset failure falls
back to a plain title and accessible menu. A resize, DPI change, focus loss or close
at any shot must preserve state ownership and immediately cancel pending transitions.
Gameplay never begins just because the sequence's timer elapsed.

## Fidelity and performance tiers

These are proposed budgets for a separate cinematic prototype, not measured costs
or a Phase14 performance default. All tiers use the same timing, typography and
transition contract. Choose only after receiving on the actual target device.

| Tier | Prototype treatment | Proposed bounded working set | Selection and fallback |
|---|---|---|---|
| Static / reduced motion | Authored still, flat wordmark and quiet menu surface; no DOF/bloom or animated swarm. | One selected image plus UI; decode at startup, no per-frame file reads. | Always available; mandatory failure and accessibility path. |
| Balanced | Pre-rendered sequence or a small received live title scene, restrained lights, stable menu composition. | If video: one stream and at most3 owned decode/presentation frames, proposed1920×1080 ceiling. If live: at most4096 visual wedge instances, no gameplay ECS. | Select one concrete route after a capability spike; never silently assume codecs or live PBR exist. |
| Cinematic | Higher-detail macro geometry, controlled DOF, bloom and denser letter edges; same bounded scene. | Live stretch at most16384 visual instances and fixed startup buffers; a4K pre-rendered option only after explicit memory/decode receiving. | Manual opt-in while evaluated; no AUTO promotion from generated art or one smooth clip. |

For a receiving prototype, propose a60Hz display target with presentation-inclusive
p95 at most16.67ms and p99 at most20ms on the named device, plus input-to-menu
response measured over skip tests. A pre-rendered30fps source can be deliberate
film cadence, but must be identified as30fps content; presenting it at60Hz is not
60fps generated motion. Record decode/upload/UI/GPU completion scopes and process
memory separately. Do not run these captures alongside Phase14 worker measurements.

A pre-rendered clip gives stable authored fidelity without a new live shader path,
but requires actual codec/license/assets/decoding lifecycle receiving. A live scene
permits clean aspect adaptation but exposes geometry, shading and GPU costs. Start
with a still-to-menu prototype and one bounded route comparison. Promote the route
whose actual fidelity, skip behavior and performance meet the contract. The menu
and mission journey ships independently if the full cinematic route remains open.

## Director's acceptance pass

Check eight-letter spelling, silhouette, material identity, letter spacing and
negative spaces at hero scale and small window size. Review the full temporal
sequence for grounded motion, purposeful lane choreography and a calm handoff;
still images cannot establish those. Check bloom off, grayscale, reduced motion,
muted audio, no audio device, resize/focus/close at every beat, replay and repeated
skip input. Record actual artifacts with source/timing/device provenance.

Ask an unfamiliar viewer what they think they will command, whether the intro feels
worth viewing once and whether they can skip and start play confidently. Record
participant observations separately from source fixtures and native capture.
