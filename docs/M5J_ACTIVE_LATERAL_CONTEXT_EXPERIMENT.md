# M5J active lateral-context driving experiment

M5J allows a controlled physical A/B test between the frozen M4 Foundation and
the exact M5I lateral-context shadow result. This is **experimental
Dino-baseline research**, not production Force behavior.

Use the hidden developer setting:

```ini
[Developer]
Force2Mode = Active
M5LateralMode = M4_ONLY
```

Accepted M5 values are `M4_ONLY` and `M5_LATERAL_ACTIVE`. Invalid or missing
values select `M4_ONLY`. Restart the game after changing the mode.

M5J-R1 resolves this setting once after configuration loading and records the
authoritative selection at startup:

```text
HYP36R M5 lateral mode: M5_LATERAL_ACTIVE
```

## Routing

`M4_ONLY` preserves the established M4 directional selection and output path:

```text
M4 directional + unchanged impact + unchanged road
    -> existing tanh and output ramp
    -> drive()
    -> DirectInput
```

`M5_LATERAL_ACTIVE` evaluates the existing M5I model against that M4
directional value. When every M5I gate is eligible, hardware selects the exact
M5I shadow directional result. Otherwise it selects M4 unchanged. Impact,
road, vibration, output conditioning, BITE/restoration, and abort behavior are
not recalculated or replaced.

```text
M4 directional
    -> exact M5I lateral-context shaping when eligible
    -> selected directional + unchanged impact + unchanged road
    -> existing tanh and output ramp
    -> drive()
    -> DirectInput
```

BITE authority is absolute. The independent `bite_active` detector directly
vetoes M5I in addition to the existing BITE-shadow phase gate. M5 shaping is
also disabled in Normal, Established/FREE, Recovering, returned-load, low
activity, mixed response, and surface contamination.

The M5I equation, thresholds, confidence behavior, normalization, deadband,
and provisional four-percent ceiling are unchanged. M5J does not increase gain
to make the experiment easier to perceive.

## Telemetry proof

M5J preserves all M5I columns and appends:

```text
m5j_mode
m5j_selected_directional
m5j_applied_modulation
```

`hardware_selected_directional` records the directional value actually used to
compose the pre-drive force. In `M4_ONLY`, it must equal M4. In active mode it
must equal `m5i_shadow_directional` only on eligible M5I frames. The explicit
M5J selection fields remove ambiguity about the active route.

Cross-car validation remains required before production deployment, universal
defaults, or vehicle-independent claims. Do not create per-car compensation.

M5J-R1 physical UAT confirmed that the active route reached hardware only on
eligible developing RELEASE frames. The communication was perceptible but
subtle. Amplitude remains unresolved and is intentionally deferred to a later
information-amplification and safety-envelope milestone.
