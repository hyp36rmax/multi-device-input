# HYP36Rforce Road 2.0 R4.2F-T — spatial timing evidence

**PART I — ENGINEERING RECORD**

## Scope and decision

R4.2F-T asks whether the supported executable knowledge, mapped native state,
or accepted captures establish a repeatable surface-related progression in the
distance domain. It is offline/static Road 2.0 research only. It changes no
runtime code, telemetry schema, Force equation, current Road, or DirectInput
output.

**Decision gate: C — NO SPATIAL TIMING EVIDENCE; TIMING SEMANTICS REMAIN
UNRESOLVED.**

**Periodic-model consequence: PERIODIC ROAD REMAINS BLOCKED.**

**Active-Road gate: ACTIVE ROAD REMAINS PAUSED.**

Distance-domain phase still has the more physical topology because it follows
travel and stops with the car, but the project does not possess an
evidence-backed spatial interval.

## Static and executable investigation

The investigation revisited the project-owned structure map, restored Xbox
controller-effect routine, supported-PC observations, R4.2N lineage, and prior
native-dynamics work. No new executable offset was assigned.

| Candidate | Writer / immediate source | Readers / update | Finding | Class |
| --- | --- | --- | --- | --- |
| `EVWORK_CAR::spd_mb_20` | native car physics; three-component velocity-like state | HYP36Rforce and game systems each car update | supports relative distance reconstruction; not a persistent distance accumulator | physical/spatial input, not a timing reference by itself |
| `EVWORK_CAR +0x1C4` | native speed-like float; writer absent from repository | restored `CalcVibrationValues()` each update | material multiplier and threshold source; no phase or accumulated travel | vehicle-dynamic candidate |
| `water_flag_24C[0..3]` / corner `+0x14` | native per-contact surface state | coefficient lookup / passive observer each update | categorical context, not within-surface progression | render/physics context candidate |
| `sub_1149C0()` result | categorical surface/context lookup | maximum then speed scaling | positive material amplitude coefficient only | procedural effect-state candidate |
| `CalcVibrationValue_FrameCnt0/1` | restored effect branches | controller-effect persistence | frame hold/decay; no distance coupling or bipolar phase | procedural effect-state candidate |
| corner `+0xE8` | unresolved native writer | passive four-corner observer | constant `1.0` through stable cobblestone; no progression | insufficient evidence |
| corner `+0xEC/+0xEE` | unresolved native writers | passive four-corner observer | vehicle/orientation-correlated state also active on normal road; absent from motor path | vehicle-dynamic candidates |
| corner `+0x28`, `+0xAC`, `+0xB0` | unresolved native physics writers | passive four-corner observer | displacement/loading and contact response candidates; no travel accumulator | vehicle-dynamic candidates |
| `VibrationLeftMotor/RightMotor` | restored `CalcVibrationValues()` | Road, Impact, telemetry, controller output | composite unsigned amplitudes; stable-surface variation follows speed | procedural effect output |

No mapped wheel rotation, wheel angular velocity, per-wheel travel distance,
track-distance accumulator, surface UV coordinate, road-segment phase,
contact-point sampling coordinate, procedural vibration phase, or
distance-based event accumulator was found. This does not prove none exists in
the executable; none is established by the current bounded evidence.

## `CalcVibrationValues()` timing-state revisit

Four categorical surface values become positive coefficients, are reduced to
their maximum, multiplied by `EVWORK_CAR +0x1C4`, mixed with vehicle/event
state, held/decayed by two frame counters, and clamped into unsigned motor
levels. There is no time/distance accumulator, oscillator, periodic lookup,
random source, wheel-travel sample, or surface-coordinate read in the mapped
routine.

The counters encode persistence duration. They advance by update count and are
set by several non-Road branches; they neither follow travel nor create a
repeatable signed carrier. The stable-surface temporal source is the speed-like
scalar. This confirms R4.2N's amplitude lineage and adds no timing semantics.

## Offline method

`research/r42ft_spatial_timing.py` parses only accepted
`HYP36R_RESEARCH_II_R1` captures. It reconstructs relative travel as:

