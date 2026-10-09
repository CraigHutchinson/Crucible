"""Record exact source, compiler commands and selected Filament artifact hashes.

This script does not run binaries or perform rendering. It rejects missing or
mismatched qualification inputs and records the actual source-build archive
closure used by the separate link-only consumer.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess


PIN = "d852e34cd5629a6f3851d613dcd17aa0ffd025a2"
ARCHIVES = (
    "filament/filament.lib",
    "filament/backend/backend.lib",
    "libs/math/math.lib",
    "libs/utils/utils.lib",
    "libs/filaflat/filaflat.lib",
    "libs/filabridge/filabridge.lib",
    "third_party/zstd/tnt/zstd.lib",
    "libs/bluevk/bluevk.lib",
    "third_party/smol-v/tnt/smol-v.lib",
    "third_party/getopt/getopt.lib",
    "third_party/imgui/tnt/imgui.lib",
)
TOOLS = ("tools/matc/matc.exe", "tools/cmgen/cmgen.exe", "tools/resgen/resgen.exe")


def artifact(path: Path, root: Path) -> dict:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return {"path": path.relative_to(root).as_posix(),
            "bytes": path.stat().st_size, "sha256": digest.hexdigest()}


def git(source: Path, *args: str) -> str:
    return subprocess.check_output(["git", "-C", str(source), *args], text=True).strip()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True)
    args = parser.parse_args()
    root = args.root.resolve(strict=True)
    repo_build = Path(__file__).resolve().parents[2] / "build"
    if not root.is_relative_to(repo_build.resolve()):
        raise RuntimeError("Inventory must stay in this checkout's owned build directory.")
    source, release = root / "source", root / "release"
    head = git(source, "rev-parse", "HEAD")
    if head != PIN or git(source, "status", "--porcelain", "--untracked-files=no"):
        raise RuntimeError("Inventory requires an unchanged exact-release source tree.")
    cache = (release / "CMakeCache.txt").read_text(encoding="utf-8")
    expected = {
        "CMAKE_BUILD_TYPE": "Release", "USE_STATIC_CRT": "OFF",
        "FILAMENT_SUPPORTS_VULKAN": "ON", "FILAMENT_SUPPORTS_OPENGL": "OFF",
        "FILAMENT_SUPPORTS_WEBGPU": "OFF", "FILAMENT_BUILD_TESTING": "OFF",
        "FILAMENT_SHORTEN_MSVC_COMPILATION": "OFF",
    }
    for name, value in expected.items():
        if not re.search(rf"^{name}:[^=\r\n]+={value}$", cache, re.MULTILINE):
            raise RuntimeError(f"Unexpected cache setting: {name} must be {value}.")
    commands = json.loads((release / "compile_commands.json").read_text(encoding="utf-8"))
    selected = {}
    for suffix in ("libs/utils/src/Panic.cpp", "filament/src/details/Engine.cpp",
                   "filament/backend/src/vulkan/VulkanDriver.cpp", "third_party/imgui/imgui.cpp"):
        item = next(command for command in commands
                    if command["file"].replace("\\", "/").endswith(suffix))
        command = item["command"]
        if not re.search(r"(?:^|\s)[/-]MD(?:\s|$)", command):
            raise RuntimeError(f"Release DLL CRT /MD not present: {suffix}")
        if re.search(r"(?:^|\s)[/-]MTd?(?:\s|$)", command):
            raise RuntimeError(f"Static CRT leaked into compile command: {suffix}")
        selected[suffix] = command
    imgui = (source / "third_party/imgui/imgui.h").read_text(encoding="utf-8")
    if not re.search(r'^#define IMGUI_VERSION\s+"1\.92\.5"', imgui, re.MULTILINE):
        raise RuntimeError("ImGui core must match Filament release1.92.5.")
    report = {
        "source": {"pin": head, "origin": git(source, "remote", "get-url", "origin"),
                   "dirty": False},
        "settings": expected, "compileCommands": selected,
        "archives": [artifact(release / name, root) for name in ARCHIVES],
        "tools": [artifact(release / name, root) for name in TOOLS],
        "imguiVersion": "1.92.5", "gpuExecuted": False,
        "limits": ["Source/build/hash inventory is not GPU or native-presentation receiving.",
                   "Unmodified Vulkan finish discards queue-wait error results.",
                   "MSVC exception macros require the separate preprocessor receipt."],
    }
    material = root / "qualification.filamat"
    if material.exists():
        report["compiledMaterial"] = artifact(material, root)
    for name in ("qualification-metal.filamat", "qualification-dfg.bin",
                 "resources/phase15_qualification.c", "resources/phase15_qualification.bin"):
        path = root / name
        if path.exists():
            report.setdefault("toolOutputs", []).append(artifact(path, root))
    macros = (root / "compiler_macros.i").read_text(encoding="utf-8")
    report["compilerMacros"] = {
        name: int(re.search(rf'P15 {name}=([01])', macros).group(1))
        for name in ("_CPPUNWIND", "__EXCEPTIONS", "UTILS_EXCEPTIONS")
    }
    report["compilerMacros"]["note"] = (
        "The separate /EHsc /MD preprocessor input includes the actual pinned Panic.h; "
        "compare the recorded dependency commands before interpreting exception behavior.")
    link = root / "link"
    if (link / "qualification.map").exists():
        link_map = (link / "qualification.map").read_text(encoding="utf-8")
        if "VulkanDriver.cpp.obj" not in link_map or "imgui.cpp.obj" not in link_map:
            raise RuntimeError("Link map did not receive the Vulkan and matched ImGui implementations.")
        report["linkOnlyConsumer"] = {
            "artifacts": [artifact(link / name, root)
                          for name in ("qualification.map", "phase15_filament_link.exe")],
            "executedByRunner": False,
        }
    output = root / "inventory.json"
    output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Received {len(ARCHIVES)} archives and {len(TOOLS)} tools; {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
