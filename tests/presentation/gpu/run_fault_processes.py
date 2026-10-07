"""Bound the unchanged GPU receiver's injected teardown failure/hang children."""
import argparse
import os
import signal
import subprocess
import sys


def kill_group(process):
    try:
        os.killpg(process.pid, signal.SIGKILL)
    except ProcessLookupError:
        pass  # The child exited between the timeout/poll and kill; still reap it.


def child(command, timeout, expect_hang):
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               text=True, start_new_session=True)
    timed_out = False
    try:
        stdout, stderr = process.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        timed_out = True
        kill_group(process)
        stdout, stderr = process.communicate(timeout=5)
    finally:
        if process.poll() is None:
            kill_group(process)
            process.wait(timeout=5)
    output = stdout + stderr
    print(output, end="", flush=True)
    if any(marker in output for marker in ("AddressSanitizer", "LeakSanitizer",
                                           "UndefinedBehaviorSanitizer", "runtime error:")):
        raise RuntimeError("sanitizer diagnostic invalidates expected child outcome")
    if expect_hang:
        if not timed_out or process.returncode != -signal.SIGKILL or "FAULT_HANG_WAIT_ENTERED" not in output:
            raise RuntimeError("hung wait did not reach its hook and get killed/reaped")
        if "FAULT_UNSAFE_RELEASE" in output:
            raise RuntimeError("hung receiver attempted resource release")
        print("FAULT_HANG_KILLED_REAPED", flush=True)
    else:
        if timed_out or process.returncode != 86 or "FAULT_DRAIN_FALSE_NO_RELEASE" not in output or "FAULT_TERMINATE_NO_RELEASE" not in output:
            raise RuntimeError("failed drain did not reach exact no-release termination")
        if "FAULT_UNSAFE_RELEASE" in output or "FAULT_UNEXPECTED_TERMINATE" in output:
            raise RuntimeError("failed drain ownership invariant failed")
        print("FAULT_EXPECTED_TERMINATION_REAPED", flush=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("executable")
    parser.add_argument("vertex")
    parser.add_argument("fragment")
    args = parser.parse_args()
    base = [args.executable, args.vertex, args.fragment]
    child(base + ["failed-drain"], 30, False)
    child(base + ["hang"], 15, True)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"GPU fault process fixture: {error}", file=sys.stderr)
        sys.exit(1)
