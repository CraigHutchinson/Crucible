# Nanite swarm intro spike

Headless, deterministic prototype of the CRUCIBLE title sequence's swarm behaviour: building
blocks S0 to S0j of the
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

**Times are choreography seconds** (the authored beats) unless marked "on screen". At the
default `--pace 0.6667`, each tick shows 1/60 s but advances the choreography by 1/90 s, so a
viewer sees everything 1.5× longer: the 15 s sequence plays over 22.5 s.

It is not production rendering, audio, a gameplay system or a native performance budget.
The images are concept2D/dev-test captures through stand-in cameras: top-down orthographic
for the lying build, and an oblique front view (screen height = z + 0.4 × depth) for the
upright builds.

## Five builds

| `--build` | Stage | How the title forms | Title complete (1536 units) |
|---|---|---|---|
| `lying` (default) | S0 | Units assemble the title flat on the substrate, each with a planned slot. | 7.66 s |
| `raise` | S0c | Each letter assembles flat behind its baseline. As soon as its last unit locks, the interlocked letter hinges up on its bottom row like a drawbridge, letters rising one after another. | 7.73 s flat, upright 8.79 s |
| `climb` | S0b, S0d | Letters grow upright in place, bottom-up. No preset slots: units queue at their letter's base and claim slots first come, first served. They climb the built face with lift magnetism, and recover from getting stuck by waiting, pushing or reclaiming. Uses the 15 s sequence. | 11.01 s |
| `hybrid` | S0e | Whole letters both ways. Letters whose build depth is above the median (the climb's critical path) assemble flat and swing up as drawbridges; the rest climb. Automatic choice here: C, R, C and E raise; U, I, B and L climb. `--raise-letters MASK` chooses by hand (bit 0 = first C). Uses the 15 s sequence. | 9.17 s; raised letters upright by 8.14 s |
| `scaffold` | S0f | Per-part drawbridges, combined with climbing. Tall, straight, upright strokes standing on the baseline become drawbridge panels: the R's stem and leg, the I, and the B, L and E stems. They assemble flat and swing up first as scaffolds, and the swarm then climbs them to finish the arms and bowls. Deep letters without such a stroke (the two Cs) raise whole; the U climbs. A climber starts up once a path from the ground reaches its slot without crossing a panel still lying down. Uses the 15 s sequence. | 10.21 s (15.3 s on screen); all 8 panels upright by 8.04 s |

The climb build runs on two connected surfaces: the ground and the wall face. The wall face
is stored as a second interaction surface, offset so the two never interact, and joined to
the ground along the wall's base. That is the "surface" interpretation of upright
assembly.

## The 3D nanite body (S0j)

Each nanite is the concepts' slender arrowhead dart (`docs/concepts/units-v1.png`): a raised
spine from the rear to the nose, faceted plates falling away from it, a steep rear facet
and a cyan core set into the spine. `NaniteBody.hpp` specifies it once:
- a **free unit's footprint** is a dart (1.9 × 0.9 diameters); a **locked unit's** is its
  morphed tile, nose at the apex, so the nanites change shape as they seat;
- **walls** rise vertically from the footprint to a low shoulder, so neighbours share wall
  quads and a tessellated letter stays gap-free in 3D;
- the **spine** runs from just behind the nose (0.22 d high) to just inside the rear edge
  (0.55 d), and five **roof facets** join it to the shoulders: the beveled planes, and in a
  letter the hero's faceted surface;
- the **core** is a cyan diamond in the spine near the rear; a locked unit's core settles to
  a steady glow, subordinate to the silver.

The simulation takes its stacking heights from the body (the rest-layer lift and the depth
layer are one spine height), and frame captures carry the dimensions in their header. The
renderer (`nanite_mesh.py`) builds every body's mesh in one batched pass, lights it with a
cool grazing key and a controlled highlight, culls back faces and paints faces far to near.
Detail is levelled by on-screen size: meshes from 6 px per unit, seams from 14 px and core
recess rings from 10 px; silhouettes below 6 px and points below 3 px.
`test_nanite_mesh.py` checks the mesh invariants (footprint and vertical walls, shared
walls between neighbours, outward faces, the spine, the core on the roof, the dart) and
that its defaults match `NaniteBody.hpp`.

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
`--claim-cap N` (96 per depth layer), `--layers N` (1), `--lod camera|hero|near|far`, `--pace P` (0.6667; 1 is the original
speed), `--seconds S` (on screen; default 12 s of choreography, 15 s for the climbing
builds), `--metrics FILE`.
Renderer: `--tiers`, `--magnets` (cyan lift, amber push, white seat), `--gif`, `--gif-mode`,
`--mp4` (H.264 at the capture's on-screen rate, 60 fps with `--every 1`; `--mp4-mode`,
`--mp4-size 1280x720`, `--hold 2` seconds on the last frame), `--yaw DEG` (oblique view
turned to show depth layers) and `--zoom cx,cy,half_width`.
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
  - Two identical runs produce the same state hash; a uniform far tier completes too.
- **Lying and raise:**
  - Title at least 99% locked by 8.4 s and 100% by 10.0 s.
  - Visible mover contacts (centres closer than 0.8 diameter) at most 3%† of visible movers
    per tick, and no visible pair closer than 0.5 diameter.
  - Raise: every letter upright by 10.0 s.
- **Lying:** a uniform hero tier completes by 12.5 s† (11.0 s for 959 row slots, scaled for 1,104 tiles).
- **Hybrid and scaffold:** every drawbridge panel upright by 13.5 s†, plus the climb gates.
- **Two layers** (`--layers 2 --count 3072`, climb and scaffold in CTest): every gate above.
- **Climb** (15 s sequence):
  - Title complete by 12.5 s, leaving a hold and the menu handoff by 15 s.
  - At most 0.5%† of climber-ticks off the built face. Attached means touching it: within a
    tile's circumradius plus one unit diameter of a built tile.
  - Visible ground contacts at most 3%† per tick, and no visible pair closer than 0.5
    diameter† (tightened from 10% and 0.3 when lane branching removed the exit crowding).

## Evidence, 2026-10-13 (pace 2/3, lane branching)

Host: shared 4-vCPU Intel Xeon 2.80 GHz cloud container, GCC 13.3, single thread. Costs are
advisory host wall time per presented tick (1/60 s on screen) with diagnostics excluded, not
a device budget. 1,066 morphed tiles. Times are choreography seconds; on screen is 1.5×.

| Run | Title complete | Upright | Worst contacts / min gap | Stuck → recovered | Tick p50 / p95 |
|---|---|---|---|---|---|
| Lying, camera LOD, 1536 | 7.66 s | – | 0% / 0.88 d | 0 → 0 | 0.11 / 1.90 ms |
| Raise, camera LOD, 1536 | 7.73 s | 8.79 s | 0% / 0.88 d | 1 → 1 | 0.22 / 2.14 ms |
| Climb, camera LOD, 1536 | 11.01 s | 11.01 s | 0% / 0.86 d | 32 → 32 | 0.41 / 2.55 ms |
| Hybrid, camera LOD, 1536 | 9.17 s | 8.14 s (raised) | 0% / 0.81 d | 2 → 2 | 0.14 / 2.13 ms |
| Scaffold, camera LOD, 1536 | 10.21 s | 8.04 s (panels) | 0% / 0.81 d | 18 → 18 | 0.48 / 2.24 ms |
| Scaffold, 4096 | 10.47 s | 7.96 s | 0.10% / 0.79 d | 36 → 36 | 0.84 / 3.46 ms |
| Scaffold, 16384 | 10.49 s | 7.92 s | 0.11% / 0.73 d | 34 → 34 | 2.55 / 6.95 ms |

Off the built face: climb 0 of 59,537 climber-ticks, hybrid 0 of 24,241, scaffold 3 of
33,327. Release, Debug and ASan/UBSan pass all thirteen CTest cases (seven `--verify`, six
diagnostics receivers). They produce the same state hashes with this compiler (22.5 s on
screen, 1536 units: lying `3c951c0d05bdb744`, raise `1efad3ea9ba609a3`, climb
`e920c8aee60c63c8`, hybrid `efa270be5513e8b4`, scaffold `b75543d06ac47437`). Cross-compiler
or cross-platform bit equality is not claimed.

Two depth layers (S0i, `--layers 2 --count 3072`, 2,146 tiles, each layer one body thick since S0j):

| Run | Title complete | Upright | Worst contacts / min gap | Stuck → recovered | Tick p50 / p95 |
|---|---|---|---|---|---|
| Lying | 8.42 s | – | 0.06% / 0.76 d | 0 → 0 | 0.25 / 4.05 ms |
| Raise | 8.49 s | 9.54 s | 0.24% / 0.71 d | 0 → 0 | 0.24 / 4.78 ms |
| Climb | 10.78 s | 10.78 s | 0.15% / 0.74 d | 39 → 39 | 0.99 / 9.92 ms |
| Hybrid | 9.31 s | 8.72 s (raised) | 0.12% / 0.73 d | 4 → 4 | 0.35 / 7.53 ms |
| Scaffold | 10.68 s | 8.59 s (panels) | 0.32% / 0.77 d | 89 → 89 | 1.29 / 7.99 ms |

Off the built face: climb 9 of 93,629 climber-ticks, scaffold 67 of 58,826; no unsupported
lock. State hashes: lying `be4fd2c08b663418`, raise `67f835e157394ef3`, climb
`21e0f4488e5a2775`, hybrid `428b8478e9a667c0`, scaffold `f82eb4dfdb76c099`. Making a layer one
body thick (0.55 d instead of 1.0) moved the front layer on screen, which shifts camera LOD
tiers and so the run, slightly. Three layers
(3,212 tiles, 4,608 units) lock 3,211 by 15 s (finding 22).

A slower pace costs nothing per presented tick: each tick advances less choreography, and
p50 fell because the per-tick work shrinks with the smaller step. The whole sequence takes
1.5× more ticks.

### Lying (S0)

![Lying build: six beats](evidence/beats.png)

Beats at 1.0, 3.0, 5.0, 6.8, 8.4 and 11.9 s: one wedge wakes; the answer wave spreads and
units turn toward their lanes; three lanes, each splaying into a fan over its second half;
tiles fill far-first while the surplus leaves for
the ribbon; title complete; camera opens.

![Lying build: LOD tiers](evidence/tiers.png)

Same frames coloured by decision tier: amber hero, cyan near, violet far.

### Raise (S0c)

![Raise build: six beats](evidence/raise.png)

At 5.0, 6.8, 7.6, 8.4, 9.2 and 11.9 s through the oblique camera: letters assembling flat;
nearly complete; the first letters rising; most upright; all upright; camera opens.

### Hybrid (S0e)

![Hybrid build: drawbridge letters and climbed letters](evidence/hybrid.png)

At 6.0, 7.0, 7.8, 8.4, 9.2 and 10.5 s: the first C has already swung up while the swarm
climbs the U, I, B and L; the R and second C rise; the E rises as the climbed letters grow;
the climbed letters finish; complete at 9.2 s.

### Scaffold (S0f)

![Scaffold build: stems swing up, the swarm climbs them](evidence/scaffold.png)

At 6.0, 7.0, 7.8, 8.6, 9.6 and 10.5 s: stems assembled flat; the first stems and the Cs swing
up; climbers gather at the bases; arms and bowls grow from the standing stems; complete.

![Scaffold close-up: B, L and E](evidence/scaffold-closeup.png)

The B, L and E at 6.6, 7.2, 7.8, 8.4, 9.2 and 10.5 s: the L and E stems rise as drawbridges
(foreshortened while rising) while climbers wait at the E's base; then they pour over the
standing stems to build the E's arms and the B's bowls.

### The 3D nanite body (S0j)

![3D nanite bodies: the wake, macro to pullback](evidence/body-macro.png)

Shot01 to shot02 at 0.6, 1.4, 2.0, 2.4, 3.0 and 4.0 s: the bed of dormant darts (dark
cores), the first core lit, the answer wave lighting neighbours, darts lifting off the bed
and the stream forming. The spines, faceted roofs and cores read at macro range; heading
away from the camera a dart shows its steep rear facet and core; by 4.0 s units are small
bodies with bright cores.

![3D nanite bodies in the title: B and L at 35 degrees of yaw](evidence/body-title.png)

The B and L at 8.6, 9.6 and 11.0 s, yawed 35°: the finished letters are faceted plates of
ridged tiles with flush walls and facet seams, like the hero concept; the L stem stands as a raised panel; climbers
still flowing over the B are lit brighter; settled cores glow steadily.

### 3D tessellation (S0i)

![Two depth layers: the scaffold build seen at 30 degrees of yaw](evidence/layers.png)

The scaffold build with two staggered layers, the camera yawed 30° so the depth shows: stem
panels assemble flat as two-layer slabs and swing up; climbers build both layers of the
arms and bowls, the front layer held by the face behind it; complete at 10.6 s.

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

At 5.0, 6.5, 8.0, 9.5, 11.0 and 14.9 s: footings; letters growing bottom-up; climbers
pouring over the face; readable; nearly complete; complete at 11.0 s with the hold and camera
opening by 15 s (22.5 s on screen).

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

## Limits

Orthographic top-down and oblique 2D stand-in cameras; no perspective, depth of field,
shadows, PBR materials or bloom. The wedge mesh is flat-shaded with one key light and
painter's ordering per face (no depth buffer), which can misorder intersecting faces of
units passing through each other (hidden-tier units, by design). One fixed seed. Glyphs are a prototype stroke font with
square caps, not the licensed wordmark asset. A unit snaps to its morphed shape when it
locks; a gradual morph while seating is a renderer detail for S1. The raise hinge is a rigid pose transform:
nothing yet shows what lifts the letter. Climbers and heaped seekers stack in layers; only
other ground movers are held apart. Depth layers (S0i) show best in a yawed oblique view; units on the wall still move in the face's
plane, and a layer's depth is placement, not a third motion axis. At most two layers
complete; overhangs and climbing onto the back face are not modelled.

Skip, reduced motion, resize and focus behaviour belong to the frontend package and are not
exercised. No GPU, audio or participant evidence.
