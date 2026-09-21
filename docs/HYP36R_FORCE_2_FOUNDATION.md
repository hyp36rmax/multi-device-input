# HYP36R Force 2.0 Foundation

This document is the permanent M4 foundation reference. The consolidated M5K
research status and experimental Force 2.1 candidate are documented in
`HYP36R_FORCE_2_1_RESEARCH_STATUS.md`.

M4 closes the first HYP36R Force 2.0 foundation. Its purpose is to communicate
how OutRun's directional authority changes through a slide and recovery without
inventing a steering target or adding torque. The validated envelope is:

```text
LOAD -> RELEASE -> FREE -> BITE -> RETURNED LOAD
```

These names describe regions of one continuous process. They are not effects
and there is no permitted bump, pulse, snap, or discrete state force between
them.

## Why HYP36R exists

The original PC input implementation does not provide a modern, inspectable
wheel-force path for multiple contemporary devices. HYP36R combines the
existing center-out directional model with validated OutRun vehicle-response
observations. It is an independent force interpretation intended to make the
car's changing directional authority understandable through the wheel.

The standing principles are:

> MORE INFORMATION, NOT MORE TORQUE.

> CONTINUOUS VEHICLE COMMUNICATION, NOT MANUFACTURED EFFECTS.

> HYP36R FORCE COMMUNICATES THE INERTIA OF GRIP; IT DOES NOT FIGHT THE SLIDE.

> NATIVE VEHICLE DIRECTION INFORMS LOAD CAPABILITY, NOT STEERING COMMAND.

> THEORY INFORMS INTERPRETATION; OBSERVED GAME BEHAVIOR REMAINS AUTHORITATIVE.

## Information sources

HYP36R keeps native, interpreted, derived, and synthetic information separate.

- **Observed OutRun state** includes the steering reference and the D38-D48
  response-state cluster read from the player car.
- **Semantic vehicle state** validates, unwraps, scales, and relates those
  observations as reference angle, response angle, response rate,
  reference/response error, authority, and transition validity.
- **Derived vehicle context** includes lateral speed, the current synthetic
  slip ratio, and synthetic grip-loss estimate. These are not native tire
  telemetry.
- **Synthetic force behavior** includes the existing speed-scaled spring,
  steering damper, road carrier, impact decay, M4C unloading, and M4F
  restoration trajectory.
- **Configuration and hardware output** include master strength, inversion,
  output ramp, clamping, and the DirectInput request.

No derived value is presented as a native tire or steering-rack measurement.

## Semantic vehicle state

The native interpreter treats the OutRun response state as vehicle-level
directional information. It establishes when the state is valid, suppresses
transitions, preserves angle continuity, and exposes the disagreement between
the driver's steering reference and the vehicle response.

`responseAngle` is observed vehicle-response information. HYP36R never treats
it as a desired wheel angle. `responseRate` and reference/response error help
identify divergence and genuine vehicle reconvergence; neither directly
generates steering torque in the M4 foundation.

## The integrated grip envelope

### LOAD

Directional authority is stable. The existing Legacy directional subtotal is
the upper boundary for Force 2.0.

### RELEASE

Native reference/response divergence develops. M4C progressively unloads the
existing directional subtotal using the validated 0.50-rad normalization and a
maximum 25% reduction. It cannot add force, reverse sign, or create force from
zero.

### FREE

The car remains dynamically displaced with reduced directional authority. The
driver can countersteer and search without Force 2.0 steering toward native
vehicle direction. FREE does not mean zero grip or zero wheel load.

### BITE

M4E identifies credible vehicle-driven reconvergence while meaningful dynamic
state remains. It explicitly distinguishes the vehicle returning from the
driver merely moving the reference toward the vehicle response.

M4F then permits part of the load removed by M4C to return progressively. BITE
means returning load capability, not full grip, a centering impulse, or a wheel
target. Early restoration remains capped, so BITE and returned load remain
different states.

### RETURNED LOAD

As the response and dynamic context settle, directional authority converges
back naturally. If recovery fails and separation resumes, restoration stops
and returns smoothly toward M4C unloading rather than switching abruptly from
light to heavy to light.

## Current active force path

