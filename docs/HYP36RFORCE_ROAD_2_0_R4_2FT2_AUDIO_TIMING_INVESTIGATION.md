# HYP36Rforce Road 2.0 R4.2F-T2 — existing surface and audio timing investigation

## Part I — Engineering record

### Decision

Track A is **A3 — existing captures do not contain defensible spatial timing
evidence**. Track B is **B5 — a targeted synchronized audio and telemetry probe
is required** if the audible Tulip Garden cadence is to be investigated
further. The overall gate is **E — targeted synchronized audio and telemetry
probe is justified**.

The passive periodic model requires targeted validation. Active Road remains
paused. No wheel output, runtime hook, telemetry field, UAT, or new physical
capture is introduced here.

### Track A — existing capture reanalysis

The accepted R2-B B01–B06 and R4.2C CST01–CST03 captures were re-read using
`research/r42ft_spatial_timing.py`. Time gaps are clamped to `[0, 0.1]` and
game-relative distance is reconstructed as:

```text
valid_dt = timestamp[n] - timestamp[n-1]
distance_increment = max(speed_magnitude, 0) * valid_dt
cumulative_distance += distance_increment
```

For every stable non-reference interval, the raw combined native motor
envelope is retained. A least-squares line `envelope = a * speed + b` is then
fit only within that interval. The residual is the recorded envelope minus
that line. Raw and residual signals are resampled separately on uniform elapsed
time and reconstructed-distance axes. Nontrivial autocorrelation is considered
only after the immediate smooth-signal shoulder crosses zero. A peak is never
treated as a frequency or wavelength by itself.

Intervals are classified separately: **uniform** means all four surface values
match; **partial** means a persistent side/corner group differs; **mixed** means
several materials or corner patterns coexist; **transitional** means entry,
exit, or re-entry dominates. Gear transitions, Impact/collision activity,
surface changes, invalid timing, and materially changing context prevent a
window from becoming primary recurrence evidence.

| Capture | Useful evidence | Timing result |
| --- | --- | --- |
| B01 asphalt control | 16.185 s, `(2,2,2,2)`, Candidate A/Road zero | Negative control; no surface recurrence |
| B02 striped partial | partial paired occupancy up to 1.670 s; clean uniform all-`2048` window 0.380 s / 24 rows | Too short; no accepted nontrivial recurrence |
| B03 striped broad | all-`1024` 2.450 s; paired mixed state 0.915 s | Occupancy/context proven, cycle count and clean recurrence absent |
| B04 rough/sand partial | 13.76 s asymmetric activity; clean all-`8192` windows 0.865 s and 0.480 s | Continuous surface authorization, but uniform windows are too short and residual is dynamics/confound scale |
| B05 rough/sand broad | all-`4` periods 2.550 s and 1.000 s; strict clean inspected segment 0.520 s / 32 rows | Nearly exact speed scaling; residual RMS `5.97e-8`; time/distance correlations both about `0.25`, with no distance advantage |
| B06 re-entry | initial all-`8` 5.465 s; strict clean window 5.065 s / 305 rows / 1.7613 relative units | Nearly exact speed scaling; residual RMS `7.19e-8`; weak `0.0912 s` / `0.04450`-unit features do not reproduce elsewhere |
| CST01/CST03 | stable reference controls, Candidate A zero | Vehicle dynamics can vary without Road timing |
| CST02 cobblestone | 7.280 s / 438 rows / 1.8552 relative units, all `0x100000` | Envelope correlation with speed `0.999999999925`; residual RMS `1.53e-7`; weak time `0.147` and distance `0.194` correlations are numerical-scale and non-repeating |

B02 and B03 show striped occupancy but not a recoverable within-material
interval. B04 is the strongest continuous authorization capture, yet its
material pattern changes and its clean uniform subwindows are under one second.
B05 is broader but clean recurrence evidence is short and the residual is at
floating/quantization scale. B06 is the longest strict non-cobblestone uniform
window and therefore the strongest nominal candidate; its small distance-domain
improvement is neither meaningful in amplitude nor reproduced by CST02/B05.
B06 primarily captures surface exit/re-entry context, not a stable material
cadence.

Across surfaces, raw spectra and autocorrelation follow speed/envelope shape.
After the transparent linear speed removal, remaining energy is tiny,
window-sensitive, and inconsistent. Reconstructed distance does not stabilize
a common lag, and no candidate passes repeatability, sufficient-cycle,
cross-window, amplitude, event-independence, and surface-relevance gates.

### Track B — audio and contact timing lineage

