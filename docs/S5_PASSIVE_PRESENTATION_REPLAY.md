# S5 passive presentation replay

S5 tests the presentation architecture offline. Nothing described here reaches
hardware. The replay does not change M4, M5, output conditioning, DirectInput,
device strength, or runtime telemetry.

## Baseline

S4 commit `e00cd7869bc60ce61aa9e387e61f3cda371cbc8d` is published on
`multi-device-input`. Windows CI run 71 completed successfully for that exact
commit.

The replay used four existing Dino captures:

| Capture | Samples | Duration | Use |
| --- | ---: | ---: | --- |
| S2 M4 output exposure | 6,238 | 103.74 s | Contrast-zero control and global presence exposure |
| S2 M5 output exposure | 6,715 | 111.70 s | Presence × Contrast exposure and headroom |
| M5J valid active | 9,718 | 161.74 s | Authoritative selected M5 delta and active-route validation |
| M5I shadow | 9,718 | 161.74 s | Independent passive delta and matched-event comparison |

No new capture was requested.

## Replay model

S5 keeps semantic mixing visible before the existing conditioner:

```text
primary = bite_shadow_directional
secondaryRaw = m5j_selected_directional - primary

directionalPresentation = primary + Contrast * secondaryRaw
semanticMix = directionalPresentation + road + impact
presentationRequest = Presence * semanticMix

projectedConditionedOutput = tanh(presentationRequest) * observedOutputRamp
```

The S2 M5 capture supplies the exact active selection in
`m5j_selected_directional`. Inactive and vetoed frames therefore produce an
exact zero secondary delta without reconstructing M5 semantics. The observed
ramp is reconstructed from `ffb_raw / s2_post_tanh` when nonzero.

The replay reports both the unconditioned presentation request and its
projected result through the unchanged current `tanh` and ramp. Future safety
would receive the visible presentation request. The projected result is used
only to compare S3 software exposure.

At Reference neutral settings (`Presence=1`, `Contrast=1`), projected output
matches recorded `ffb_raw` within `1.7e-7`. That residual is consistent with
CSV precision. Reference is therefore exactly reconstructable for practical
purposes.

## What Presence changes

Presence scales the requested presentation after primary and secondary
directional information have been combined. It does not read or modify M4
phase, M5 classification, M5 eligibility, confidence, contamination, BITE, or
restoration. Changing Presence leaves the set of 243 active S2 M5 samples
unchanged.

The 1.00, 1.10, 1.20, and 1.30 research points remain inside the observed S3
software comfort region. The 1.40 point is retained only as a headroom-aware
reference. None is a proposed default or hardware-safe setting.

## What Contrast changes

Contrast scales only the exact eligible M5 difference. The S2 M4 capture has
no M5 delta, and every Contrast value from 0 through 8 produces identical M4
output. In the S2 M5 capture, Contrast acts on 243 of 6,715 samples, or 3.62%.

| Contrast | Median requested delta | P95 delta | Maximum delta | P95 of delta / primary | Maximum delta / primary |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 0 | 0 | 0 | 0 | 0 | 0 |
| 1 | .000189 | .000805 | .002496 | .30% | .55% |
| 2 | .000377 | .001609 | .004993 | .60% | 1.10% |
| 4 | .000755 | .003218 | .009985 | 1.19% | 2.21% |
| 8 | .001510 | .006437 | .019970 | 2.39% | 4.42% |

No tested Contrast value reverses sign or creates directional Force from zero.
When the recorded delta is zero, Contrast has exactly zero effect. Contrast
does not globally amplify M4.

The independent M5J and M5I captures show nearly the same scale:

| Capture | Active samples | Raw delta median | Raw delta P95 | Raw delta max | Contrast 8 max share of primary |
| --- | ---: | ---: | ---: | ---: | ---: |
| M5J active | 450 / 9,718 (4.63%) | .000207 | .000928 | .002370 | 4.79% |
| M5I shadow | 441 / 9,718 (4.54%) | .000163 | .000766 | .001330 | 4.73% |

Contrast 8 remains small relative to primary load, but it is not semantically
free. Two samples in each research capture would exceed the existing Legacy
directional magnitude if no authority cap were retained. That is why a future
contrast budget must preserve the existing Legacy boundary in addition to any
percentage budget.

## Presence × Contrast matrix

The matrix uses the S2 M5 capture and the unified-channel Presence equation
above. Magnitudes are projected through the unchanged conditioner. `R1` and
`R3` are rolling one- and three-second RMS. `H05` is fifth-percentile remaining
normalized headroom. Occupancy at .75, .90, and .98 is zero in every row.

