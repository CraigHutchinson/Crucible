#!/usr/bin/env python3
"""Plot the complete production relay-density study, requiring full replay evidence."""

import argparse
import json
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

STRATEGIES = ("passive", "radial", "straight-flow")
CHECKPOINTS = (0, 60, 120, 240, 480, 900)
COST = 32
STARTUP = 49


def integer(row, field):
    value = row.get(field)
    if type(value) is not int or value < 0:
        raise ValueError(f"{field} must be a nonnegative integer")
    return value


def read_study(source):
    density = {strategy: {} for strategy in STRATEGIES}
    replayed = set()
    for line in source.read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        row = json.loads(line)
        if not isinstance(row, dict) or row.get("strategy") not in STRATEGIES:
            raise ValueError("record must identify one of the three study strategies")
        strategy = row["strategy"]
        tick = integer(row, "tick")
        if row.get("kind") == "replay":
            expected_commands = 0 if strategy == "passive" else 1
            if (strategy in replayed or tick != 900 or
                    row.get("status") != "full-state-pass" or
                    integer(row, "commands") != expected_commands):
                raise ValueError("require one successful full tick900 replay per strategy")
            replayed.add(strategy)
        elif row.get("kind") == "density":
            if tick not in CHECKPOINTS or tick in density[strategy]:
                raise ValueError("require unique full-study density checkpoints")
            count = integer(row, "count")
            longest = integer(row, "longest_eligible_run_completed_ticks")
            current = integer(row, "eligible_run_completed_ticks")
            minimum = integer(row, "minimum_count_through_tick")
            maximum = integer(row, "maximum_count_through_tick")
            if (not minimum <= count <= maximum <= 2048 or
                    minimum > STARTUP or maximum < STARTUP or
                    current > longest or longest > tick or
                    row.get("required") != COST or row.get("radius") != 4 or
                    row.get("relay_x") != 48.5 or row.get("relay_y") != 16.5 or
                    row.get("eligible") is not (count >= COST) or
                    row.get("delta_from_startup") != count - STARTUP):
                raise ValueError("density record does not match the frozen study contract")
            ledger = row.get("ledger")
            if not isinstance(ledger, dict):
                raise ValueError("density record must include the complete biomass ledger")
            quantities = {key: integer(ledger, key) for key in (
                "initial_total", "remaining_stock", "mobile_mass", "reserve", "harvested", "work_actions")}
            if (quantities["initial_total"] != 10240 or quantities["mobile_mass"] != 2048 or
                    quantities["initial_total"] != quantities["remaining_stock"] +
                    quantities["mobile_mass"] + quantities["reserve"]):
                raise ValueError("density ledger does not conserve the frozen startup biomass")
            density[strategy][tick] = row
        else:
            raise ValueError("unknown study record kind")
    if replayed != set(STRATEGIES):
        raise ValueError("missing full tick900 replay evidence; short --verify output is insufficient")
    for checkpoints in density.values():
        if set(checkpoints) != set(CHECKPOINTS) or checkpoints[0]["count"] != STARTUP:
            raise ValueError("missing full-study checkpoint or incorrect startup density")
        longest_runs = [checkpoints[tick]["longest_eligible_run_completed_ticks"] for tick in CHECKPOINTS]
        if longest_runs != sorted(longest_runs):
            raise ValueError("longest consecutive eligibility cannot decrease")
    return density


def plot(study, destination):
    # Fixed IDs and omitted timestamp keep SVG output deterministic and independent
    # of the input/output paths, current date and host environment metadata.
    with matplotlib.rc_context({"svg.hashsalt": "crucible-relay-density-phase9-v1",
                                "font.family": "DejaVu Sans", "svg.fonttype": "path"}):
        figure, axes = plt.subplots(2, 1, figsize=(10, 7), sharex=True,
                                    layout="constrained", facecolor="white")
        colors = ("#536577", "#007d83", "#ac4c00")
        labels = ("Passive", "Radial attractor", "Straight flow")
        for strategy, color, label in zip(STRATEGIES, colors, labels):
            rows = study[strategy]
            axes[0].plot(CHECKPOINTS, [rows[tick]["count"] for tick in CHECKPOINTS],
                         color=color, marker="o", linewidth=2, label=label)
            axes[1].plot(CHECKPOINTS,
                         [rows[tick]["longest_eligible_run_completed_ticks"] for tick in CHECKPOINTS],
                         color=color, marker="o", linewidth=2, label=label)
        axes[0].axhline(COST, color="#555555", linestyle="--", linewidth=1.2,
                           label="Investigated threshold: 32")
        axes[0].axhline(STARTUP, color="#777777", linestyle=":", linewidth=1.5,
                           label="Startup density: 49")
        axes[0].set_ylabel("Mobile samples within radius 4")
        axes[0].set_title("Prospective relay at (48.5, 16.5); checkpoint counts")
        axes[0].legend(loc="best", fontsize=9)
        axes[1].set_ylabel("Longest consecutive completed ticks\nwith ≥32 samples (not relay hold)")
        axes[1].set_xlabel("Completed simulation tick")
        axes[1].set_title("Eligibility measured every tick; shown at six checkpoints")
        for axis in axes:
            axis.set_xlim(0, 900)
            axis.set_ylim(bottom=0)
            axis.set_xticks(CHECKPOINTS)
            axis.grid(alpha=0.2)
            axis.spines[["top", "right"]].set_visible(False)
        figure.suptitle("Actual production state: relay-density study — no fusion", fontsize=14)
        figure.supxlabel("All three strategies replay full state through tick 900. Lines connect checkpoint observations.",
                           fontsize=9)
        destination.parent.mkdir(parents=True, exist_ok=True)
        figure.savefig(destination, format="svg", metadata={"Date": None, "Creator": "Crucible"})
        plt.close(figure)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="complete tick900 study JSONL")
    parser.add_argument("output", type=Path, help="destination SVG")
    arguments = parser.parse_args()
    try:
        study = read_study(arguments.input)
    except (OSError, ValueError) as error:
        parser.exit(1, f"relay-density plot rejected input: {error}\n")
    plot(study, arguments.output)


if __name__ == "__main__":
    main()
