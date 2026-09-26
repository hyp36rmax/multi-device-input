# HYP36R Force Research II — R2-B surface and Road analysis

## Decision

**R2-B COMPLETE — PROCEED TO R2-C.** No scenario requires repetition. B01 and
B02 contain less in-game sampled time than their wall-clock session duration,
but both retain clean 60 Hz frame spacing and enough accepted evidence for the
questions they answer.

The campaign establishes that OutRun preserves independent per-corner surface
classification through partial and broad occupancy. The current Road channel
does not preserve that four-corner identity. It uses the restored Xbox
vibration routine's already-combined right-motor result as the amplitude of a
synthetic 6 Hz carrier. Road is therefore **both magnitude-limited and
information-limited** in this evidence: its normalized amplitude is small,
and gain alone cannot restore the spatial information discarded before Road is
formed. This is an architecture result, not approval for a higher Road ceiling
or a new surface model.

## Evidence and method

The authoritative input is `R2_B_UAT.zip`. It contains one accepted attempt for
each expected scenario, all produced by commit
`f7c2030f7a7970a9a2b21aa9853615d952760e42` with telemetry schema
`HYP36R_RESEARCH_II_R1`. No retry, rejected, or cancelled attempt is present.

All 5,860 data rows were parsed at their declared 222-column width. Metrics use
the telemetry's in-game `elapsed_time`; session wall-clock duration is retained
separately. Absolute values are used for signed Road, Impact, Directional, and
output magnitude summaries. A signal is considered active when its magnitude
is greater than `1e-9`. Rolling RMS uses the measured in-game sample cadence.
The gain replay changes only `road_pre_gain` and recomposes the recorded
same-cycle signals.

## Attempt inventory and capture quality

| Scenario | Attempt / review | Session duration | In-game rows / span | Session rate | Median / P95 / max gap | Writes / malformed | Nonfinite | Result |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| B01 asphalt control | 1 / accepted | 20.00 s | 972 / 16.185 s | 60.01 Hz | 15.214 / 20.350 / 23.823 ms | 0 / 0 | 0 | Usable with caveat |
| B02 striped partial | 1 / accepted | 18.00 s | 580 / 9.649 s | 56.91 Hz | 15.178 / 20.351 / 23.669 ms | 0 / 0 | 0 | Usable with caveat |
| B03 striped full | 1 / accepted | 18.00 s | 1,080 / 17.980 s | 60.03 Hz | 15.194 / 20.290 / 23.903 ms | 0 / 0 | 0 | Clean |
| B04 rough/sand partial | 1 / accepted | 18.00 s | 1,080 / 17.980 s | 60.03 Hz | 15.222 / 20.270 / 23.875 ms | 0 / 0 | 0 | Clean |
| B05 rough/sand full | 1 / accepted | 18.00 s | 1,080 / 17.985 s | 60.01 Hz | 15.174 / 20.264 / 23.961 ms | 0 / 0 | 0 | Clean |
| B06 re-entry | 1 / accepted | 18.00 s | 1,068 / 17.785 s | 59.37 Hz | 15.199 / 20.321 / 23.925 ms | 0 / 0 | 0 | Clean |

The session-rate discrepancy in B02 is a wall-clock measure. Its CSV's 579
in-game intervals span 9.649 seconds, or 60.01 sampled frames/s, with no large
gap or malformed row. B01 has the same wall-clock versus sampled-time pattern.
This reduces coverage, not frame integrity. Expected blanks are the deliberately
unimplemented `xforce`, the first-row previous-surface cells, and one initial
response-rate cell in B02. No emitted numeric NaN or infinity was found.

## Reference+ reconstruction and configuration

Every accepted row reports `force2_active`, `REFERENCE_PLUS_EXPERIMENTAL`,
Presence `1.44`, Contrast `4`, Strength `1.00`, Steering Load `100`, Road Detail
`100`, Impact `100`, and inversion enabled. `presentation_profile=Reference`
names the fallback policy base; `s9_mode=REFERENCE_PLUS_EXPERIMENTAL` is the
authoritative final presentation selector. No DirectInput clamp was active.

