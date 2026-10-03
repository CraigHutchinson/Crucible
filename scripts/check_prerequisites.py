"""Bounded capability diagnostics; a receipt is infrastructure evidence, not test acceptance."""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import platform
import re
import shlex
import signal
import subprocess
import sys
import tempfile
import threading
import time

GPU_TESTS = {"gpu_instanced_readback", "gpu_fault_receiving", "gpu_fault_process"}
EXPECTED_SOURCE = """#include <expected>
#include <memory>
#include <stop_token>
#include <thread>
int main() {
    const std::expected<int, int> value{23};
    const auto owned = std::make_unique<int>(*value);
    std::jthread worker{[](std::stop_token token) { (void)token.stop_requested(); }};
    worker.request_stop();
    return *owned == 23 ? 0 : 1;
}
"""
X11_PROBE = """import ctypes, ctypes.util, sys
lib = ctypes.CDLL(ctypes.util.find_library('X11') or 'libX11.so.6')
lib.XOpenDisplay.argtypes = [ctypes.c_char_p]
lib.XOpenDisplay.restype = ctypes.c_void_p
lib.XCloseDisplay.argtypes = [ctypes.c_void_p]
display = lib.XOpenDisplay(None)
if not display:
    sys.exit('XOpenDisplay failed in the current execution namespace')
lib.XCloseDisplay(display)
print('XOpenDisplay connected')
"""


def split_arguments(value: str) -> list[str]:
    """Preserve Windows backslashes and quoted flag paths; POSIX uses shell words."""
    if platform.system() == "Windows":
        return [word.replace('"', '') for word in re.findall(r'(?:[^\s"]+|"[^"]*")+', value)]
    return shlex.split(value)


def read_cache(build_dir: Path | None) -> dict[str, str]:
    """Read configured values without guessing from a preset name."""
    if build_dir is None:
        return {}
    cache = {}
    for line in (build_dir / "CMakeCache.txt").read_text().splitlines():
        if line and not line.startswith(("#", "//")) and "=" in line:
            key, value = line.split("=", 1)
            cache[key.split(":", 1)[0]] = value
    return cache