```text
valid_dt = clamp(timestamp[n] - timestamp[n-1], 0, 0.1)
distance_increment = max(speed_magnitude, 0) * valid_dt
cumulative_distance += distance_increment
```

Distance is in **game-relative speed-distance units, not meters**. For stable,
uniform non-reference intervals the script fits the combined native motor
envelope against speed, resamples the residual uniformly in time and relative
distance, and compares nontrivial autocorrelation recurrence. A recurrence
after speed removal would still be a candidate, not an automatic wavelength.

Gear, collision/Impact, surface-transition, and reference-surface periods are
excluded from stable-interval interpretation. Steering, Directional,
LOAD/RELEASE/FREE/BITE, and derived grip never authorize a Road interval.

## Cobblestone result

The accepted CST02 interval is rows 0–437: 438 samples, 7.280 seconds, and
1.8552 reconstructed relative-distance units. Speed spans `0.132707` to
`0.388211`, with median `0.2542975`.

The combined native envelope is almost exactly linear in speed:

```text
combined = 0.1491003215 * speed - 0.0000001161
correlation = 0.999999999925
residual RMS = 0.0000001533
combined RMS = 0.0399533
```

The residual is approximately 0.00038% of combined RMS. Its strongest accepted
nontrivial recurrence is weak in both representations: about `0.147` at
`2.873 s` and `0.194` at `0.806` relative-distance units. Because this operates
on near-quantization residual from one short interval with few possible cycles,
the small difference is not physical spatial-period evidence. No repeatable
peaks/troughs survive removal of speed.

Candidate A provides a continuous surface-conditioned amplitude envelope, not
a measured bump-spacing carrier. The prior approximately `0.60 Hz`
raw-envelope spectral peak is speed trend/envelope modulation, not stable
independent texture timing.

## Rough, sand, runoff, and re-entry

R2-B provides surface persistence but not a controlled constant-material,
multi-speed timing experiment. Partial occupancy changes class and corner mix;
broad intervals are short or include event state. Motor envelopes also include
different coefficients, flags, counters, transitions, and vehicle dynamics.

Where clean uniform intervals are long enough to inspect, speed-scaled
amplitude again dominates:

- B06 all-`8`, 305 samples / 5.065 s / 1.7613 relative units: speed correlation
  `0.999999999999`, residual RMS `7.19e-8`;
- B05 all-`4` inspected segment, 32 samples / 0.520 s: speed correlation
  `0.999999761`, residual RMS `5.97e-8`;
- shorter B02/B04 uniform segments are too brief and confounded.

B06's weak residual recurrence near `0.091 s` / `0.0445` relative units is at
numerical scale and does not reproduce in CST02. B05 likewise yields only a
short-window residual feature. No shared spatial interval or evidence-backed
material-specific interval is supported. Mixed occupancy and re-entry are
state transitions, not within-material cycles.

## Controls and isolation

CST01 and CST03 remain uniformly reference surface and produce exactly zero
Candidate A. They do show vehicle dynamics, steering, gear/event activity, and
EC/EE variation, confirming that these cannot become Road timing merely because
they oscillate.

Stable CST02 contains no gear transition, collision candidate, Impact, surface
transition, or unknown event. Directional is present, but normal-road controls
contain equal/greater Directional with zero Candidate A. `fieldE8` is constant
at every corner. EC/EE correlations differ in sign/scale and continue outside
surface activity; neither has writer/reader lineage to Road timing.

## Spectral and autocorrelation assessment

Raw-envelope spectra are dominated by speed/envelope trends. Removing the
known stable-surface speed relationship removes effectively all cobblestone
energy. Autocorrelation of the numerical-scale residual does not produce a
repeatable lag across cobblestone, rough/sand, and re-entry. Distance resampling
does not materially stabilize a common recurrence relative to time resampling.

No FFT peak, autocorrelation peak, crossing count, or visual stripe spacing is
promoted to frequency or wavelength. The evidence fails repeatability and
cross-context tests.

## Candidate inventory and confidence

