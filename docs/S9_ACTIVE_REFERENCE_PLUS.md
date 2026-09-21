# S9 active Reference+ presentation candidate

S9 is the first controlled active presentation experiment. It uses the exact
S8 policy implementation for both telemetry and hardware selection; there is
no second active equation. The baseline is S8 commit
`1926867d171016f6fcd014fd7a3cae318f5d573a`, validated by Windows run 75.

## Modes

The hidden developer setting is:

```ini
[Developer]
PresentationMode = REFERENCE
```

Accepted values are `REFERENCE` and `REFERENCE_PLUS_EXPERIMENTAL`. Missing,
unknown, or malformed values select Reference. Reference+ also falls back when
the M4 composer is not active or any presentation input/result is nonfinite.

Reference selects the unchanged M4-only directional value, Presence 1.00, and
no M5 information. Reference+ is a fixed research candidate:

```text
Presence = 1.20
Contrast = 4
linear secondary budget = 5% * abs(M4 primary)
```

These values are not production defaults. Presence 1.20 was selected because
S3/S5 placed it comfortably inside the observed software comfort region.
Contrast 4 improves mathematical separation of eligible M5 information while
remaining subordinate to M4 and bounded by the linear budget and Legacy
directional authority.

## Active architecture

The shared S8/S9 policy evaluates before the final hardware composition:

```text
M4 primary
    + eligible M5 delta * Contrast
    -> 5% primary-relative budget
    -> zero/sign preservation
    -> Legacy secondary-authority boundary
    = directional pre-Presence
    * Presence
    = hardware-selected directional
    + unchanged impact + unchanged road
    -> existing tanh
    -> existing output ramp
    -> drive()
    -> DirectInput
```

Reference selects M4 before `tanh` and therefore reproduces M4-only hardware
behavior. Reference+ selects the policy frame's exact post-Presence value.
Presence applies only to directional information. Road, impact, vibration,
output ramp, wheel strength, inversion, and DirectInput conditioning retain
their existing paths and values.

M5 uses the established M5I prospective delta and all existing gates. It is
eligible only during developing RELEASE and remains disabled during FREE,
BITE, surface contamination, low activity/confidence, and mixed response.
M5 never supplies steering direction.

## Telemetry authority

S9 preserves S2 and S8 fields and appends:

```text
s9_mode
s9_directional_pre_presence
s9_directional_post_presence
s9_hardware_directional_selected
```

`s9_hardware_directional_selected` is the exact directional value used in the
same frame's `s2_composer_input` and `ffb_raw`. `ffb_final` remains the final
DirectInput request after inversion, user strength, and clamp. This removes
ambiguity between prospective M5, presentation request, and hardware routing.

## Replay evidence

The exact S9 equation was replayed across the S2 M4, S2 M5, M5J, and M5I
datasets. Semantic invariants cover 32,389 recorded frames. Exposure can be
reconstructed from the two S2 captures, which contain the required S2 output
boundary fields.

| Capture | Reference P95/max | Reference+ P95/max |
| --- | ---: | ---: |
| S2 M4 | .333 / .488 | .391 / .549 |
| S2 M5 | .310 / .510 | .361 / .558 |

The S2 M5 result matches the earlier S5/S6 1.20 prediction. Across all replayed
samples:

- Reference directional selection equals M4 exactly;
- no output is created from zero primary;
- primary sign is preserved;
- eligible M5 stays within the five-percent linear budget;
- Legacy directional authority remains enforced for the M5 contribution;
- no M5 sample is active during FREE or active BITE;
- all calculated presentation values are finite;
- road, impact, and vibration inputs are unchanged.

No five-percent budget intervention occurs at Contrast 4 in these captures.
That does not remove the boundary; it confirms the candidate remains below it
in the current evidence.

The older M5I capture contains four shadow-active flags overlapping BITE from
before M5J added the independent BITE veto. S9 replay reapplies the current
authoritative gate, so those historical flags permit no secondary information.
The newer M5J and S2 captures already record the corrected behavior directly.

## Fail-safe and limits

Invalid configuration selects Reference at startup. A nonfinite runtime input,
inactive M4 composer, or nonfinite presentation result selects Reference for
that frame. Nonfinite road or impact input is suppressed only for that invalid
frame, and a nonfinite final force request resolves to zero rather than being
sent to DirectInput.

S2/S3 exposure remains observational. S9 does not dynamically reduce output,
change hardware strength, tune road or impact, implement AER, or certify device
safety. Perceptual benefit, fatigue, clipping on other cars/routes, and broader
hardware behavior remain unknown and require controlled physical comparison.

## Physical UAT

Use the same car, route, wheel settings, and driving sequence for two short
runs. First run `REFERENCE`; then restart with
`REFERENCE_PLUS_EXPERIMENTAL`. Confirm Reference feels unchanged, Reference+
makes directional load/release easier to read, road and collision strength do
not change unexpectedly, and no oscillation, harshness, clipping, or device
protection occurs. Stop immediately if the wheel behaves unexpectedly and
retain both telemetry captures with subjective notes.
