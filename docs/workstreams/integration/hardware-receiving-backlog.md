# Development prerequisites and hardware receiving backlog

2026-10-03. Planning infrastructure, not Phase 8 dispatch, device inventory or
hardware reservation. Architect accountable; architect plus at most two workers.
Consume the [Phase 8 proposal](../../phases/phase8.md),
[Phase 7 review](../../sprint-reviews/phase-07.md),
[receiving](phase7-validation.md) and [publication receipt](phase7-publication.json).
Verify the current merged baseline at dispatch; the Phase 7 PR17 acceptance and
PR18 documentation closeout are historical inputs, not acceptance for a new head.
The [P07-F01 recovery receipt](phase7-evidence/recovery.json) now records verified
original source/capture bytes and hashes, hosted artifact inspection and merged
baseline reconciliation. Publication of this recovery increment remains pending;
finish its exact-head acceptance/publication before implementation starts. This
backlog does not supply the publication proof or close a hardware/human gate.

## Audit before work or delegation

Portable development readiness and device receiving are distinct. A C++23
compiler, CMake/Ninja, pinned dependencies and passing headless tests establish
portable prerequisites. They cannot establish that a target GPU accepts the actual
shader, supports the required formats or executes its upload/retirement contract.
Existing public Linux/Windows/macOS CI compilation is not physical GPU receiving;
Phase 7's executing Vulkan evidence is explicitly llvmpipe software evidence.
No macOS Metal, Windows D3D12 or iOS physical inventory is asserted here.

Before a build, identify the selected preset, intended test/capture scope, fresh or
restored build tree, compiler/architecture and trusted source checkpoint. Audit
only consumed capabilities: headless selection needs no GPU, display server,
shader translator or Apple signing identity. Desktop/GPU selections additionally
need their declared SDL/platform development libraries, runtime loaders, shader
tools and display facilities. Check restored helpers as well as launchers: the
Phase 7 Xvfb launcher alone did not prove its xkbcomp helper existed.

Before delegating hardware work, the architect records available capability and
matches it against the scoped gate. An ordinary agent session on the same machine
adds labor, not a different GPU, OS, trust scope or uncontended measurement host.
A **hardware-backed session** is a verified environment with the required target
device, toolchain, access policy and reserved execution window. Request one only
when the current environment cannot meet a consumed gate. If unavailable, retain
the gate as blocked and continue independent portable work; do not pretend an
agent spawn supplies hardware or book/lease unverified hardware from this plan.
Remote access or leasing requires a separately authorized provider workflow and
known cost limits before use.

| Required capability | Receiving scope | Insufficient substitute |
|---|---|---|
| Public Linux physical Vulkan device, compatible loader and display | Actual declared shader/device execution; later window/present and bounded device behavior | llvmpipe, shader compilation or withheld-query wrappers |
| Public Windows physical D3D12 device and public shader/toolchain support | Actual second receiver ABI/color/geometry/upload/drain on the selected Windows device | Desktop CI compilation or a Linux Vulkan run |
| Public macOS physical Metal device and public Apple toolchain | Actual second receiver and selected Metal shader artifact; later native window/lifecycle | A generated Metal artifact or macOS desktop compilation |
| Public iOS SDK plus authorized signing/package path and physical device | Installable package, touch coordinates, background/foreground, teardown and lifecycle | Simulator/build-only result, macOS Metal execution or an unsigned bundle |
| Physical display/input and consenting participants | DPI/readability, input-to-present, comprehension/accessibility with recorded confounds | Offscreen pixels, automated timing routes or moderator opinion |

Keep source/platform packages within their declared public trust scope. Non-public
SDKs, headers, platform documentation, logs or captures stay outside this public
repository. Public inventory omits device serial numbers, account identities,
credentials, signing secrets and private SDK material. Sanitized capability and
blocking reason suffice; restricted work requires its own authorized boundary.

## Build-and-test preflight contract

The companion [`scripts/check_prerequisites.py`](../../../scripts/check_prerequisites.py)
complements
[`scripts/run_tests.py`](../../../scripts/run_tests.py); it does not replace CTest
or widen its execute-bit repair scope. Its development and selected-test stages
return a receipt with `ok`, `checks`, `selected_tests` and `scope`; required failures
or unknown capabilities block the caller. The implementation owner freezes its
CLI/schema and records actual automated scope during receiving. The requirements
below include retained audit work, not a claim that every check is automated.