The bounded static search used the validated replacement executable
`68ceb386829066f8455b9d027320af962584321f3e2e8a79c72841495a6134c3`, existing
repository address knowledge, audio hooks, native-surface lineage, printable
asset references, and direct disassembly around the game's sound request path.
It searched for surface readers, material lookup results, tire/road/contact
audio, trigger counters, phase/distance accumulators, speed-dependent intervals,
and looping sound requests. No external implementation offsets or constants
were used.

The established surface path remains:

```text
water_flag_24C[0..3]
  -> sub_1149C0 material coefficients
  -> maximum coefficient
  -> speed/context scaling
  -> unsigned Xbox motor envelope
```

No proven branch from those four readers, `field14`, or the mapped material
coefficient reaches an audio trigger. The PC executable imports DirectSound and
uses the internal `PrjSndRequest` dispatcher at `0x4249F0`, but its many callers
do not by themselves establish surface semantics.

One concrete timing-shaped audio path was identified at `0x44BB30`. It starts
looping sound `0x8D` at counter zero, requests another sound at frame-count
moduli 200/320, and stops at counter 600. Repository lineage independently
identifies `0x8D` as the Pegasus clopping sound and applies
`FixPegasusClopping` when that scripted effect fails to stop. Startup also
explicitly stops `0x8D`. This is **B3 — looping/presentation audio**, driven by
a scripted counter, with no demonstrated surface-state reader. It is a useful
negative control, not evidence for cobblestone spacing.

The audible Tulip Garden thump itself remains **B5 — insufficient evidence**.
No timer, counter, wheel rotation, travel accumulator, surface coordinate,
per-wheel contact trigger, or material-dependent sound branch was linked to it.
It may be a looping sample, procedural presentation, contact event, or another
unmapped subsystem. A sample loop would still not establish physical spacing.

Track A and Track B therefore remain independent. No telemetry recurrence can
be matched to a traced audio event, and the known counter-driven clop path is
unrelated to the accepted surface captures. Existing audio information is not
semantically suitable for steering-wheel Road presentation.

### Minimum future probe, if separately approved

The next useful step is not a Force change. It is one synchronized observation
of the existing Tulip Garden pass with lossless game audio and the existing
telemetry clock aligned by a deliberate, harmless synchronization event. Audio
thump timestamps would be compared against surface occupancy, speed,
reconstructed distance, transitions, Impact/events, and steering/load context.
The probe must first establish that events repeat, then test whether time or
distance spacing is more stable across at least two materially different
speeds. It must not feed audio into Force.

No public telemetry-schema change is justified now. If static work first finds
an internal audio trigger, the minimum versioned extension would append only:

- trigger-active/event counter;
- sound/effect identifier or anonymized candidate identifier;
- source timer/phase/cooldown, if one actually exists;
- same-frame surface and vehicle association already present in the schema.

Without a traced internal trigger, synchronized external audio timestamps are
the measurement and the 222-column schema remains sufficient.

### Unresolved questions

- Is the heard cadence a repeated sample, repeated trigger, or unrelated scene
  audio?
- Does it remain tied to cobblestone occupancy when speed changes?
- Does its period scale with time, distance, wheel rotation, or neither?
- Is there an unmapped surface/contact sound dispatcher outside the restored
  controller-effect lineage?
- Can a future synchronized probe provide enough clean events in the short
  Tulip Garden section?

### Credits and reference context

The audible observation originated in the project's own Tulip Garden testing.
GATS remains experiential context. THP32 remains **EXTERNAL REFERENCE — NOT
INTEGRATED**. No external code, offset, frequency, period, wavelength, audio
trigger, or effect parameter was adopted. ALPHA/Force Character, R5/AER, and
Event 2.0 remain separate lanes.

## Part II — Development journey recap

We could hear a repeating cobblestone thump, but the mapped controller
vibration contained no recoverable spatial cadence. That raised the possibility
that another game subsystem, such as audio, retained timing information that
the tactile path discarded.

Rechecking every accepted surface capture did not reveal that missing cadence.
The motor envelope continued to behave as surface coefficient multiplied by
speed, and its tiny residual features did not become reliably more stable in
distance. The static audio search found a real counter-driven looping clop
example, but it belongs to scripted presentation rather than the road-surface
path. That distinction matters: hearing rhythm is not proof of physical road
spacing.

The result is a narrower, testable lead rather than a Road waveform. A future
synchronized audio/telemetry observation can ask whether the heard events are
real, repeatable, and distance-related. Until then, periodic Road remains
passive and Active Road remains paused.
