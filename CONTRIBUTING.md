# Development workflow

For parallel sessions, start with [the workstream map](docs/workstreams/README.md).
Each stream owns local source/test manifests; shared wiring goes through the integrator.

Requirements: CMake 3.25+, Ninja, Git with Git LFS, Python 3.10+ and a C++23 toolchain with
std::expected (GCC 13+ or current MSVC). Use a VS developer prompt on Windows.
Clang requires a C++ standard library implementing std::expected.

From the repository root:

```sh
python scripts/check_prerequisites.py --stage development
cmake --preset debug
cmake --build --preset debug --parallel 4
python scripts/run_tests.py --preset debug
cmake --preset release
cmake --build --preset release --parallel 4
python scripts/run_tests.py --preset release
cmake --preset sanitize
cmake --build --preset sanitize --parallel 4
python scripts/run_tests.py --preset sanitize
```

The sanitizer preset requires GCC/Clang. Run it separately from timing work.
Use `scripts/run_tests.py` for local and CI execution. It asks CTest for the selected
commands and their explicit `REQUIRED_FILES` native prerequisites, repairs lost execute bits on owned ELF/Mach-O test artifacts under `build/`,
reports repairs, then runs the requested suite once. It preserves read/write modes,
ignores source/scripts/symlinks/external tools and leaves Windows modes unchanged.
Pass ordinary CTest filters after `--preset`; `--ctest path` selects a tool outside
PATH. A real test failure propagates without automatic retry. After a test that
failed to launch, use the runner with `--rerun-failed` instead of repeating the suite.
Use clang-format with .clang-format. Avoid global compiler flags and hidden fetches.
Only benchmark tools are opt-in; correctness tests stay enabled.

Dependency integration changes must configure from a fresh build tree and run the
stack round-trip test. Change full SHA pins deliberately, record the source branch,
and verify the Pub v2, ECS master and Pipeline main APIs. CPM_SOURCE_CACHE can be set in the environment;
CPM_Sub0ECS_SOURCE and equivalent package overrides support local checkouts.
The CPM bootstrap is SHA256-verified and dependencies are not installed system-wide.

Review gates: explain behavior, validate ownership and data access, report tests
actually run, document remaining gaps. For hot-path changes capture baseline/current
Release results on the same machine as described in docs/benchmarking.md.

When execution is interrupted, retain the branch, build directories and evidence.
Record the commit, completed checks, tool paths and next action in
`docs/ACTIVE_WORK_LOG.md`; resume from that checkpoint. Run the test wrapper before
rebuilding unchanged code. If only a tool permission or launch problem blocked a
case, repair that specific owned artifact/tool and rerun the failed selection; do
not repeat successful checks unless code or configuration changed.

Sprint completion includes visually inspected examples wherever viable. Plan the
capture with the package, preserve reproduction/provenance and link it from the
review; see [visual evidence criteria](docs/phases/README.md#visual-examples-at-each-iteration).

Before long validation, publish a coherently reviewed source checkpoint to the sprint
branch and record its remote SHA (draft PR optional). Preserve acceptance outputs
with that checkpoint and label pending/partial results. A local commit does not
protect against loss of the execution service. Use hosted exact-head CI if local
transport fails; do not infer a pass or rerun successful checks. Reconcile local
branches/build artifacts after recovery. Exact authored-file recovery must preserve
blob identity; do not recreate code from an approximate description.

Before expensive builds, run `python scripts/check_prerequisites.py --stage development`.
Add `--sanitize` for ASan/UBSan compile and normal-exit runtime checks; add `--gpu`
for the optional Vulkan/shader profile. Use `--build-dir build/<preset>` to probe
an existing cached compiler and flags. The development canary exercises C++23
`std::expected`, thread startup and executable loading. Failures/unknown required
capabilities block; the checker never installs tools or disables leak detection.

The test runner checks the exact selected CTest inventory after its narrow mode
repair. Portable tests require no GPU. Selected executing GPU cases additionally
check Linux X11 connectivity, Vulkan candidate inventory and declared SPIR-V magic;
actual shader/device/readback acceptance remains the test's job. Other display
backends need a supported receiving session; current diagnostics report unknown.
`--require-hardware` rejects software/mixed/unknown inventories and establishes
candidate availability only. `--prerequisite-report <path>` saves the receipt;
custom/UserPresets use `--prerequisite-build-dir <path>`. Inspection via `-N` or
`--show-only` launches no capability probes. Dependency fetch/link, platform SDK,
Xvfb helper and complete preset requirements remain explicit setup audits.

See the [hardware receiving backlog](docs/workstreams/integration/hardware-receiving-backlog.md)
for dedicated-session capability matching, public/private trust boundaries, scoped
stop gates and evidence handoff. An additional agent on this host supplies labor,
not additional hardware. Missing hardware leaves its gate open while independent
portable work proceeds. Windows descendant-process timeout cleanup remains HW-01.

## Interactive review

For interactive work, pass the matching automated behavior/lifetime checks first,
then provide [personal review steps and expected results](docs/playtests/phase12-personal-review.md)
for the exact build. Manual review addresses comprehension/presentation; native
receiving and synthetic fixtures remain distinct. Use computer control mainly
to diagnose a reported failed step. Review README as a fresh viewer before closing
an increment: features, launch, controls, optional prerequisites and limits must
be readily discoverable.

## Binary assets and Git LFS

Install [Git LFS](https://git-lfs.com/) before cloning. For a new or existing
checkout, run:

```sh
git lfs install --local
git lfs pull
git lfs ls-files
```

The root `.gitattributes` tracks raster art, editable art sources, binary models,
audio and video through LFS. Keep source code, SVG, JSON, logs and other text
in ordinary Git. Add new binary formats deliberately with `git lfs track`, then
commit `.gitattributes` with the assets. Use `git add` and `git commit` normally;
the LFS pre-push hook uploads the payloads before publishing their pointers.
GitHub Actions checkouts request LFS payloads explicitly.

After committing assets, run `git lfs fsck --pointers` and `git lfs fsck`.
The setup converts current tracked PNGs to LFS without rewriting published
history. Historical Git blobs remain in older commits. Do not run
`git lfs migrate import` on shared branches without a separately coordinated
history migration.