| Presence | Contrast | P50 | P75 | P90 | P95 | P99 | Max | R1 P95 | R1 Max | R3 P95 | R3 Max | Occ >=.50 | H05 | Minimum headroom |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1.00 | 0 | .035 | .127 | .236 | .310 | .405 | .510 | .261 | .380 | .229 | .271 | .01% | .690 | .490 |
| 1.00 | 1 | .035 | .127 | .236 | .310 | .405 | .510 | .261 | .380 | .229 | .271 | .01% | .690 | .490 |
| 1.00 | 2 | .035 | .127 | .236 | .310 | .405 | .510 | .261 | .380 | .229 | .271 | .01% | .690 | .490 |
| 1.00 | 4 | .035 | .127 | .236 | .311 | .405 | .510 | .261 | .380 | .230 | .271 | .01% | .689 | .490 |
| 1.00 | 8 | .035 | .127 | .236 | .311 | .405 | .510 | .261 | .380 | .230 | .271 | .01% | .689 | .490 |
| 1.10 | 1 | .039 | .140 | .258 | .339 | .440 | .550 | .285 | .413 | .250 | .295 | .13% | .661 | .450 |
| 1.10 | 2 | .039 | .140 | .258 | .339 | .441 | .550 | .285 | .413 | .250 | .295 | .13% | .661 | .450 |
| 1.10 | 4 | .039 | .140 | .258 | .340 | .441 | .550 | .285 | .413 | .251 | .295 | .13% | .660 | .450 |
| 1.20 | 1 | .042 | .152 | .281 | .367 | .474 | .588 | .308 | .445 | .271 | .319 | .43% | .633 | .412 |
| 1.20 | 2 | .042 | .152 | .281 | .367 | .475 | .588 | .308 | .445 | .271 | .319 | .43% | .633 | .412 |
| 1.20 | 4 | .042 | .152 | .281 | .368 | .475 | .588 | .308 | .445 | .271 | .319 | .45% | .632 | .412 |
| 1.30 | 1 | .046 | .165 | .303 | .395 | .507 | .624 | .331 | .476 | .291 | .341 | 1.25% | .605 | .376 |
| 1.30 | 2 | .046 | .165 | .303 | .395 | .507 | .624 | .331 | .476 | .291 | .341 | 1.25% | .605 | .376 |
| 1.30 | 4 | .046 | .165 | .303 | .395 | .507 | .624 | .331 | .476 | .291 | .342 | 1.25% | .605 | .376 |
| 1.30 | 8 | .046 | .165 | .303 | .395 | .507 | .624 | .331 | .476 | .292 | .342 | 1.28% | .605 | .376 |
| 1.40 | 1 | .049 | .177 | .324 | .421 | .538 | .657 | .353 | .505 | .311 | .363 | 2.22% | .579 | .343 |
| 1.40 | 4 | .049 | .177 | .324 | .422 | .539 | .657 | .353 | .505 | .311 | .364 | 2.23% | .578 | .343 |

Presence determines aggregate exposure in this matrix. Contrast changes the
eligible local distinctions but barely moves whole-drive percentiles because
it acts on only 3.62% of samples and the validated delta is small. All tested
rows remain either S3 Comfort (through Presence 1.30) or Headroom-Aware
(Presence 1.40). None approaches compression or saturation in these captures.

## Can Contrast separate similar M4 events?

Each active M5 sample was paired with the nearest inactive sample from the
same capture, M5 release phase, and primary sign. This follows the M5E/M5I
question: similar primary M4 state, different M5 context.

The M5J pairs have median primary mismatch .00103 and P95 .00567. The M5I
pairs have median mismatch .000685 and P95 .00640. A stricter .001 primary
tolerance retains 218 M5J and 279 M5I pairs.

After subtracting the matched primary mismatch, the additional directional
separation is exactly `Contrast * secondaryRaw`:

| Capture | Contrast | Median added separation | P95 | Maximum |
| --- | ---: | ---: | ---: | ---: |
| M5J | 0 | 0 | 0 | 0 |
| M5J | 1 | .000207 | .000928 | .002370 |
| M5J | 2 | .000414 | .001855 | .004741 |
| M5J | 4 | .000829 | .003711 | .009482 |
| M5J | 8 | .001657 | .007421 | .018963 |
| M5I | 0 | 0 | 0 | 0 |
| M5I | 1 | .000163 | .000766 | .001330 |
| M5I | 2 | .000326 | .001532 | .002659 |
| M5I | 4 | .000652 | .003065 | .005318 |
| M5I | 8 | .001304 | .006130 | .010637 |

Contrast therefore increases mathematical separability independently of
Presence and without altering classification. At Contrast 4, the added median
distinction is still comparable to or below normal matched-event primary
variation. At Contrast 8, P95 added separation becomes comparable to the P95
matching uncertainty. This is useful evidence that the contrast concept works,
but it does not establish perceptibility or a preferred multiplier.

## Secondary-information budget

Two candidate budget families were replayed.

**Relative-to-primary budget**

```text
abs(secondaryShaped) <= researchFraction * abs(M4 primary)
```

