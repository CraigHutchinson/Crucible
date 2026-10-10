# Phase 15 framework tool and link receiving

The first production consumer is the live silver/cyan cinematic scene and themed
ImGui intro/menu. Filament 1.77.3 now has an actual isolated Windows Release source,
material-tool and C++23 link receipt. Rendering, window submission, capture,
font-atlas extremes and physical-device receiving remain open. This increment does
not advance the performance or Apple/native-presentation gates.

## Reproduce the bounded qualification

During an assigned CPU handoff, from the checkout root:

```powershell
powershell -NoProfile -File scripts/production/qualify_filament.ps1 -Stage all -Jobs 3
```

Use `pwsh` if Windows PowerShell does not support the installed Visual Studio
developer-shell module. Stages `prepare`, `configure`, `build`, `tools`, `link` and
`inspect` can resume retained work. The helper receives only Windows MSVC Release;
it never runs the link consumer, tests or a GPU workload. It refuses an external
artifact directory, junction/symlink ancestors, a modified dependency source, a
different pin or a different compiler toolset. Its artifact lock prevents two
invocations sharing one cache. The three-job cap does not itself reserve the host;
coordinate CPU ownership in `docs/ACTIVE_WORK_LOG.md` first.

The helper clones a separate full source tree under this checkout's ignored
`build/phase15-qualification/source`. It leaves the primary sparse dependency
checkout and all shared caches untouched. Logs and argument-array receipts are
timestamped and retained on failure. Re-running a stage updates its working
outputs and `inventory.json`; preserve the timestamped receipts when comparing
attempts. No upstream source adaptation, installation, global Git setting or TLS
bypass was used.

Source cleanliness includes untracked files, not only tracked modifications. The
actual full clone also passed `git status --porcelain --untracked-files=all` before
this tightening; ignored build outputs remain outside the dependency source.

## Actual receiving, 2026-10-09

The receiving checkout was `.worktrees/phase15-performance`, base `8bdf587`.
Artifacts remain under `build/phase15-qualification/` in that checkout.

| Item | Actual result and artifact |
| --- | --- |
| Source | Full shallow `v1.77.3` clone, exact clean `d852e34cd5629a6f3851d613dcd17aa0ffd025a2`; bundled ImGui 1.92.5 |
| Toolchain | VS 18.7.11822.327 Insiders; toolset directory 14.51.36231; MSVC 19.51.36246.0 x64; CMake 4.2.3; Ninja |
| Initial configure | Failed C1902 PDB-manager mismatch in compiler try-compile with `/Zi`; `logs/configure-release-01.log` retained |
| Corrected configure | Passed with CMP0141 NEW and Embedded debug information (`/Z7`); `logs/configure-release-02.log` |
| Release build | `filament matc cmgen resgen imgui`, parallel 3, 764/764 steps, exit 0; `logs/build-release-01.log` |
| Material tools | Desktop Vulkan and mobile Metal material compilation, 16-pixel DFG generation, and C resource packaging all exited 0; `logs/20261009T161538968Z-*.log` |
| Exception macros | Actual pinned `utils/Panic.h` under `/EHsc /MD`: `_CPPUNWIND=1`, `__EXCEPTIONS=0`, `UTILS_EXCEPTIONS=0`; `compiler_macros.i` and `logs/20261009T161541836Z-compiler-macros.log` |
| Link-only caller | Isolated C++23 `/MD` executable linked actual VulkanDriver and ImGui objects; `link/qualification.map` and `logs/20261009T161627755Z-*.log`; executable never run |
| Hash and command inventory | `inventory.json` records source, cache, selected production compile commands, eleven archives, three tools, generated outputs and link artifacts |

The initial clone/configure/build commands were run directly with the same flags
now captured by the helper. The helper's corrected `tools`, `inspect` and `link`
stages were actually run. The entire `all` stage was not repeated after authoring
the helper. A first tool attempt treated `cmgen`/`resgen --version` as supported;
the failing resgen receipt was retained, and the helper now receives actual tool
work. A first link attempt omitted the public `filabridge` include directory and
failed on `filament/MaterialEnums.h`; the corrected closure passed. These are
resolved configure/import defects, not native-rendering failures.

The final configure enables Vulkan, exceptions and RTTI, disables static CRT,
OpenGL, Metal runtime, WebGPU, samples, SDL2, tests, matdbg, fgviewer and LTO, and
uses upstream's Windows CI build mode with shortened compilation disabled. The
upstream `/Zi` argument is overridden by final `/Z7`; the warning remains in the
build log. All selected runtime/ImGui commands contain `/MD`, not `/MT`.
Unmodified upstream warning policy remains isolated from Crucible targets.

