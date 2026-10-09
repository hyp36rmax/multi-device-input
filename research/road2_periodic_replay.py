#!/usr/bin/env python3
"""Offline R4.2F-P topology replay. Research normalizations are not product tuning."""

import csv
import json
import math
import statistics
import sys
from pathlib import Path

MAX_DT = 0.1
ENVELOPE_SECONDS = 0.05


def percentile(values, q):
    if not values:
        return 0.0
    ordered = sorted(values)
    pos = (len(ordered) - 1) * q
    lo = int(math.floor(pos))
    hi = int(math.ceil(pos))
    return ordered[lo] if lo == hi else ordered[lo] * (hi - pos) + ordered[hi] * (pos - lo)


def rows(path):
    with open(path, newline="", encoding="utf-8-sig") as stream:
        lines = (line for line in stream if not line.startswith("#"))
        yield from csv.DictReader(lines)


def replay(label, path, reference):
    output = []
    elapsed_previous = None
    time_phase = distance_phase = distance = envelope = 0.0
    resets = {}
    crossings = [0, 0]
    previous = [0.0, 0.0]
    active = false_positive = event_false = 0
    speed_pairs = []

    for row in rows(path):
        elapsed = float(row["elapsed_time"])
        dt = 0.0 if elapsed_previous is None else elapsed - elapsed_previous
        elapsed_previous = elapsed
        speed = max(0.0, abs(float(row["speed"])))
        surfaces = tuple(int(float(row[f"surface_{n}"])) for n in range(4))
        left = abs(float(row["vibration_left_raw"]))
        right = abs(float(row["vibration_right_raw"]))
        event = row.get("gear_transition", "0") not in ("0", "false", "False", "")
        rise = float(row.get("vibration_rise", "0") or 0.0)
        event = event or rise > 0.12  # Mirrors Signal State's collision/unknown-event gate.
        non_reference = any(value != reference for value in surfaces)
        source = min(1.0, math.sqrt((min(1.0, left) ** 2 + min(1.0, right) ** 2) * 0.5))
        authorized = non_reference and source > 1e-6 and not event

        reason = None
        if not math.isfinite(dt) or dt <= 0.0:
            reason = "invalid_timing"
        elif dt > MAX_DT:
            reason = "timing_discontinuity"
        elif not authorized:
            reason = "event_excluded" if event else "road_inactive"

        if reason:
            time_phase = distance_phase = distance = envelope = 0.0
            request = [0.0, 0.0]
            resets[reason] = resets.get(reason, 0) + 1
            if event and any(abs(value) > 1e-12 for value in request):
                event_false += 1
        else:
            step = min(1.0, dt / ENVELOPE_SECONDS)
            envelope = min(source, envelope + step) if source > envelope else max(source, envelope - step)
            time_phase = (time_phase + dt) % 1.0
            increment = speed * dt
            distance += increment
            distance_phase = (distance_phase + increment) % 1.0
            request = [math.sin(2.0 * math.pi * time_phase) * envelope,
                       math.sin(2.0 * math.pi * distance_phase) * envelope]
            active += 1
            speed_pairs.append((speed, abs(request[1])))

        if not non_reference and any(abs(value) > 1e-12 for value in request):
            false_positive += 1
        for n in range(2):
            if request[n] and previous[n] and (request[n] > 0) != (previous[n] > 0):
                crossings[n] += 1
            previous[n] = request[n]
        output.append((source, envelope, request[0], request[1]))

    def stats(index):
        values = [row[index] for row in output]
        absolute = [abs(value) for value in values]
        return {"mean_dc": statistics.fmean(values) if values else 0.0,
                "rms": math.sqrt(statistics.fmean(value * value for value in values)) if values else 0.0,
                "p95_abs": percentile(absolute, 0.95), "p99_abs": percentile(absolute, 0.99),
                "max_abs": max(absolute, default=0.0)}

    return {"scenario": label, "rows": len(output), "active_rows": active,
            "activity_coverage": active / len(output) if output else 0.0,
            "time": stats(2), "distance": stats(3), "zero_crossings": crossings,
            "normal_false_positives": false_positive, "event_false_positives": event_false,
            "reset_reasons": resets, "bound_occupancy": 0,
            "determinism_digest": [round(sum(row[n] for row in output), 9) for n in range(4)]}


def main(arguments):
    if len(arguments) < 3:
        raise SystemExit("usage: road2_periodic_replay.py REFERENCE_RAW label=csv ...")
    reference = int(arguments[1])
    results = []
    for item in arguments[2:]:
        label, path = item.split("=", 1)
        results.append(replay(label, Path(path), reference))
    print(json.dumps({"model": "HYP36R_ROAD_PERIODIC_V1_SHADOW",
                      "time_rate": "1 normalized cycle/second; NOT PHYSICAL",
                      "distance_rate": "1 normalized cycle/speed-distance unit; NOT PHYSICAL",
                      "results": results}, indent=2))


if __name__ == "__main__":
    main(sys.argv)