1. Select a repository preset and declared scope, then report resolved tool paths,
   versions, platform/architecture and preset requirements. Distinguish build-time
   compiler/shader tooling from runtime loader/display/device capabilities.
2. Probe tools and consumed compile/link/load requirements in an isolated temporary
   directory with bounded subprocesses. A restored cache or executable cannot
   substitute for source/toolchain identity. Dependency access checks preserve
   pinned full commits, TLS/checksum verification and consumer option isolation.
3. For sanitizer selection, run a small normal-exit ASan/UBSan/LSan canary before
   expensive acceptance. A fatal LSan namespace/process-inspection failure blocks
   local sanitizer acceptance. Never use `detect_leaks=0` to turn that gate green;
   preserve diagnostics and receive leak-enabled acceptance on a capable host.
4. For declared display/GPU scope, report launcher/helper and loader availability
   separately from actual shader/device execution. Inventory classifies physical,
   software or unknown; only an actual required receiver run may close its device
   gate. Tool presence or a probe from another backend cannot infer shader support.
5. Return actionable capability results with commands, exit status, duration,
   timeout and bounded output. Missing external capability is blocked; a receiver
   behavior failure remains failed. A receipt may encode unknown capability or a
   failed probe; the integration record explains why acceptance is blocked. Neither
   retries, package installation, global
   chmod, SDK changes nor system repairs occur silently. Artifact permission repair
   remains the existing owned native command/REQUIRED_FILES mechanism.

Preflight is a cheap decision gate before allocating build CPU or a device session.
After it passes, run the actual supported Debug/Release/sanitizer acceptance through
`python scripts/run_tests.py --preset <preset>`. A preflight pass does not replace
the suite, capture inspection, exact-head CI or merge-baseline verification.

## Bounded infrastructure backlog

HW IDs identify infrastructure work and evidence packages; they supplement the
existing stable follow-ups below. All are proposed/open until their named evidence
is received. Role owners become named session owners only at explicit dispatch.

| ID / accountable owner | Next bounded package | Receiving and stop gate |
|---|---|---|
| **HW-01 / integration** | Receive companion preflight on selected headless, desktop and GPU presets, including restored-tool and sanitizer-canary diagnostics | Check scope selection, bounded probe failures and truthful blocked status; headless must not require a device. Preserve run_tests repair boundaries. Retain Windows Job Objects descendant cleanup: current Windows timeout kills/reaps only the direct child and cannot claim a bounded process tree. Do this before expensive builds/device requests |
| **HW-02 / integration + platform** | Public hardware availability/remote-session spike: compare existing authorized runners with one authorized remote lease workflow, if available | Record required versus available capabilities, access/trust boundaries, reservation/cost/timeout/recovery policy and evidence return path. No invented inventory, provisioning or spending; unavailable targets remain blocked. Match capabilities before delegation |
| **HW-03 / rendering** | At most two shader-production prototypes: build-time versioned backend artifacts versus pinned runtime shadercross, using the current vertex/fragment pair and owned packet | Record compiler/source/output hashes, clean reproduction and bounded error cleanup. Choose an approach only after one available second backend consumes actual output; stop without a winner if unavailable. Keep the default SDL renderer |
| **HW-04 / rendering + integration** | One verified hardware-backed public desktop session at a time: Linux Vulkan reference, then whichever of Windows D3D12/macOS Metal is actually available | Receive declared ABI/color/geometry, three-slot ownership, receipt identity, pending destination preservation and drain against exact source. Separate compilation-only, software and physical results. No automatic three-platform dispatch |
| **HW-05 / presentation + integration** | After a complete production world/HUD/window consumer exists, physical full-frame 2K/100K/150K shootout on a reserved device | Receive window/input/resize/minimize/present, DPI and evolving tick/capture/pack/upload/present p95/p99, memory/allocations and input-to-present. Freeze workloads and baseline/current pairs first; no speed/FPS claim from offscreen or isolated loops |
| **HW-06 / rendering + platform** | Bounded physical delayed-saturation and failure-policy session only on an approved safe device/fixture | Define backpressure, last-frame behavior, resource/timeout bounds and platform recovery policy first. Safe manual platform-policy observations or bounded approved fixtures may receive a declared subset. Never induce a risky whole-host driver hang; leave unobserved loss/hang gates open |
| **HW-07 / platform + mission/presentation** | Separate iOS packaging/touch/signing/lifecycle receiving from a short human/DPI/accessibility session on available physical devices | Receive install/run/lifecycle under authorized signing without public secrets. Record consent, build/device, display scale/input method, participant/task/confounds and observations for human work. No humans means prepared protocol only; iOS build-only means compilation only |

