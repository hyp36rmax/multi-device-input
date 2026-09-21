# HYP36R Force 2.1 research status

M5K freezes the current research baseline at M5J-R1 commit
`c268565abfd8349f21529c8a56eb960562e83a60`. It consolidates what M4 and M5
have established. It does not change Force behavior, output strength, or
hardware calibration.

The permanent M4 Foundation reference is
`42118c029340db95e5f77539b8696bc6b824a08d`.

## Force 2.1 candidate

The current experimental Force 2.1 candidate is:

```text
M4 Force 2.0 Foundation
    +
validated M5 lateral-context characterization during developing RELEASE
```

M4 remains authoritative. M5 is subordinate and may characterize the existing
M4 directional load only during eligible developing RELEASE frames. M5 does
not modify NORMAL, FREE, BITE, RECOVERING, returned load, road, impact, or
tire-slip vibration.

`M4_ONLY` remains the safe default. `M5_LATERAL_ACTIVE` remains an experimental
research option. It is not a production default.

## Proven

### M4 foundation

M4 communicates one continuous grip-authority envelope:

```text
LOAD -> RELEASE -> FREE -> BITE -> RETURNED LOAD
```

It unloads the existing directional force during developing divergence and
permits bounded load restoration only after credible vehicle-driven
reconvergence. It does not invent a steering target or use native vehicle
direction as a wheel command.

### M5 research lineage

- **M5A:** discovered the native four-corner candidates.
- **M5B:** recorded raw four-corner telemetry.
- **M5C:** validated the FL/FR/RL/RR ordering and candidate relationships.
- **M5D:** introduced `FourCornerContext` without affecting Force.
- **M5E:** studied which context was relevant to the existing force intent.
- **M5F:** introduced passive contextual Force intent.
- **M5G:** validated contextual intent at runtime.
- **M5H:** studied perceptual mapping. Cross-car work was deferred because
  additional cars were locked.
- **M5I:** introduced passive lateral-context communication.
- **M5J:** allowed controlled experimental hardware selection of the validated
  M5I result.
- **M5J-R1:** corrected mode selection and completed valid active physical UAT.

The M5J-R1 active capture confirmed that `M5_LATERAL_ACTIVE` reached hardware
on eligible RELEASE frames. There was no activation during BITE, FREE,
stationary running, or surface contamination. The capture found no sign
reversal, force created from zero, or Legacy magnitude violation.

### Native corner order

The validated order is:

```text
0 = FL
1 = FR
2 = RL
3 = RR
```

The evidence combines static front/rear identification, same-side correlation,
surface grouping, and a controlled known-left surface test.

## Evidence boundaries

These names state only what the current evidence supports.

| Native value | Supported meaning | Unsupported interpretation |
| --- | --- | --- |
| `+0x28` | Reference-relative corner displacement/loading context | Suspension travel, normal load, or tire load |
| `+0xAC` | Native lateral contact-plane response candidate | Lateral tire force or SAT |
| `+0xB0` | Native longitudinal contact-plane response candidate | Longitudinal tire force |
| `sqrt(AC^2 + B0^2)` | Native combined response magnitude | Grip percentage or friction-circle utilization |
| `surface_0..surface_3` | Authoritative per-corner surface/contact classification | A derived grip percentage |

No rear-response value commands steering. M5 has no independent steering
direction. Native corner information qualifies the character of an existing
directional force; it does not create one.

## Experimental

`M5_LATERAL_ACTIVE` is a Dino-baseline experiment. During a validated
developing RELEASE frame, it may select the exact M5I directional result. M4
remains selected everywhere else. BITE has absolute priority.

The current physical test proved that the lateral information can reach the
driver. Its presentation is subtle. The final amplitude is intentionally
unresolved. This is not evidence of a physics deficiency, and M5K does not
increase the four-percent research ceiling.

## Telemetry authority

Use the following fields when tracing the current route:

| Question | Authoritative field |
| --- | --- |
| Selected M5 mode | `m5j_mode` |
| M4 directional result | `bite_shadow_directional` |
| Prospective M5 directional result | `m5i_shadow_directional` |
| M5 experiment selection | `m5j_selected_directional` |
| Directional value used to compose hardware output | `hardware_selected_directional` |
| Final normalized value passed to `drive()` | `ffb_raw` |
| Final DirectInput request after strength, inversion, and clamp | `ffb_final` |

`m5j_applied_modulation` explains whether M5 changed the M4 selection on that
frame. `m4c_unloaded_directional`, `bite_shadow_current_m4c_unloading`,
`bite_shadow_unloading`, and `hardware_selected_unloading` remain the
authoritative M4 unloading/restoration chain.

Historical fields such as `active_directional_component`,
`active_unloading_applied`, and `force2_shadow_directional` remain for capture
compatibility. They are not the final hardware-routing authority.

## Regression freeze

The Force 2.1 candidate preserves these boundaries:

1. `M4_ONLY` is identical to the validated M4 path.
2. `M5_LATERAL_ACTIVE` retains the validated M5J behavior.
3. BITE always overrides M5.
4. FREE always disables M5.
5. Surface contamination disables M5.
6. M5 cannot supply steering direction or use rear response as a steering
   command.
7. M5 cannot create force from zero, reverse directional sign, or exceed
   Legacy directional magnitude.
8. Road, impact, tire-slip vibration, output ramp, master strength, inversion,
   and the final clamp remain unchanged.

## Deferred

Cross-car validation remains required before universal M5 normalization,
production M5 defaults, vehicle-independent claims, or final active M5
deployment. Additional comparison cars are currently locked. This does not
block the present research freeze, and no per-car compensation is authorized.

## Unproven

Current M5 evidence does not establish tire force, SAT, suspension travel,
normal load, grip percentage, friction-circle utilization, universal
normalization, a production-safe M5 amplitude, or universal hardware mapping.

## Future calibration

Research decides what information belongs in Force. Later calibration work
will decide how clearly it is presented and how normalized output maps to a
device:

```text
VEHICLE INFORMATION
        |
        v
FORCE MODEL
        |
        v
INFORMATION PRESENTATION / AMPLITUDE
        |
        v
SAFETY / HEADROOM ENVELOPE
        |
        v
DEVICE CALIBRATION
        |
        v
HARDWARE
```

### HYP36R Information Amplification & Safety Envelope

A later milestone should determine useful perceptual amplification, software
saturation and clipping, available headroom, limiter behavior, sustained
output behavior, transient peaks, device protection behavior, and a safe
margin before undesirable device response.

Earlier high-strength testing caused one Fanatec DD2 to enter a protection or
power-shutdown condition. That is device-specific evidence, not a universal
limit. M5K does not test shutdown thresholds.

## Future productization

### Gameplay: Unlock All Content

The preferred future design is a reversible managed save/profile clone. The
legitimate progression save must remain untouched. The unlocked profile must
stay isolated, be removable at any time, and allow immediate restoration of
the original progression. Artificial progress must never be merged silently
into the legitimate save.

This feature could also unblock full-car research without requiring
progression grinding. It is separate from Force research.

### Arcade Experience Reference

AER means **Arcade Experience Reference**. Future work may use validated
original Sega/OutRun arcade FFB communication to inform presentation. AER does
not replace native OutRun physics or HYP36R Force semantics. Unvalidated
interpreted third-party Force models are not AER evidence.

