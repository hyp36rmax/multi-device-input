# M5I lateral-context shadow communication

M5I is passive lateral-context communication research. It asks whether the
front/rear localization already described by M5D and M5F could subtly qualify
M4's existing directional-load character. It does not add Force and its result
is written only to telemetry.

M4 remains authoritative for overall grip and directional authority, RELEASE,
FREE, BITE, load restoration, and the hardware request. M5 remains localized
native vehicle context. M5I compares those layers without connecting them at
the hardware boundary.

## Evidence boundary

All constants in M5I are **provisional Dino-baseline research values**. The
baseline is the Ferrari Dino 246 GTS at Sunny Beach with a Fanatec DD2 at 50%
hardware FFB and 100% game FFB. They are not universal, production,
vehicle-independent, or final values. Cross-car validation remains required
before active M5 integration, universal normalization, or production defaults.

M5I does not classify understeer, oversteer, grip, axle saturation, steering
rack torque, or self-aligning torque. The conditioned M5D lateral channels are
native contact-plane response candidates with unknown units.

## Shadow hypothesis

Front response is treated as primary steering-relevance context because the
front axle is steered. Rear response qualifies the wider vehicle state and
never supplies steering direction. During M4 `Emerging` only, M5I tests whether
front localization can preserve a small amount of the existing M4 directional
load while rear localization can qualify that load downward.

The continuous balance is inherited from M5F:

```text
balance = (abs(frontLateral) - abs(rearLateral))
        / (abs(frontLateral) + abs(rearLateral) + epsilon)
```

After a `0.02` balance deadband, balance magnitude is normalized over the
remaining range. Lateral activity is gated at `0.08` and reaches full research
weight at `0.25`. Existing M5F confidence supplies the final qualification.

```text
modulation = 0.04 * signedBalanceWeight * activityWeight * confidence
candidate  = M4Directional * (1 + modulation)
M5IShadow  = clampMagnitude(candidate, abs(LegacyDirectional))
```

The independent four-percent ceiling is intentionally smaller than M4's
unloading budget. It bounds a new, unproven information family rather than
tuning for strength. In the Dino replay, observed modulation remained below
`0.56%` because the measured balance was usually near neutral.

M5I cannot create force from zero, reverse the M4 sign, or exceed Legacy
directional magnitude. Surface-contaminated samples, low activity, mixed-sign
axle response, M4 Normal, M4 Established/FREE, and every non-baseline BITE
phase remain observational. Longitudinal and chassis context are retained for
later audit only and do not enter the equation.

## Passivity

The implementation order is deliberately one-way:

```text
M4 selects hardware directional
    -> drive()
    -> M5D/M5F context
    -> M5I shadow comparison
    -> telemetry only
```

No M5I field is read by hardware selection, spring, damper, road, impact,
`WheelForceFeedback::drive()`, or DirectInput. BITE and abort behavior are
unchanged.

## Runtime shadow UAT

Use the existing Dino baseline and record scenario `M5I_DINO_SHADOW_RUNTIME`.
Repeat stationary departure, left and right corners, a developing release,
FREE/countersteer, clean BITE, aborted BITE, and a one-side surface transition.
The run is valid only if hardware remains M4 and the M5I columns remain passive.

