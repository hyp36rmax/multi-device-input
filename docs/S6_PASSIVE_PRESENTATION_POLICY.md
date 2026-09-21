# S6 passive presentation policy

S6 turns the S5 replay model into a deterministic policy without connecting it
to hardware. Existing Force, M4/M5 semantics, output conditioning, DirectInput,
and runtime telemetry remain unchanged.

Baseline `eb543027034e5026b0789a9d1407623ad1e6d137` passed Windows CI in run
72. S5 established that Presence controls aggregate exposure, Contrast acts
only on the existing eligible M5 delta, and the two controls are independent.
It also found four Contrast-8 samples that would exceed existing Legacy
directional authority without an explicit boundary.

## Policy frame

The passive design keeps every decision inspectable:

```text
PresentationPolicyFrame
    profile
    presenceRequest
    contrastRequest

    directionalPrimary
    directionalSecondaryRaw
    directionalSecondaryRequested
    directionalSecondaryPermitted
    directionalRequest

    roadRequest
    impactRequest
    vibrationRequest

    secondaryBudgetActive
    legacyBoundaryActive
    softwareRegion
    fallbackReason
```

Directional policy is evaluated in this order:

```text
secondaryRequested = Contrast * validatedSecondary
secondaryBudget = researchFraction * abs(primary)
secondaryBudgeted = clampMagnitude(secondaryRequested, secondaryBudget)

candidate = primary + secondaryBudgeted
candidate = preservePrimarySign(candidate)
directionalPresentation = clampMagnitude(candidate, abs(legacyDirectional))
secondaryPermitted = directionalPresentation - primary
```

If primary is zero, secondary permitted is zero. The smallest applicable
constraint wins. Software headroom cannot raise semantic authority.

No production budget fraction is selected here. Two, five, and ten percent
are replay probes only.

## Reference semantics

`Reference` means the current safe/default product route: `M4_ONLY`, neutral
Presence, and no M5 presentation delta. It is not a newly tuned profile.

The existing `M5_LATERAL_ACTIVE` route remains an experimental upstream
research selection. When that research route is being studied, neutral
Presence and Contrast 1 reproduce its currently selected validated M5 result.
That identity property does not make M5 part of the default Reference profile.

Invalid profile, Presence, Contrast, or budget data falls back to
`Reference / M4_ONLY / neutral Presence`. M4 remains available. Fallback clears
any stale secondary request.

## Secondary-budget replay

The M5J active and M5I shadow captures were replayed through Contrast 0, 1, 2,
4, and 8. Neither sign reversal nor force-from-zero occurred at any research
point.

| Capture | Contrast | Primary-relative budget | Budget interventions | Legacy interventions after budget |
| --- | ---: | ---: | ---: | ---: |
| M5J | 0, 1, 2 | 2%, 5%, 10% | 0 | 0 |
| M5J | 4 | 2% | 2 / 450 (.44%) | 0 |
| M5J | 4 | 5%, 10% | 0 | 0 |
| M5J | 8 | 2% | 33 / 450 (7.33%) | 0 |
| M5J | 8 | 5%, 10% | 0 | 2 / 450 (.44%) |
| M5I | 0, 1, 2 | 2%, 5%, 10% | 0 | 0 |
| M5I | 4 | 2% | 1 / 441 (.23%) | 0 |
| M5I | 4 | 5%, 10% | 0 | 0 |
| M5I | 8 | 2% | 27 / 441 (6.12%) | 0 |
| M5I | 8 | 5%, 10% | 0 | 2 / 441 (.45%) |

At Contrast 8, the two-percent probe absorbs the four Legacy crossings earlier
as budget interventions. The five- and ten-percent probes do not bind those
samples, so the Legacy boundary deterministically catches two in each capture.
All resulting requests remain within Legacy magnitude and preserve primary
sign.

The behavior explains why both constraints are required. A relative budget
defines M5 subordination. The Legacy clamp preserves existing directional
authority when primary is already close to its maximum. Neither substitutes
for the other.

## Requested and permitted secondary information

Representative M5J results show how the policy changes the request:

| Contrast | Budget | Requested P95 | Requested max | Permitted P95 | Permitted max |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 2–10% | .000928 | .002370 | .000928 | .002370 |
| 2 | 2–10% | .001855 | .004741 | .001855 | .004741 |
| 4 | 2% | .003711 | .009482 | .003711 | .008452 |
| 4 | 5–10% | .003711 | .009482 | .003711 | .009482 |
| 8 | 2% | .007421 | .018963 | .006798 | .009033 |
| 8 | 5–10% | .007421 | .018963 | .007421 | .018963* |

`*` The largest permitted delta is not itself the Legacy-crossing sample. Two
other samples are trimmed by the Legacy boundary. The frame-level intervention
flags, rather than an aggregate maximum, are authoritative.

M5I independently shows the same pattern. Contrast 8 with a two-percent probe
reduces P95 permitted delta from .006130 to .004959. Five- and ten-percent
probes leave P95 unchanged while the Legacy boundary trims two samples.

