#!/usr/bin/env python3
"""Deterministic R4.2F-T6 native sound-request ownership analysis."""

from __future__ import annotations

import argparse
import csv
import json
import sys
import tempfile
from collections import Counter, defaultdict
from pathlib import Path

SCHEMA = "HYP36R_SOUND_REQUEST_TRACE_V1"
COBBLESTONE = 0x100000
PEGASUS = 0x8D
BOUNDARIES = ("SET_SND_QUEUE", "PRJ_SND_REQUEST", "LOWER_PLAY_ROUTE")
PHASES = ("normal_before", "entry", "full_cobblestone", "exit", "normal_after")


def as_int(text: str) -> int:
    return int(text, 0)


def load_trace(path: Path):
    metadata, data_lines = {}, []
    for line in path.read_text(encoding="utf-8-sig").splitlines():
        if line.startswith("# ") and "=" in line:
            key, value = line[2:].split("=", 1)
            metadata[key] = value
        elif line and not line.startswith("#"):
            data_lines.append(line)
    if metadata.get("trace_schema") != SCHEMA:
        raise ValueError(f"expected {SCHEMA}, found {metadata.get('trace_schema', 'missing')}")
    rows = list(csv.DictReader(data_lines))
    if not rows:
        raise ValueError("trace contains no rows")
    return metadata, rows


def surface(row):
    return tuple(as_int(row[f"surface_{i}"]) for i in range(4))


def assign_phases(rows):
    states = [r for r in rows if r["boundary"] == "STATE"]
    full_indices = [i for i, r in enumerate(states) if surface(r) == (COBBLESTONE,) * 4]
    if not full_indices:
        raise ValueError("no full 0x100000 cobblestone occupancy found")
    runs, start, previous = [], full_indices[0], full_indices[0]
    for index in full_indices[1:]:
        if index != previous + 1:
            runs.append((start, previous))
            start = index
        previous = index
    runs.append((start, previous))
    full_start, full_end = max(runs, key=lambda item: item[1] - item[0])
    any_target = [any(value == COBBLESTONE for value in surface(r)) for r in states]
    entry_start = full_start
    while entry_start > 0 and any_target[entry_start - 1]:
        entry_start -= 1
    exit_end = full_end
    while exit_end + 1 < len(states) and any_target[exit_end + 1]:
        exit_end += 1
    frame_phase = {}
    for i, row in enumerate(states):
        if i < entry_start:
            phase = "normal_before"
        elif i < full_start:
            phase = "entry"
        elif i <= full_end:
            phase = "full_cobblestone"
        elif i <= exit_end:
            phase = "exit"
        else:
            phase = "normal_after"
        frame_phase[as_int(row["frame"])] = phase
    ordered_frames = sorted(frame_phase)
    for row in rows:
        frame = as_int(row["frame"])
        if frame not in frame_phase:
            nearest = min(ordered_frames, key=lambda candidate: abs(candidate - frame))
            frame_phase[frame] = frame_phase[nearest]
        row["phase"] = frame_phase[frame]
    return states, (entry_start, full_start, full_end, exit_end)


def duration_by_phase(states):
    times = defaultdict(list)
    for row in states:
        times[row["phase"]].append(float(row["elapsed_time"]))
    return {phase: max(times[phase]) - min(times[phase]) if len(times[phase]) > 1 else 0.0
            for phase in PHASES}


