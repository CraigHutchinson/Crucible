"""Receive the real desktop destructor through the existing SDL dummy fixture."""
import json
import subprocess
import sys

result = subprocess.run([sys.argv[1], '--export-diagnostics'], capture_output=True,
                        check=True, timeout=20)
record, = [json.loads(line) for line in result.stdout.splitlines()]
health, = [json.loads(line) for line in result.stderr.splitlines() if line.startswith(b'{')]
assert record['schema'] == 1 and record['kind'] == 'message'
assert all(arg['type'] == 9 for arg in record['args'])
args = [int(arg['value']) for arg in record['args']]
assert len(args) == 8 and args[:5] == [1, 3, 1, 1, 2] and args[6:] == [1780, 1]
assert health['schema'] == 1 and health['kind'] == 'crucible_health'
assert health['valid'] and health['output_ok'] and health['records'] == '1'
for key in ('producer_dropped_records', 'producer_truncated_records',
            'unreadable_bytes', 'undecodable_records'):
    assert health[key] == '0', (key, health)
print('desktop shutdown JSONL and separate loss-free health received')
