# HYP36R Force Research II — R2-C transient and Impact analysis

**R2-C COMPLETE — PHYSICAL R2 COMPLETE.** Both returned captures are accepted.
They are sufficient for the later integrated Impact study; no physical repeat is
required. This record changes no Force behavior and does not select a production
Impact ceiling.

## Evidence and method

The authoritative inputs are the two CSV/session pairs returned in
`R2_C_UAT.zip`, produced by commit
`a947b49b80ce688b2f02452408c04adf66552fd0` with the 222-column
`HYP36R_RESEARCH_II_R1` schema. The session records mark both attempt 1 captures
accepted.

Replay reconstructed each same-cycle output as:

```text
directional_post_gain + road_post_gain + impact_post_gain
    -> tanh
    -> output_ramp
    -> force_pre_drive
    -> inversion
    -> master Strength
    -> DirectInput nominal clamp/request
```

The Impact-only sweep holds Steering Load and Road Detail at `1.00x`, changes
only `impact_pre_gain`, then re-runs composition and `tanh`. Event windows extend
0.75 seconds from each canonical C01 gear transition or each high-confidence C02
physical-impact candidate. Rolling RMS is calculated on the original continuous
timeline, not across artificially joined windows.

## Capture quality

| Capture | Duration | Samples | Session rate | Median / P95 / max gap | Writes | Malformed | Nonfinite | Result |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| C01 Gear Shifts | 25.00 s | 1,500 | 60.03 Hz | 15.225 / 20.372 / 24.294 ms | 0 | 0 | 0 unexpected | **CLEAN** |
| C02 Controlled Impact | 20.00 s | 1,200 | 60.01 Hz | 15.235 / 20.378 / 23.897 ms | 0 | 0 | 0 unexpected | **CLEAN** |

The recomputed `(samples - 1) / elapsed span` rate is 60.00 Hz for both files.
`xforce` is intentionally blank on every row and the four `surface*_previous`
fields are intentionally blank on the first row; these are schema states, not
NaN/Inf failures. All populated numeric observations are finite.

## Configuration and reconstruction

Both captures remain constant at Reference+ (`s9_mode =
REFERENCE_PLUS_EXPERIMENTAL`), Presence `1.44`, Contrast `4`, Strength `100%`,
Steering Load `100%`, Road Detail `100%`, Impact `100%`, output ramp `1.0`, and
Invert enabled. The internal presentation label remains `Reference`, as expected
for Reference+'s Reference foundation plus the S9 selector.

| Reconstruction stage | C01 max / mean absolute error | C02 max / mean absolute error |
|---|---:|---:|
| Composition | 1.00e-7 / 1.49e-8 | 1.00e-7 / 2.33e-8 |
| `tanh` | 1.43e-7 / 3.55e-8 | 1.66e-7 / 3.87e-8 |
| Pre-drive | 1.43e-7 / 3.55e-8 | 1.66e-7 / 3.87e-8 |
| Unclamped DirectInput request | 1.43e-7 / 3.54e-8 | 1.66e-7 / 3.88e-8 |
| Quantized final output, magnitude | 1.00e-4 / 4.81e-5 | 9.99e-5 / 5.01e-5 |

The last difference is the expected DirectInput/telemetry quantization. No
clamp occurred.

## C01 gear-shift inventory

All six expected transitions are present. `Impact` below is both pre- and
post-gain because Impact was 100%.

| Frame | Time | Gear | Left | Right | Combined / rise | Impact | Road | Directional | Pre-drive | Final output |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 312 | 5.204 s | 1→2 | .14000 | 0 | .14000 | +.07735 | 0 | -.00679 | +.07044 | -.0704 |
| 419 | 6.988 s | 2→3 | .14000 | 0 | .14000 | +.07735 | 0 | +.02400 | +.10101 | -.1010 |
| 534 | 8.903 s | 3→4 | .14000 | 0 | .14000 | +.07735 | 0 | +.03604 | +.11291 | -.1129 |
| 879 | 14.653 s | 4→5 | .14000 | 0 | .14000 | +.07735 | 0 | +.05401 | +.13061 | -.1306 |
| 1,158 | 19.303 s | 5→4 | .14000 | 0 | .14000 | -.07735 | 0 | -.15497 | -.22823 | +.2282 |
| 1,430 | 23.838 s | 4→3 | .14000 | 0 | .14000 | -.07735 | 0 | -.03770 | -.11454 | +.1145 |

