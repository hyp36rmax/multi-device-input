#!/usr/bin/env python3
"""Detect HYP36R_AUDIO_SYNC_V1/V2 markers and validate audio/telemetry drift."""

import argparse
import csv
import json
import math
import struct
import tempfile
import wave
from pathlib import Path

RATE = 48000
PULSE = int(RATE * 0.020)
GAP = int(RATE * 0.040)
HOP = int(RATE * 0.002)


def tone_score(samples, start, frequency):
    section = samples[start:start + PULSE]
    if len(section) != PULSE:
        return 0.0
    real = imag = energy = 0.0
    for i, value in enumerate(section):
        angle = 2.0 * math.pi * frequency * i / RATE
        real += value * math.cos(angle)
        imag -= value * math.sin(angle)
        energy += value * value
    return math.sqrt(real * real + imag * imag) / math.sqrt(energy * PULSE) if energy else 0.0


def load_wav(path):
    with wave.open(str(path), "rb") as wav:
        if wav.getframerate() != RATE or wav.getnchannels() != 2 or wav.getsampwidth() not in (2, 3):
            raise ValueError("expected 48 kHz stereo 16/24-bit PCM WAV")
        width = wav.getsampwidth()
        raw = wav.readframes(wav.getnframes())
    mid = []
    side = []
    step = width * 2
    for at in range(0, len(raw), step):
        if width == 2:
            left, right = struct.unpack_from("<hh", raw, at)
        else:
            def s24(offset):
                value = int.from_bytes(raw[offset:offset + 3], "little", signed=False)
                return value - (1 << 24) if value & (1 << 23) else value
            left, right = s24(at), s24(at + 3)
        mid.append((left + right) * 0.5)
        side.append((left - right) * 0.5)
    return mid, side, width


def detect(samples):
    candidates = []
    separation = PULSE + GAP
    for start in range(0, len(samples) - separation - PULSE, HOP):
        first = tone_score(samples, start, 3500.0)
        second = tone_score(samples, start + separation, 5500.0)
        score = min(first, second)
        if score >= 0.60:
            candidates.append((score, start))
    groups = []
    for score, start in candidates:
        if not groups or start - groups[-1][-1][1] > int(RATE * 0.15):
            groups.append([])
        groups[-1].append((score, start))
    return [max(group)[::-1] for group in groups]


def load_csv_timeline(path, sidecar):
    metadata = {}
    data_lines = []
    for line in path.read_text(encoding="utf-8-sig").splitlines():
        if line.startswith("# ") and "=" in line:
            key, value = line[2:].split("=", 1)
            metadata[key] = value
        elif line and not line.startswith("#"):
            data_lines.append(line)
    rows = list(csv.DictReader(data_lines))
    if not rows or len(rows[0]) not in {222, 228, 233, 240, 242}:
        raise ValueError("expected non-empty 222-, 228-, 233-, 240-, 242-, or 244-column telemetry CSV")
    if metadata.get("build_commit") != sidecar.get("build_commit"):
        raise ValueError("CSV/sidecar build identity mismatch")
    if metadata.get("test_scenario") != sidecar.get("scenario"):
        raise ValueError("CSV/sidecar scenario mismatch")
    if path.stem != sidecar.get("session_id"):
        raise ValueError("CSV/sidecar session identity mismatch")
    elapsed = [float(row["elapsed_time"]) for row in rows]
    frames = [int(row["frame"]) for row in rows]
    gaps = [later - earlier for earlier, later in zip(elapsed, elapsed[1:])]
    if any(gap <= 0.0 for gap in gaps) or any(b - a != 1 for a, b in zip(frames, frames[1:])):
        raise ValueError("invalid telemetry timeline")
    return {
        "sample_count": len(rows),
        "first_frame": frames[0],
        "last_frame": frames[-1],
        "first_elapsed_seconds": elapsed[0],
        "last_elapsed_seconds": elapsed[-1],
        "maximum_gap_seconds": max(gaps, default=0.0),
    }


