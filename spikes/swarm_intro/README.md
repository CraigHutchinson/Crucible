# Nanite swarm intro spike

Headless, deterministic prototype of the CRUCIBLE title sequence's swarm behaviour: building
blocks S0 to S0n of the
[nanite swarm design](../../docs/concepts/phase15-experience/nanite-swarm-design.md).
It answers a narrow question: **can a few thousand view-owned nanites perform the
director's beats with intelligent-looking behaviour near the camera, cheap non-penetrating
motion elsewhere, and a deterministic outcome that does not depend on the level of detail?**

User direction, 2026-10-12:
- The final title **stands upright**; a ground-based build is acceptable for early milestones.
- Units decide their slot when they reach the base, in arrival order, with no preset order.
- They work through conflicts by queueing, waiting and selective magnetism.
- The sequence may run to **15 s**.
- Tiles interlock into a solid join.
- Behaviour is recorded through **Sub0Log's JSONL diagnostics**.
- Climbing is closest to the intent, but the drawbridge stays available to combine with it
  (the hybrid build).
- Nanites morph their triangle a little so complex outlines come out smooth.
- Each iteration is pushed and merged.
- 2026-10-13: play the intro at about **66% speed** so the climbing reads (the default pace
  is 2/3); render videos as MP4.
- 2026-10-13: consider **3D tessellation** (S0i, `--layers`).
- 2026-10-13: the nanite becomes a **3D shape like the original concepts** (S0j).
- 2026-10-13: a **cinematic camera**: close up, panning along and around the build so the
  construction reads, then out to the whole logo; the length may grow (S0l).
- 2026-10-13: every discrete LOD tier must converge in isolation, for the whole swarm, as a
  quality gate; decision LOD by activity, the camera changing only presentation fidelity;
  later, off-screen units reach any goal within a reasonable equality to the high-LOD logic
  (S0m).
- 2026-10-13: LOD has several drivers: camera focus for the actor fidelity of motion and
  collision, activity for complex interaction; their levers dynamic and adjustable, camera
  fidelity variable to meet a tick budget; heroes on an interleaved duty cycle (a System 1
  every tick, a System 2 at a lower cadence); presentation interpolated or extrapolated to
  the display rate (S0n).
- 2026-10-13: consolidate the design: the [nanite design space](../../docs/concepts/nanite/README.md)
  owns the unit's form; the body here is a proxy of it (S0k).
- 2026-10-13: moving nanites are elongated: the core never changes, the shell reshapes (S0k).
  Backlog: organic, freeform construction (design section 5, O-stages).

**Times are choreography seconds** (the authored beats) unless marked "on screen". At the
default `--pace 0.6667`, each tick shows 1/60 s but advances the choreography by 1/90 s, so a
viewer sees everything 1.5× longer. While the camera tours the build close up (S0l) the pace
eases to `--close-pace 0.5`, twice as long on screen, and the sequence ends with a 1.5 s
pull-out and hold: the 15 s climbing sequence plays over 28.8 s, the 12 s ones over 22.8 s.

**Decision LOD blends camera focus and activity** (S0n, `--lod blend`, the default). Each
mover scores `(viewWeight x view + activityWeight x activity) / (viewWeight + activityWeight)`:
- *view* is the camera driver, actor fidelity of motion and collision where the viewer looks:
  0 off screen, up to 1 at the frame centre at 10 or more pixels per unit;
- *activity* is the interaction driver: stuck 1.0, pushing 0.95, docking 0.85, climbing
  0.75, queueing 0.65, seeking 0.55, lane and ribbon 0.15.

A score of `heroMin` (0.55) or more makes a hero candidate (the best `heroCap`, 128, win),
`nearMin` (0.30) or more the near tier, anything else far; off screen, calm units (lane,
ribbon) go hidden and building units never do. The levers (`LodLevers`) are dynamic:
`SwarmIntro::setLevers` changes them between ticks.

**Two brains per hero.** System 2, the sampled velocity-obstacle deliberation, runs every
`heroPeriod` ticks (2), staggered by identity. Between deliberations System 1 holds the last
decision as a turn and speed scale relative to the current desire and separates reactively,
every tick. Cheap tiers do the same for calm units' goal steering (`routePeriod`, 1 unless
the budget stretches it).

**A decision budget** (`--budget`, cost units = thousands of instructions per tick, off by
default) is held by a deterministic controller: from the tier populations it predicts the
tick's cost and lowers a fidelity factor when over budget (a smaller hero cap, a higher near
threshold), and below half fidelity it also stretches the route cadence and lets far units
off screen skip the resolve pass. It never reads the clock, so runs stay replayable.

`--lod camera` (on-screen only) and `--lod activity` (activity only) remain as presets.

**The camera** is a stand-in for the brief's lens path:
1. A macro on the first unit to wake.
2. From 2.2 s, a follow camera: the head of the stream until building starts, then the
   busiest part of the title (docking, climbing and just-locked units, with hysteresis so it
   holds a section until another is clearly busier). The close-up is about two letters
   wide and, in the oblique view, turns from one side of the title to the other as it moves
   along it (frame headers carry its yaw; the renderer follows it).
3. Four seconds before the sequence ends, the pull-out to the whole title, the hold and the
   opening toward the menu.

It is not production rendering, audio, a gameplay system or a native performance budget.
The images are concept2D/dev-test captures through stand-in cameras: top-down orthographic
for the lying build, and an oblique front view (screen height = z + 0.4 × depth) for the
upright builds.

## Five builds

