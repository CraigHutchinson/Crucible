# P13-01 C++ review

Style profile: sub0 (declared in AGENTS.md at user direction); overlay: none.
Primary-agent cpp-review over the combined package, 2026-10-07. No independent
reviewer was dispatched. This is a diff review, not a whole-repository style audit.

Production wiring: toolbar geometry/action/enablement are consumed by ScenePainter
and DesktopApp; WindowMode is consumed by desktop startup; drawFeedback is called
by the production painter. No new unconsumed API, framework or dependency surface.

File-by-file findings:

- SceneUi: shared geometry, typed actions, bounds precondition and copied/borrowed
  value contract are explicit. Enablement was duplicated between paint/input;
  unified through `isToolbarActionEnabled`. New failure/query results are nodiscard.
- DesktopApp header/source: main-thread lifetime and RAII retain renderer-before-
  window destruction; constructor mode is consumed. Exit originally depended on
  setting fullscreen mode again; that unnecessary prerequisite was removed.
  Resize/DPI/fullscreen events now cancel unfinished gestures/reset wall time,
  preserving accepted receipts and fixed simulation ownership.
- Desktop main: `--fullscreen` reaches the constructor; callback token ownership,
  main-thread event guard and unknown-option failure are retained.
- ScenePainter: new feedback uses fixed stack text, synchronous borrowed storage,
  bounded two-row wrapping and ellipsis; render scale is restored. Fullscreen's
  return label originally exceeded its hit target and now reads F11 WIN. The
  world camera/clipping/numeric behavior is unchanged. No new hot-path allocation.
- New receiving test: snake_case filename and camelCase helpers; SDL resources
  owned by RAII, main-thread synthetic events, exact-sized outputs and independent
  visible pixel-row checks. Legacy disabled controls, accepted receipt preservation,
  resize cancellation, restart and repeated fullscreen lifetime are consumed.
- Remaining exporter/caller edits are shared-canvas/include-path updates. CMake
  includes the desktop source root and registers the real receiver explicitly.

No unresolved MUST findings in the new work. Pre-existing deviations remain:
PascalCase APIs/files, snake_case aggregate/member fields and compact brace layout.
The package adopts the profile and updates new/changed helper APIs; it does not
claim a repository-wide migration. C++23, Crucible namespace/CRUCIBLE_ options and
existing expected/exception policy resolve profile open items locally; no naming
or error-policy redesign is implied. Native display/device behavior and test
correctness/performance are separate receiving gates.
