# HYP36R Force 2.0 R4.2C — formal cobblestone evidence analysis

R4.2C closes the passive cobblestone evidence gate. The accepted physical UAT
was recorded by commit `23fa792f7d85d442eac1bc9be4335bb19f5d7ee1`
with `HYP36R_RESEARCH_II_R1`, Reference+, Presence 1.44, Contrast 4, all four
player Force controls at 100%, and Invert enabled.

## Decision

**Hypothesis: SUPPORTED.** Stable controlled cobblestone occupancy contains
continuous bilateral native activity that is absent from the local normal-road
surface context. Candidate A preserves it without producing false Road activity
on either normal-road control.

**Repeat: NO REPEAT REQUIRED.** The 438-sample, approximately 7.3-second stable
window is technically clean and represents the available physical length of the
in-game section rather than an aborted or missed test.

**Gate: ACTIVE ROAD PROTOTYPE APPROVED WITH CAVEAT.** The first prototype may
use only a strictly bounded, symmetric presentation of Candidate A's continuous
envelope at conservative authority. This evidence does not authorize physical
handedness, spatial torque, transition kicks, material-specific gain, a 2.00x
v1-derived ceiling, E8/EC/EE, or event-aware Impact.

The caveat is temporal: the captured envelope varies meaningfully, but within
this short window its magnitude is effectively proportional to vehicle speed.
It does not establish an independent physical bump frequency. That does not
invalidate the continuous Road carrier.

## Capture integrity

All three attempt-1 captures were runner-accepted. Each has 222 columns, zero
malformed rows, zero nonfinite values, zero write failures, and the same build,
schema, Force profile, configuration and inversion identity. Blank `xforce` is
the documented unavailable source. The first-frame blanks in response rate and
previous-surface fields are expected initialization, not malformed data.

| Capture | Samples | Session duration | CSV timestamp span | Row rate | Median / P95 / max gap | Result |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| CST01 normal control | 826 | 15.00 s | 13.751 s | 59.998 Hz | 15.18 / 20.35 / 23.01 ms | CLEAN |
| CST02 cobblestone | 868 | 15.00 s | 14.446 s | 60.017 Hz | 15.26 / 20.31 / 24.03 ms | CLEAN |
| CST03 normal return | 848 | 15.00 s | 14.120 s | 59.987 Hz | 15.27 / 20.20 / 23.90 ms | USABLE WITH CAVEAT |

The session rate uses the complete runner interval while the row rate uses the
first-to-last CSV timestamp; neither indicates dropped or stalled recording.
CST03 is a valid surface false-positive control but is faster and contains four
ordinary gear transitions, so it is not a speed-matched native-event control.
CST01 is the closer speed control.

## Surface context and controls

CST01 is uniformly `(2,2,2,2)` for all 826 frames. CST03 is uniformly
`(2,2,2,2)` for all 848 frames. Neither contains a surface transition. Both
produce zero Candidate A and zero v1 Road despite vehicle Directional activity,
native event-channel activity, and CST03 gear changes. They therefore provide
suitable local controls for the Road-context question.

CST02 contains:

| Four-channel surface tuple | Frames |
| --- | ---: |
| `(1048576,1048576,1048576,1048576)` | 438 |
| `(2,2,2,2)` | 416 |
| transitional mixed tuples | 14 |

The longest stable `1048576` run is rows 0–437, elapsed time 0.538–7.817 s:
438 consecutive samples, 7.280 s from timestamp endpoints or approximately
7.298 s at the measured inclusive row rate. It has zero surface-change frames.

Raw value `1048576` is therefore **CONFIRMED IN CONTROLLED CONTEXT** as the
uniform four-channel state of this specific cobblestone section. This is not a
claim that the value universally means cobblestone elsewhere in the game.

## Native activity

Absolute native activity statistics are shown below. Coverage uses the existing
`1e-6` policy threshold.