| `--build` | Stage | How the title forms | Title complete (1536 units) |
|---|---|---|---|
| `lying` (default) | S0 | Units assemble the title flat on the substrate, each with a planned slot. | 7.71 s |
| `raise` | S0c | Each letter assembles flat behind its baseline. As soon as its last unit locks, the interlocked letter hinges up on its bottom row like a drawbridge, letters rising one after another. | 7.78 s flat, upright 8.83 s |
| `climb` | S0b, S0d | Letters grow upright in place, bottom-up. No preset slots: units queue at their letter's base and claim slots first come, first served. They climb the built face with lift magnetism, and recover from getting stuck by waiting, pushing or reclaiming. Uses the 15 s sequence. | 10.85 s |
| `hybrid` | S0e | Whole letters both ways. Letters whose build depth is above the median (the climb's critical path) assemble flat and swing up as drawbridges; the rest climb. Automatic choice here: C, R, C and E raise; U, I, B and L climb. `--raise-letters MASK` chooses by hand (bit 0 = first C). Uses the 15 s sequence. | 9.00 s; raised letters upright by 7.99 s |
| `scaffold` | S0f | Per-part drawbridges, combined with climbing. Tall, straight, upright strokes standing on the baseline become drawbridge panels: the R's stem and leg, the I, and the B, L and E stems. They assemble flat and swing up first as scaffolds, and the swarm then climbs them to finish the arms and bowls. Deep letters without such a stroke (the two Cs) raise whole; the U climbs. A climber starts up once a path from the ground reaches its slot without crossing a panel still lying down. Uses the 15 s sequence. | 10.57 s (19.9 s on screen); all 8 panels upright by 7.93 s |

The climb build runs on two connected surfaces: the ground and the wall face. The wall face
is stored as a second interaction surface, offset so the two never interact, and joined to
the ground along the wall's base. That is the "surface" interpretation of upright
assembly.

## The nanite body (S0j, S0k)

What a nanite looks like is owned by the [nanite design space](../../docs/concepts/nanite/README.md):
a compact silver tetrahedral shell around one rigid tetrahedral core. This spike renders a
low-poly **proxy** of that direction; it is not the approved rig. `NaniteBody.hpp` holds the
values the simulation and the renderer share:
- **height** (0.55 d) is one stacking layer: the rest-layer lift for climbers and the depth
  of a tessellation layer. The shell's apex stands at this height;
- a **free unit's footprint** is an equilateral triangle (`span`, 1.1 d); a **locked unit's**
  is its morphed tile, so neighbouring shells share base edges and a letter stays gap-free
  (a disclosed docking envelope, not a claim that the articulated unit tessellates);
- the apex **leans** toward the nose so the heading reads;
- the **core** is one regular tetrahedron, base down under the apex, its edge 0.40 of the
  shell's base edge, clamped to stay 0.04 d inside every face;
- a **moving** free unit's shell **stretches** with speed (`stretch` 0.9 of the span at
  `stretchSpeed` 24 d/s, eased): the nose reaches ahead while the rear edge, the apex and the
  core stay. The stretched shell contains the compact one, so the core stays enclosed and
  unchanged; locked units never stretch. Captures carry each unit's speed.

The renderer (`nanite_mesh.py`) splits each shell face into graphite vertex hubs, silver
edge rails and shield segments recessed around a window, view-owned decoration. All four
windows open onto the same core; a locked unit's core settles to a steady glow, subordinate
to the silver. It builds every body in one batched pass, lights it with a cool grazing key
and a controlled highlight, culls back faces and paints bodies far to near: the shell's
inside, then the core, then the shell's front. Detail is levelled by on-screen size: meshes
with open windows from 6 px per unit, hubs, rails, recessed shields and seams from 14 px;
silhouettes below 6 px and points below 3 px. Not attempted: articulated fingers, opening
shields, the oblique three-point rest, the underside, the reactor housing and crystal, and a
gradual morph as a free unit seats.

S0j's first body was the original concepts' arrowhead dart; the design space's review of
PR #43 superseded it. Swapping the proxy in changed no simulation state: every state hash
is identical. `test_nanite_mesh.py` checks the proxy's invariants (base is the tile, shared
base edges between neighbours, apex one layer up and leaned toward the nose, outward faces,
one regular core inside the shell with clearance, a moving shell stretching around the
unchanged core, parts covering each face around one window, shields recessed) and that its defaults match `NaniteBody.hpp`.

Evidence images are PNG sheets stored through Git LFS (256-colour palette, tens of
kilobytes each), not SVG: mesh frames made vector sheets of up to 2 MB.

## 3D tessellation (S0i)

`--layers N` gives the title depth: N layers of the same tiling, each one body height (0.55
diameter) in front of the last, with every odd layer on a lattice shifted half a tile side.
Each wedge then overlaps two or three wedges of the layer behind, a staggered bond like
brickwork. Overlapping wedges of adjacent layers are linked in the build graph (face
links), so the one support rule now spans three axes: a tile needs the ground or a locked
neighbour of lower build depth, and a front tile can be held by the face behind it. Flat
assembly seats a front wedge only on a locked wedge of the same flat body. Lane width and
each letter's waiting band scale with the layer count, and a multi-layer letter's waiting
climbers heap and flow over each other. With one layer, every state hash is unchanged.

Two layers need about twice the units: `--layers 2 --count 3072`. The renderer's `--yaw`
turns the oblique camera so the depth shows; deeper layers are shaded darker, and locked
tiles are drawn with their edges.

## Tessellated title (solid join) and morphing tiles

Slots are equilateral triangles of side √3 diameters (each one's inscribed circle is one
unit). They sit on a triangular lattice clipped to the letters' strokes and alternate up and
down, so neighbours share edges and a finished letter is one solid, gap-free face. Square
stroke caps make every letter stand on the baseline with a flat edge. A tile looks the same
every 120°, so a unit seats at the nearest equivalent heading: flipping between an up tile
and a down tile costs 60°, not 180°.

**Morphing.** When a unit locks, it morphs its triangle into its tile's shape so the letter's
outline follows the stroke instead of a saw-tooth:

1. **Fill**: a notch between two included tiles in the same row is filled.
2. **Trim**: one pass drops any tile held by a single edge, a spike. Ground-row tiles stay.
3. **Corner moves**: each lattice corner on the outline (touching an excluded tile) moves
   toward the true stroke outline, by at most `--morph` × side (default 0.35). Ground-row
   corners stay on the ground.
4. **Quality**: any tile below half its equilateral area scales back its corners' moves
   until it recovers. The moves belong to corners, not tiles, so every tile sharing a
   corner moves it identically and the joins stay seamless.

Result: 1,066 tiles; the 408 outline corners' RMS distance from the true outline falls from
0.624 to 0.308 units (−51%). The largest corner move is 0.61 units and every tile keeps at
least 51% of its area. `--morph 0` keeps equilateral tiles. `--rows` restores the earlier
stroke-row slots for comparison with the lying build only; climbing is tuned for tiles.

