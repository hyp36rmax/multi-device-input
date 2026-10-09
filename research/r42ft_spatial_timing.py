#!/usr/bin/env python3
"""R4.2F-T offline spatial-timing evidence study.

Uses only accepted HYP36R_RESEARCH_II_R1 CSV captures.  Distance is reconstructed
in game-relative units as integral(max(speed, 0) * dt).  No physical units or
product timing constants are inferred.
"""

import argparse
import csv
import json
import math
from pathlib import Path
from statistics import mean, median


def load(path):
    metadata = {}
    with path.open(encoding="utf-8-sig", newline="") as handle:
        while True:
            line = handle.readline()
            if not line:
                raise ValueError(f"missing CSV header: {path}")
            if line.startswith("#"):
                key, _, value = line[1:].strip().partition("=")
                metadata[key] = value
                continue
            header = next(csv.reader([line]))
            rows = [dict(zip(header, row)) for row in csv.reader(handle)]
            return metadata, rows


def number(row, name, default=0.0):
    try:
        value = float(row[name])
        return value if math.isfinite(value) else default
    except (KeyError, TypeError, ValueError):
        return default


def corr(a, b):
    if len(a) < 3:
        return 0.0
    ma, mb = mean(a), mean(b)
    da = sum((x - ma) ** 2 for x in a)
    db = sum((x - mb) ** 2 for x in b)
    return sum((x - ma) * (y - mb) for x, y in zip(a, b)) / math.sqrt(da * db) if da and db else 0.0


def linear_residual(x, y):
    mx, my = mean(x), mean(y)
    denom = sum((v - mx) ** 2 for v in x)
    slope = sum((a - mx) * (b - my) for a, b in zip(x, y)) / denom if denom else 0.0
    intercept = my - slope * mx
    residual = [b - (slope * a + intercept) for a, b in zip(x, y)]
    rms = math.sqrt(mean([v * v for v in residual])) if residual else 0.0
    return slope, intercept, residual, rms


def quantile(values, q):
    if not values:
        return 0.0
    ordered = sorted(values)
    at = q * (len(ordered) - 1)
    lo = int(at)
    hi = min(lo + 1, len(ordered) - 1)
    return ordered[lo] + (ordered[hi] - ordered[lo]) * (at - lo)


def interpolate(axis, values, step):
    if len(axis) < 2 or axis[-1] <= axis[0] or step <= 0:
        return []
    result, j = [], 0
    target = axis[0]
    while target <= axis[-1]:
        while j + 1 < len(axis) and axis[j + 1] < target:
            j += 1
        if j + 1 >= len(axis):
            break
        width = axis[j + 1] - axis[j]
        if width > 0:
            f = (target - axis[j]) / width
            result.append(values[j] + f * (values[j + 1] - values[j]))
        target += step
    return result


