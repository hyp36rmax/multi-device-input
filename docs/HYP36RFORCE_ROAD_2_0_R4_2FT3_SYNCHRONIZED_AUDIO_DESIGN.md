# HYP36Rforce Road 2.0 R4.2F-T3 — synchronized cobblestone audio observation design

## Part I — Engineering record

### Decision

**B — the probe is feasible but requires a small observational synchronization
addition.** The existing telemetry is sufficient for driving context and the
audio can be captured digitally, but the current runner does not emit a marker
that is both audible in the captured game stream and timestamped by telemetry.
Do not build or run the probe until that one addition receives separate approval.

Audio remains an observation channel. It is not Road input, a wheel waveform,
physics, or a material-strength signal. Active Road remains paused regardless
of the observation result.

### Synchronization audit

The following methods were considered:

| Method | Assessment |
| --- | --- |
| Assume audio and telemetry start together | Rejected. Their files and clocks start independently. |
| Manual visual or audible alignment | Rejected as primary evidence. It adds operator and frame uncertainty. |
| Capture-container timestamps alone | Useful for diagnostics, but not a proven common clock with the telemetry timestamp. |
| Collision, gear shift, or wheel effect | Rejected because it contaminates the measured gameplay or Force. |
| Existing UI sound plus inferred menu state | Rejected. The current schema does not record the exact sound request, and UI-to-audio latency is unmeasured. |
| Arbitrary existing game sound | Rejected until its ownership and lifecycle are known. T2 showed how easily a scripted sound can be misclassified. |
| Runner-generated audio pulse plus same-action telemetry marker | Selected. Deterministic, narrow, and observational. |

The minimum future addition is a short, unmistakable two-pulse PCM marker mixed
into the same digital output captured with the game. The runner records the
queue action's telemetry-relative timestamp and frame in the session sidecar.
A second marker after the pass measures offset drift. The marker must be silent
outside research mode, must not invoke Force, and must not reuse a collision,
gear, or unknown native sound.

Queue-to-speaker latency means the logged queue timestamp and audible sample
are not literally simultaneous. The start/end pair makes constant offset
irrelevant to interval comparisons and bounds drift. The initial acceptance
budget is **±25 ms**, including one 60 Hz game update (`16.7 ms`) plus audio
block/detection uncertainty. Measured start/end drift must remain below 10 ms
and each pulse must be located within one audio block. If the observed system
cannot meet ±25 ms, the experiment is nonviable until the marker is timestamped
closer to the audio-render callback. This budget is expected to be much smaller
than a genuine audible inter-event interval; that must be verified before
hypothesis testing.

### Digital audio capture

Capture the game's digital output directly through Windows application-audio
capture or WASAPI loopback. Microphone, voice chat, alerts, and unrelated
desktop sources must be excluded. Use **48 kHz, 24-bit stereo PCM WAV** (or a
lossless container decoded to that representation). Stereo is retained in case
the road/contact presentation is spatialized. Higher rates and depths add no
required information here.

Disable game music. Keep sound effects enabled because they contain the target.
Keep engine/drivetrain audio enabled unless the game offers a clean independent
control; removing it could change the normal presentation being studied. The
analysis treats it as interference using band-limited/onset methods and the
normal-road control. No game audio asset or mix is replaced.

### Minimum future capture design

Use OutRun 2, Tulip Garden, cobblestone immediately after the first corner.
Verify the stable target interval as all four surfaces equal to `0x100000`.
The future runner has three separately accepted recordings:

1. **Nearby normal-road control** — similar gear and speed range, minimal
   steering, no collision. This is required to reject engine, drivetrain,
   general road noise, and loop structure.
2. **SLOW cobblestone** — naturally slower and controlled; do not crawl merely
   to extend the section.
3. **FAST cobblestone** — same approximate line, materially faster, while
   retaining full surface occupancy and control.

Only two cobblestone speeds are used. FAST must have a target-surface median
speed at least **40% above SLOW**, and preferably `FAST P25 > SLOW P75`. Failure
to meet the median criterion makes the cadence comparison inconclusive rather
than negative.

Each recording follows:

```text
READY
  -> start digital audio capture
  -> Start Test / 3-2-1
  -> start sync pulse + sidecar marker
  -> approach and traverse target
  -> end sync pulse + sidecar marker
  -> automatic telemetry stop
  -> stop digital audio capture
  -> Accept / Retry
```

The 222-column telemetry already supplies frame/time, speed, four surface
states and transitions, native L/R tactile activity, gear, Impact/event state,
steering, grip/load context, and timing gaps. No periodic-shadow field is
needed. The marker belongs in versioned research-session metadata, not a new
public telemetry column.

### Audio event detection