## What it does

A view-owned Sub0ECS world holds `Identity`, `Pose`, `Motion` and `Mind` components plus one
LOD tier tag (`HeroTier`, `NearTier`, `FarTier`, `HiddenTier`). Each tier tag is a separate
partition, so a tier's system iterates only that tier. Systems run in a fixed order on a
frozen tick-start gather. Tier migration, a structural change, happens only after every
system has finished. Serial systems (claim, negotiation, contingency) run in a fixed
arrival or identity order, so every build is deterministic.

| System | Behaviour | Prior art |
|---|---|---|
| Planner (load time) | Predicts the answer wave by BFS hop count from the first wedge, and splits units over three lanes by bed position. Lying and raise: pairs wake order with far-first slot order. Climb: gives each unit a destination letter only. | Kilobot seed gradient; CAPT goal assignment |
| Build graph (load time) | Links tiles sharing an edge or a vertex. Gives each tile a depth from its letter's ground row and records each letter's footprint on the ground. | TERMES-style structure compilation |
| Signal | A dormant unit wakes once a neighbour has been awake for one hop delay: the traveling answer of shot02. | Kilobot gradient |
| Claim (climb) | Units queued at a letter's base claim slots in arrival order. A slot is claimable once it could soon be supported: on the ground row, or with a lower neighbour locked or claimed. Picks the lowest such slot nearest the unit; at most `claimCap` (96) claims in flight per letter. | First come, first served; Kilobot local decisions |
| Behaviour: Hero | Sampled velocity obstacles against senior neighbours (right of way by wake order). Turns toward its goal before moving (anticipation), climbs over resting units, docks with a ritual. At most `heroCap` units. | RVO / ORCA, Disney animation principles |
| Behaviour: Near / Far / Hidden | Reactive separation; O(1) density gradient; analytic goal motion. All respond to a neighbour's push magnetism. | Boids, continuum crowds, crowd LOD |
| Build modes | Each tile is climbed, or belongs to a drawbridge panel (a whole letter, or one stroke in the scaffold build), or lies flat. Panel tiles use planned slots and hinge up together. A tile is wall support only once it **stands**: climbed tiles on locking, panel tiles when their panel is upright. Climbers route over standing tiles at their standing positions, step onto the wall only beside a ground tile they can stand on, and do not step on before anything in their letter stands (unless their own tile is on the ground row). Climbed tiles above a panel can be claimed while the panel is still flat, so their units are waiting at the base when it swings up. Negotiation never swaps across modes or panels. | Scaffold-first construction |
| Climb (climb) | Walks to the base under its slot and steps onto the face wherever it meets the base. Routes over built units by shortest path, with the ground linking strokes; waypoints keep momentum. Clings to a built neighbour's edge beside an unsupported slot. Slides in once a lower neighbour is seated, and locks only on a locked support. | M-Blocks, Big Hero 6 microbots |
| Negotiation | Neighbouring seekers in the same lane swap slots when that lowers summed squared distance (climb: same depth only, owners updated). | d-CAPT |
| Contingency | Detects a stall (no progress for 40 ticks) and chooses a response (next table). | Freezing-robot avoidance; multi-robot deadlock recovery |
| Resolve | Eight Jacobi position-projection passes over visible ground movers. Resting units form a layer movers glide over. Climbers on the face, and seekers heaped within 4 units of the wall's base, flow over each other in layers instead of colliding. | PBD; Big Hero 6 microbots sliding over one another |
| LOD | Tier per unit from the scripted camera, through each build's view projection. Hero near the focus (capped), near or far by screen size and radius, hidden outside the view plus margin. Six ticks of hysteresis; leaving hidden is immediate. | Crowd LOD |

Acts: lying and raise title units go `Dormant → Waking → Lane → Seek → Dock → Locked`.
Climb units go `… → Lane → Seek → Queue → (claim) Seek → Climb → Dock → Locked`. Ribbon units
go `Waking → Moor → Dock → Moored`; surplus units go `Waking → Reserve`.

### Cooperation when things go wrong

| Situation | What the unit decides | Event |
|---|---|---|
| Arrives at its letter's base | Joins the letter's queue; a slot is claimed in arrival order | Claim (rank, queued ticks) |
| No progress for 40 ticks while a senior unit beside it is still moving | **Wait**: it is queued behind that unit, so it keeps its place | Stuck, contingency 1 |
| No progress and nothing senior moving nearby | **Push**: repelling magnetism for 30 ticks; juniors and idle units within 2.5 units step aside | Stuck, contingency 2; Magnet push on/off |
| Still stuck after three times as long | **Reclaim**: a seeker releases its slot and rejoins the queue; a climber swaps to the nearest supported free slot | Stuck, contingency 3 |
| Moves again | The episode ends | Recovered (stuck ticks) |

Progress is measured per act: lane progress plus closing in on the lane, or distance to the
current aim. Units that are legitimately waiting (queueing, resting, climbers whose slot has
no support yet) are not tracked.

### Selective magnetism

| Mode | When | Effect |
|---|---|---|
| Lift | A climber in contact with the built face, or clinging beside its slot | The structure draws it up: climb speed × 1.6 |
| Push | Push contingency | Neighbours are repelled to clear a path |
| Seat | A climber whose slot is supported and within capture range | Pulled into the slot until it locks |

## Sub0Log diagnostics

`--events-jsonl FILE --health-json FILE` records the run through a bounded in-memory Sub0Log
segment (4 MiB, allocated once; recording never allocates, formats or locks). After the last
tick it is exported as upstream schema-1 JSONL plus one `swarm_health` receipt. This follows
Crucible's runtime diagnostics pattern: Sub0Log owns encoding and provenance; the spike owns
the event meaning.

Every event's arguments are unsigned 64-bit: `[app_schema=1, event, tick, …]`.