Directional, Road, and Impact pre/post-gain pairs are identical on all 5,860
rows. Same-cycle reconstruction errors are:

| Relationship | Maximum absolute error | Rows above `1e-6` |
| --- | ---: | ---: |
| `directional + road + impact` → `composer_pre_tanh` | `1.00e-7` | 0 |
| `tanh(pre_tanh)` → `composer_post_tanh` | `1.01e-7` | 0 |
| `post_tanh * output_ramp` → `force_pre_drive` | `7.61e-8` | 0 |
| inversion × Strength → `directinput_unclamped_request` | `1.00e-7` | 0 |
| recorded `ffb_raw` versus pre-drive | `5.00e-7` | 0 |
| unclamped versus quantized `ffb_final` | `1.00e-4` | quantization only |

This validates the expected approximately `1e-7` floating-point precision and
the final DirectInput integer-resolution boundary.

## B01 — local asphalt control

All 972 frames are exactly `(2,2,2,2)`. There are zero surface transitions and
zero asymmetric frames. `field14` is also `(2,2,2,2)` throughout; `fieldE8` is
`(1,1,1,1)`. Road is exactly zero pre- and post-gain on every frame.

Restored vibration is not globally zero: it is active on 18.21% of frames,
with P95/max `0.210/0.210`, and six rises exceed the Impact trigger. Impact is
active through its decay tail on 67.80% of frames (P95/max
`0.04111/0.10442`). These events occur without any surface-classification
change, which is direct evidence that restored vibration and Impact are not
material-identity channels. Directional magnitude is P50/P95/max
`0.06861/0.13628/0.14465`. M4 stays in `normal`, BITE stays inactive, and
synthetic grip loss is zero.

Within this controlled local capture, raw value `2` is a **strong controlled
asphalt candidate**. R2-B does not establish that `2` means asphalt everywhere
in the game.

## `field14` relationship

Across six accepted captures, four corners, and every valid row:

| Comparisons | Matches | Mismatches | Maximum difference |
| ---: | ---: | ---: | ---: |
| 23,440 | 23,440 | 0 | 0 |

The instrumentation does not read the same address twice. `surface_0..3` come
from `EVWORK_CAR::water_flag_24C[0..3]` in `hooks_forcefeedback.cpp`.
`corner0..3_field14` come from `PhysicsContext::corners_258[i] + 0x14` through
`NativeFourCorner::observe()`. The code therefore proves two distinct runtime
locations and two independent observation paths, while the campaign proves
perfect copied/mirrored values. The defensible classification is **same
underlying source** at the game-state level, not a telemetry alias and not an
independently varying field. A future schema may remove one representation
after writer lineage confirms the game's copy direction. The present 222-column
schema remains unchanged.

## Raw surface-value inventory

Counts below are corner-samples. Approximate corner-seconds divide those counts
by the scenario's in-game frame rate. Every value appeared in all four channel
indices somewhere in R2-B, although sustained paired patterns were common.

| Raw value | Scenario corner-samples | Total / corner-seconds | Evidence and confidence |
| ---: | --- | ---: | --- |
| 2 | B01 3,888; B02 1,612; B03 3,286; B04 876; B05 2,479; B06 2,312 | 14,453 / 240.9 s | Controlled local asphalt state; **confirmed observed**, **supported** as local asphalt candidate only |
| 4 | B04 1,139; B05 1,273 | 2,412 / 40.2 s | Restricted to rough/sand campaigns and often broad; **supported** target-surface class, exact material ambiguous |
| 8 | B06 1,510 | 1,510 / 25.2 s | Sustained off-surface/re-entry state; **supported** campaign-context class, material unknown |
| 1024 | B02 158; B03 731; B06 144 | 1,033 / 17.2 s | Appears in striped transition/occupancy and re-entry; **ambiguous** boundary/contact classification |
| 2048 | B02 550; B03 201; B05 68; B06 306 | 1,125 / 18.8 s | Common transition/intermediate state across targets; **ambiguous** |
| 8192 | B03 102; B04 2,305; B05 500 | 2,907 / 48.4 s | Dominant in rough/sand partial and present elsewhere; **supported** rough/surface-context class, exact material unknown |