`matc --version` prints `77`, the material format version. The release identity
comes from the exact source pin and recorded tool hash, not that output. The
Vulkan tool fixture hash is
`6a1e95b28448490575913309c4ca17b5f668616db15ba7f4ba0e976ede359885`.
The fixture proves compilation of a lit instanced material with a uniform core
intensity; it does not receive a distinct intensity for each instance.

## Imported closure and application boundary

The standalone fixture imports these eleven archives: `filament`, `backend`,
`math`, `utils`, `filaflat`, `filabridge`, `zstd`, `bluevk`, `smol-v`, `getopt` and
release-matched `imgui`. It also links `Shlwapi` and CMake's Windows system-library
closure. Public include directories are `filament/include`,
`filament/backend/include`, `libs/filabridge/include`, `libs/math/include`,
`libs/utils/include` and `third_party/imgui`. `math` is an actual archive in this
release. The fixture gives exact archive paths rather than adding Filament's
global flags/targets to Crucible's root configuration.

Root owns the SDL window, sole ImGui context/platform backend and application
flow. The concrete production renderer will own Filament resources and a narrow
attributed UI adapter. It must preserve root's context, theme and ini policy;
upstream `FilaguiHelper` cannot be used unchanged because it destroys even an
external context and allocates per draw list. No second ImGui core is permitted.

The actual MSVC macro result means pinned `utils::Panic` uses its fatal path even
with Filament's exception option enabled. The received binaries do not provide
catchable recovery from GPU loss. Public `Fence` observes render-thread progress;
`flushAndWait` is a qualified drain, and the Vulkan implementation discards queue
wait error results. Neither receipt proves checked GPU success, scanout or native
handoff. Completion callbacks must settle before their owned staging is reused;
a timeout retains that storage through a later qualified drain/teardown.

## First consumer storage envelope

The architect accepted these first-consumer ceilings after inspecting the helper
and two-font/menu envelope. They are contract bounds, not measured atlas/internal
engine peaks:

| Adapter-owned item | Ceiling |
| --- | --- |
| Whole UI frame | 32,768 vertices, 98,304 uint16 indices, 16 lists, 256 commands; 851,968 bytes at received default ImDrawVert layout |
| Live UI textures | Four, each at most 4096x4096 RGBA8; combined resident/retired storage at most 128 MiB |
| Texture actions | Four distinct actions/frame; 64 MiB aggregate bytes, one outstanding upload batch |
| Scene upload staging | 512 KiB/frame, including copied poses, transforms and per-instance core payload |
| Combined UI/scene slots | Three slots, 2 MiB each; no free slot means whole-frame skip before beginFrame |
| Cold capture | One owned RGBA buffer, at most 64 MiB |
| External CPU staging | 136 MiB = 6 MiB ring + 64 MiB texture batch + 64 MiB capture + 2 MiB metadata slack |

These bounds exclude Filament's internal heap, command buffers and GPU resources.
Reject the whole UI frame when preflight exceeds a bound; do not partially render
controls. Texture retirement still counts against the resident ceiling. The
first native receiver may explicitly restrict font/DPI combinations until the
complete dynamic-atlas envelope is received.

The source-supported first scene route uses automatic transform batches no larger
than `Engine::getMaxAutomaticInstances()` and a 64x64 RGBA32F core-intensity atlas.
Each batch's material fetches `batchBase + getInstanceIndex()`; arithmetic preserves
each supplied intensity. At 4096 poses, proposed copied pose/core/matrix payload
is 475,136 bytes if their eventual compiled layouts are 36/16/64 bytes. Those
sizes require compile-time receiving before acceptance. The core atlas and float
texture support also require actual hardware capture across 63/64/65 and 4096
instances with distinct 0/1/4 intensities. This route is not a 150K performance
promise; manual batching (maximum 32,767 per batch) remains a separate gate.

## Open receiving gates

No renderer, SDL surface, GPU, capture or presentation was executed by this
qualification. Debug, sanitizer, Apple/Metal runtime, iOS packaging, DX12 route,
full font-atlas limits, checked device-loss behavior and performance remain open.
Next receive the bounded beveled silver/cyan wedge, lit floor, per-instance core
intensity and actual themed UI through one synchronous coordinator caller, then
inspect capture/resize/skip/teardown receipts on real hardware. Only promote the
parts supported by that evidence; keep the phase's remaining gates explicit.