This formulation scales down naturally as M4 approaches zero, cannot create a
large secondary request from a tiny primary, and directly expresses M5
subordination. Candidate research ceilings from one through eight percent
were evaluated without selecting a production value. In M5J, a one-percent
ceiling would bind 7.3% of active Contrast-4 samples and 32.7% of active
Contrast-8 samples. A four-percent ceiling would bind only 0.44% of active
Contrast-8 samples. M5I shows the same pattern: 6.1%, 27.0%, and 0.23%.

**Available-headroom budget**

```text
abs(secondaryShaped) <= researchFraction * current normalized headroom
```

This formulation rarely binds in these low-exposure captures. More
importantly, available headroom can be large when M4 is small, allowing a
secondary request to become too large relative to its authority. Headroom is
therefore a safety constraint, not a stable semantic budget.

The stable architecture is:

1. relative-to-primary subordination;
2. the existing Legacy directional magnitude boundary;
3. a separate future safety/headroom decision on the complete presentation.

No production fraction is selected in S5.

## Presence, road, and impact

Two scope policies were compared at Presence 1.30:

1. **Unified Presence:** scale directional, road, and impact together.
2. **Directional Presence:** scale the primary/secondary directional mix while
   leaving recorded road and impact at Reference amplitude.

At Contrast 4, Unified Presence produces P95 .395, maximum .624, and minimum
headroom .376. Directional Presence produces P95 .386, maximum .581, and
minimum headroom .419. The difference comes from scaling transient and texture
channels, not M5.

Unified Presence best matches the plain-language meaning of stronger overall
Force and preserves the existing channel ratios. Directional Presence retains
more transient reserve, but silently changes the balance of directional load
against impacts and road detail. Neither should be hidden inside one gain.

The recommended model keeps channel identity through presentation:

```text
M4 primary + bounded M5 contrast
    -> directional presentation

road texture request ------------+
impact transient request --------+-> explicit profile mix
directional presentation --------+       -> Global Presence
                                           -> presentation request
                                           -> future safety
```

Reference uses unity expression for every family and therefore scales the
whole validated mix uniformly. A future profile may request different texture
or transient character only through explicit, observable profile policy.
This keeps `Force Strength` predictable while avoiding an accidental decision
that every future profile must scale every family identically.

At the tested Presence values, impact and road retain substantial normalized
range. Presence 1.30 leaves at least .376 reserve in the S2 M5 capture;
Presence 1.40 leaves .343. Contrast contributes almost no aggregate reserve
pressure. S5 does not prove that these reserves cover every collision, road,
car, or route.

## Invariants and fallback

The passive model satisfies these checks:

- neutral Reference reproduces recorded output within CSV precision;
- Presence does not change M4/M5 interpretation or eligibility;
- Contrast has no effect when the validated delta is zero;
- no tested matrix point reverses primary sign;
- no tested matrix point creates directional output from zero;
- FREE, BITE, invalid, contaminated, and other vetoed samples retain zero M5
  delta because the replay uses the authoritative selected value;
- M4-only output is identical for Contrast 0, 1, 2, 4, and 8;
- the existing Legacy boundary remains necessary at high research contrast.

Missing, invalid, nonfinite, or unsupported presentation configuration must
select Reference with neutral Presence and neutral Contrast. A stale secondary
delta must never survive fallback.

## Minimum future telemetry

Runtime work, if later authorized, needs only enough fields to make routing
unambiguous:

```text
presentation_profile
presentation_presence
presentation_contrast
presentation_primary
presentation_secondary_raw
presentation_secondary_eligible
presentation_secondary_shaped
presentation_secondary_budgeted
presentation_road_request
presentation_impact_request
presentation_request
```

The future safety output, device-calibrated output, and final DirectInput
request remain separate downstream fields. Shadow, requested, budgeted, and
hardware-selected values must not share names. S5 adds no runtime telemetry.

## Limits of the result

This is Dino-baseline software replay. It does not establish perceptibility,
preferred feel, a production contrast factor, a production budget, a global
Presence default, a hardware-safe value, cross-car M5 normalization, an Arcade
profile, AER character, or a safety policy. M5 remains experimental and is not
a production-default secondary source.

The captures cannot prove every impact and road combination retains enough
reserve. Matched-event separation is mathematical, not a human detection
result. Large offline Contrast values are meaningful only through their small
resulting deltas.

## Recommended S6

**S6 — Passive presentation policy and budget shadow.**

S6 should add no hardware path. It should define one versioned Reference
identity policy and passively compute, at runtime or by deterministic replay:

- the exact primary, raw secondary, shaped secondary, and authority-capped
  secondary values;
- explicit directional, road, and impact presentation requests;
- the complete presentation request offered to the existing S2 observer;
- reasons for every eligibility, budget, Legacy-boundary, and fallback result.

The main unresolved research choice is the relative-to-primary contrast budget
and its behavior near zero. S6 should compare candidate curves and hysteresis
offline, retain the Legacy boundary, and prove Reference identity before any
active presentation or physical UAT is considered.