The target surfaces use multiple classes. No one-to-one global material table
is justified.

## Scenario findings

### B02 — partial striped runoff

The recorded baseline is `(2,2,2,2)` for 317 frames. Entry begins at 3.606 s
with channel 1, then 3, then 0 and 2. Sustained paired states repeatedly use
channels `(1,3)` or `(0,2)`: `(2,2048,2,2048)` lasts 1.670 s in the longest
pass, while `(2048,1024,2048,1024)` lasts 0.400 s and an all-`1024` state lasts
0.235 s. The return resolves through the same staged channel groups to the
baseline at 8.572 s. Spatial asymmetry occupies 221 frames, 38.10% or about
3.68 sampled seconds. There are 24 transition frames; each channel changes six
times.

Road is active on 23.79% of all frames and on 50.23% of asymmetric frames.
Restored vibration is active on 44.66%; transition-frame mean is `0.0680`.
None of the transition frames produces a rise above `0.12`; transition-frame
Impact averages only `0.000095` and peaks at `0.00141`. Directional output
continues independently (asymmetric-frame mean magnitude `0.04461`). M4 stays
`normal`, BITE stays inactive, synthetic grip loss remains zero, and M5 intent
activity is intermittent rather than a surface effect. `fieldE8` stays 1.0 at
all corners. EC/EE vary during the capture but also vary on B01, so they are
dynamic context rather than striped identity evidence.

### B03 — broader/full striped runoff

B03 includes a sustained all-corner `(1024,1024,1024,1024)` state for 2.450 s,
plus `(2048,1024,2048,1024)` for 0.915 s. Later passes use paired
`(8192,2,8192,2)`. It changes more corners into a common non-2 classification
than B02, but spends less total time asymmetric: 13.98% versus 38.10%.

Compared with B02, Road is active less often (9.81% versus 23.79%) and has a
lower P95 (`0.00210` versus `0.00300`) despite a larger isolated maximum
(`0.01068` versus `0.00462`). Restored vibration is active 19.63% versus
44.66%. Impact P95 is higher (`0.04111` versus `0.02997`) but that does not
follow occupancy monotonically. Synthetic grip loss remains zero, M4 remains
normal, and BITE remains inactive. Broader occupancy is proven by the surface
channels; stronger texture, Impact, or altered grip is not. Speed, steering,
and physical path differ, so output differences cannot be attributed to
occupancy alone.

### B04 — partial rough/sand

B04 is the clearest continuous-surface case. Asymmetry occupies 826 frames,
76.48%, or about 13.76 seconds. Long states include `(4,8192,4,8192)`,
`(8192,4,8192,4)`, and paired `8192/2`, with shorter all-`8192` intervals.
Road is active on 86.57% of all frames and 95.40% of asymmetric frames.
Restored vibration is active on 87.50%, with asymmetric-frame mean `0.07847`.

No vibration rise exceeds `0.12`. Impact is active above numerical zero on
only 5.56%, has P95 `1e-7`, and peaks at `0.0000258`. M4 remains normal, BITE
inactive, and synthetic grip loss zero. Continuous Road and restored vibration
therefore coexist with effectively no Impact. This strongly separates
sustained surface activity from the transient Impact detector.

### B05 — broader/full rough/sand

B05 adds sustained all-corner value `4` intervals of 2.550 s and 1.000 s,
alongside partial `(8192,4,8192,4)` occupancy. Non-2 state occupies 493 frames;
asymmetry occupies 272 frames, 25.19%, or about 4.53 seconds. Road is active on
42.31% overall and 92.70% while any corner is non-2. Its P95/max
`0.00880/0.00958` exceed B04's `0.00394/0.00679`. Restored vibration is active
45.65%, P95/max `0.22995/0.525`.