def autocorrelation(values, max_lag):
    if len(values) < 8:
        return []
    m = mean(values)
    centered = [v - m for v in values]
    denom = sum(v * v for v in centered)
    if denom <= 0:
        return []
    return [sum(centered[i] * centered[i + lag] for i in range(len(centered) - lag)) / denom
            for lag in range(1, min(max_lag, len(centered) // 2) + 1)]


def best_nontrivial_ac(values):
    ac = autocorrelation(values, 240)
    # Ignore the immediate smooth-signal shoulder. A candidate must first cross
    # zero, then produce a positive recurrence peak.
    start = next((i + 1 for i, v in enumerate(ac) if v <= 0), None)
    if start is None or start >= len(ac):
        return {"lag": 0, "correlation": 0.0}
    peaks = [(i + 1, ac[i]) for i in range(max(start, 1), len(ac) - 1)
             if ac[i] > 0 and ac[i] >= ac[i - 1] and ac[i] >= ac[i + 1]]
    lag, value = max(peaks, key=lambda item: item[1], default=(0, 0.0))
    return {"lag": lag, "correlation": value}


def stable_runs(rows):
    mask = []
    for row in rows:
        surfaces = [int(number(row, f"surface_{i}")) for i in range(4)]
        non_reference = any(v != 2 for v in surfaces)
        stable = len(set(surfaces)) == 1 and non_reference and not any(int(number(row, f"surface{i}_changed")) for i in range(4))
        event = int(number(row, "gear_transition")) != 0 or abs(number(row, "impact_pre_gain")) > 1.0e-9
        mask.append(stable and not event)
    runs, begin = [], None
    for index, valid in enumerate(mask + [False]):
        if valid and begin is None:
            begin = index
        elif not valid and begin is not None:
            if index - begin >= 20:
                runs.append((begin, index))
            begin = None
    return runs


def analyze(path):
    metadata, rows = load(path)
    time = [number(row, "timestamp") for row in rows]
    distance = [0.0]
    for i in range(1, len(rows)):
        dt = max(0.0, min(0.1, time[i] - time[i - 1]))
        distance.append(distance[-1] + max(0.0, number(rows[i], "speed")) * dt)
    runs = stable_runs(rows)
    if metadata.get("test_scenario") == "R4_2C_CST02_COBBLESTONE_STABLE":
        # The formally accepted R4.2C stable interval is rows 0..437. Preserve
        # that reviewed boundary instead of rediscovering it heuristically.
        runs = [(0, 438)]
    summaries = []
    for begin, end in runs:
        section = rows[begin:end]
        x = [number(row, "speed") for row in section]
        y = [number(row, "vibration_combined_raw") for row in section]
        slope, intercept, residual, residual_rms = linear_residual(x, y)
        section_t = [time[i] - time[begin] for i in range(begin, end)]
        section_d = [distance[i] - distance[begin] for i in range(begin, end)]
        dt = median([section_t[i] - section_t[i - 1] for i in range(1, len(section_t))])
        dd_values = [section_d[i] - section_d[i - 1] for i in range(1, len(section_d)) if section_d[i] > section_d[i - 1]]
        dd = median(dd_values) if dd_values else 0.0
        time_residual = interpolate(section_t, residual, dt)
        distance_residual = interpolate(section_d, residual, dd)
        tac = best_nontrivial_ac(time_residual)
        dac = best_nontrivial_ac(distance_residual)
        summaries.append({
            "rows": end - begin,
            "duration_s": section_t[-1] if section_t else 0.0,
            "relative_distance": section_d[-1] if section_d else 0.0,
            "surface": [int(number(section[0], f"surface_{i}")) for i in range(4)],
            "speed_min": min(x), "speed_median": median(x), "speed_max": max(x),
            "combined_speed_correlation": corr(x, y),
            "combined_speed_slope": slope, "combined_speed_intercept": intercept,
            "combined_residual_rms": residual_rms,
            "combined_rms": math.sqrt(mean([v * v for v in y])),
            "time_recurrence_lag_s": tac["lag"] * dt,
            "time_recurrence_correlation": tac["correlation"],
            "distance_recurrence_lag_relative": dac["lag"] * dd,
            "distance_recurrence_correlation": dac["correlation"],
            "fieldE8_unique": [len(set(number(row, f"corner{i}_fieldE8") for row in section)) for i in range(4)],
            "ec_speed_abs_correlation": corr(x, [mean(abs(number(row, f"corner{i}_fieldEC")) for i in range(4)) for row in section]),
            "ee_speed_abs_correlation": corr(x, [mean(abs(number(row, f"corner{i}_fieldEE")) for i in range(4)) for row in section]),
        })
    current_road_active = sum(1 for row in rows if abs(number(row, "road_pre_gain")) > 1.0e-9)
    return {
        "scenario": metadata.get("test_scenario", path.stem),
        "rows": len(rows),
        "duration_s": time[-1] - time[0] if len(time) > 1 else 0.0,
        "relative_distance": distance[-1],
        "current_road_active_rows": current_road_active,
        "stable_nonreference_runs": summaries,
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("paths", nargs="+", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    report = {"distance_units": "game-relative speed-distance units; NOT METERS",
              "captures": [analyze(path) for path in args.paths]}
    encoded = json.dumps(report, indent=2, sort_keys=True)
    if args.output:
        args.output.write_text(encoded + "\n", encoding="utf-8")
    print(encoded)


if __name__ == "__main__":
    main()