### Canonical shift signature

For left, right, combined, and rise, the means/medians/ranges are respectively
`.14/.14/.14–.14`, `0/0/0–0`, `.14/.14/.14–.14`, and
`.14/.14/.14–.14`; every variance is zero at recorded precision. Absolute
Impact is `.07735` for every shift, also with zero variance. Upshift and
downshift **magnitude does not differ**. The observed sign differs, but code
shows the sign is selected from steering direction, alternating near center;
it does not encode upshift versus downshift.

After a trigger, stored Impact is multiplied by `0.90` per update. Across both
captures the median observed eligible decay ratio is exactly `.90` (minor
CSV-rounding range approximately `.896–.904`). At 60 Hz this has a roughly
110 ms half-life, reaches 10% in about 0.37 s, and 1% in about 0.73 s unless a
new qualifying rise replaces it.

C01 is a clean canonical **transition-row** reference: all four surfaces remain
raw `2`, there are no surface transitions, Road is zero on all 1,500 rows, and
no bilateral collision signature occurs. C01 is not an assertion that every
non-transition row is event-free. Five additional left-only rises above `.12`
occur, including follow-on/retrigger activity 6–13 frames after several shifts
and one early `.21` event. They have no right-motor or surface corroboration and
are not classified as physical collisions.

## C02 event inventory

C02 contains seven ordinary gear transitions. Each again has the exact
`.14 left / 0 right / .14 rise / .07735 absolute Impact` signature.

Four rows are high-confidence physical-impact candidates because they are not
gear transitions, are bilateral, exceed the canonical shift pattern, and occur
in the controlled-impact scenario:

| Frame | Time | Left | Right | Rise | Impact | Road | Directional | Pre-drive | Final | Surface tuple | Context |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---|
| 385 | 6.426 s | .52500 | .07530 | .29866 | -.19443 | 0 | -.01618 | -.20755 | +.2075 | 8/8/8/8 | strongest Impact; surface change 20 frames earlier |
| 796 | 13.276 s | .28477 | .09492 | .14851 | -.09977 | 0 | -.14819 | -.24300 | +.2430 | 2048/8/2048/2048 | coincident surface transition |
| 843 | 14.062 s | .52500 | .09420 | .24180 | -.17041 | 0 | -.19057 | -.34608 | +.3460 | 8/8/8/8 | strong bilateral event |
| 856 | 14.276 s | .52500 | .08688 | .26390 | +.17975 | 0 | +.03571 | +.21218 | -.2121 | 8/8/8/8 | second rise 0.214 s later |

These verify the preliminary ranges: left reaches `.525`, right later reaches
`.350` during the event/decay sequence, rise reaches `.298658`, absolute Impact
reaches `.194433`, and capture-wide pre-drive reaches `.373016`. They do not
establish global game maxima.

Frame 135 is **ambiguous**, not counted as a controlled physical collision: it
is a non-gear `.16258` left-only rise on stable `2/2/2/2`, 0.216 s after a
canonical shift, with Impact `.08983`. The seven gear rows, four physical-event
onsets, their decay tails, 30 surface-transition frames, and this ambiguous
left-only event are kept separate.

## Gear versus physical collision

**CLEARLY DISTINGUISHABLE in this controlled dataset**, with an important
generalization boundary.

| Evidence | Gear shift | Physical-impact candidates |
|---|---|---|
| Explicit gear flag | Always true at canonical onset | False |
| Left/right pattern | `.14 / 0` exactly | `.285–.525 / .075–.095` at onset |
| Rise | `.14` exactly | `.149–.299` |
| Absolute Impact onset | `.07735` exactly | `.09977–.19443` |
| Surface context | stable `2/2/2/2` in C01 | mixed; one onset coincides with change |
| Decay | shared synthetic 0.90/update envelope | same shared envelope after onset |

The explicit gear transition is the strongest discriminator. Bilateral pattern
and amplitude corroborate physical contact here, but should not be promoted to
a universal collision classifier from four events. Decay alone cannot identify
the source because both events enter the same HYP36R envelope.

## Current Impact path and event identity