| Candidate | Classification | Spatial-timing decision | Confidence |
| --- | --- | --- | --- |
| reconstructed `integral(speed * dt)` | A — physical/spatial candidate | valid relative axis, but supplies no interval | high for progression; physical scale unknown |
| `+0x1C4` / captured speed | C — vehicle-dynamic candidate | envelope source, not material spacing | high |
| raw surface / `field14` | D — context candidate | material gate; no within-material progression | high |
| material coefficient | B — procedural effect state | amplitude only | high |
| effect frame counters | B — procedural effect state | update-count hold/decay | high |
| E8 | E — insufficient evidence | constant in cobblestone; no timing lineage | medium |
| EC/EE and AC/B0 | C — vehicle-dynamic candidates | normal-road and steering/load confounds | high as rejection; physical semantics incomplete |
| detrended motor/Candidate A residual | E — insufficient evidence | numerical-scale, weak, non-generalizing recurrence | high as rejection of current captures |

The strongest spatial evidence is only reconstructed travel itself. It proves
how distance phase could advance, not how often a Road waveform should repeat.

## Targeted passive probe requirement

**TARGETED PASSIVE PROBE REQUIRED** before distance timing can advance.

Missing evidence is repeatable within-material structure at materially
different speeds. Existing captures are short, use one pass per controlled
surface, and record no direct distance/wheel-travel/native sampling coordinate.

The minimum future passive probe should record session/frame and valid delta
time, full `spd_mb_20` plus speed magnitude, four raw surfaces and transitions,
native L/R motors and Candidate A, event exclusions, and steering/M4 only as
contamination controls. It should add one raw distance/rotation/sampling field
only if a static writer/reader trace first identifies it.

The physical scenario would be repeated passes over the same sufficiently long
uniform surface at low and high steady speed, bracketed by local normal road.
Required evidence is a recurrence whose time period changes with speed but
whose distance interval remains stable. No probe or UAT is created here;
project-owner approval is required.

## Unresolved questions

- Does an unmapped native travel, wheel-rotation, or surface-sampling state
  exist outside the controller-effect lineage?
- Is captured speed proportional to physical travel at one stable scale across
  cars, stages, and frame conditions?
- Can a longer uniform surface supply enough cycles for robust comparison?
- Do materials require separate intervals, or should Road 2.0 deliberately use
  one HYP36Rforce presentation policy?
- If no native interval exists, should Road remain envelope-only or should a
  synthetic rate become an explicit later presentation-design decision?

## Credits & Reference Context

GATS feedback remains experiential context. THP32 material remains **EXTERNAL
REFERENCE — NOT INTEGRATED**. No external code, constant, period, frequency,
wavelength, phase, effect parameter, or equation was used.

Any useful R5/AER discovery is a **CROSS-LANE CANDIDATE — NOT INTEGRATED** until
separately reviewed. R4.2F-T performs no arcade/PS2 force-submission trace.

## Recommended next step

Keep both `HYP36R_ROAD_PERIODIC_V1_SHADOW` models passive and active Road
paused. Do not select time timing as fallback. If approved, the next Road-only
step is the narrow passive probe above after a static trace identifies an exact
native candidate or confirms reconstructed travel is the only available axis.

**PART II — DEVELOPMENT JOURNEY RECAP**

### What were we trying to learn?

The passive prototype proved both time and distance phase can be safe. This
milestone asked whether OutRun tells us how far the car travels between repeated
Road sensations.

### What did the game state tell us?

The mapped vibration path has surface type, speed, event state, and short hold
counters. It has no recovered bump counter, wheel-travel phase, or surface
coordinate. Stable cobblestone is almost perfectly surface coefficient times
speed.

### What did the captures tell us?

Relative distance is a valid comparison axis, but it reveals no repeatable
spacing. Once speed is removed, cobblestone variation is numerical-scale and
does not generalize to other surfaces.

### Why is the negative result useful?

It prevents a guessed waveform rate from being presented as recovered OutRun
behavior. Distance timing remains the better concept, not an evidence-backed
product parameter. Active Road stays paused until a targeted passive test can
separate time behavior from travel behavior.
