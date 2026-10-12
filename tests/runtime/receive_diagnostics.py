"""Parse actual production-session outcomes without interpreting message prose."""
import json
import subprocess
import sys


def receive(*args):
    result = subprocess.run([sys.argv[1], *args], capture_output=True, timeout=30, check=True)
    records = [json.loads(line) for line in result.stdout.splitlines()]
    health, = [json.loads(line) for line in result.stderr.splitlines()]
    assert health['schema'] == 1 and health['kind'] == 'crucible_health'
    assert health['valid'] and health['output_ok']
    assert int(health['records']) == len(records)
    assert health['unreadable_bytes'] == health['undecodable_records'] == '0'
    assert int(health['unwritten_bytes']) > 0
    assert health['producer_truncated_records'] == '0'
    events = {}
    for record in records:
        assert record['schema'] == 1 and record['kind'] == 'message'
        assert record['subsystem_id'] == 1 and not record['truncated']
        assert int(record['segment_generation']) != 0 and int(record['record_offset']) > 0
        fields = record['args']
        assert all(field['type'] == 9 for field in fields[:7])
        values = [int(field['value']) for field in fields[:7]]
        if values[1] == 3:
            assert fields[7]['type'] == 9
            values.append(int(fields[7]['value']))
        assert len(fields) == {1: 15, 2: 7, 3: 8}[values[1]]
        assert values[0] == 1 and values[2] == 1  # app schema and session run
        events.setdefault(values[1], []).append(values)
    return events, health


events, health = receive()
assert health['producer_dropped_records'] == '0'
assert set(events) == {1, 2, 3}
# Admission: queue full at tick0. Structural: shatter empty at tick1.
assert len(events[1]) == 1 and events[1][0][:5] == [1, 1, 1, 0, 1]
assert len(events[2]) == 1 and events[2][0][:5] == [1, 2, 1, 1, 4]
# Terminal loss at deadline4; target1780 and deadline are copied numeric fields.
assert len(events[3]) == 1 and events[3][0][:5] == [1, 3, 1, 4, 2]
assert events[3][0][6:] == [1780, 4]
exhausted, health = receive('exhaust')
assert int(health['producer_dropped_records']) > 0
assert sum(map(len, exhausted.values())) < 10000
assert set(exhausted) == {1}  # retained prefix, not falsely complete mission history
print('production JSONL: numeric events, mission deadline, bounded losses and health received')