def run_probe(name: str, command: list[str], *, timeout: float, env: dict) -> dict:
    """Drain stdout into a bounded tail; POSIX timeouts kill/reap the probe group."""
    started = time.monotonic()
    result = {"command": command, "timeout_seconds": timeout,
              "process_scope": "POSIX process group" if os.name == "posix" else "Windows direct child only"}
    tail = bytearray()
    total = [0]
    try:
        process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                   env=env, start_new_session=os.name == "posix", bufsize=0)

        def drain():
            try:
                while chunk := process.stdout.read(4096):
                    total[0] += len(chunk)
                    tail.extend(chunk)
                    del tail[:-6000]
            except (OSError, ValueError):
                pass

        def kill_group():
            if os.name == "posix":
                try:
                    os.killpg(process.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
            else:
                process.kill()

        reader = threading.Thread(target=drain, daemon=True)
        reader.start()
        try:
            returncode = process.wait(timeout=timeout)
            result.update(status="pass" if returncode == 0 else "fail", returncode=returncode)
        except subprocess.TimeoutExpired:
            kill_group()
            try:
                process.wait(timeout=1)
                cleanup = ""
            except subprocess.TimeoutExpired:
                cleanup = "; killed child could not be reaped within 1s"
            result.update(status="unknown", returncode=process.returncode,
                          output=f"{name} timed out after {timeout:g}s" + cleanup)
        reader.join(timeout=0.2)
        if reader.is_alive():
            # A finished launcher must not leave an unbounded drain or live POSIX helpers.
            if os.name == "posix":
                kill_group()
            result.update(status="unknown", output=f"{name} descendants retained the output pipe")
            reader.join(timeout=0.2)
        process.stdout.close()
        result["output"] = (result.get("output", "") + "\n" + bytes(tail).decode("utf-8", errors="replace")).strip()[-6000:]
        result["output_truncated"] = total[0] > 6000
    except OSError as error:
        result.update(status="fail", output=str(error))
    result["elapsed_seconds"] = round(time.monotonic() - started, 3)
    return result


def disabled_leak_options(environment: dict) -> list[str]:
    """Runtime duplicate options use the last value; required leaks must remain on."""
    disabled = []
    for variable in ("ASAN_OPTIONS", "LSAN_OPTIONS"):
        options = {}
        for item in environment.get(variable, "").split(":"):
            if "=" in item:
                key, value = item.split("=", 1)
                options[key.strip()] = value.strip().lower()
        for key in ("detect_leaks", "leak_check_at_exit"):
            if options.get(key) in {"0", "false", "no"}:
                disabled.append(f"{variable}:{key}={options[key]}")
    return disabled


def parse_vulkan_devices(output: str) -> list[dict]:
    """Keep Vulkan's reported device identity/type, including CPU and unknown devices."""
    devices = []
    for block in re.split(r"(?m)^GPU\d+:", output)[1:]:
        name = re.search(r"deviceName\s*=\s*(.+)", block)
        kind = re.search(r"deviceType\s*=\s*(.+)", block)
        devices.append({"name": name.group(1).strip() if name else "unknown",
                        "type": kind.group(1).strip() if kind else "unknown"})
    return devices


def check_prerequisites(build_dir: Path | None = None, tests: list[dict] | None = None,
                        *, stage: str = "test", compiler: str | None = None,
                        sanitize: bool = False, require_hardware: bool = False,
                        gpu_requested: bool = False,
                        probe=None, env: dict | None = None, timeout: float = 10,
                        budget: float = 45) -> dict:
    """Return a machine-readable receipt; required fail/unknown checks block the caller.

    Test callers supply the *selected* CTest JSON tests after applying all filters.
    The injected probe has run_probe's signature; it exercises failure paths without
    executing native suites. Compiler canaries use temporary files, not build outputs.
    """
    if stage not in {"development", "test"} or timeout <= 0 or budget <= 0:
        raise ValueError("invalid stage or probe bounds")
    probe = probe or run_probe
    environment = dict(os.environ if env is None else env)
    start = time.monotonic()
    checks = []
    selected = tests if tests is not None else []
    receipt = {"schema": 1, "stage": stage, "ok": False, "checks": checks,
               "selected_tests": [test.get("name", "") for test in selected],
               "scope": "Capability preflight only; no performance, physical-console or suite acceptance claim",
               "capacity": {"cpu_count": os.cpu_count(), "memory_bytes": None,
                            "requirement": "No invented CPU/RAM minimum"}}

    def add(name, status, detail, required=True, **metadata):
        item = {"name": name, "status": status, "required": required,
                "detail": detail, **metadata}
        checks.append(item)
        return item

    def execute(name, command, *, probe_env=None):
        remaining = budget - (time.monotonic() - start)
        if remaining <= 0:
            return {"status": "unknown", "output": "Overall prerequisite probe budget exhausted"}
        return probe(name, command, timeout=min(timeout, remaining),
                     env=probe_env if probe_env is not None else environment)

    def record(name, result, **metadata):
        return add(name, result["status"], result.get("output", ""),
                   returncode=result.get("returncode"), command=result.get("command"),
                   elapsed_seconds=result.get("elapsed_seconds"),
                   process_scope=result.get("process_scope"),
                   output_truncated=result.get("output_truncated", False), **metadata)

    python_supported = sys.version_info >= (3, 10)
    add("python", "pass" if python_supported else "fail", "Python >=3.10 required",
        version=platform.python_version(), executable=sys.executable)
    if not python_supported:
        return receipt
    try:
        cache = read_cache(build_dir)
    except (OSError, ValueError) as error:
        add("configured_cache", "fail", str(error))
        return receipt
    if stage == "test" and tests is None:
        add("selected_inventory", "unknown", "Supply the filtered CTest JSON inventory")
        return receipt
    if stage == "test" and not cache:
        add("configured_cache", "unknown", "Configured CMakeCache.txt is required")
        return receipt
    if stage == "development":
        for tool in ("cmake", "ninja", "git"):
            result = execute(tool, [tool, "--version"])
            if tool == "cmake" and result["status"] == "pass":
                version = re.search(r"cmake version (\d+)\.(\d+)", result.get("output", ""))
                if not version:
                    result.update(status="unknown", output="Cannot determine CMake version")
                elif tuple(map(int, version.groups())) < (3, 25):
                    result.update(status="fail", output="CMake >=3.25 required")
            record(tool, result)

    compiler = compiler or cache.get("CMAKE_CXX_COMPILER") or environment.get("CXX")
    configuration = cache.get("CMAKE_BUILD_TYPE", "").upper()
    flags = split_arguments(cache.get("CMAKE_CXX_FLAGS", ""))
    flags += split_arguments(cache.get(f"CMAKE_CXX_FLAGS_{configuration}", ""))
    link_flags = split_arguments(cache.get("CMAKE_EXE_LINKER_FLAGS", ""))
    link_flags += split_arguments(cache.get(f"CMAKE_EXE_LINKER_FLAGS_{configuration}", ""))
    if cache.get("CMAKE_SYSROOT"):
        flags += ["--sysroot=" + cache["CMAKE_SYSROOT"]]
    if cache.get("CMAKE_CXX_COMPILER_TARGET"):
        flags += ["--target=" + cache["CMAKE_CXX_COMPILER_TARGET"]]
    if stage == "test" and not compiler:
        add("compiler", "unknown", "Configured C++ compiler is missing")
    if not compiler and stage == "development":
        llvm = environment.get("CRUCIBLE_LLVM_ROOT")
        if platform.system() == "Darwin" and llvm:
            compiler = str(Path(llvm) / "bin/clang++")
            flags = ["-nostdinc++", "-isystem", str(Path(llvm) / "include/c++/v1"),
                     "-D_LIBCPP_DISABLE_AVAILABILITY"]
            link_flags = [f"-L{llvm}/lib/c++", f"-Wl,-rpath,{llvm}/lib/c++",
                          f"-L{llvm}/lib/unwind", f"-Wl,-rpath,{llvm}/lib/unwind", "-lunwind"]
        else:
            compiler = "cl" if platform.system() == "Windows" else "c++"
    sanitize = sanitize or cache.get("CRUCIBLE_ENABLE_SANITIZERS", "OFF").upper() in {"ON", "TRUE", "1", "YES"}
    receipt["compiler"] = compiler
    receipt["sanitizers"] = sanitize
    if sanitize:
        disabled = disabled_leak_options(environment)
        if disabled:
            add("sanitizer_environment", "fail", "Required leak checking disabled: " + ", ".join(disabled))
            return receipt
    if stage == "development" or sanitize:
        if not compiler:
            add("compiler", "unknown", "Configured C++ compiler is missing")
        else:
            with tempfile.TemporaryDirectory(prefix="crucible-prerequisites-") as directory:
                source = Path(directory) / "capability.cpp"
                binary = Path(directory) / ("canary.exe" if os.name == "nt" else "canary")
                source.write_text(EXPECTED_SOURCE)
                compiler_command = split_arguments(compiler) if not Path(compiler).is_file() else [compiler]
                compiler_command += split_arguments(cache.get("CMAKE_CXX_COMPILER_ARG1", ""))
                is_msvc = Path(compiler_command[0]).name.lower() in {"cl", "cl.exe"}
                if is_msvc and sanitize:
                    add("sanitizer_canary", "fail", "ASan/UBSan preset requires GCC/Clang")
                else:
                    command = (compiler_command + flags + ["/nologo", "/std:c++latest", str(source), f"/Fe:{binary}", f"/Fo:{Path(directory) / 'canary.obj'}", "/link"] + link_flags
                               if is_msvc else compiler_command + flags + ["-std=c++23", "-pthread", str(source), "-o", str(binary)] + link_flags)
                    if sanitize:
                        command += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
                    result = execute("compiler_canary", command)
                    record("cxx23_expected", result, compiler=compiler)
                    if sanitize and result["status"] == "pass":
                        canary_env = dict(environment)
                        # Force the required check on; preserve all other user options.
                        canary_env["ASAN_OPTIONS"] = environment.get("ASAN_OPTIONS", "") + ":detect_leaks=1"
                        canary_env["LSAN_OPTIONS"] = environment.get("LSAN_OPTIONS", "") + ":leak_check_at_exit=1"
                        record("sanitizer_canary", execute("sanitizer_canary", [str(binary)], probe_env=canary_env),
                               leak_checking=True)
                    elif sanitize:
                        add("sanitizer_canary", "unknown", "Cannot execute canary because compilation failed")
                    elif result["status"] == "pass" and stage == "development":
                        record("runtime_canary", execute("runtime_canary", [str(binary)]),
                               scope="Bundled standard library load and consumed thread startup")

    gpu = any(test.get("name") in GPU_TESTS for test in selected)
    if stage == "development":
        gpu = gpu_requested or require_hardware
    receipt["gpu_required"] = gpu
    if require_hardware and not gpu:
        add("hardware_selection", "unknown", "Hardware requirement needs a selected GPU receiver test")
    if gpu:
        if stage == "development":
            record("shader_compiler", execute("shader_compiler", ["glslangValidator", "--version"]))
        if stage == "test":
            backend = environment.get("SDL_VIDEO_DRIVER", environment.get("SDL_VIDEODRIVER", ""))
            if platform.system() == "Linux" and backend in {"", "x11"} and environment.get("DISPLAY"):
                record("display", execute("display", [sys.executable, "-c", X11_PROBE]),
                       backend="x11", display=environment.get("DISPLAY"),
                       scope="XOpenDisplay in the same execution namespace; SDL initialization remains a test gate")
            else:
                add("display", "unknown", "Receiver requires SDL video; this diagnostic verifies Linux X11 only. Dummy/Wayland/other backends require a dedicated supported session.",
                    backend=backend or "unknown")
        result = execute("vulkan", ["vulkaninfo", "--summary"])
        devices = parse_vulkan_devices(result.get("output", ""))
        if result.get("output_truncated"):
            result.update(status="unknown", output="Vulkan output truncated; device inventory is incomplete")
        if result["status"] == "pass" and not devices:
            result.update(status="unknown", output="Vulkan summary contains no identifiable devices")
        record("vulkan", result, devices=devices, backend="Vulkan")
        if require_hardware:
            physical = [device for device in devices
                        if device["type"] in {"PHYSICAL_DEVICE_TYPE_DISCRETE_GPU", "PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU"}
                        and not re.search(r"llvmpipe|lavapipe|swiftshader|software|virtual|unknown", device["name"], re.I)]
            all_physical = bool(devices) and len(physical) == len(devices)
            hardware_status = "unknown" if result["status"] == "unknown" else "pass" if result["status"] == "pass" and all_physical else "fail"
            add("hardware_gpu", hardware_status,
                "Complete inventory reports only integrated/discrete candidates; caller must isolate the required ICD" if all_physical else "Mixed/CPU/software/virtual/unknown devices cannot identify the SDL receiver as hardware",
                devices=devices, scope="Reported physical candidate inventory only; no physical readback, physical-console or performance acceptance")
        for test in selected:
            if test.get("name") not in GPU_TESTS:
                continue
            shaders = [Path(arg) for arg in test.get("command", []) if str(arg).endswith(".spv")]
            if len(shaders) < 2:
                add("spirv", "unknown", "Selected receiver command must declare its vertex and fragment SPIR-V paths", test=test.get("name"))
            for shader in shaders:
                if not shader.is_absolute() and build_dir:
                    shader = build_dir / shader
                try:
                    with shader.open("rb") as stream:
                        valid = stream.read(4) == b"\x03\x02\x23\x07"
                except OSError:
                    valid = False
                add("spirv", "pass" if valid else "fail", str(shader), test=test.get("name"))
    receipt["ok"] = all(item["status"] == "pass" for item in checks if item["required"])
    receipt["elapsed_seconds"] = round(time.monotonic() - start, 3)
    return receipt


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage", choices=["development", "test"], default="development")
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--inventory", type=Path, help="Selected CTest --show-only=json-v1 receipt")
    parser.add_argument("--compiler")
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--require-hardware", action="store_true")
    parser.add_argument("--gpu", action="store_true", help="Development shader/Vulkan capability profile")
    parser.add_argument("--timeout", type=float, default=10)
    parser.add_argument("--budget", type=float, default=45)
    args = parser.parse_args()
    try:
        tests = json.loads(args.inventory.read_text())["tests"] if args.inventory else None
        receipt = check_prerequisites(args.build_dir, tests, stage=args.stage,
                                      compiler=args.compiler, sanitize=args.sanitize,
                                      require_hardware=args.require_hardware, gpu_requested=args.gpu,
                                      timeout=args.timeout, budget=args.budget)
    except (OSError, ValueError, KeyError) as error:
        receipt = {"schema": 1, "ok": False, "error": str(error)}
    print(json.dumps(receipt, indent=2))
    return 0 if receipt["ok"] else 1


if __name__ == "__main__":
    sys.exit(main())