Impact is materially larger than B04: P95/max `0.11229/0.21565`, with 12
vibration rises above `0.12`. The increase cannot be labeled “full rough
texture”: B04 proves rough occupancy can persist without Impact, while B05's
transients and different vehicle dynamics activate the rise detector.
Directional magnitude is also higher (P95 `0.19636` versus `0.06891`). M4
remains normal, BITE inactive, and synthetic grip loss zero. Occupancy and
driving dynamics are therefore confounded in the force difference.

### B06 — surface re-entry

B06 begins with all corners at `8` for 5.465 s, passes through staged
`8/2048`, all-`2048`, paired `2048/2`, and reaches all-`2` at 6.738 s. A second
sequence begins at 9.238 s, includes paired `1024/8` and `1024/2`, and reaches
all-`2` at 11.803 s. Surface asymmetry occupies 19.57%, about 3.48 seconds.

The raw surface state changes first. Restored vibration and Road may rise or
remain sustained during intermediate states; they do not uniquely mark the
boundary. On the second approach, vibration rises to `0.197` at 10.018 s while
the surface is still mixed, but the largest re-entry Impact (`0.08790`) occurs
at 11.788 s, immediately before all-`2` at 11.803 s. The native M4 phase remains
`normal` through the actual surface transitions. A separate LOAD sequence does
not begin until 16.618 s—about 4.8 seconds after the second all-2 state—and
runs emerging → established → recovering → normal. BITE never activates.
This ordering separates surface identity transition from later vehicle-load
response. M5 intent is contextual/intermittent and not a surface-transition
marker. Synthetic grip loss is effectively zero except tiny later all-2
values.

`fieldE8` tracks a related contact/context state: it is all `0.7` during the
initial all-8 interval, mixed during some partial states, and returns to all
1.0 before or around the final all-2 state. It is not a pure raw-class alias.
EC/EE continue varying after surfaces are stable and peak during dynamic motion,
supporting dynamic/vehicle-state correlation rather than identity.

## Spatial ordering status

R2-B repeatedly confirms side grouping: channels 0 and 2 change together, and
channels 1 and 3 change together. That is independent partial-occupancy
evidence across striped, rough/sand, and re-entry scenarios. It is consistent
with the project's earlier controlled mapping of `0=FL, 1=FR, 2=RL, 3=RR` and
therefore preserves the established ordering. R2-B alone does not add a new
handedness proof because the session notes do not state which physical side
was placed on the target. Neutral channel names remain appropriate in the raw
schema.

## Current Road pipeline and information loss

```text
four EVWORK_CAR surface classifications + other native car state
    ↓ CalcVibrationValues() restored Xbox effect routine
two composite gamepad motor levels (left and right)
    ↓ HYP36R uses only clamped right motor for Road amplitude
right × WheelFFBRoadStrength × 0.25 × synthetic 6 Hz sine
    ↓ suppressed on vibrationRise > 0.12
Road pre-gain
    ↓ Road Detail percentage
Road post-gain
    ↓ directional + Road + Impact → tanh → output ramp → drive()
```

Independent spatial identity ceases at `CalcVibrationValues()`: four surface
inputs and other native state become two composite motor levels. HYP36R then
collapses further by using only the right motor for Road. The surviving signal
is a scalar composite envelope; the 6 Hz oscillation is synthetic. Telemetry
shows Road can respond during sustained non-2 occupancy and transitions, but
raw identity is neither necessary nor sufficient for a particular Road value.
This is not automatically a defect—the restored effect may encode useful game
intent—but it cannot preserve four-corner identity.

## Road magnitude

| Scenario | P50 | P95 | P99 | Max | 1 s RMS P95 / max | 3 s RMS P95 / max | Active |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| B01 | 0 | 0 | 0 | 0 | 0 / 0 | 0 / 0 | 0% |
| B02 | 0 | .003005 | .004259 | .004619 | .002530 / .002761 | .001893 / .001933 | 23.79% |
| B03 | 0 | .002102 | .003886 | .010675 | .002213 / .002532 | .001551 / .001759 | 9.81% |
| B04 | .001654 | .003936 | .005174 | .006787 | .003357 / .003500 | .002992 / .003148 | 86.57% |
| B05 | 0 | .008801 | .009485 | .009577 | .006911 / .007120 | .006422 / .006738 | 42.31% |
| B06 | 0 | .006967 | .010971 | .014233 | .005968 / .006203 | .004003 / .004093 | 46.63% |

