# HYP36Rforce Road 2.0 R4.2F-T3S — audio/telemetry synchronization instrumentation

## Part I — Engineering record

### Result

**B — synchronization works with one caveat: the real Windows application/
loopback endpoint still requires one stationary recording validation.** The
marker, metadata, detector, affine mapping, drift calculation, completion gates,
and synthetic validation are complete. No driving or cobblestone test is needed
for that remaining check.

### Architecture

The Debug overlay exposes **Road Research Audio Sync → Start 10-second sync
validation**. It starts ordinary research telemetry under scenario
`R4_2FT3S_SYNC_VALIDATION`, creates an `HYP36R_AUDIO_SYNC_V1` JSON sidecar,
requests START, waits ten stationary seconds, requests END, closes both files,
and marks the sidecar complete. Cancel closes it as `cancelled` without pairing
a stale START with a later session.

The marker is generated in process and sent asynchronously through Windows
`PlaySoundW` to the default multimedia endpoint. Application-audio capture or
WASAPI loopback should therefore observe it with the game process/default
endpoint. Normal gameplay never calls this path. It does not replace a game
asset or alter the normal mix outside the short marker.

Each marker event contains two 20 ms mono-compatible stereo sine pulses at
3.5 kHz and 5.5 kHz, separated by 40 ms, with a short silent tail. Peak source
amplitude is 6,500 of 32,767 (about -14 dBFS), leaving substantial mixing
headroom. It is non-looping and deterministic. START and END are separate marker
events; the two tones inside each event are its detection signature.

### Sidecar contract

The JSON sidecar is named from the telemetry identity, for example
`audio_sync_telemetry_YYYYMMDD_HHMMSS.json`, beside the matching CSV. It records:

- schema `HYP36R_AUDIO_SYNC_V1`;
- product, `1.0.0-dev+<shortSHA>` build identity, and full commit;
- session ID derived from the telemetry filename;
- sanitized scenario;
- marker format;
- completion state;
- START and END presence, marker ID, frame, and telemetry timestamp.

The public 222-column schema is unchanged.

### Clock mapping and detector

`research/audio_sync_validate.py` reads 48 kHz stereo 16- or 24-bit PCM WAV and
the sidecar. A transparent two-tone matched-energy detector finds exactly two
marker events and reports their sample indices and scores. With start sample
`Sa`, end sample `Sb`, telemetry timestamps `Ta` and `Tb`:

```text
audio_elapsed = (Sb - Sa) / 48000
telemetry_elapsed = Tb - Ta
drift = audio_elapsed - telemetry_elapsed
scale = telemetry_elapsed / audio_elapsed
telemetry_relative = ((sample - Sa) / 48000) * scale
```

The captured marker is authoritative for audio time; request-to-render latency
is treated as constant latency corrected by START. START/END scaling corrects
relative clock drift. The detector uses a 2 ms search hop. Combined with one
60 Hz telemetry frame, the conservative estimated alignment uncertainty is
18.67 ms, within the ±25 ms design budget.

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
| Estimated uncertainty | 18.67 ms |
| Unexpected markers | 0 |
| Result | PASS |

An engine-like 180 Hz control produced zero markers. A sidecar changed to
`cancelled` failed validation as required. Marker samples remain far below PCM
clipping. Representative real game audio was not available as a lossless
fixture, so false-detection validation against the actual game mix remains part
of the one stationary Windows capture.

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
sidecar recording when each request occurred. The offline tool uses both ends
to separate constant audio latency from clock drift. The model passes its
deterministic tests, including false-marker and cancelled-session checks.

The final uncertainty is physical rather than conceptual: Windows must capture
the in-process marker through the same digital endpoint as the game with low
enough jitter. One stationary loopback recording can answer that. Until it does,
no driving campaign and no audio interpretation should begin.
