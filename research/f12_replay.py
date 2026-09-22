"""Offline F1.2 replay of synchronized S9 telemetry; never used by the game.

Usage: python research/f12_replay.py /path/to/telemetry.csv
The input is a local private capture, not a repository asset.
"""

import csv
import sys

import numpy as np


def rolling_rms(values, window):
    squares = values * values
    cumulative = np.concatenate(([0.0], np.cumsum(squares)))
    start = np.maximum(np.arange(len(values)) - window + 1, 0)
    return np.sqrt((cumulative[1:] - cumulative[start]) / (np.arange(len(values)) - start + 1))


def metrics(composer, output, directional, road, impact, baseline, elapsed):
    absolute = np.abs(output)
    rms1 = rolling_rms(output, 60)
    rms3 = rolling_rms(output, 180)
    difference = np.diff(output)
    delta = np.diff(elapsed)
    valid_delta = (delta > 0) & (delta <= .5)
    slew = np.abs(difference[valid_delta] / delta[valid_delta])
    return [
        *np.quantile(absolute, [.95, .99]).round(4), round(float(absolute.max()), 4),
        round(float(np.quantile(rms1, .95)), 4), round(float(rms1.max()), 4),
        round(float(np.quantile(rms3, .95)), 4), round(float(rms3.max()), 4),
        *(round(float(np.mean(absolute >= threshold) * 100), 2) for threshold in [.75, .90, .98]),
        round(float(np.mean(np.abs(composer) > 1) * 100), 2),
        round(float(slew.max()), 3),
        round(float(np.mean(np.abs(road) > np.abs(directional)) * 100), 2),
        round(float(np.mean(np.abs(impact) > np.abs(directional)) * 100), 2),
        int(np.count_nonzero((np.abs(baseline) > .01) & (np.sign(output) != np.sign(baseline)))),
        bool(np.isfinite(output).all()),
    ]


def main(path):
    with open(path, newline="", encoding="utf-8-sig") as handle:
        rows = list(csv.DictReader(line for line in handle if not line.startswith("#")))
    def column(name):
        return np.asarray([float(row[name]) for row in rows], dtype=np.float64)

    pre_presence = column("s9_directional_pre_presence")
    captured_presence = column("presentation_presence")
    primary = pre_presence * 1.44
    road = column("presentation_road_request")
    impact = column("presentation_impact_request")
    measured_composer = column("s2_composer_input")
    measured_output = column("ffb_raw")
    measured_tanh = column("s2_post_tanh")
    elapsed = column("elapsed_time")
    ramp = np.ones(len(rows))
    stable = np.abs(measured_tanh) > .01
    ramp[stable] = np.clip(measured_output[stable] / measured_tanh[stable], 0, 1)
    observed_replay = pre_presence * captured_presence + road + impact
    reference_composer = primary + road + impact
    reference = np.tanh(reference_composer) * ramp
    if np.max(np.abs(observed_replay - measured_composer)) > 1e-5:
        raise ValueError("capture does not reconstruct the channel composition")
    if np.max(np.abs(np.tanh(observed_replay) * ramp - measured_output)) > 1e-5:
        raise ValueError("capture does not reconstruct the recorded output")
    if not np.allclose(captured_presence, 1.2, atol=1e-5):
        raise ValueError("this replay requires the recorded S9 Presence 1.20 capture")
    print("samples", len(rows), "presence", 1.44)
    print("captured_presence", np.unique(captured_presence),
          "observed_composer_error", float(np.max(np.abs(observed_replay - measured_composer))))
    print("observed_output_error", float(np.max(np.abs(np.tanh(observed_replay) * ramp - measured_output))))
    print("counterfactual_1.44_delta_max", float(np.max(np.abs(reference_composer - measured_composer))))
    print("ramp_below_0.99", int(np.count_nonzero(ramp < .99)))
    print("fields: case gain p95 p99 max rms1_p95 rms1_max rms3_p95 rms3_max occ75% occ90% occ98% pre_tanh_over1% max_slew_per_s road_gt_primary% impact_gt_primary% sign_flips finite")
    def report(label, gain, steering_gain, road_gain, impact_gain):
        d, r, i = primary * steering_gain, road * road_gain, impact * impact_gain
        composer = d + r + i
        output = np.tanh(composer) * ramp
        print(label, gain, *metrics(composer, output, d, r, i, reference, elapsed))
    report("reference", 1, 1, 1, 1)
    gains = [0, .5, .75, 1, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.75, 2]
    for label in ["steering", "road", "impact"]:
        for gain in gains:
            settings = dict(steering_gain=1, road_gain=1, impact_gain=1)
            settings[label + "_gain"] = gain
            report(label, gain, **settings)
    for gain in gains[3:]:
        report("all", gain, gain, gain, gain)
    for s, r, i in [(1.2, 1.3, 1.2), (1.3, 1.5, 1.3), (1.3, 1.5, 1.5),
                    (1.4, 1.5, 1.4), (1.5, 2.0, 2.0)]:
        report("combined", f"{s}/{r}/{i}", s, r, i)
    print("source activity: road", int(np.count_nonzero(np.abs(road) > .001)),
          "impact", int(np.count_nonzero(np.abs(impact) > .001)))


if __name__ == "__main__":
    main(sys.argv[1])
