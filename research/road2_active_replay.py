#!/usr/bin/env python3
"""Deterministic offline replay of HYP36R_ROAD2_ACTIVE_V0.

This mirrors the small active generator for accepted-capture safety review. It
does not invent native state: surface occupancy and recorded native motor
evidence remain the only authorization sources.
"""

import csv
import hashlib
import json
import math
import sys

SEED = 0x48595036
STEP = 1.0 / 120.0
CEILING = 0.06
MAX_SLEW = 0.90


def rows(path):
    with open(path, newline="", encoding="utf-8-sig") as stream:
        yield from csv.DictReader(line for line in stream if not line.startswith("#"))


def replay(label, path, reference):
    random_state = SEED
    accumulator = fast = slow = dc_in = dc_out = envelope = previous = 0.0
    prior_time = None
    outputs = []
    authorized_rows = event_rejections = normal_false_positives = slew_hits = 0

    def random_value():
        nonlocal random_state
        value = random_state
        value ^= (value << 13) & 0xFFFFFFFF
        value ^= value >> 17
        value ^= (value << 5) & 0xFFFFFFFF
        random_state = value & 0xFFFFFFFF or SEED
        return ((random_state & 0x00FFFFFF) / 0x007FFFFF) - 1.0

    for row in rows(path):
        elapsed = float(row["elapsed_time"])
        dt = 0.0 if prior_time is None else elapsed - prior_time
        prior_time = elapsed
        surfaces = tuple(int(float(row[f"surface_{n}"])) for n in range(4))
        differing = sum(value != reference for value in surfaces)
        left = abs(float(row["vibration_left_raw"]))
        right = abs(float(row["vibration_right_raw"]))
        rise = float(row.get("vibration_rise", "0") or 0.0)
        gear = row.get("gear_transition", "0") not in ("0", "false", "False", "")
        event = gear or rise > 0.12
        authority = min(1.0, math.sqrt((min(1.0, left) ** 2 + min(1.0, right) ** 2) * 0.5))
        valid_dt = math.isfinite(dt) and 0.0 < dt <= 0.1
        authorized = valid_dt and differing > 0 and authority > 1e-6 and not event
        if event and differing:
            event_rejections += 1
        if authorized:
            authorized_rows += 1
        else:
            authority = 0.0

        coverage = differing * 0.25 if authorized else 0.0

        if not valid_dt:
            random_state = SEED
            accumulator = fast = slow = dc_in = dc_out = envelope = previous = 0.0
            output = 0.0
        else:
            accumulator += dt
            while accumulator >= STEP:
                rate = 7.5 if coverage > envelope else 11.0
                delta = rate * STEP
                envelope = min(coverage, envelope + delta) if envelope < coverage else max(coverage, envelope - delta)
                white = random_value()
                fast += 0.34 * (white - fast)
                slow += 0.055 * (white - slow)
                band = fast - slow
                blocked = band - dc_in + 0.992 * dc_out
                dc_in = band
                dc_out = max(-1.0, min(1.0, blocked))
                if authority > 0.0:
                    target = max(-CEILING, min(CEILING, authority * envelope * dc_out * CEILING))
                    maximum_delta = MAX_SLEW * STEP
                    previous = min(target, previous + maximum_delta) if previous < target else max(target, previous - maximum_delta)
                accumulator -= STEP
            if authority <= 0.0:
                previous = output = 0.0
            else:
                target = max(-CEILING, min(CEILING, authority * envelope * dc_out * CEILING))
                output = previous
                if abs(output - target) > 1e-7:
                    slew_hits += 1

        if differing == 0 and abs(output) > 1e-12:
            normal_false_positives += 1
        outputs.append(output)

    digest = hashlib.sha256(",".join(f"{value:.9f}" for value in outputs).encode()).hexdigest()
    mean = sum(outputs) / len(outputs) if outputs else 0.0
    rms = math.sqrt(sum(value * value for value in outputs) / len(outputs)) if outputs else 0.0
    return {"scenario": label, "rows": len(outputs), "authorized_rows": authorized_rows,
            "event_rejections": event_rejections, "normal_false_positives": normal_false_positives,
            "mean_dc": mean, "rms": rms, "max_abs": max(map(abs, outputs), default=0.0),
            "slew_limited_rows": slew_hits, "determinism_sha256": digest}


def main(arguments):
    if len(arguments) < 3:
        raise SystemExit("usage: road2_active_replay.py REFERENCE_RAW label=csv ...")
    reference = int(arguments[1], 0)
    results = [replay(*item.split("=", 1), reference) for item in arguments[2:]]
    print(json.dumps({"model": "HYP36R_ROAD2_ACTIVE_V0", "reference_raw": reference,
                      "results": results}, indent=2))


if __name__ == "__main__":
    main(sys.argv)