These are software-normalized values, not wheel torque or perceptual thresholds.

## Interim Road-only gain replay

Steering and Impact remain `1.00x`. Results are absolute pre-drive output over
all accepted R2-B rows. No value is a production recommendation.

| Road gain | P95 / P99 / max | 1 s RMS P95 / max | 3 s RMS P95 / max | >=.75 / .90 / .98 | Min headroom | Slew P99 / max | Sign changes vs 1x |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1.00 | .15780 / .23285 / .50244 | .16334 / .28472 | .14093 / .17968 | 0 / 0 / 0% | .49756 | 1.8538 / 12.9191 | 0 |
| 1.10 | .15781 / .23291 / .50244 | .16339 / .28472 | .14093 / .17968 | 0 / 0 / 0% | .49756 | 1.8908 / 12.9011 | 8 |
| 1.20 | .15783 / .23281 / .50244 | .16343 / .28473 | .14093 / .17968 | 0 / 0 / 0% | .49756 | 1.9030 / 12.8830 | 11 |
| 1.30 | .15813 / .23219 / .50244 | .16348 / .28473 | .14093 / .17968 | 0 / 0 / 0% | .49756 | 1.9030 / 12.8649 | 13 |
| 1.40 | .15828 / .23153 / .50244 | .16354 / .28474 | .14093 / .17969 | 0 / 0 / 0% | .49756 | 1.9072 / 12.8469 | 14 |
| 1.50 | .15840 / .23115 / .50244 | .16359 / .28474 | .14093 / .17969 | 0 / 0 / 0% | .49756 | 1.9136 / 12.8288 | 20 |
| 1.60 | .15852 / .23129 / .50244 | .16364 / .28475 | .14093 / .17969 | 0 / 0 / 0% | .49756 | 1.9136 / 12.8108 | 25 |
| 1.75 | .15865 / .23120 / .50244 | .16372 / .28476 | .14093 / .17970 | 0 / 0 / 0% | .49756 | 1.9494 / 12.7837 | 34 |
| 2.00 | .15828 / .23358 / .50244 | .16379 / .28479 | .14094 / .17971 | 0 / 0 / 0% | .49756 | 2.0460 / 12.7385 | 53 |

All outputs are finite. No DirectInput/software clamp, `|pre-tanh| > 1`, or
high-occupancy exposure occurs. The apparent sign changes are only mixed-output
crossings around cancellation: even at 2.00x the largest old/new magnitudes on
those 53 frames are `0.00839/0.00827`. Positive Road gain does not reverse the
Directional channel. Global output statistics barely move because Road is
sparse and small relative to Directional/Impact; local surface perceptibility
may still change. This sweep establishes headroom only for these recordings,
not a final ceiling or hardware safety.

## Gain versus information

Classification: **BOTH**.

- Magnitude-limited: Road P95 is at most `0.00880` by scenario, and even a 2x
  replay barely changes campaign-wide output exposure.
- Information-limited: partial scenarios retain four spatial classifications,
  but current Road receives only one composite motor envelope and cannot tell
  which corner or how many corners caused it.
- Not gain-only: B02/B03 broader occupancy does not monotonically increase Road,
  while B04 partial rough occupancy produces highly persistent Road.
- Not information-only: current scalar Road clearly carries sustained surface
  activity and can be amplified without hitting the observed software envelope.

## Signal evidence matrix

