# S8 passive runtime presentation shadow

S8 moves the validated S4–S7 presentation policy into synchronized runtime
observation. It does not activate presentation. The hardware path remains the
M4/M5 path frozen before S8, and the passive frame is evaluated only after
`WheelForceFeedback::drive()` has received that unchanged value.

Baseline `2908e14bfd8507bfa9452e9c5f91eade10e43040` preserves the S7 decision
to use a linear primary-relative secondary budget without a deadband or taper.

## Runtime frame

`HYP36RPresentation::Frame` records one authoritative presentation request:

- Reference profile, Presence, Contrast, and research budget fraction;
- current M4 directional primary;
- eligible M5 raw, requested, and policy-permitted secondary deltas;
- Presence-scaled directional request;
- independent Reference road, impact, and vibration requests;
- secondary-budget and Legacy-boundary interventions;
- descriptive software region and fail-safe state.

The policy order is:

```text
eligible M5 delta * Contrast
    -> linear budget: 5% * abs(M4 primary)
    -> preserve primary sign and zero rule
    -> preserve Legacy directional authority
    -> add to M4 primary
    -> apply passive Presence request
```

Five percent is a conservative research default used in S6/S7 replay. It is
not a production or perceptual value. The setting remains explicit so later
authorized research can identify the exact request under observation.

## Configuration

The hidden developer settings are:

```ini
[Developer]
PresentationProfile = Reference
PresentationPresence = 1.00
PresentationContrast = 0
PresentationSecondaryBudget = 0.05
```

Supported Presence values are `1.00`, `1.10`, `1.20`, `1.30`, and `1.40`.
Supported Contrast values are `0`, `1`, `2`, `4`, and `8`. Missing or invalid
profile, nonfinite or unsupported Presence/Contrast, and invalid budget data
select neutral Reference: Presence 1.00, Contrast 0, and the research budget
default. Fallback clears secondary information.

Startup records the resolved configuration once:

```text
HYP36R presentation shadow: profile=Reference presence=1.20 contrast=4 budget=0.050
```

## Reference and channel identity

Neutral Reference reconstructs M4 exactly: Contrast zero permits no secondary
delta and Presence one leaves the primary unchanged. Road, impact, and
vibration are copied into separate request fields at Reference expression.
Presence affects only the passive directional request in S8. It does not alter
M4/M5 interpretation, M5 eligibility, BITE, FREE, road, impact, vibration, the
output ramp, or hardware strength.

The software region is descriptive evidence from S3/S5. Supported settings
classify Presence through 1.30 as `comfort` and 1.40 as `headroom-aware`.
`compression-risk` and `saturation-risk` remain named for future observations;
S8 does not authorize or expose settings in those regions.

## Telemetry

S8 appends these fields without replacing prior telemetry:

```text
presentation_profile
presentation_presence
presentation_contrast
presentation_directional_primary
presentation_secondary_raw
presentation_secondary_requested
presentation_secondary_permitted
presentation_directional_request
presentation_road_request
presentation_impact_request
presentation_vibration_request
presentation_secondary_budget_active
presentation_legacy_boundary_active
presentation_software_region
presentation_fallback_active
```

The raw secondary field is nonzero only when the established M5I result is
eligible. Contrast does not reinterpret M5 or bypass its gates.

## Passivity and runtime cost

The implementation order is one way:

```text
existing Force/M4/M5 hardware selection
    -> output exposure observation
    -> drive()
    -> S8 presentation evaluation
    -> telemetry
```

No presentation field is read by hardware selection, spring, damper, road,
impact, output ramp, `drive()`, or DirectInput. Evaluation uses fixed scalar
math and one static frame. It performs no allocation, blocking, or file I/O.
Only the existing optional telemetry writer records the frame.

## Replay validation

The exact runtime ordering and equations were replayed over the existing S2 M5,
M5J active, and M5I captures: 26,151 frames total. Each capture was evaluated
at five Presence values and five Contrast values with the five-percent linear
budget.

| Check | Result |
| --- | --- |
| Neutral Reference reconstructs M4 | Exact; maximum error 0 |
| Contrast zero without eligible M5 | Exact identity |
| Zero primary permits zero secondary | Pass |
| Primary sign preserved | Pass |
| Secondary inside linear budget | Pass |
| Legacy directional authority retained | Pass |
| Presence leaves semantic policy unchanged | Pass |
| Road/impact remain independent | Pass |
| All outputs finite | Pass |

The budget did not intervene at the representative five-percent setting. At
Contrast 8, the Legacy boundary intervened on the same two M5J and two M5I
samples identified by S6. That is expected and keeps the reason observable.

## Remaining unknowns

S8 does not establish perceptibility, a preferred Presence or Contrast,
production M5 defaults, cross-car normalization, hardware safety, dynamic
protection, or AER character. No physical UAT is required because no request
reaches hardware.

Recommended S9: capture the synchronized passive S8 schema under controlled
M4-only and experimental-M5 scenarios, then verify runtime identity and
intervention flags before considering any active presentation experiment.
