"""Receive finite native composed captures and retain exact source/input identities.

Run during the assigned exclusive GPU handoff. This is readback receiving, not
scanout, participant or frame-time performance evidence. Existing output is refused.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess


def digest(path: Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=Path("build/production-release"))
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[2]
    build = (repo / args.build_dir).resolve(strict=True)
    output = (repo / args.output).resolve()
    owned_build = (repo / "build").resolve()
    if not build.is_relative_to(owned_build) or not output.is_relative_to(owned_build):
        raise RuntimeError("Native build and output must remain beneath this checkout/build")
    output.mkdir(parents=True, exist_ok=False)
    exe = build / "src/game/crucible_game.exe"
    resources = build / "src/presentation/production/crucible/presentation/production/resources"
    inputs = [exe, *sorted(resources.glob("*.filamat")),
              *sorted((exe.parent / "assets/production/fonts").rglob("*.ttf"))]
    if len(inputs) != 6:
        raise RuntimeError("Expected executable, three compiled materials and two packaged fonts")
    source = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=repo, text=True).strip()
    diff = subprocess.check_output(["git", "diff", "HEAD", "--"], cwd=repo)
    report = {"schema": 1, "source": source,
              "tracked_worktree_diff_sha256": hashlib.sha256(diff).hexdigest(),
              "inputs": [{"path": str(path.relative_to(repo)), "sha256": digest(path)} for path in inputs],
              "scanout_verified": False, "performance_measurement": False, "cases": []}
    cases = [("macro", ["--capture-ms", "500"]),
             ("assembly", ["--capture-ms", "6000"]),
             ("hero", ["--capture-ms", "9000"]),
             ("menu", ["--menu"]), ("options", ["--screen", "options"]),
             ("missions", ["--screen", "missions"]),
             ("world", ["--route", "--capture-frame", "64"])]
    for name, flags in cases:
        print(f"Receiving {name}", flush=True)
        png = output / f"{name}.png"
        command = [str(exe), "--capture", str(png), *flags]
        process = subprocess.run(command, cwd=repo, stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT, timeout=35, check=False)
        log = output / f"{name}.log"
        log.write_bytes(process.stdout)
        if process.returncode:
            raise RuntimeError(f"Native {name} failed with {process.returncode}; retained {log}")
        if b"sampler parameters not set" in process.stdout or b"was set between begin/endRenderPass" in process.stdout:
            raise RuntimeError(f"Native descriptor regression in {name}; retained {log}")
        receipt = json.loads(Path(str(png) + ".json").read_text(encoding="utf-8"))
        with png.open("rb") as image:
            header = image.read(24)
        if header[:8] != b"\x89PNG\r\n\x1a\n" or header[12:16] != b"IHDR":
            raise RuntimeError("Native output is not a PNG")
        size = struct.unpack(">II", header[16:24])
        if size != (receipt["width"], receipt["height"]) or receipt["scanout_verified"]:
            raise RuntimeError("Readback dimensions or evidence classification mismatch")
        if name == "world" and (receipt["sequence"] != "committed-mission-world" or receipt["completed_tick"] < 64):
            raise RuntimeError("World capture did not receive advancing committed mission state")
        report["cases"].append({"name": name, "command": command, "exit_code": process.returncode,
                                "png_sha256": digest(png), "log_sha256": digest(log), "receipt": receipt})
        (output / "receiving.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Received {len(cases)} native readbacks; no scanout/performance claim", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
