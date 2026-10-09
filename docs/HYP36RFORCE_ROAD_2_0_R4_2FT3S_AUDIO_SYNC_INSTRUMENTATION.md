# HYP36Rforce Road 2.0 R4.2F-T3S — audio/telemetry synchronization instrumentation

## Part I — Engineering record

### Result

**C — the first real Windows capture exposed two correctable instrumentation
issues.** H2 corrects the marker presentation and sidecar clock contract. A
second stationary application/loopback recording is required before T3S can be
closed. No driving or cobblestone test is authorized yet.

### Architecture

The Debug overlay exposes **Road Research Audio Sync → Start 10-second sync
validation**. It starts ordinary research telemetry under scenario
`R4_2FT3S_SYNC_VALIDATION`, creates an `HYP36R_AUDIO_SYNC_V2` JSON sidecar,
requests START, waits ten stationary seconds, requests END, closes both files,
and marks the sidecar complete. Cancel closes it as `cancelled` without pairing
a stale START with a later session.

The marker is generated in process and sent asynchronously through Windows
`PlaySoundW` to the default multimedia endpoint. Application-audio capture or
WASAPI loopback should therefore observe it with the game process/default
endpoint. Normal gameplay never calls this path. It does not replace a game
asset or alter the normal mix outside the short marker.

Each marker event contains two 20 ms stereo sine pulses at 3.5 kHz and 5.5 kHz,
separated by 40 ms, with a short silent tail. Left and right use opposite
polarity, placing the marker in the stereo side channel where the supplied real
game recording has substantially less competing energy. Peak source amplitude
remains 6,500 of 32,767 (about -14 dBFS), so H2 does not make the marker louder.
It is non-looping and deterministic. START and END are separate marker events;
the two tones inside each event are its detection signature.

### Sidecar contract

The JSON sidecar is named from the telemetry identity, for example
`audio_sync_telemetry_YYYYMMDD_HHMMSS.json`, beside the matching CSV. It records:

- schema `HYP36R_AUDIO_SYNC_V2`;
- product, `1.0.0-dev+<shortSHA>` build identity, and full commit;
- session ID derived from the telemetry filename;
- sanitized scenario;
- marker format, clock, and time units;
- completion state;
- START and END presence, marker ID, frame, and session-relative telemetry time.

V2 is intentionally versioned because V1 mixed a reset START snapshot with an
absolute END timestamp and serialized the latter with insufficient precision.
V2 samples the telemetry session's `steady_clock` at each marker request,
subtracts the START origin, and writes seconds with nine decimal places. START
is therefore `0.000000000`; END is the actual monotonic interval. These values
are checked against the matching CSV's frame and `elapsed_time` boundaries.

The public 222-column schema is unchanged.

### Clock mapping and detector

`research/audio_sync_validate.py` reads 48 kHz stereo 16- or 24-bit PCM WAV,
the V2 sidecar, and matching telemetry CSV. The V2 detector uses the stereo side
channel; V1 remains readable with its original mid-channel semantics. A
transparent two-tone matched-energy detector finds exactly two marker events
and reports their sample indices and scores. With start sample `Sa`, end sample
`Sb`, and session-relative times `Ta` and `Tb`:

```text
audio_elapsed = (Sb - Sa) / 48000
telemetry_elapsed = Tb - Ta
drift = audio_elapsed - telemetry_elapsed
scale = telemetry_elapsed / audio_elapsed
telemetry_relative = ((sample - Sa) / 48000) * scale
```

The captured marker is authoritative for audio time; request-to-render latency
is treated as constant latency corrected by START. START/END scaling corrects
relative clock drift. The marker request now reads the telemetry session clock
directly rather than inheriting the most recent 60 Hz sample, so a full telemetry
frame is no longer part of timestamp uncertainty. The conservative deterministic
budget is the 2 ms detector hop plus one 48 kHz sample, or 2.021 ms, plus the
measured absolute START/END drift. Differential Windows render latency is thus
represented by real capture evidence rather than an assumed telemetry frame.
Any capture that passes the separate 10 ms drift gate therefore remains below
12.021 ms on this conservative endpoint model.

### Local validation

The deterministic self-test generated stereo PCM with two markers ten seconds
apart plus low-level ordinary tone interference:

| Result | Value |
| --- | ---: |
| START sample | 24,000 |
| END sample | 504,000 |
| START/END score | 0.6774 / 0.6774 |
| Audio elapsed | 10.000 s |
| Telemetry elapsed | 10.000 s |
| Drift | 0.000 s |
| Estimated uncertainty | 2.021 ms |
| Unexpected markers | 0 |
| Result | PASS |

An engine-like 180 Hz control produced zero markers. A sidecar changed to
`cancelled` failed validation as required. The supplied failed OBS capture also
produced zero false V2 side-channel detections. Injecting the corrected marker
into that real background produced exactly two detections with scores 0.6391
and 0.6774, no additional marker, and no marker-window clipping. This is an
offline robustness check, not a claim that the corrected marker has already
passed through the real Windows render/capture path.

### Validation gate

A real stationary validation passes only when digital audio contains exactly
one START and one END marker, the matching sidecar is complete, identity and
format match, measured drift is below 10 ms, and estimated uncertainty remains
at or below ±25 ms. A cancelled/retried session cannot pass.

If that check passes, the minimum later physical T3 campaign remains one nearby
normal-road control, one SLOW cobblestone pass, and one FAST cobblestone pass.
It must be authorized separately. T3S performs no thump detection or Road timing
analysis.

### Credits and reference context

GATS remains experiential context. THP32 remains **EXTERNAL REFERENCE — NOT
INTEGRATED**. No external marker, sound identifier, cadence, frequency, period,
or Force behavior was adopted. ALPHA/Force Character, R5/AER, and Event 2.0
remain separate.

## Part II — Development journey recap

We first had to prove that audio and telemetry could be aligned accurately.
Otherwise an apparent relationship between a sound and vehicle state could
simply be timing error.

T3S adds only that measuring ruler: a recognizable sound at each end and a
sidecar recording when each request occurred. The first real recording showed
that the original common-mode marker competed too heavily with game audio and
that V1 mixed two clock representations. H2 moves the unchanged-level marker to
the stereo side channel and introduces the explicit relative-time V2 contract.
The offline tool uses both ends to separate constant audio latency from clock
drift. The corrected model passes its deterministic, real-background,
false-marker, CSV-pairing, and cancelled-session checks.

The final uncertainty is physical rather than conceptual: Windows must capture
the in-process marker through the same digital endpoint as the game with low
enough jitter. One stationary loopback recording can answer that. Until it does,
no driving campaign and no audio interpretation should begin.