```text
native PC car/game state
    -> restored Xbox CalcVibrationValues()
       (gear mismatch is still explicit here)
    -> VibrationLeftMotor / VibrationRightMotor
       (source meaning partly retained by channel pattern)
    -> vibration = max(left, right)
       (left/right identity collapses)
    -> positive rise detector, threshold > .12
    -> steering-selected direction
    -> 0.65 * min(.55, rise*.65 + vibration*.20)
    -> one scalar impactForce, decayed .90/update
       (event-source identity is gone)
    -> Impact Force Character gain
    -> directional + Road + Impact
    -> tanh -> output ramp -> drive()
    -> inversion -> master Strength -> DirectInput request/clamp
```

Classification: **IDENTITY AVAILABLE UPSTREAM BUT COLLAPSED IN HYP36R**.
Telemetry can still observe gear transition and both motor channels beside the
scalar Impact, so research can distinguish the controlled events offline. The
active Impact carrier itself cannot say why it fired. Identity is partly
preserved through restored vibration, lost at `max(left,right)` and the common
rise/envelope path, and not recovered later.

## Impact magnitude

Absolute Impact metrics use the unscaled `impact_pre_gain` carrier.

| Case | P50 | P95 | P99 | Max | 1 s RMS P95 / max | 3 s RMS P95 / max | Active frames |
|---|---:|---:|---:|---:|---:|---:|---:|
| C01 full capture | .000001 | .04488 | .07735 | .12871 | .03471 / .04110 | .02514 / .02625 | 48.67% |
| C01 0.75 s shift windows | .01627 | .07735 | .09943 | .12871 | .04102 / .04109 | .02614 / .02624 | 100% |
| C02 full capture | .000212 | .06265 | .12426 | .19443 | .05491 / .06934 | .04481 / .04488 | 77.67% |
| C02 0.75 s physical-event windows | .01849 | .14405 | .17746 | .19443 | .06897 / .06934 | .04488 / .04488 | 100% |

“Active” means absolute Impact above `1e-6`; the long decay and retriggers make
it an exposure measure, not an event count. The C01 window maximum above the
canonical `.07735` comes from left-only retriggers after shifts and is why the
canonical transition-row signature is reported separately.

## Interim Impact-only gain sweep

### C01 gear-shift windows

| Impact gain | P95 | P99 | Max | 1 s RMS P95 / max | 3 s RMS P95 / max | Min headroom | Max slew/s | Sign crossovers vs 1x |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 1.00 | .143 | .201 | .228 | .155/.158 | .133/.134 | .772 | 8.03 | 0 |
| 1.10 | .151 | .207 | .236 | .156/.160 | .134/.134 | .764 | 8.77 | 2 |
| 1.20 | .158 | .213 | .243 | .157/.161 | .134/.135 | .757 | 9.51 | 2 |
| 1.30 | .164 | .220 | .250 | .158/.162 | .135/.135 | .750 | 10.26 | 3 |
| 1.40 | .171 | .226 | .257 | .160/.164 | .135/.136 | .743 | 11.00 | 5 |
| 1.50 | .178 | .232 | .265 | .161/.165 | .136/.137 | .735 | 11.74 | 7 |
| 1.60 | .186 | .239 | .272 | .163/.167 | .137/.137 | .728 | 12.47 | 7 |
| 1.75 | .195 | .254 | .282 | .165/.169 | .138/.138 | .718 | 13.58 | 10 |
| 2.00 | .211 | .275 | .300 | .169/.173 | .139/.140 | .700 | 15.40 | 13 |

### C02 physical-impact windows

| Impact gain | P95 | P99 | Max | 1 s RMS P95 / max | 3 s RMS P95 / max | Min headroom | Max slew/s | Sign crossovers vs 1x |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 1.00 | .246 | .347 | .373 | .206/.210 | .176/.176 | .627 | 16.19 | 0 |
| 1.10 | .256 | .362 | .387 | .211/.214 | .179/.179 | .613 | 17.74 | 0 |
| 1.20 | .267 | .376 | .401 | .216/.220 | .182/.182 | .599 | 19.28 | 0 |
| 1.30 | .277 | .389 | .414 | .221/.225 | .185/.185 | .586 | 20.81 | 0 |
| 1.40 | .287 | .403 | .427 | .225/.230 | .188/.188 | .573 | 22.33 | 0 |
| 1.50 | .301 | .417 | .440 | .230/.235 | .190/.191 | .560 | 23.84 | 0 |
| 1.60 | .315 | .430 | .453 | .235/.240 | .193/.194 | .547 | 25.33 | 0 |
| 1.75 | .340 | .449 | .472 | .243/.248 | .198/.198 | .528 | 27.55 | 0 |
| 2.00 | .374 | .481 | .503 | .255/.260 | .205/.206 | .497 | 31.18 | 0 |