Analysis begins by confirming that discrete thumps exist; it does not begin by
estimating “cobblestone frequency.” Preserve the raw stereo recording, then:

1. inspect left, right, mid `(L+R)/2`, and side `(L-R)/2` channels;
2. remove only DC and apply a documented broad band-pass chosen from the
   visible target-event energy, while retaining an unfiltered comparison;
3. compute short-window energy and positive spectral flux;
4. threshold against a local median/MAD noise floor;
5. merge detections inside a predeclared refractory window;
6. manually verify every accepted onset against waveform/spectrogram context.

No opaque classifier is required. A pass needs at least **five accepted target
events**, producing four inter-event intervals. This exploratory threshold
separates a repeating pattern from one or two impacts without pretending to
meet the later five-cycle Road claim standard.

Only detections inside the continuous all-four-`0x100000` interval are primary.
Reject or flag detections overlapping a surface transition, gear change,
Impact/collision, timing gap, large steering correction, or another identified
sound-producing event. The normal-road control must not show the same event
signature and spacing at comparable confidence.

### Alignment and distance reconstruction

Let `sync_sample_start` and `sync_timestamp_start` identify the detected audio
pulse and logged telemetry marker. Initially:

```text
audio_relative_time = (sample_index - sync_sample_start) / sample_rate
telemetry_relative_time = telemetry_timestamp - sync_timestamp_start
```

Use the end pair to fit a linear offset/drift mapping rather than assuming both
clocks run identically. Interpolate speed, surface, cumulative distance, and
contamination fields at each mapped event timestamp.

Game-relative distance remains:

```text
valid_dt = timestamp[n] - timestamp[n-1]
distance_increment = max(speed[n], 0) * valid_dt
cumulative_distance += distance_increment
```

Inter-event distance is the difference between interpolated cumulative-distance
values. No meters or physical wheel travel are claimed.

### Predeclared interpretation

- **H1 — distance-related:** FAST has materially shorter inter-event time than
  SLOW, while inter-event distance has lower pooled variation and agrees across
  speeds within exploratory uncertainty. This is an audio timing lead, not
  authorization for FFB.
- **H2 — time-based:** inter-event time remains comparatively stable despite
  verified speed separation, while distance spacing scales with speed.
- **H3 — presentation/loop:** neither time nor distance maps coherently to
  travel, or the same signature appears on normal road / follows a fixed audio
  loop independent of occupancy.
- **H4 — unreliable cadence:** fewer than five clean events, unstable detection,
  synchronization outside budget, insufficient speed separation, or excessive
  contamination.

Use medians, P05/P95, coefficient of variation, and bootstrap uncertainty for
the small interval sets. H1 requires the distance interpretation to improve
across both passes, not merely produce one attractive ratio. Mixed evidence is
reported unresolved.

A positive H1 result justifies targeted reverse engineering around the runtime
sound request at accepted event timestamps: caller identity, sound identifier,
surface reader, counter/timer, and speed/travel inputs. It does not justify
audio-fed Force or DirectInput output. H2/H3 redirects research toward audio
presentation ownership. H4 permits one focused repeat only if the precise
failure is correctable; otherwise the audio lead closes.

### Unresolved questions

- Can the research-only pulse be mixed into exactly the captured game-audio
  endpoint without depending on a particular capture application?
- What queue-to-render latency and jitter occur on the target Windows system?
- Which frequency band best separates the observed thump from engine audio?
- Can the short cobblestone interval provide five uncontaminated onsets at both
  speeds?
- Does stereo position help distinguish tire/contact sound from global ambience?

### Credits and reference context

The audible observation comes from this project's Tulip Garden testing. GATS
remains experiential context. THP32 remains **EXTERNAL REFERENCE — NOT
INTEGRATED**. No external code, cadence, frequency, period, wavelength, sound
identifier, or effect parameter is adopted. ALPHA/Force Character, R5/AER, and
Event 2.0 remain separate lanes.

## Part II — Development journey recap

The controller-vibration data did not preserve a usable cobblestone rhythm,
even though a repeating thump seemed audible in game. T3 asks how to test that
observation without quietly turning it into a Force signal.

The difficult part is synchronization. Starting an audio recorder and telemetry
at roughly the same moment is not evidence-grade alignment. A small research
marker heard by the recorder and timestamped by the runner gives both streams a
shared reference. A second marker measures drift instead of assuming the two
clocks stay aligned.

The eventual observation remains deliberately small: nearby normal road, one
slow pass, and one fast pass. If discrete events survive surface, event, and
engine-noise controls, their time and game-relative distance spacing can be
compared. Whatever the result, the next question is ownership of the internal
event—not how quickly to send it to a wheel.