| Category | Strongest useful signals | R2-B result | Boundary |
| --- | --- | --- | --- |
| Surface identity | `surface_0..3` / mirrored `field14` | Discrete, spatially independent states and repeatable partial/broad combinations | Human material names remain incomplete |
| Continuous surface activity / texture | restored motor levels, Road, sustained raw occupancy; `fieldE8` as context | B04 sustains vibration and Road with almost no Impact | Restored effect is composite; Road carrier is synthetic |
| Physical transient / event | `vibration_rise`, Impact decay, output slew | Impacts can occur on stable value 2 and are absent during sustained B04 rough occupancy | R2-C must control actual collisions/curbs/shifts |
| Grip / vehicle response | native M4 phases, BITE, directional response, four-corner dynamic fields; synthetic slip only as secondary context | B06 surface transition precedes a later LOAD sequence; surface state does not itself imply grip response | No tire load, SAT, or grip percentage is established |

## Restored vibration and Impact

Restored vibration is related to surface occupancy but not identical to it. It
is active during B02/B03 striped and B04/B05 rough states, persists through
some transitions, and also appears on B01 with an unchanged `(2,2,2,2)` state.
It combines multiple native inputs into two motor levels, so it is evidence of
the restored game's effect intent rather than raw physical truth.

Impact is a positive-rise detector over `max(left,right)`, followed by a decay.
B04 proves partial rough occupancy can remain active with no trigger rise above
`0.12` and a maximum Impact of only `0.0000258`. B05 contains larger Impact,
but also larger vibration rises and different dynamics. B06 shows some boundary
transitions with zero Impact and one re-entry transient with a large kick.
Impact therefore aligns better with transient effect changes than with material
identity. R2-B informs but does not replace the controlled R2-C campaign.

## `fieldE8`, `fieldEC`, and `fieldEE`

| Field | Finding | Classification |
| --- | --- | --- |
| E8 | Constant 1.0 in B01–B03; toggles between 1.0 and 0.7 in B04–B06. It often follows per-corner value 4 or 8, but value 8192 can have either level and timing can lag raw transitions. | Surface/contact-context correlation candidate; not a pure identity alias |
| EC | Hundreds of signed values even on stable B01; magnitude and sign vary with driving. | Dynamic/vehicle-state correlation candidate; no useful new identity evidence |
| EE | Same broad behavior as EC, with still larger excursions in B05/B06 and continued variation after stable surfaces. | Dynamic/vehicle-state correlation candidate; no useful new identity evidence |

Compared with R2-A, E8 gains useful controlled surface context; EC/EE remain
dynamic candidates. No physical semantics are assigned.

## @GATS feedback assessment

The independent campaign supports the concern without treating it as proof.
OutRun retains spatial information during partial occupancy: B02 and B04 hold
different values on paired corner channels for seconds. Current HYP36R Road
does not preserve all of it because the four inputs are reduced to restored
motor levels and Road uses one scalar motor channel. Road magnitude is also
small in the controlled captures. The reported weak one-side edge/sand feel is
therefore plausibly consistent with **both low Road magnitude and lost spatial
information**. Wheel/driver presentation and subjective threshold remain
uncontrolled.

## HYP36R Force 2.0 surface milestone assessment

| Goal | Status | Reason |
| --- | --- | --- |
| 1. Independently understood spatial/per-corner surface behavior | Understanding established | Four independent channels, side grouping, partial/broad occupancy, discrete classes, and field14 mirroring are demonstrated; global material names remain open |
| 2. Materially improved Road Detail based on those signals | Not started | No Force or Road behavior changed in R2-B |
| 6. Meaningful separation of texture, physical events, and grip behavior | Ready for implementation research | B04 separates continuous texture from Impact; B06 separates surface transition from later LOAD; controlled transient work is still needed |

## Completeness and next step

**R2-B COMPLETE — PROCEED TO R2-C.** No targeted repeat is required. B01/B02
coverage caveats do not block their conclusions. The next research milestone
should be **R2-C controlled transients** to isolate collisions, curbs, shifts,
and boundary events from sustained surface activity. Do not change Road or
select a production ceiling before that evidence exists.

This milestone changes documentation only. HYP36R Force, Reference+, Road,
Impact, Steering Load, M4, M5, gear behavior, DirectInput, controller behavior,
and the 222-column schema are unchanged.