Every replay is finite. Occupancy at `.75`, `.90`, and `.98`, DirectInput clamp,
and pre-`tanh` magnitude above unity are all zero at every tested gain. The
Impact channel itself never reverses sign. The C01 “crossovers” are low-level
net-composition sign changes where directional and Impact oppose and the
larger Impact wins; they are not an Impact-sign defect.

By software headroom alone, `1.00–1.60x` is **clearly comfortable** in these
captures. `1.75–2.00x` remains unclamped and below `.504`, but is
**headroom-aware** because it is farther extrapolation from physical evidence,
raises transient slew, and changes a small number of C01 cancellation outcomes.
No tested region reaches compression/clamp. This is evidence for the integrated
study, not a ceiling selection.

## @GATS gear-shift feedback

The Simucube 3 Pro report is compatible with the data: every canonical shift
injects the same exact restored left-motor pulse and therefore a repeatable
`.07735` Impact onset. It is not random noise. Controlled physical impacts are
typically larger and bilateral, reaching 2.51 times the canonical shift Impact
at onset (`.194433 / .07735`). Gear is technically separable upstream through
the explicit gear flag and exact channel signature, even though current HYP36R
feeds both into the same Impact carrier. A future 2.0 design could therefore
evaluate event-specific presentation. One hardware preference report is not a
basis for changing the current shift.

## Road and surface contamination

C01 is the clean control: one surface tuple (`2/2/2/2`), no changes, and Road
zero throughout. C02 contains 24 surface tuples and 30 transition frames. Its
most common states are `2/2/2/2` (967 rows), `8/8/8/8` (115), and
`2048/8/2048/8` (27). Road is nonzero on 207 rows and reaches `.04337`.

Road is zero on every listed impact onset because HYP36R suppresses Road when
rise exceeds `.12`. It can resume on following decay frames while Impact is
still active, so event tails must not be interpreted as Impact-only. Frame 796
coincides with a surface transition; the other three high-confidence physical
onsets do not. Surface state/context and Impact onset are therefore reported as
parallel evidence, not collapsed into a collision claim.

## Surface, event, and grip separation

| Meaning | Independent evidence now available | Current presentation boundary |
|---|---|---|
| Surface identity | four raw per-corner surface indices | R2-B showed identity is collapsed before scalar Road |
| Continuous surface activity | right motor, Road, per-corner context | Road is a synthetic 6 Hz scalar carrier |
| Surface transition | four explicit change flags | can coincide with, but is not itself proof of, collision |
| Gear transient | explicit gear mismatch plus exact left-only signature | collapsed into common scalar Impact |
| Physical collision transient | controlled bilateral rise/amplitude pattern, no gear transition | collapsed into common scalar Impact |
| Grip/load response | synchronized native vehicle/four-corner and M4/M5 state | independent directional architecture; not Road or Impact identity |

R2-B and R2-C jointly show that the telemetry can observe these meanings
independently, while current Road and Impact intentionally present composites.
That is a strong architectural basis for later research, not permission to
infer semantic labels from amplitude alone.

## Decisions

- **Impact ceiling readiness: SUFFICIENT WITH CAVEAT.** The captures contain an
  exact six-event gear reference and four high-confidence physical candidates
  with useful amplitude separation. The integrated study must retain surface
  overlap and the small physical-event sample as uncertainty; no repeat is
  required before that study.
- **Physical R2: COMPLETE.** R2-A Vehicle/Force, R2-B Surface/Road, and R2-C
  Transients/Impact now form the controlled physical evidence set. Request no
  more driving unless integrated analysis names one specific missing case.
- **2.0 transient milestone:** R2-C establishes that surface activity, gear
  events, physical impacts, and grip/load response have distinguishable
  upstream evidence even though current presentation collapses several of them.
  This supports later event-aware Impact and separated haptics/pedal/motion
  signal architecture. None is implemented here.

## Next recommendation

Stop physical capture and proceed, only when explicitly requested, to one
integrated R2-A/B/C analysis. That work should combine the separate Steering,
Road, and Impact replay envelopes, preserve the uncertainty boundaries above,
and only then recommend player-facing ceilings. Do not begin 2.0 Force changes
from this document alone.
