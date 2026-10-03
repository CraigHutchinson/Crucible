# Reclamation challenge: prepared human observation protocol

Status: preparation only. No human session, recruitment, physical-device validation
or balance conclusion is recorded by this document. Automated strategy/replay
results are separate evidence. Conduct sessions only with consenting participants;
do not silently substitute agent-generated actions for participant input.

## Session record

Record date, participant pseudonym, prior strategy-game experience, exact build/head,
public operating system, physical input/display device, window dimensions, display
scale and any assistive needs volunteered. Record interruptions, pause duration,
completed ticks at first action and any moderator assistance. Do not collect names
or unnecessary personal information. Capture screen/video only with consent.

Keep quota 1780, deadline 900 and existing startup settings. The prototype starts
running immediately; ask the participant to press Space before briefing and record
the completed tick actually reached. This delay is a confound, not an alternative
seed. Restart immediately before each attempt; do not call elapsed wall time the
simulation deadline. Pause is an allowed planning tool.

## Tasks and observation

1. While paused, ask what the objective, recovered biomass and deadline mean and
   which things they think they can control. Record their words before explaining.
2. Explain only the available controls: 1 attract, 2 repel, 3 erase; Tab selects one
   of four slots; click places/replaces that slot's radial field; Delete removes the
   selected slot. Space pauses/resumes, R restarts, F fits the camera, middle drag
   pans and the wheel zooms. Radius 8 and magnitude 4 are fixed. Escape clears the
   current preview; it does not undo an already admitted command.
3. Ask them to place an attractor while paused, predict when it will take effect,
   resume and identify confirmation. Ask them to move that same slot and remove
   it. Observe whether preview, queued input and committed field are distinguished.
4. Restart and allow an unaided attempt to recover the quota before the deadline.
   Ask them to think aloud; avoid explaining the reference sweep during this
   attempt. Record first-input tick, pauses, tool/slot choices, outcome/tick,
   recovery and confusions. A failed attempt is a useful observation.
5. Ask them to explain the terminal state and find restart. On a fresh attempt,
   show the reference route only after the exploratory observations: successive
   slot-0 attractors at (8,8), (24,8), (40,8), (56,8), then (8,24), and so on.
   Automated requests every 60 ticks are a reference, not a demanded human cadence
   or a guarantee of the same outcome. Coordinates are world units; the prototype
   offers no typed coordinate entry. Record placement/timing deviations.
6. Ask how attract and repel differ visually, whether the resource recovery is
   understandable, and what they would change. Test panning, zooming and refitting
   while paused. Do not describe arbitrary colors as allied/hostile factions.

## Observation form

| Item | Participant explanation/action | Evidence tick or time | Assistance/confound |
|---|---|---|---|
| Objective and deadline comprehension | | | |
| Preview / queued / committed input | | | |
| Attract versus repel recognition | | | |
| Selected slot and replacement/removal | | | |
| Pause and planning | | | |
| Unaided outcome and restart | | | |
| Assisted route deviation/outcome | | | |
| Camera, readability and accessibility | | | |

Current usability questions include the rapidly progressing initial mission,
fixed field parameters, ring-based attract/repel distinctions, discovering slot
replacement, and understanding why a route changes reclamation. These are questions
to observe, not proven participant failures. Flow painting, field parameter editing,
fusion, relay and touch controls are absent; do not ask participants to complete
those tasks or score their absence as failed implementation in this increment.

After sessions, separate verbatim observations from moderator interpretation;
report the participant count, devices and limitations. Propose a bounded change
with a receiving fixture before tuning rules. Human feedback may warrant a new
increment; this protocol alone does not close mission comprehension or balance.
