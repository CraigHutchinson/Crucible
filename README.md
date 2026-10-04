# Crucible

Crucible is a macro RTS about shaping a machine swarm to reclaim an industrial
surface overtaken by cellular Blight. Draw currents, place attractors and repulsors,
and redirect autonomous nanites across a spreading frontier. The longer-term loop
adds fusion into a holding lattice and shattering back into a smaller mobile swarm.

![Resource concept: substrate stock, mobile mass, reserve and a future lattice](docs/concepts/resources-v2.png)

*Generated resource study. Stock and infection are separate; mobile and reserve are
forms of the same biomass. The lattice and shatter trail are future mechanics.*

## Playable now

The desktop prototype is a bounded reclamation challenge: recover **1,780 biomass
quanta before tick 900**. Pause stops the deadline. Flow and radial tools change
movement; the HUD reports progress, admission feedback and a latched win or loss.
Restart begins a fresh run. The reference scenario has 2,048 samples and conserves
`initial = remaining stock + mobile mass + reserve`.

![Actual Phase 8 software frame with applied flow and dashed uncommitted preview](docs/concepts/exports/phase8-flow-fit.png)

*Actual production SDL software export at tick 60. Direction arrows distinguish
applied fields from the dashed preview. [Capture provenance](docs/workstreams/integration/phase8-validation.md).*

Phase 8 was recovered and [merged in PR 21](https://github.com/CraigHutchinson/Crucible/pull/21).
Its [review](docs/sprint-reviews/phase-08.md) separates original local validation from
publication. The default desktop uses the concrete software painter; an optional
Vulkan instancing receiver has separate readback and fault fixtures. Human usability,
physical-device performance and iOS receiving remain open.

## Direction and next increment

The proposed **Secure the relay** loop makes concentration a tradeoff: mass anchored
at an objective cannot reclaim elsewhere, and shattering loses material. Phase 9
selects the structural loop after comparing a local faction arena and receiving
actual relay-density evidence. The next reference lattice anchors64 identities;
shatter returns48 and records16 lost. Implementation and full-loop receiving follow. See the
[Phase 9 plan](docs/phases/phase9.md) and [review](docs/sprint-reviews/phase-09.md).
Primitive cells, points, arrows and structure marks come before visual fidelity.

![Faction concept with four shape and color identities, both organism variants and a proposed deathmatch arena](docs/concepts/factions-deathmatch-v2.png)

*Generated future-mode study. Circle/cyan, triangle/amber, diamond/violet and
square/lime identify factions independently of nanite or Blight kind. “Last faction
standing” is a proposal; authority, combat, elimination and networking are future rules.*

The [game design](docs/game-design.md) owns product intent. The
[concept gallery](docs/concepts/README.md) records art interpretation and provenance;
the [architecture](docs/architecture.md) owns technical boundaries. Terrain height,
mining and permanent bridges remain [later world work](docs/decisions/terrain-and-world-extension.md).
The target of 100,000–150,000 entities at 60 FPS needs complete tick/frame measurements;
it is not an achieved game result.

## Build and play

Requires CMake 3.25+, Ninja, Git and a C++23 toolchain (GCC 13+ or current MSVC).
For images and other binary assets, [install and fetch Git LFS](CONTRIBUTING.md).

```sh
cmake --preset desktop-release
cmake --build --preset desktop-release --parallel 4
python scripts/run_tests.py --preset desktop-release
./build/desktop-release/src/desktop/crucible_desktop
```

Windows uses a VS developer prompt and `.exe` suffix. Linux needs X11 or Wayland
development libraries and a display. Use **1/2/3** for Attract/Repel/Erase, **4** for
Flow; **Tab** selects a field slot. Click for a radial edit, drag and release for a
straight current. Middle drag pans, wheel zooms, **F** fits, **Space** pauses,
**R** restarts, **Delete** erases and **Escape** cancels a preview. Admitted edits
apply at the next tick boundary; paused edits wait for resume. Terminal runs permit
camera inspection and restart, and refuse field edits.

macOS CI uses Homebrew LLVM 21 and its matching C++23 standard library:

```sh
brew install llvm@21
export CRUCIBLE_LLVM_ROOT="$(brew --prefix llvm@21)"
cmake --preset macos-release
cmake --build --preset macos-release --parallel 4
python scripts/run_tests.py --preset macos-release
./build/macos-release/src/desktop/crucible_desktop
```

This is a developer build, with runtime paths into LLVM. Apple redistribution,
iOS signing/touch and lifecycle validation need separate receiving.

## Headless scenarios and investigations

The default graph stays SDL-free:

```sh
cmake --preset release
cmake --build --preset release --parallel 4
python scripts/run_tests.py --preset release
./build/release/crucible --mission
./build/release/crucible --mission --route
```

The passive and swept-attractor cases report outcomes, conserved biomass and exact
full-state replay. They are reproducible scenarios, not human playtests or benchmarks.
Historical [movement](docs/concepts/phase2-state.md) and
[finite reclamation](docs/decisions/phase3-resource-rules.md) exports remain available.
The opt-in [relay-density investigation](spikes/structural/README.md) uses the same
production Simulation, command ingress and owned snapshots; it adds no gameplay engine.

## Development

[CONTRIBUTING.md](CONTRIBUTING.md) covers presets, prerequisite checks, Debug/Release
and ASan/UBSan. Functional CI covers Linux, Windows and macOS, SDL-free headless and
software Vulkan receiving. [Benchmarking](docs/benchmarking.md) defines advisory
measurement and reproducible evidence.

Dependencies use a checksum-verified CPM bootstrap, namespaced targets and
[full commit pins](cmake/DependencyPins.cmake): Sub0ECS v2, Sub0Pub v2, Sub0Pipeline,
Sub0Log and Sub0HexGrid H2. Consumer builds disable dependency development tools.
The [reuse catalog](docs/reuse/README.md) tracks concrete feedback to those libraries;
game policy stays local until an independently consumed boundary justifies extraction.

[Reviewed phases](docs/phases/README.md) select increments from the
[capability backlog](docs/work-breakdown.md). The [workstream guide](docs/workstreams/README.md)
defines ownership; the [active log](docs/ACTIVE_WORK_LOG.md) reserves paths and CPU.
[Sprint reviews](docs/sprint-reviews/README.md) retain findings, evidence and follow-ups.
The repository's own license remains undecided; dependency licenses stay with their projects.
