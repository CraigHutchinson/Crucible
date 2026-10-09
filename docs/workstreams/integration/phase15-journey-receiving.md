# Phase15 application journey and profile receiving

This increment gives the production frontend one application owner for screens,
attempts and mission-boundary continuation. The real InspectorSession remains
authoritative for gameplay. The first receiving caller is the headless `crucible
--journey` path; native scene/input receiving belongs to the production renderer
checkpoint and is not established by these tests.

## Received behavior

Intro skip, menu, briefing, active play, pause/options, actual mission result,
fresh retry and confirmed quit are consumed through typed screen/serial intents.
Stale inputs and unknown mission/typography values are rejected before mutation.
Only active play pumps Runtime. Pause/options/confirmation clear the suspended
clock fraction; returning from pause to the menu explicitly ends that attempt.
The paused view warns about this discard policy. New construction precedes session
replacement, and application attempt identity does not reuse session-local IDs.

An optional ProfileStore receives at most256 bytes of version1 text. It stores
continuation mission, relay unlock and reduced-motion/fullscreen/text-scale
preferences. Mission IDs0/1 and typography0/1/2 have explicit wire values. It stores
no ECS state, command trace, tick checkpoint or active attempt. Continue opens a
fresh mission briefing; it never fabricates a restored live mission.

Saves acquire an exclusive sibling `.pending` directory, validate the current
destination's version/format, write complete temporary bytes, then replace the
same-parent file. A busy transaction, unsupported/malformed destination or invalid
value preserves the existing file. Only the transaction owner cleans its own
temporary file/empty directory. No recursive cleanup occurs. Windows receives
replacement through [MoveFileExW](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-movefileexw);
other platforms use filesystem rename. Uncoordinated edits during the lease and
crash/power-loss durability are outside this contract.

Save failure is an explicit player screen with retry or return without saving.
Retry restores the interrupted result/options screen; the old profile is retained
until replacement succeeds. Invalid files at startup cannot manufacture Continue
or be silently overwritten by later preferences. A live version change after load
is also refused at the next owned save transaction.

## Actual local checks

MSVC19.51.36246, C++23, Ninja, separate headless Release/Debug builds using the
unchanged Sub0 pins and received CPM cache. Both actual caller/test builds passed
50 steps; review fixes passed their9-step incremental rebuilds.

Five application CTest cases passed in each configuration through the required
prerequisite-aware runner. Final Release3.51s and Debug21.40s are suite execution
receipts, not performance benchmarks. They cover real mission win/loss, stale
intent rejection, suspended fraction reset, independent attempts, unlock/next,
abandonment, bounded profile format, replacement/refusal, live unsupported-file
change, failure/retry and actual win->write->relaunch continuation.

```powershell
python scripts/run_tests.py --preset release --prerequisite-build-dir build/phase15-journey-release --test-dir build/phase15-journey-release -L application --output-on-failure
python scripts/run_tests.py --preset debug --prerequisite-build-dir build/phase15-journey-debug --test-dir build/phase15-journey-debug -L application --output-on-failure
build/phase15-journey-release/crucible.exe --journey
build/phase15-journey-release/crucible.exe --journey --route --profile build/phase15-journey-release/manual-progress.txt
```

Logs remain in those owned build directories. Native code is tested with the
existing Crucible assertion/CTest convention; introducing a separate framework
solely for these fixtures was deferred. A reviewed the initial session/controller;
B reviewed the profile/shared-value integration and found two concrete consumer
defects: paused Back and live-file version refusal. Both were fixed and received
with dedicated cases. No unresolved consequential cpp-review MUST was reported.

## Open gates

Hosted exact-head platform/sanitizer/full-suite checks and publication receipts
are recorded in the phase review when complete. These local cases do not receive
ImGui input, native fullscreen, real scene/material/UI captures, GPU retirement,
audio, iOS package/touch or human comprehension. Rendering migration and scale
G3-G5 remain separate. Native fullscreen must receive platform success/error
before a saved preference is presented as achieved device state.