```text
OUTRUN NATIVE STATE
  steering reference + D38-D48 response cluster
                    |
                    v
OBSERVED / SEMANTIC VEHICLE STATE
  validity, reference, response, error, response rate, authority
                    |
                    +------------------------------+
                    |                              |
                    v                              v
DRM REFERENCE                         DERIVED VEHICLE CONTEXT
  interpretation only                  lateral speed, synthetic slip/grip
                    |                              |
                    +---------------+--------------+
                                    v
GRIP ENVELOPE
  LOAD -> RELEASE -> FREE -> BITE -> RETURNED LOAD
                                    |
                                    v
FORCE INTENT
  M4C bounded unloading + M4F bounded restoration
                                    |
                                    v
COMPOSER / MODE SELECTION
  Legacy: Legacy directional
  Shadow: Legacy directional; Force 2.0 diagnostic only
  Active: exact M4F-restored directional
                                    |
                                    v
PRESENTATION PROFILE — FUTURE <----- AER — FUTURE REFERENCE
  expression only                    validated arcade presentation evidence
                                    |
                                    v
HARDWARE CALIBRATION — FUTURE
  wheel-specific physical mapping
                                    |
                                    v
UNCHANGED OUTPUT CONDITIONING
  road + impact -> tanh -> ramp -> master -> inversion -> clamp
                                    |
                                    v
DIRECTINPUT
```

The future AER and presentation/profile layers do not feed vehicle semantics or
the grip envelope. The current hardware path goes directly from mode selection
through the unchanged output conditioning stages.

## Telemetry authority after M4G-R1

Future routing analysis should use this chain:

| Stage | Authoritative field |
| --- | --- |
| Legacy directional | `legacy_directional_component` |
| M4C unloaded directional | `m4c_unloaded_directional` |
| M4F restored directional | `bite_shadow_directional` |
| Hardware-selected directional | `hardware_selected_directional` |
| M4C unloading | `bite_shadow_current_m4c_unloading` |
| M4F restored unloading | `bite_shadow_unloading` |
| Hardware-selected unloading | `hardware_selected_unloading` |
| Final pre-drive force | `ffb_raw` |
| Final DirectInput request | `ffb_final` |

Additional current evidence fields include `state_validity`,
`reference_response_error_rad`, `response_rate_rad_s`, `bite_state`,
`bite_error_closing_rate`, `bite_vehicle_convergence`,
`bite_driver_convergence`, `bite_shadow_load_restoration`,
`bite_shadow_restoration_rate`, and `bite_shadow_abort_active`. These explain
why the active routing value changed but do not replace the authoritative
hardware-selected fields.

### Compatibility and diagnostic fields

The schema remains append-only for capture compatibility.

- `active_directional_component` and `active_unloading_applied` are historical
  M4C composer values. Their names predate the downstream M4G selection and
  must not be interpreted as the current hardware-selected result.
- `force2_shadow_directional`, `shadow_*`, and `force2_shadow_output` describe
  the M4B budgeted shadow composer. They remain useful for historical replay,
  not current hardware-route authority.
- `legacy_force_output` is the counterfactual Legacy result after final output
  conditioning.
- `composer_native_*`, `composer_event_phase`, `intent_*`, BITE confidence and
  phase fields are diagnostic or explanatory.
- `candidate_D*` and `native_*` fields are raw research observations.
  `xforce` remains intentionally empty because no current field is proven to
  be Howard Casto's historical X-Force signal.
- `synthetic_lateral_speed`, `synthetic_slip_ratio`, and
  `synthetic_grip_loss` are derived context, not native OutRun tire state.

## Foundation safety audit

The M4 foundation preserves these invariants:

1. Legacy mode follows the unchanged Legacy hardware path.
2. Shadow mode is hardware-identical to Legacy.
3. Active directional magnitude cannot exceed Legacy due to Force 2.0 grip
   modulation.
4. Force 2.0 cannot create directional force from zero.
5. RELEASE cannot reverse directional sign.
6. BITE restoration cannot reverse directional sign.
7. Active unloading remains within 0..0.25.
8. Invalid or transition-suppressed native state cannot advance restoration.
9. Driver-only convergence cannot advance restoration.
10. Failed recovery returns toward M4C at the bounded abort rate.
11. Native reacquisition is rate-limited and cannot create a one-frame load
    jump.
12. Road remains independent of Force 2.0 directional modulation.
13. Impact remains independent of Force 2.0 directional modulation.
14. Tire-slip vibration remains on its independent path.
15. `tanh`, output ramp, master strength, inversion, and final clamp remain in
    the output path.