These probes describe policy behavior only. They do not identify a preferred
or perceptible budget.

## Presence and channel identity

Global Presence is a profile-level request, not one anonymous scalar applied
blindly to every channel. The profile retains independent directional, road,
impact, and vibration requests before the final mix.

- **Directional:** Presence may request stronger M4 plus permitted M5
  presentation.
- **Road:** remains at Reference expression until road presentation is studied.
- **Impact:** remains at Reference expression until transient presentation is
  studied.
- **Tire-slip/vibration:** keeps its separate periodic-effect identity and is
  not folded into the directional policy.

This is option B from the S6 design question. A single scalar preserves current
ratios but also strengthens sustained load, texture, and impact together,
spending transient reserve without an explicit decision. Profile-level
Presence keeps those choices visible. It does not authorize independent tuning
in S6.

For replay, Presence 1.00–1.40 applies to the directional request while road
and impact remain at their recorded Reference values. A representative
five-percent secondary budget is used only to exercise the policy; it does not
bind the S2 M5 capture at any tested Contrast.

| Presence | Contrast range | P95 output | Maximum | P95 1s RMS | Max 1s RMS | P95 3s RMS | Max 3s RMS | Occ >=.50 | Minimum headroom | Region |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 1.00 | 0–8 | .310–.311 | .510 | .261 | .380 | .229–.230 | .271 | .01% | .490 | comfort |
| 1.10 | 0–8 | .335–.337 | .534 | .282–.283 | .408 | .249 | .293 | .09% | .466 | comfort |
| 1.20 | 0–8 | .360–.362 | .558 | .303–.304 | .436 | .268–.269 | .314 | .30–.34% | .442 | comfort |
| 1.30 | 0–8 | .385–.387 | .581 | .325 | .462 | .287 | .334 | .86–.88% | .419 | comfort |
| 1.40 | 0–8 | .410–.411 | .607 | .345 | .488 | .305–.306 | .354 | 1.71% | .393 | headroom-aware |

Occupancy at .75, .90, and .98 remains zero throughout. Presence controls the
exposure change; Contrast remains locally meaningful but nearly invisible in
whole-drive statistics.

The S3/S5 classification remains descriptive software evidence. Comfort does
not mean hardware-safe, and Headroom-Aware does not define an intervention.

## Transient reserve

The same channel-preserving replay was inspected during road activity, impact
activity, and the highest five percent of one-second RMS windows. Values below
are remaining normalized range at neutral Contrast.

| Presence | Context | Median reserve | P05 reserve | Minimum reserve |
| ---: | --- | ---: | ---: | ---: |
| 1.00 | Road active | .897 | .641 | .509 |
| 1.00 | Impact active | .918 | .649 | .490 |
| 1.00 | Sustained-high window | .804 | .588 | .490 |
| 1.20 | Road active | .878 | .583 | .461 |
| 1.20 | Impact active | .904 | .591 | .442 |
| 1.20 | Sustained-high window | .757 | .522 | .442 |
| 1.30 | Road active | .869 | .555 | .438 |
| 1.30 | Impact active | .897 | .565 | .419 |
| 1.30 | Sustained-high window | .737 | .495 | .419 |
| 1.40 | Road active | .859 | .527 | .416 |
| 1.40 | Impact active | .890 | .540 | .393 |
| 1.40 | Sustained-high window | .719 | .469 | .393 |

The studied requests retain substantial range, but Presence 1.40 is still
classified Headroom-Aware because the broader S3 evidence showed growing
exposure and reduced reserve. S6 does not allocate transient capacity or claim
that these drives contain the worst possible impact.

## Passive/runtime boundary

S6 remains offline because the existing captures fully exercise the proposed
ordering, candidate budgets, Legacy crossings, sign rule, zero rule, and
software exposure. Adding runtime shadow code would not resolve the remaining
unknown value: which budget policy is perceptually useful without becoming
manufactured.

If a later passive implementation is authorized, it must run only after the
existing selected Force has been sent to `drive()`:

```text
existing M4/M5 selection -> drive() -> S6 shadow evaluation -> telemetry
```

No S6 result may feed `drive()`, the composer, M4/M5 state, or DirectInput.
Minimum audit fields are the frame members listed above; existing M4/M5 values
should be referenced rather than duplicated.

## Evidence limits and next step

S6 establishes deterministic policy behavior for the existing Dino research
captures. It does not establish preferred Presence or Contrast, a production
secondary budget, physical torque, a hardware-safe setting, cross-car M5
normalization, dynamic protection, Arcade presentation, or AER behavior.

The policy decision is complete: primary-relative budget first, Legacy
authority second, future safety after the complete presentation request. The
unresolved question is how the relative budget should behave near zero and
across the observed primary range.

Recommended S7: **near-zero secondary-budget curve study**. It should remain
offline and compare linear, deadbanded, and softly tapered primary-relative
budgets using the same captures. Acceptance should require continuity,
monotonicity, no force from zero, no sign reversal, preserved Legacy authority,
and exact Reference fallback. It must not select a production value or connect
presentation to hardware.

