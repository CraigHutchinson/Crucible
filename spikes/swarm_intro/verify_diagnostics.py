#!/usr/bin/env python3
"""Independent receiver for the swarm intro's Sub0Log JSONL diagnostics.

Runs crucible_swarm_intro with --events-jsonl/--health-json, parses every line with the
standard JSON parser and cross-checks the application events against each other and
against the run's own summary line. Exit status 0 means every check passed.

    python3 verify_diagnostics.py path/to/crucible_swarm_intro --build climb
"""
import collections
import json
import pathlib
import subprocess
import sys
import tempfile

LOCK, CLAIM, STUCK, RECOVERED, MAGNET, LETTER, SUMMARY, SUPPORTED = 2, 1, 3, 4, 5, 6, 7, 8
U64 = 9   # Sub0Log TypeCode for unsigned 64-bit arguments


def fail(message):
    print(json.dumps({"diagnostics": "FAIL", "check": message}))
    sys.exit(1)


def main():
    binary, extra = sys.argv[1], sys.argv[2:]
    with tempfile.TemporaryDirectory() as tmp:
        events_path = pathlib.Path(tmp, "events.jsonl")
        health_path = pathlib.Path(tmp, "health.json")
        result = subprocess.run([binary, *extra, "--events-jsonl", str(events_path), "--health-json", str(health_path)],
                                capture_output=True, text=True, check=False)
        if result.returncode != 0:
            fail(f"run exited {result.returncode}: {result.stderr.strip()}")
        summary = json.loads(result.stdout.splitlines()[0])
        health = [json.loads(line) for line in health_path.read_text().splitlines()]
        events = [json.loads(line) for line in events_path.read_text().splitlines()]

    if len(health) != 1 or health[0].get("kind") != "swarm_health":
        fail("exactly one swarm_health receipt")
    h = health[0]
    if not (h["valid"] and h["output_ok"]):
        fail("receipt valid and output ok")
    for key in ("producer_dropped_records", "producer_truncated_records", "unreadable_bytes", "undecodable_records"):
        if int(h[key]) != 0:
            fail(f"receipt {key} is zero")
    if int(h["records"]) != len(events):
        fail("receipt record count matches the JSONL lines")

    by_event = collections.defaultdict(list)
    for record in events:
        if record.get("schema") != 1 or record.get("kind") != "message":
            fail("every line is an upstream schema-1 message")
        if record["subsystem_name"]["value"] != "swarm_intro":
            fail("every message names the swarm_intro subsystem")
        args = record["args"]
        if any(a["type"] != U64 for a in args):
            fail("application arguments are unsigned 64-bit")
        values = [int(a["value"]) for a in args]
        if values[0] != 1:
            fail("application schema 1")
        by_event[values[1]].append(values)

    slots = summary["slots"]
    locks = by_event[LOCK]
    if len(locks) != summary["locked_at_end"] or len({v[4] for v in locks}) != len(locks):
        fail("one lock event per locked slot, no slot locked twice")
    if len({v[3] for v in locks}) != len(locks):
        fail("no unit locks twice")
    summaries = by_event[SUMMARY]
    if len(summaries) != 1:
        fail("exactly one summary event")
    _, _, _, s_slots, s_locked, s_moored, s_claims, s_stuck, s_recovered = summaries[0]
    if s_slots != slots or s_locked != len(locks):
        fail("summary event matches the run")
    completes = [v for v in by_event[LETTER] if v[4] == 1]
    if summary["locked_at_end"] == slots and len(completes) != 8:
        fail("eight letter-complete events when the title is complete")
    uprights = {v[3] for v in by_event[LETTER] if v[4] == 2}
    raised = {k for k in range(8) if (summary["raised_letters"] >> k) & 1}
    if summary["all_raised_s"] >= 0 and uprights != raised:
        fail("an upright event for exactly the raised letters")

    claims = by_event[CLAIM]
    if len(claims) != s_claims or len(claims) != summary["claims"]:
        fail("claim events match the summary")
    if summary["claims"] > 0:
        # First come, first served: every locked slot was claimed at a letter's base, never
        # assigned in advance, and no unit waited in a queue longer than the sequence.
        # Hybrid builds raise some letters with planned slots: check the letters that claim.
        claimed = {v[4] for v in claims}
        claiming_letters = {v[5] for v in claims}
        if any(v[4] not in claimed for v in locks if v[5] in claiming_letters):
            fail("every locked slot of a climbed letter was claimed first")
        if any(v[8] > 15 * 60 for v in claims):
            fail("no claim waited in the queue longer than the sequence")

    ticks = [int(r["args"][2]["value"]) for r in events]
    if any(b < a for a, b in zip(ticks, ticks[1:])):
        fail("events are recorded in tick order")

    stuck = [v for v in by_event[STUCK] if v[6] in (1, 2)]
    if len(stuck) != s_stuck or len(stuck) != summary["stuck"]:
        fail("stuck events (wait/push) match the summary")
    if len(by_event[RECOVERED]) != s_recovered or len(by_event[RECOVERED]) != summary["recovered"]:
        fail("recovered events match the summary")
    if summary["stuck_at_end"] == 0 and len(by_event[RECOVERED]) < len(stuck):
        fail("every stuck episode has a recovery when none remain stuck")

    on = collections.Counter()
    for v in by_event[MAGNET]:
        on[(v[3], v[4])] += 1 if v[5] else -1
        if not 0 <= on[(v[3], v[4])] <= 1:
            fail("magnet modes alternate on/off per unit")

    print(json.dumps({"diagnostics": "PASS", "records": len(events), "locks": len(locks), "claims": len(claims),
                      "stuck": len(stuck), "recovered": len(by_event[RECOVERED]), "magnet": len(by_event[MAGNET]),
                      "supported": len(by_event[SUPPORTED])}))


if __name__ == "__main__":
    main()