| Window / channel | Active | P50 | P95 | P99 | Max | RMS |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| CST01 left, full capture | 16.46% | 0 | .21000 | .21000 | .21000 | .08521 |
| CST01 right, full capture | 0% | 0 | 0 | 0 | 0 | 0 |
| Stable cobble left | 100% | .03792 | .05702 | .05782 | .05788 | .03995 |
| Stable cobble right | 100% | .00948 | .01425 | .01446 | .01447 | .00999 |
| Stable cobble combined RMS envelope | 100% | .02764 | .04156 | .04214 | .04219 | .02912 |
| CST03 left, full capture | 14.62% | 0 | .14000 | .14000 | .18058 | .05388 |
| CST03 right, full capture | 0% | 0 | 0 | 0 | 0 | 0 |

The normal captures' isolated left-channel values occur while the surface is
the established reference and are therefore correctly rejected as Road by the
policy. In the stable cobblestone window both channels remain active on every
frame. Their median magnitude ratio is 4.0:1 and remains within approximately
4.0000 across the P05–P95 range. This is sustained bilateral evidence, not an
entry/exit transient.

Rise is nonzero on 173 of 438 stable frames (39.50%), with P95 `.0003403` and
one maximum of `.0210975`; it never reaches the `.12` event-class threshold.
Four surface-change frames exist in the complete CST02 capture, but none falls
inside the stable interval.

## Temporal structure

The stable combined envelope has mean `.02766`, standard deviation `.00911`
and coefficient of variation `.329`. A direct spectrum's strongest low-frequency
component is approximately `.60 Hz`, but the envelope-to-speed correlation is
`0.9999999999`. Removing a linear speed relationship leaves only about
`1.1e-7` residual standard deviation. The native L/R channels carry the same
speed-following shape and fixed ratio.

The safe conclusion is therefore:

- activity is continuous and changes over time;
- its captured magnitude is predominantly speed-related;
- the data does not resolve a separate repeating bump frequency;
- no physical frequency or bump-spacing claim is made.

The owner's audible repeating thump and faint tactile feel are consistent with
the presence of a continuous surface-conditioned carrier, but remain subjective
context rather than proof of periodic structure.

## Candidate A and v1 Road

Candidate A is reconstructed exactly as the direct bounded RMS of native L/R
when Road2Policy has non-reference surface context and no excluded event.

| Window | Candidate A coverage | P50 | P95 | P99 | Max | RMS |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| CST01 normal control | 0% | 0 | 0 | 0 | 0 | 0 |
| Stable cobblestone | 100% | .02764 | .04156 | .04214 | .04219 | .02912 |
| CST03 normal return | 0% | 0 | 0 | 0 | 0 | 0 |

Within stable cobblestone, v1 Road is also active on every frame: P50
`.0006886`, P95 `.0015872`, P99 `.0017505`, maximum `.0017943`, RMS
`.0008832`. Both carriers identify the same continuous context. Candidate A
additionally preserves independently bounded bilateral native magnitudes and
native-channel contrast before any later symmetric presentation. The v1 Road
column is a scalar carrier and cannot retain those dimensions.

This comparison is informational only. Different numeric scales do not imply
a gain recommendation or a physical feel judgment.

## Event and Force isolation

The stable 438-frame window contains:

- zero surface changes;
- zero gear transitions, with gear 1 throughout;
- zero Impact-active frames and maximum Impact exactly zero;
- zero collision candidates;
- zero unknown native events.

Directional is present because the vehicle is being driven: absolute P50
`.01028`, P95 `.01582`, maximum `.01829`. It does not explain Candidate A,
because the two normal-road controls contain equal or greater Directional
activity while Candidate A remains exactly zero. Stable cobblestone speed has
P05 `.13333`, P50 `.25430`, P95 `.38240`.

The stable evidence is therefore independent of Impact, gear, collision,
surface transition and Directional contamination.

## First active prototype boundary

The next milestone may design one active experiment with:

- Candidate A continuous envelope only;
- symmetric wheel presentation only;
- a strict independent authority bound;
- conservative first-test authority;
- v1 Road immediately selectable as fallback/reference;
- an explicit A/B physical test at ordinary wheel-side strength.

It must keep spatial imbalance, signed group torque, transition presentation,
material tables, arbitrary cobblestone gain, E8/EC/EE and event-aware Impact
inactive. Its authority must not be inherited from the old v1 Road 2.00x
ceiling, and the first physical test must not explore maximum range.

No Force output, telemetry schema, current Road, Directional, Impact, gear feel,
M4/M5, DirectInput, controller behavior or player setting changes are part of
R4.2C.