def analyze(path: Path):
    metadata, rows = load_trace(path)
    states, indices = assign_phases(rows)
    requests = [r for r in rows if r["boundary"] in BOUNDARIES]
    durations = duration_by_phase(states)
    grouped, clean_grouped, command_groups, lifecycle, surface_groups, contamination = (
        Counter(), Counter(), Counter(), Counter(), Counter(), Counter())
    for row in requests:
        sound_id = as_int(row["sound_id"])
        grouped[(row["phase"], row["boundary"], sound_id, row["caller_rva"])] += 1
        contaminated = as_int(row["gear_transition"]) or abs(float(row["impact"])) > 0.001
        if not contaminated:
            clean_grouped[(row["phase"], row["boundary"], sound_id, row["caller_rva"])] += 1
        command_groups[(row["phase"], row["boundary"], as_int(row["command"]), row["caller_rva"])] += 1
        lifecycle[(row["phase"], sound_id, "stop" if as_int(row["stop"]) else
                   "loop_start" if as_int(row["loop"]) else "request")] += 1
        surface_groups[(row["phase"], surface(row), row["boundary"], sound_id)] += 1
        if sound_id == PEGASUS:
            contamination["pegasus_0x8d"] += 1
        if as_int(row["gear_transition"]):
            contamination["gear_transition"] += 1
        if abs(float(row["impact"])) > 0.001:
            contamination["impact"] += 1
    candidates = []
    identities = {(boundary, sound_id, caller) for _, boundary, sound_id, caller in grouped}
    for boundary, sound_id, caller in sorted(identities):
        counts = {phase: grouped[(phase, boundary, sound_id, caller)] for phase in PHASES}
        clean_counts = {phase: clean_grouped[(phase, boundary, sound_id, caller)] for phase in PHASES}
        control = clean_counts["normal_before"] + clean_counts["normal_after"]
        cobble = clean_counts["entry"] + clean_counts["full_cobblestone"] + clean_counts["exit"]
        if sound_id == PEGASUS:
            classification = "rejected_known_pegasus"
        elif cobble >= 3 and control == 0:
            classification = "surface_specific_candidate"
        elif cobble >= max(4, control * 4):
            classification = "surface_enriched_candidate"
        else:
            classification = "not_surface_specific"
        candidates.append({"boundary": boundary, "sound_id": f"0x{sound_id:X}",
                           "caller_rva": caller, "counts": counts, "clean_counts": clean_counts,
                           "contaminated_count": sum(counts.values()) - sum(clean_counts.values()),
                           "classification": classification})
    return {
        "schema": metadata["trace_schema"], "scenario": metadata.get("scenario", ""),
        "build_commit": metadata.get("build_commit", ""), "records": len(rows),
        "requests": len(requests), "dropped": int(metadata.get("dropped_count", "0")),
        "phase_state_indices": dict(zip(("entry_start", "full_start", "full_end", "exit_end"), indices)),
        "phase_durations_seconds": durations,
        "phase_request_counts": dict(Counter(r["phase"] for r in requests)),
        "contamination": dict(contamination),
        "groups": [{"phase": phase, "boundary": boundary, "sound_id": f"0x{sound_id:X}",
                    "caller_rva": caller, "count": count,
                    "rate_hz": count / durations[phase] if durations[phase] > 0 else None}
                   for (phase, boundary, sound_id, caller), count in sorted(grouped.items())],
        "command_groups": [{"phase": phase, "boundary": boundary, "command": f"0x{command:X}",
                            "caller_rva": caller, "count": count}
                           for (phase, boundary, command, caller), count in sorted(command_groups.items())],
        "lifecycle": [{"phase": phase, "sound_id": f"0x{sound_id:X}", "kind": kind, "count": count}
                      for (phase, sound_id, kind), count in sorted(lifecycle.items())],
        "surface_groups": [{"phase": phase, "surface": [f"0x{x:X}" for x in values],
                            "boundary": boundary, "sound_id": f"0x{sound_id:X}", "count": count}
                           for (phase, values, boundary, sound_id), count in sorted(surface_groups.items())],
        "candidates": candidates,
        "decision_gate": "DEFERRED_UNTIL_ACCEPTED_RUNTIME_CAPTURE",
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("trace", type=Path, nargs="?")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        fields = ("boundary,command,sound_id,loop,pan_left,pan_right,pan_leftright,stop,"
                  "caller_rva,frame,elapsed_time,surface_0,surface_1,surface_2,surface_3,"
                  "previous_surface_0,previous_surface_1,previous_surface_2,previous_surface_3,"
                  "surface_transition_mask,speed,gear,gear_transition,impact,steering")
        rows = [
            "STATE,0,0,0,0,0,0,0,0x0,0,0.0,2,2,2,2,2,2,2,2,0,1,1,0,0,0",
            "SET_SND_QUEUE,2116,68,0,0,0,0,0,0x111,0,0.1,2,2,2,2,2,2,2,2,0,1,1,0,0,0",
            "STATE,0,0,0,0,0,0,0,0x0,1,1.0,1048576,2,2,2,2,2,2,2,1,2,1,0,0,0",
            "PRJ_SND_REQUEST,2116,68,0,0,0,0,0,0x222,1,1.1,1048576,2,2,2,2,2,2,2,1,2,1,0,0,0",
            "STATE,0,0,0,0,0,0,0,0x0,2,2.0,1048576,1048576,1048576,1048576,1048576,2,2,2,14,3,1,0,0,0",
            "LOWER_PLAY_ROUTE,2189,141,0,0,0,0,0,0x333,2,2.1,1048576,1048576,1048576,1048576,1048576,2,2,2,14,3,1,0,0,0",
            "STATE,0,0,0,0,0,0,0,0x0,3,3.0,2,1048576,2,2,1048576,1048576,1048576,1048576,13,2,1,0,0,0",
            "STATE,0,0,0,0,0,0,0,0x0,4,4.0,2,2,2,2,2,1048576,2,2,2,1,1,0,0,0",
        ]
        content = "# trace_schema=" + SCHEMA + "\n# dropped_count=0\n" + fields + "\n" + "\n".join(rows) + "\n"
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "trace.csv"
            path.write_text(content, encoding="utf-8")
            result = analyze(path)
        assert result["phase_request_counts"]["entry"] == 1
        assert result["phase_request_counts"]["full_cobblestone"] == 1
        assert result["contamination"]["pegasus_0x8d"] == 1
        assert result["decision_gate"] == "DEFERRED_UNTIL_ACCEPTED_RUNTIME_CAPTURE"
        return 0
    if not args.trace:
        parser.error("trace is required unless --self-test is used")
    rendered = json.dumps(analyze(args.trace), indent=2, sort_keys=True)
    if args.output:
        args.output.write_text(rendered + "\n", encoding="utf-8")
    else:
        print(rendered)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(2)
