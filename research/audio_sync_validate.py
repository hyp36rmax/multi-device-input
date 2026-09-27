#!/usr/bin/env python3
"""Detect HYP36R_AUDIO_SYNC_V1 markers and validate audio/telemetry drift."""

import argparse
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
    values = []
    step = width * 2
    for at in range(0, len(raw), step):
        if width == 2:
            left, right = struct.unpack_from("<hh", raw, at)
        else:
            def s24(offset):
                value = int.from_bytes(raw[offset:offset + 3], "little", signed=False)
                return value - (1 << 24) if value & (1 << 23) else value
            left, right = s24(at), s24(at + 3)
        values.append((left + right) * 0.5)
    return values, width


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


def validate(sidecar_path, wav_path):
    sidecar = json.loads(sidecar_path.read_text(encoding="utf-8"))
    if sidecar.get("schema") != "HYP36R_AUDIO_SYNC_V1":
        raise ValueError("unsupported sidecar schema")
    samples, width = load_wav(wav_path)
    markers = detect(samples)
    if len(markers) != 2:
        return {"result": "FAIL", "reason": f"expected 2 markers, found {len(markers)}", "markers": markers}
    start_sample, start_score = markers[0]
    end_sample, end_score = markers[1]
    telemetry_duration = (sidecar["END"]["telemetry_timestamp"] -
                          sidecar["START"]["telemetry_timestamp"])
    audio_duration = (end_sample - start_sample) / RATE
    drift = audio_duration - telemetry_duration
    uncertainty_ms = 1000.0 * (HOP / RATE + 1.0 / 60.0)
    passed = (sidecar.get("completion_state") == "completed" and
              sidecar["START"]["present"] and sidecar["END"]["present"] and
              abs(drift) < 0.010 and uncertainty_ms <= 25.0)
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
        "audio_to_telemetry": {
            "telemetry_relative_seconds": "(audio_sample-start_sample)/48000 * scale",
            "scale": scale,
        },
    }


def write_marker(samples, start):
    for begin, frequency in ((start, 3500.0), (start + PULSE + GAP, 5500.0)):
        for i in range(PULSE):
            edge = min(i, PULSE - 1 - i) / 120.0
            envelope = min(1.0, edge)
            samples[begin + i] += int(6500 * envelope * math.sin(2 * math.pi * frequency * i / RATE))


def self_test():
    with tempfile.TemporaryDirectory() as folder:
        folder = Path(folder)
        samples = [int(60 * math.sin(2 * math.pi * 317 * i / RATE)) for i in range(RATE * 11)]
        write_marker(samples, RATE // 2)
        write_marker(samples, RATE // 2 + RATE * 10)
        wav_path = folder / "capture.wav"
        with wave.open(str(wav_path), "wb") as wav:
            wav.setnchannels(2); wav.setsampwidth(2); wav.setframerate(RATE)
            wav.writeframes(b"".join(struct.pack("<hh", value, value) for value in samples))
        sidecar_path = folder / "sync.json"
        sidecar_path.write_text(json.dumps({
            "schema": "HYP36R_AUDIO_SYNC_V1", "session_id": "SELF_TEST",
            "completion_state": "completed",
            "START": {"present": True, "telemetry_timestamp": 100.0},
            "END": {"present": True, "telemetry_timestamp": 110.0},
        }), encoding="utf-8")
        result = validate(sidecar_path, wav_path)
        assert result["result"] == "PASS", result
        assert result["start_sample"] == RATE // 2, result
        assert abs(result["drift_seconds"]) < 1e-9, result
        # The representative deterministic engine-like tone must not produce a
        # marker, and a cancelled sidecar must never validate as complete.
        ordinary = [int(900 * math.sin(2 * math.pi * 180 * i / RATE)) for i in range(RATE * 2)]
        assert detect(ordinary) == []
        cancelled = json.loads(sidecar_path.read_text(encoding="utf-8"))
        cancelled["completion_state"] = "cancelled"
        sidecar_path.write_text(json.dumps(cancelled), encoding="utf-8")
        assert validate(sidecar_path, wav_path)["result"] == "FAIL"
        return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("sidecar", nargs="?", type=Path)
    parser.add_argument("wav", nargs="?", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    result = self_test() if args.self_test else validate(args.sidecar, args.wav)
    print(json.dumps(result, indent=2, sort_keys=True))
    raise SystemExit(0 if result["result"] == "PASS" else 1)


if __name__ == "__main__":
    main()