| Event | Remaining arguments |
|---|---|
| 1 claim | unit, slot, letter, depth, arrival rank, queued ticks |
| 2 lock | unit, slot, letter, depth, mode (0 planned, 1 claimed) |
| 3 stuck | unit, act, stalled ticks, contingency (1 wait, 2 push, 3 reclaim) |
| 4 recovered | unit, act, stuck ticks |
| 5 magnet | unit, mode (1 lift, 2 push, 3 seat), on |
| 6 letter | letter, kind (1 complete, 2 upright) |
| 7 summary | slots, locked, moored, claims, stuck, recovered |
| 8 supported | slot, letter, depth, owner (or u64 max), owner act, owner distance (milli-units) |

Acts are numbered 0 dormant, 1 waking, 2 lane, 3 seek, 4 climb, 5 dock, 6 locked, 7 moor,
8 moored, 9 reserve, 10 queue.

`verify_diagnostics.py` is an independent receiver, run by CTest for each build. It runs the
spike, parses every line with Python's standard JSON parser and checks:

- The receipt is valid and complete: no producer drops or truncation, no damage, and a
  record count that matches the JSONL.
- Every record is a schema-1 `swarm_intro` message with unsigned 64-bit arguments, in tick
  order.
- One lock per slot and unit, matching the summary event and the run's own JSON line.
- Eight letter-complete events when the title is complete.
- Claim, stuck and recovered counts match the summary; every locked slot was claimed first
  (climb); no queue wait exceeds the sequence.
- Each unit's magnet modes alternate on and off.

The diagnostics were also the main debugging tool for S0d. The `supported` event showed that
an owner was a median 6 units from its slot when the slot became supported. Frame traces then
showed climbers braking at every waypoint. Removing that took the climb from never finishing
to 11.8 s (with the earlier 1,104 unmorphed tiles).

## Build and run

```sh
cmake --preset release -DCRUCIBLE_BUILD_SWARM_SPIKE=ON
cmake --build --preset release --target crucible_swarm_intro
ctest --preset release -R swarm_intro      # 7 --verify cases, 6 diagnostics receivers, the mesh test
./build/release/spikes/swarm_intro/crucible_swarm_intro --build scaffold \
    --events-jsonl events.jsonl --health-json health.json --frames frames --every 1
python3 spikes/swarm_intro/render_swarm.py frames --sheet beats.png --magnets magnets.png \
    --times 6.0,7.0,7.8,8.6,9.6,11.3 --mp4 intro.mp4     # MP4 needs ffmpeg on PATH
```