def validate(sidecar_path, wav_path, csv_path=None):
    sidecar = json.loads(sidecar_path.read_text(encoding="utf-8"))
    schema = sidecar.get("schema")
    if schema not in ("HYP36R_AUDIO_SYNC_V1", "HYP36R_AUDIO_SYNC_V2"):
        raise ValueError("unsupported sidecar schema")
    if schema == "HYP36R_AUDIO_SYNC_V2" and csv_path is None:
        csv_path = sidecar_path.parent / f"{sidecar['session_id']}.csv"
        if not csv_path.is_file():
            raise ValueError("matching telemetry CSV is required for V2")
    mid, side, width = load_wav(wav_path)
    samples = side if schema == "HYP36R_AUDIO_SYNC_V2" else mid
    markers = detect(samples)
    if len(markers) != 2:
        return {"result": "FAIL", "reason": f"expected 2 markers, found {len(markers)}", "markers": markers}
    start_sample, start_score = markers[0]
    end_sample, end_score = markers[1]
    timestamp_key = "relative_time_seconds" if schema == "HYP36R_AUDIO_SYNC_V2" else "telemetry_timestamp"
    telemetry_duration = sidecar["END"][timestamp_key] - sidecar["START"][timestamp_key]
    audio_duration = (end_sample - start_sample) / RATE
    drift = audio_duration - telemetry_duration
    detector_uncertainty = HOP / RATE
    if schema == "HYP36R_AUDIO_SYNC_V2":
        uncertainty_ms = 1000.0 * (detector_uncertainty + 1.0 / RATE + abs(drift))
    else:
        uncertainty_ms = 1000.0 * (detector_uncertainty + 1.0 / 60.0)
    csv_timeline = load_csv_timeline(csv_path, sidecar) if csv_path else None
    csv_matches = not csv_timeline or (
        sidecar["START"]["frame"] <= csv_timeline["first_frame"] and
        sidecar["END"]["frame"] == csv_timeline["last_frame"] + 1 and
        sidecar["START"][timestamp_key] <= csv_timeline["first_elapsed_seconds"] and
        sidecar["END"][timestamp_key] >= csv_timeline["last_elapsed_seconds"] and
        sidecar["END"][timestamp_key] - csv_timeline["last_elapsed_seconds"] <=
            csv_timeline["maximum_gap_seconds"])
    passed = (sidecar.get("completion_state") == "completed" and
              sidecar["START"]["present"] and sidecar["END"]["present"] and
              csv_matches and abs(drift) < 0.010 and uncertainty_ms <= 25.0)
    scale = telemetry_duration / audio_duration
    return {
        "result": "PASS" if passed else "FAIL",
        "session_id": sidecar.get("session_id"),
        "audio_format": f"48000Hz stereo {width * 8}-bit PCM",
        "start_sample": start_sample,
        "end_sample": end_sample,
        "start_score": start_score,
        "end_score": end_score,
        "unexpected_marker_count": max(0, len(markers) - 2),
        "audio_elapsed_seconds": audio_duration,
        "telemetry_elapsed_seconds": telemetry_duration,
        "drift_seconds": drift,
        "estimated_alignment_uncertainty_ms": uncertainty_ms,
        "csv_timeline": csv_timeline,
        "csv_sidecar_reconciled": csv_matches,
        "audio_to_telemetry": {
            "telemetry_relative_seconds": "(audio_sample-start_sample)/48000 * scale",
            "scale": scale,
        },
    }


def write_marker(left, right, start):
    for begin, frequency in ((start, 3500.0), (start + PULSE + GAP, 5500.0)):
        for i in range(PULSE):
            edge = min(i, PULSE - 1 - i) / 120.0
            envelope = min(1.0, edge)
            value = int(6500 * envelope * math.sin(2 * math.pi * frequency * i / RATE))
            left[begin + i] += value
            right[begin + i] -= value


def self_test():
    with tempfile.TemporaryDirectory() as folder:
        folder = Path(folder)
        left = [int(60 * math.sin(2 * math.pi * 317 * i / RATE)) for i in range(RATE * 11)]
        right = left.copy()
        write_marker(left, right, RATE // 2)
        write_marker(left, right, RATE // 2 + RATE * 10)
        wav_path = folder / "capture.wav"
        with wave.open(str(wav_path), "wb") as wav:
            wav.setnchannels(2); wav.setsampwidth(2); wav.setframerate(RATE)
            wav.writeframes(b"".join(struct.pack("<hh", l, r) for l, r in zip(left, right)))
        sidecar_path = folder / "sync.json"
        sidecar_path.write_text(json.dumps({
            "schema": "HYP36R_AUDIO_SYNC_V2", "session_id": "telemetry_SELF_TEST",
            "build_commit": "SELF_TEST", "scenario": "SELF_TEST",
            "completion_state": "completed",
            "START": {"present": True, "frame": 0, "relative_time_seconds": 0.0},
            "END": {"present": True, "frame": 601, "relative_time_seconds": 10.0},
        }), encoding="utf-8")
        csv_path = folder / "telemetry_SELF_TEST.csv"
        columns = ["timestamp", "frame", "elapsed_time"] + [f"field_{i}" for i in range(219)]
        with csv_path.open("w", encoding="utf-8", newline="") as out:
            out.write("# build_commit=SELF_TEST\n# test_scenario=SELF_TEST\n")
            writer = csv.writer(out); writer.writerow(columns)
            for frame in range(601):
                elapsed = frame / 60.0
                writer.writerow([100.0 + elapsed, frame, elapsed] + [0] * 219)
        result = validate(sidecar_path, wav_path, csv_path)
        assert result["result"] == "PASS", result
        assert result["start_sample"] == RATE // 2, result
        assert abs(result["drift_seconds"]) < 1e-9, result
        # The representative deterministic engine-like tone must not produce a
        # marker, and a cancelled sidecar must never validate as complete.
        ordinary_left = [int(900 * math.sin(2 * math.pi * 180 * i / RATE)) for i in range(RATE * 2)]
        ordinary_right = ordinary_left.copy()
        ordinary_side = [(left - right) * 0.5 for left, right in zip(ordinary_left, ordinary_right)]
        assert detect(ordinary_side) == []
        cancelled = json.loads(sidecar_path.read_text(encoding="utf-8"))
        cancelled["completion_state"] = "cancelled"
        sidecar_path.write_text(json.dumps(cancelled), encoding="utf-8")
        assert validate(sidecar_path, wav_path, csv_path)["result"] == "FAIL"
        return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("sidecar", nargs="?", type=Path)
    parser.add_argument("wav", nargs="?", type=Path)
    parser.add_argument("csv", nargs="?", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    result = self_test() if args.self_test else validate(args.sidecar, args.wav, args.csv)
    print(json.dumps(result, indent=2, sort_keys=True))
    raise SystemExit(0 if result["result"] == "PASS" else 1)


if __name__ == "__main__":
    main()