HW-02 and HW-03 are decision spikes before receiver expansion. HW-04 does not
promote GPU default; HW-05/06 require a real complete consumer and declared safe
scope. HW-07 is two independently scoped sessions, not a requirement to create
permanent platform agents. Keep rendering/shader/lifetime work consolidated until
the contract is consumed and availability justifies a split.

## Hardware session handoff and evidence

Before execution, the architect supplies the session an exact source package:
commit/tree, package digest or tracked-source manifest, verified merged baseline,
dependency pins, relevant shader hashes/artifacts, preset, scoped task, required
capabilities, frozen oracle/tolerances, exclusive paths and acceptance limits.
Any necessary patch receives a new checkpoint before expensive device work. No
reconstruction of missing original evidence or claims against a different head.
Reserve CPU/device time serially, with no competing build or benchmark on the host.

The receiving record contains source head/tree and actual checkout (including a
synthetic CI merge), dirty state, source/package/executable/shader hashes, commands,
raw stdout/stderr/results, exit statuses and timeouts. Record sanitized OS/toolchain,
public SDK/runtime/driver versions, backend/device model and physical/software/
unknown classification, display/DPI/input configuration, capability probe outcomes,
trust scope, elapsed time, CPU/GPU/resource cost, reservation limits and confounds.
Preserve failed attempts and reasons separately from successful runs.

Captures include raw output and converted image hashes, capture commands/tools,
dimensions, inspected cases, declared masks/tolerances and reviewer findings.
Archive evidence with artifact identity/hash/retention and exact source linkage;
record whether contents were independently reopened and visually inspected.
Submission, successful readback and presentation are separate events. No new speed
claim follows without controlled complete-workload measurements under
[benchmarking rules](../../benchmarking.md). Hosted/shared timings stay advisory.

## Stable follow-up mapping and audit gaps

| Existing ID | Infrastructure support / preserved receiving gate |
|---|---|
| P07-F01 | [Recovery proof](phase7-evidence/recovery.json) verifies original bytes/hashes, hosted artifact inspection and checkout reconciliation; current increment acceptance/publication remains the final gate before dispatch |
| P05-F02 | HW-03/04/05: selected actual second backend, physical public devices and complete world/HUD 2K/100K/150K measurements; software offscreen acceptance remains partial |
| P06-F02 | HW-04/06: preserve Phase 7 controlled faults; physical delayed saturation/loss/hang remain open until safely observed within declared scope |
| P04-F01 / P04-F03 | HW-07: iOS package/touch/device/lifecycle and Apple redistribution/signing gates remain distinct |
| P04-F02 / P05-F01 | HW-05/07: physical DPI/input-to-present and actual human accessibility/comprehension/balance evidence; prepared protocols/automated routes cannot close them |
| P06-F01 | Retain [faction groundwork](../../decisions/faction-extensibility.md): organism/faction/controller/infection/resource identity and authority at a real consumer. Synthetic colors and cell/marker ranges are not factions |
| P02-F02 / P01-F03 / P01-F04 / P01-F05 | Retain delayed/concurrent exchange, structural/fusion ledger, relay and joined scale gates; hardware availability does not dispatch these systems |
| P02-F03 / P03-F01 / HX-07 | Retain terrain/traversal, spatial-cost and exact geometry/oracle receiving before any topology or pin change |

Current audit gaps are target physical-device availability, authorized remote
session/lease workflow, actual second-backend shader execution, complete interactive
GPU world/HUD/window receiving, safe physical failure policy, iOS package/signing/
touch/lifecycle and participant evidence. This document adds no devices, booking,
performance result, faction APIs or default-backend decision. Reconcile recovery
and the companion script's delivered scope separately during integration; then
dispatch only the smallest package whose capabilities and gates are actually ready.