Options: `--build lying|raise|climb|hybrid|scaffold`, `--raise-letters MASK`, `--morph F` (0.35), `--rows`, `--count N` (1536), `--hero-cap N` (128),
`--claim-cap N` (96 per depth layer), `--layers N` (1), `--lod blend|camera|activity|hero|near|far|hidden` (blend), the levers `--view-weight W`
(0.5), `--activity-weight W` (1), `--hero-min S` (0.55), `--near-min S` (0.30),
`--hero-period N` (2), `--route-period N` (1), `--budget C` (off), `--pace P` (0.6667; 1 is the original
speed), `--close-pace P` (0.5 during the close-up tour; equal to `--pace` for a constant
pace), `--seconds S` (on screen; default the build's sequence: 12 s of choreography, 15 s
for the climbing builds, plus the 1.5 s pull-out), `--metrics FILE`.
Renderer: `--tiers`, `--magnets` (cyan lift, amber push, white seat), `--gif`, `--gif-mode`,
`--mp4` (H.264 at the capture's on-screen rate, 60 fps with `--every 1`; `--mp4-mode`,
`--mp4-size 1280x720`, `--hold 2` seconds on the last frame), `--yaw DEG` (oblique view
turned; default the camera path's yaw, 0 with `--zoom`), `--zoom cx,cy,half_width`, `--fps F`
(GIF/MP4 at F frames per second, interpolating between captures; `--extrapolate`
dead-reckons from the latest capture instead) and `--reconstruct STRIDE` (position error of
rebuilding every frame between every STRIDE-th capture, both ways).
Sheet `--times` are choreography seconds; frame labels also show the time on screen.

## Acceptance (`--verify`)

The lying and raise timing gates come from the director brief and were written before
tuning. Gates set or changed after a measurement are marked †. Gate times are choreography
seconds, so they scale with the pace.

- **All builds:**
  - No lock without a locked support.
  - Morphing (tiles): no tile below half its area†, and outline error cut by at least 40%†.
  - Hero tier within its cap; ribbon fully moored by 10.0 s.
  - Diagnostics export valid, complete and undamaged.
  - No unit still stuck at the end.
  - Two identical runs produce the same state hash.
  - **Isolation:** the other LOD mode, and each discrete tier on its own for the whole swarm
    (hero, near, far, hidden), lock (and raise) the whole title by the end of the build's
    sequence (12 s, or 15 s when climbing), with every lock supported and no unit left
    stuck; hero, near and far also keep the contact gates (hidden is never drawn). The
    hero isolation deliberates every tick (its full logic). The spread of their completion
    times is reported, not gated (finding 24).
  - **Dynamic levers and budget** (S0n): a blend run whose levers switch mid-run (camera-led
    from 4 s, then activity-led with System 2 every 3 ticks from 8 s), and a blend run held
    to a budget of 4,000 cost units, both converge with the visible contact gates.
- **Lying and raise:**
  - Title at least 99% locked by 8.4 s and 100% by 10.0 s.
  - Visible mover contacts (centres closer than 0.8 diameter) at most 3%† of visible movers
    per tick, and no visible pair closer than 0.5 diameter.
  - Raise: every letter upright by 10.0 s.
- **Hybrid and scaffold:** every drawbridge panel upright by 13.5 s†, plus the climb gates.
- **Two layers** (`--layers 2 --count 3072`, climb and scaffold in CTest): every gate above.
- **Climb** (15 s sequence):
  - Title complete by 12.5 s, leaving a hold and the menu handoff by 15 s.
  - At most 0.5%† of climber-ticks off the built face. Attached means touching it: within a
    tile's circumradius plus one unit diameter of a built tile.
  - Visible ground contacts at most 3%† per tick, and no visible pair closer than 0.5
    diameter† (tightened from 10% and 0.3 when lane branching removed the exit crowding).

## Evidence, 2026-10-13 (blend LOD, System 2 every 2 ticks, tour camera)

Host: shared 4-vCPU Intel Xeon 2.80 GHz cloud container, GCC 13.3, single thread, serial
runs. Costs are advisory host wall time per presented tick (1/60 s on screen) with
diagnostics excluded, not a device budget. 1,066 morphed tiles. Times are choreography
seconds; on screen the tour plays slower (12 s builds run 22.8 s on screen, 15 s builds
28.8 s).

| Run | Title complete | Upright | Worst contacts / min gap | Stuck → recovered | Tick p50 / p95 |
|---|---|---|---|---|---|
| Lying, 1536 | 7.71 s | – | 0% / 0.96 d | 0 → 0 | 0.41 / 1.09 ms |
| Raise, 1536 | 7.78 s | 8.83 s | 0% / 0.94 d | 0 → 0 | 0.37 / 1.44 ms |
| Climb, 1536 | 10.85 s | 10.85 s | 0% / 0.94 d | 36 → 36 | 0.64 / 3.42 ms |
| Hybrid, 1536 | 9.00 s | 7.99 s (raised) | 0% / 0.83 d | 2 → 2 | 0.46 / 2.71 ms |
| Scaffold, 1536 | 10.57 s | 7.93 s (panels) | 0% / 0.83 d | 25 → 25 | 0.79 / 2.65 ms |
| Scaffold, 4096 | 10.70 s | 7.82 s | 1.33% / 0.80 d | 30 → 30 | 1.40 / 4.04 ms |
| Scaffold, 16384 | 10.27 s | 8.02 s | 0% / 0.84 d | 30 → 30 | 3.48 / 7.44 ms |

Off the built face: climb 5 of 78,919 climber-ticks, hybrid 0 of 31,894, scaffold 4 of
43,638. Release and ASan/UBSan pass all fifteen CTest cases (seven `--verify`, each running
eleven configurations; six diagnostics receivers; the mesh and playback tests). State
hashes with this compiler (1536 units, the default length): lying `3389b8c993dc4914`, raise
`b9829cc0a68b7b77`, climb `2dea93508feb6830`, hybrid `0dbc85e37884e977`, scaffold
`c085a4a21e500332`. Cross-compiler or cross-platform bit equality is not claimed.

Blend keeps on-screen collision fidelity (visible contacts back to 0% from activity LOD's
1.2%) and spends heroes on contention: 3,000 to 5,600 deliberations per run against 16,000
to 24,000 under activity LOD.

**Isolation** (`--verify`, 1536 units, title complete in choreography seconds; the
two-layer rows use 3,072 units):

| Build | blend | camera | activity | hero | near | far | hidden | spread |
|---|---|---|---|---|---|---|---|---|
| Lying | 7.71 | 7.71 | 7.81 | 8.44 | 7.78 | 7.80 | 7.71 | 0.73 s |
| Raise (upright) | 8.83 | 8.83 | 8.85 | 9.87 | 8.85 | 8.88 | 8.74 | 1.13 s |
| Climb | 10.85 | 10.38 | 10.97 | 12.34 | 10.67 | 10.68 | 10.50 | 1.96 s |
| Hybrid | 9.00 | 9.13 | 9.66 | 10.70 | 9.16 | 8.96 | 8.79 | 1.91 s |
| Scaffold | 10.57 | 10.39 | 10.36 | 11.73 | 10.40 | 10.27 | 10.23 | 1.49 s |
| Climb, two layers | 10.99 | 10.93 | 11.63 | 13.63 | 10.96 | 11.07 | 10.80 | 2.83 s |
| Scaffold, two layers | 10.83 | 10.76 | 11.53 | 13.10 | 10.78 | 10.94 | 10.73 | 2.38 s |

The spread is title completion across the seven modes (raise: all panels upright). Hidden
units overlap freely (contacts 43–108% of movers), which is allowed: they are never drawn.
Levers switched mid-run: climb 11.43 s, scaffold 11.40 s (contacts 2.9%), two-layer climb
12.43 s. Budget 4,000: climb 10.67 s, scaffold 10.27 s, two-layer climb 12.18 s, contacts 0%.

Two depth layers (S0i, `--layers 2 --count 3072`, 2,146 tiles, each layer one body thick since S0j):

| Run | Title complete | Upright | Worst contacts / min gap | Stuck → recovered | Tick p50 / p95 |
|---|---|---|---|---|---|
| Lying | 8.60 s | – | 0% / 0.89 d | 0 → 0 | 0.73 / 2.04 ms |
| Raise | 8.62 s | 9.67 s | 0% / 0.85 d | 0 → 0 | 0.73 / 2.99 ms |
| Climb | 10.99 s | 10.99 s | 0% / 0.82 d | 43 → 43 | 1.43 / 11.89 ms |
| Hybrid | 9.92 s | 8.77 s (raised) | 0% / 0.85 d | 9 → 9 | 0.83 / 8.65 ms |
| Scaffold | 10.83 s | 8.54 s (panels) | 0% / 0.84 d | 75 → 75 | 1.60 / 10.78 ms |

Off the built face: climb 51 of 124,279 climber-ticks, scaffold 23 of 75,288; no unsupported
lock. State hashes: lying `0ea159c2056fbf21`, raise `7fe9cbd5d0ad1fc8`, climb
`2e0266b5f69a9389`, hybrid `07a7043ce1e2358b`, scaffold `c387eab62bdff823`. Three layers
(3,212 tiles, 4,608 units) locked 3,211 by 15 s when measured at S0i (finding 22); not
re-measured since.

### Cost, the two brains and playback (S0n)

Cost per mover-tick, scaffold, first 10 s, callgrind (deterministic instruction counts;
host time is too noisy on a shared container to compare levers):

| Tier | Behaviour | Resolve | Whole tick |
|---|---|---|---|
| Hero, System 2 every tick | 10,054 | 4,125 | 16,828 |
| Hero, System 2 every 2 ticks | 6,154 | 4,094 | 12,899 |
| Near | 2,000 | 4,346 | 9,171 |
| Far | 1,070 | 4,411 | 8,293 |
| Hidden | 987 | 1,880 | 5,774 |

A deliberation costs about 7,800 instructions and holding a plan about 2,250, so System 2
every 2 ticks cuts a hero's behaviour by 39%. Before S0n every tier also paid for a full
lane projection and a push query nobody needed (finding 25); with those fixed a default
scaffold tick fell from 3.85 to 3.19 billion instructions over 10 s, with no change in
behaviour. The resolve pass is now the largest cost (46%).

Budgets (scaffold, cost units): unbounded p95 6,082; at 5,000 and 4,000 the controller
drives fidelity to its floor and holds p95 near 5,300, with the title complete at 10.29 s
and 10.27 s and no visible contact. Lower budgets cannot be met: at the wave's peak most
movers are on screen or building, and each keeps its resolve pass.

Presentation from sparser captures (scaffold, `--reconstruct`, ground movers, diameters):

| Kept capture | Interpolated: mean / p95 / max | Extrapolated: mean / p95 / max |
|---|---|---|
| Every 2nd (30 Hz) | 0.004 / 0.008 / 0.33 | 0.016 / 0.066 / 0.67 |
| Every 4th (15 Hz) | 0.009 / 0.024 / 0.82 | 0.035 / 0.140 / 0.82 |
| Every 8th (7.5 Hz) | 0.026 / 0.078 / 1.22 | 0.079 / 0.296 / 1.51 |

The worst cases are act changes (lifting off, seating), which neither method predicts.

A slower pace costs nothing per presented tick: each tick advances less choreography. The
whole sequence takes more ticks.

### The cinematic camera (tour)

![The close-up tour, scaffold build](evidence/tour.png)

The scaffold build through its camera at 1.0, 2.6, 3.2, 4.0, 5.5, 6.5, 7.5, 8.5, 9.5, 10.5,
12.5 and 15.5 s: the macro on the first unit to wake; the bed lifting off and the stream
head; units arriving at the title; then close-ups that follow wherever letters are being
built (the C and I, the R and U, the I and B, the U and second C, the B and L, the L and
E), the view turning from one side of the title to the other; the pull-out to the whole
title; the hold and opening.
Frame labels give the time on screen: the close-up plays at half speed.

### Lying (S0)

![Lying build: six beats](evidence/beats.png)

Beats through the tour camera at 1.0, 3.0, 5.0, 6.8, 8.4 and 11.9 s: one unit wakes; the
stream head in close-up, stretched units leaving their lanes; the first letters filling;
the U and C complete; the B, L and E complete; the camera pulled out over the whole title
and the ribbon.

![Lying build: LOD tiers](evidence/tiers.png)

Same frames coloured by decision tier: amber hero, cyan near, violet far, grey hidden.
Blend LOD (S0n): in the close-up, building units and cruising units under the camera are
near, the most contended or most central are heroes; off screen, building units stay
near or far and calm ones go hidden; resting units have no tier to spend.

### Raise (S0c)

![Raise build: six beats](evidence/raise.png)

At 5.0, 6.8, 7.6, 8.4, 9.2 and 11.9 s, framed wide (`--zoom 0,5,80`) to show the whole title: letters assembling flat;
nearly complete; the first letters rising; most upright; all upright; camera opens.

### Hybrid (S0e)

![Hybrid build: drawbridge letters and climbed letters](evidence/hybrid.png)

At 6.0, 7.0, 7.8, 8.4, 9.3 and 10.2 s, framed wide: the first C has already swung up while
the swarm climbs the U, I, B and L; the R and second C rise; the E rises as the climbed
letters grow; the climbed letters finish; complete at 9.00 s.

### Scaffold (S0f)

![Scaffold build: stems swing up, the swarm climbs them](evidence/scaffold.png)

At 6.0, 7.0, 7.8, 8.6, 9.6 and 10.5 s, framed wide: stems assembled flat; the first stems and the Cs swing
up; climbers gather at the bases; arms and bowls grow from the standing stems; complete.

![Scaffold close-up: B, L and E](evidence/scaffold-closeup.png)

The B, L and E at 6.6, 7.2, 7.8, 8.4, 9.2 and 10.5 s: the L and E stems rise as drawbridges
(foreshortened while rising) while climbers wait at the E's base; then they pour over the
standing stems to build the E's arms and the B's bowls.

### The nanite body (S0k)

![Nanite bodies: the wake, macro to pullback](evidence/body-macro.png)

Shot01 to shot02 at 0.6, 1.4, 2.0, 2.4, 3.0 and 4.0 s: the bed of dormant shells (dark
cores), the first core lit, the answer wave lighting neighbours, units lifting off the bed
and the stream forming. At macro range each shell shows its hubs, rails and recessed
shields, with the one core seen through the windows; by 4.0 s units are small bodies with
bright windows.

![Moving nanites: shells stretched in the lanes](evidence/body-motion.png)

A lane at 3.0, 3.6 and 4.2 s (scaffold build): cruising units are long low bodies leading
with the nose, the core unchanged at the rear hump; units slowing at the lane's end compact
again.

![Nanite bodies in the title: B and L at 35 degrees of yaw](evidence/body-title.png)

The B and L at 8.6, 9.6 and 11.0 s, yawed 35°: the finished letters are fields of faceted
pyramids (docked shells on their tiles) with cyan glimpses, like the hero concept; the L stem
stands as a raised panel; climbers still flowing over the B are lit brighter; settled cores
glow steadily.

### 3D tessellation (S0i)

![Two depth layers: the scaffold build seen at 30 degrees of yaw](evidence/layers.png)

The scaffold build with two staggered layers, the camera yawed 30° so the depth shows: stem
panels assemble flat as two-layer slabs and swing up; climbers build both layers of the
arms and bowls, the front layer held by the face behind it; complete at 10.83 s (frames at
6.0, 7.0, 7.8, 8.6, 9.6 and 11.4 s). Framed wide.

![Two depth layers, close-up of the B, L and E](evidence/layers-closeup.png)

The B, L and E at 35° of yaw: the L stem rising as a slab, its thickness and the back
layer's darker edge visible; the staggered joints between the layers show where tile
edges cross.

### Morphing tiles

![Equilateral tiles: saw-tooth outline](evidence/outline-unmorphed.png)

![Morphed tiles: outline follows the strokes](evidence/outline-morphed.png)

The C, R and U at 12.5, 13.5 and 14.9 s in the climb build, first with `--morph 0`, then
with the default morph. The bowls, counters and stroke ends follow the outline; joins stay
seamless.

### Climb (S0b, S0d)

![Climb build: six beats](evidence/climb.png)

At 5.0, 6.5, 8.0, 9.5, 10.9 and 14.9 s, framed wide: footings; letters growing bottom-up;
climbers pouring over the face; readable; complete at 10.85 s; the hold, with the camera
opening by 16.5 s (28.8 s on screen).

![Climb build: selective magnetism](evidence/climb-magnets.png)

The same frames by magnet mode: cyan lift, white seat, amber push; grey units are off.

![Climb build: close-up of the C and R](evidence/climb-closeup.png)

At 6.5–12.0 s: climbers (lighter) flowing over the solid tiles, the build front rising, and
the finished letters as single faces.

## Findings

1. **Full intelligence everywhere is slower, costlier and can freeze.** With reciprocal
   avoidance on every unit, an early uniform-hero run reached only 82% of the title by 10 s.
   Right of way by wake order fixed the freeze. Uniform hero is still 3.8 s slower than the
   camera-LOD run at about seven times its median cost, and needed 125 stuck recoveries.
2. **Lane throughput sets the lying timeline** (width × cruise speed).
3. **Decentralised slot swaps remove most crossings** (about 7,100 per lying run).
4. **The signal system dominates at 16K**, because every dormant unit polls its neighbours
   each tick. An event-driven wave front is the next optimisation.
5. **Lock sounds need aggregation:** up to 70 locks fall in one 100 ms window.
6. **Hidden units interpenetrate by design** and resolve before reaching the frame edge.
7. **With a preset order, honest climbing missed the brief by about 5 s** (13.5 s, rows).
8. **Climbing needs a vocabulary of rules**:
   - Which surface a unit is on follows its state, not its position.
   - Units stage on the ground before stepping onto the face.
   - Climbers flow over each other in layers.
   - Routes are shortest paths over the built face, with the ground linking strokes.
   - Seating is pipelined: slide in once the support is nearly seated, lock only on a
     locked support.
9. **Raise keeps the brief's timing and the upright ending** at no extra simulation cost.
10. **First-come claiming at the base beats a preset order.** Claims are just-in-time, each
    the lowest supportable tile nearest the unit, and the build finished at 11.8 s instead of
    13.5 s (1,104 equilateral tiles; 11.4 s with today's 1,066 morphed tiles and entry rules). Three changes made the difference, all found through the diagnostics log:
    - waypoints keep momentum;
    - climbers cling beside an unsupported slot;
    - seekers heaped at the wall's base flow over each other.

    No preset order means no planner coupling between wake order and build depth.
11. **Stuck intelligence resolves every episode in these runs.** Climb: 50 stuck episodes,
    37 pushes, no reclaims; all recovered (median 8 ticks, 90th percentile 32, longest 45). A
    whole-lane car-following rule was tried and rejected: lanes starting from rest jammed
    and the lying build slowed from 8 s to 15 s.
12. **Tessellation interlocks into a solid face.** It also makes the build shallower:
    vertex-sharing support gives 13–17 levels instead of 22. Equilateral tiles leave a
    stepped outline; morphing (finding 15) addresses it.
13. **The climb build's lane exits were crowded**: up to 9.8% visible ground contacts with
    1536 units, more at scale. That was a lane design issue (exits narrow as flow peaks), not
    a tuning one; finding 20 resolves it.

14. **The whole-letter hybrid halves the crowding.** Raising the four deepest letters removes
    much of the climb's critical path and leaves fewer units converging on the wall:
    contacts fall from 9.1% to 4.3%. It finishes at 11.2 s against pure climbing's 11.4 s;
    when measured before the S0f entry rules, the gap was 10.9 s against 12.2 s. Per-part
    drawbridges (finding 16) go further.
15. **A little morph smooths a lot.** Moving outline corners by up to 0.35 of a side halves
    the outline error (−51%) with every tile keeping half its area. Larger limits help little
    (0.5 gives −50%, 0.7 gives −52%) because the area-quality pass holds them back. Fill and
    trim were needed first: without them the lone spikes at stroke ends cannot move far
    enough. Faint steps remain on some vertical stems, the clearest place for a later
    sub-tile edge piece.

16. **Per-part drawbridges make scaffolds.** Raising the tall grounded stems first, then
    climbing them, completed at 10.75 s at pace 1 (11.23 s at pace 2/3; finding 18), with all eight
    panels upright by 8.6 s. Two rules made it work: raised tiles support climbers only once
    upright, at their standing positions (otherwise climbers steered to the flat tiles'
    ground coordinates and stalled); and arm tiles can be claimed while their stem is flat,
    so units wait at the base ready for the swing.
17. **Entry points matter as much as routes.** Stepping onto the wall over a gap between
    strokes left climbers hanging 2–2.7 units from anything built. Entering only beside a
    ground tile they can stand on brought scaffold off-surface time from 1.2% to 0.4%, and
    also sped up pure climbing (12.2 s to 11.4 s). The hybrid slowed from 10.3 s to 11.2 s
    under the same rule: its climbed letters now wait for real footholds.
18. **Slowing down exposed latent faults, so slow by step, not by retuning.** The pace
    shortens each simulation step and changes nothing else; every gate still held in
    choreography seconds, but three faults surfaced:
    - A climber carried waypoint momentum around a corner (across the R's base, then up its
      stem) and overshot three units into the counter. Waypoints now brake into corners by
      route look-ahead: the unit must be able to stop within the waypoint distance plus a
      run-on that shrinks with the turn (none before the goal).
    - Flat-locked panel tiles counted as "built", so scaffold climbers went up beside a stem
      still lying down and hung off the face (1.2% of climber-ticks). Waiting on a single BFS
      support chain fixed that, but delayed the R to 12.9 s because its chain ran through the
      late leg. A slot is now climbable once any path from the ground reaches it without
      crossing a panel that is not yet upright.
    - A ribbon unit sent across the stage moored 0.26 s after the 10 s beat. Ribbon moorings
      are now matched by x rank.

    Off-surface time fell to almost nothing, and climbing got faster: climb 11.42 s, hybrid
    10.66 s (contacts 4.3% to 1.6%), scaffold 11.23 s.
19. **Decision LOD could follow activity, not only the view** (backlog, user suggestion).
    Claims, docking, corners, contention and stuck episodes are where a decision shows; a
    unit cruising an empty lane needs none even when large on screen. Activity tiers depend
    only on simulation state, which would make decision LOD replay-safe for gameplay. See the
    design document, section 4.
20. **Branch off the lane, don't funnel to its end** (S0h). Contacts were 93% between units
    in a lane or just past it, at the exit of the lane serving C, R and U, where the S0 rule
    narrowed the stream to a third of its width before it split three ways. Units now leave
    the lane over its last 26 units of length and head straight for their slot or staging
    spot, so each lane splays into a fan sorted by destination. Visible contacts fell to 0%
    in every build (climb from 9.8%, scaffold from 5.0%) with the minimum gap at 0.8
    diameter or more, and everything finished sooner: hybrid 10.66 to 9.17 s, scaffold 11.23
    to 10.21 s, climb 11.42 to 11.01 s, lying 7.98 to 7.66 s. Branching length was swept:
    8 units left 4% contacts, 14 left 2%, 26 or more left none; beyond 32 the lanes no
    longer read as lanes (shot02). At 4096 and 16384 units contacts stay near 0.1%.
21. **Measure progress against the current goal.** Branching made negotiation swaps
    common, and the lying build reported 161 stuck episodes while units ran at full speed:
    after a swap handed a unit a farther slot, its distance never beat the best distance it
    had reached toward the old one. The progress baseline now resets when the goal changes,
    as it already did when the act changed; the lying build has no stuck episodes.
22. **Staggered layers give 3D interlocking with the same support rule** (S0i). Face links
    between overlapping wedges of adjacent layers let build depth count through the bond,
    so a front tile can be held by the face behind it; nothing else in the climbing
    vocabulary changed. Two layers pass every gate. Doubling the tiles exposed three
    capacity limits, each fixed by scaling with the layer count (single-layer hashes are
    unchanged):
    - a front panel wedge waited on a climbed wedge behind it, and the panel never rose:
      flat assembly now counts face backing only within the same flat body;
    - lanes carried twice the flow: lane width now scales with the layers (lying contacts
      1.2% to 0.06%);
    - climbing letters' waiting crowds packed into contact (U: 300 climbers, 6–8%): they now
      heap and flow over each other like the wall-base heap (0.2–0.3%).

    Three layers leave one enclosed front-layer pocket unfilled at 15 s: TERMES-style
    construction avoids this with a no-enclosed-holes build order, the follow-up before
    deeper titles. Cost: scaffold p50 1.1–1.3 ms per presented tick against 0.5 ms with one
    layer, mostly surface routing over the larger letter graphs.
23. **The camera is a behaviour input under camera LOD** (S0l). The close-up tour leaves most
    of the title off screen, and camera LOD hid the units building it; hidden motion has no
    separation, so on the climb five climbers on the R and six seekers bunched below them
    stalled from 9.5 s until the pull-out (complete at 13.45 s, gate 12.5 s). An activity
    floor keeps building units (seek, queue, climb, dock) at least far off screen; every gate
    passes again near the wide-camera times. A first step toward finding 19, which S0m
    adopted: decision LOD now follows activity by default.
24. **Every tier converges in isolation; hero is the slow one** (S0m). Running the whole
    swarm at each discrete tier found hidden never finishing the climb and scaffold: hidden
    units set velocity straight to their desire, overshot a waypoint every tick and turned
    back forever (two seekers oscillating at 16 d/s from 3.8 s). Hidden now shares the
    other tiers' motion model and converges within 0.3 s of near. Full deliberation is the
    slowest tier everywhere (0.9 to 2.1 s behind the fastest; 3.5 s on the two-layer
    climb): reasonable equality between tiers, the later gate, starts there.
25. **Profile before tiering: the shared work was the cost** (S0n). Wall-time calibration
    gave every tier about 1.6 µs per unit of shared work, so tiers barely mattered.
    Callgrind showed why: a lane projection searched its whole window every tick (13% of a
    tick) and every unit queried its neighbours for pushers that almost never existed (5%).
    Stopping the search at the first rise and skipping the push query when nobody pushes
    took a scaffold tick from 3.85 to 3.19 billion instructions with identical behaviour,
    and made the tiers' costs distinct. Resolve is now 46% of a tick.
26. **Two brains and blended drivers** (S0n). System 2 every 2 ticks halves deliberations
    at no cost to completion or contacts; every 3 and 4 still converge but contacts rise on
    the scaffold (2.1%, 2.7%). Blending camera focus with activity restores on-screen
    collision fidelity and fixes activity LOD's slow two-layer climb (12.25 to 10.99 s).
    A deterministic budget controller holds what the tiers control; below the peak's floor
    only fewer resolve passes or parallel rows would help (S3).
27. **Interpolate between ticks, extrapolate only when late** (S0n). Rebuilding the frames
    between every 4th capture (a 15 Hz simulation shown at 60 Hz) puts interpolated units
    within 0.024 diameters at p95, extrapolated ones within 0.14; both fail at act changes.

## Limits

Orthographic top-down and oblique 2D stand-in cameras; no perspective, depth of field,
shadows, PBR materials or bloom. The body mesh is a proxy of the design space's form,
flat-shaded with one key light and painter's ordering per body (no depth buffer), which can
misorder units that overlap or pass through each other (hidden-tier units, by design). One fixed seed. Glyphs are a prototype stroke font with
square caps, not the licensed wordmark asset. A unit snaps to its morphed shape when it
locks; a gradual morph while seating is a renderer detail for S1. The raise hinge is a rigid pose transform:
nothing yet shows what lifts the letter. Climbers and heaped seekers stack in layers; only
other ground movers are held apart. Depth layers (S0i) show best in a yawed oblique view; units on the wall still move in the face's
plane, and a layer's depth is placement, not a third motion axis. At most two layers
complete; overhangs and climbing onto the back face are not modelled. The tour camera's
section choice reads only counts of building units; it does not yet anticipate where the
next letter will start, so it can arrive just after a section's first units seat.

Skip, reduced motion, resize and focus behaviour belong to the frontend package and are not
exercised. No GPU, audio or participant evidence.