16. Non-finite model inputs are sanitized before they can reach hardware.
17. Directional modulation is sign-symmetric for left and right behavior.
18. Native vehicle direction never directly commands wheel direction.

M4G-R1 runtime validation established that Active hardware selection equals
the M4F-restored directional and unloading values. Earlier M4F/M4G replay
audits found no magnitude, sign, bounds, invalid-state, driver-only, abort-rate,
or non-finite violations.

The Run #58 `M4G_R1_ROUTINE_CHECK` capture provides the final routing and
foundation audit evidence:

- 13,083 samples over 217.83 seconds at 60.06 Hz, all in Active mode;
- exact equality between hardware-selected and M4F-restored directional and
  unloading values on every sample;
- 1,372 active-restoration samples, each measurably distinct from M4C-only
  unloading, with a maximum restoration allowance of 0.0590774;
- zero magnitude, sign, zero-origin, unloading-bound, invalid-state,
  driver-only, abort-rate, or non-finite violations;
- bounded restoration-rate range of -0.08..0.0800001 per second;
- 49 restoring/holding-to-aborting transitions covering failed recovery;
- 120 transition-suppressed samples with no restoration advancement; and
- equivalent directional equation residuals for 4,424 positive and 4,555
  negative Legacy-direction samples, within CSV precision.

The final pre-drive trace also follows the selected directional plus unchanged
road and impact through `tanh` and the existing output ramp. The only larger
pre-drive residual interval follows a 0.635-second game-update pause and shows
the expected 30-update output-ramp restart, not a force-model discontinuity.

## DRM boundary

The HYP36R Dynamics Reference Model is theory used to assess whether an
interpretation is physically coherent. It does not replace OutRun physics,
generate force, invent tire state, create suspension forces, or turn
`responseAngle` into steering torque. Observed OutRun behavior remains the
authority.

## AER boundary

AER means **Arcade Experience Reference**. It is a future presentation
reference built only from validated original arcade force communication and
hardware behavior. It may eventually inform strength, presence, contrast,
texture character, and impact character. It may not redefine LOAD, RELEASE,
FREE, BITE, native validity, vehicle semantics, or driver-versus-vehicle
convergence. No AER dataset or Arcade profile is implemented in M4.

Unvalidated third-party interpreted force models are not AER evidence.

## Vehicle, force, presentation, and hardware

These remain independent layers:

- **Vehicle character:** what measured OutRun state says the car does.
- **Force model:** which validated information HYP36R communicates.
- **Presentation profile:** how that information may eventually be expressed.
- **Hardware calibration:** how normalized output maps to a particular wheel.

Presentation cannot rewrite vehicle meaning, and hardware strength cannot be
used to validate the information architecture.

## Evidence boundaries

The M4 foundation does not prove or claim:

- steering-rack torque;
- self-aligning torque (SAT);
- true tire slip angle;
- true grip percentage;
- independent FL/FR/RL/RR tire-force integration;
- complete suspension-geometry integration;
- a validated AER dataset; or
- universal hardware calibration.

The current foundation uses validated vehicle-level OutRun response state and
derived context. Those limits remain explicit research boundaries.

## Final foundation UAT

Use the M4G-R1 controlled build with `Force2Mode = Active`, Fanatec DD2 hardware
FFB at 50%, and game FFB at 100%. Do not tune strength during the test.

Run one continuous session containing:

1. Normal-grip driving and gentle direction changes.
2. A progressively induced slide through LOAD -> RELEASE.
3. Sustained FREE state with deliberate driver-controlled countersteer/search.
4. A clean catch through BITE to returned LOAD.
5. A failed catch: FREE -> BITE -> renewed separation -> FREE.
6. Equivalent left- and right-direction episodes.
7. Normal road texture and one mild impact.
8. Any naturally occurring transition-suppressed state without trying to
   manufacture one.

Evaluate the communication, not absolute torque:

- Does the car communicate continuously through the wheel?
- Is loss of directional authority progressive and understandable?
- Can the driver search freely during FREE without the wheel correcting the
  slide?
- Does load begin returning progressively during BITE?
- Does a failed recovery remain continuous rather than heavy/light stepping?
- Does returned LOAD feel natural?
- Do left and right behave equivalently?
- Are road texture and impact still distinct and unchanged?
- Does any transition feel like a manufactured effect?

Preserve the telemetry capture and subjective notes together. A foundation
pass requires coherent continuous communication with no safety-invariant
failure or discrete manufactured transition.
