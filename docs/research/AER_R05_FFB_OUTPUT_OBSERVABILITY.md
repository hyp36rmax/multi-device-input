# AER-R05 — End-to-End FFB Output Observability

This research-only instrumentation observes the existing HYP36rforce output
path. It does not change a force equation, limit, update cadence, effect
lifetime, device selection rule, or safety action. DirectInput success means
that the API accepted a request; it does **not** prove physical wheel torque.

## Signal lineage

| Stage | Implemented source | Units / range | Availability and cadence |
|---|---|---|---|
| Steering input | `EVWORK_CAR::steer_input_raw_78` read in `CalcLoop()` (`hooks_forcefeedback.cpp`) | normalized input, approximately -1..1 | player car update |
| Speed | `EVWORK_CAR::speed_10` | native game speed | player car update |
| Native response state | D38–D48 fields interpreted by `HYP36RVehicleState::Interpreter` | radians, rad/s, confidence | valid player car state; explicit validity |
| Raw road evidence | restored vibration values and Road 2.0 policy inputs | normalized internal values | player car update; not a material identity |
| Raw impact evidence | vibration rise / impact path in `CalcLoop()` | normalized internal value | event dependent |
| Raw grip evidence | derived synthetic slip/grip-loss and native response interpretation | normalized, derived | not native tire telemetry |
| Directional component | `directionalPreGain` / `directionalPostGain` | normalized force request | player car update |
| Road component | `roadPreGain` through `roadPostSafetyCeiling` | normalized force request | selected road renderer and safety state |
| Impact component | `impactPreGain` / `impactPostGain` | normalized force request | event dependent |
| AER interpretation | `HYP36RAer::AerTelemetry` | normalized requests plus validity | only when Arcade Experience is selected; evidence gates remain authoritative |
| Combined / pre-limit | `composerPreTanh`, `forcePreDrive` | normalized force request | player car update |
| Post-limit | `composerPostTanh`, output exposure clamp state | normalized -1..1 | player car update |
| Final requested force | `WheelForceFeedback::drive()` after invert, master gain and clamp | normalized -1..1 | only when the output boundary is reached |
| DirectInput request | actual descriptor magnitude and direction passed to `CreateEffect` / `SetParameters` / `Start` | DirectInput nominal units, direction hundredths of degrees for polar | only for an attempted API operation |
| API result | returned `HRESULT` from the existing call | HRESULT | actual API call only |

There is no separate implemented “grip force” component. Grip-related values
remain inputs or derived context, and are not relabelled as a force.

## Existing output paths

- Two-axis constant force uses a polar descriptor and persistent
  `SetParameters` updates.
- The proven fallback uses a one-axis Cartesian constant descriptor and effect
  recreation at the existing 66 ms (15 Hz compatibility) interval.
- Surface uses the selected Sine, Triangle, or Square periodic descriptor and
  persistent updates.
- Bump is an existing one-shot Cartesian constant effect.

The observer records which path was actually selected. It adds no probe calls
to DirectInput and performs no manufacturer inference.

## Final Force availability finding

`WheelForceFeedback::drive()` produced the value and `observe_ffb()` stored it
in a pending slot. `TelemetryProbe::sample()` published that slot only after a
capture had been requested. The Advanced Output Telemetry view therefore saw
`Unavailable` during ordinary hardware sessions even though the wheel output
boundary had received a request. R05 publishes that same authoritative value
to the live snapshot immediately while retaining the pending, frame-aligned
CSV path. Telemetry disabled or an output boundary that has not been reached
still reports unavailable; zero is never substituted for missing evidence.

## R05 schema and timing

`HYP36R_RESEARCH_II_R1_AER_OUTPUT_V11` contains 294 append-only columns. The 26
R05 fields record game-update/calculation/combination timing, availability,
force request, descriptor/path, axis count,
device readiness, safety state, last API operation/result, monotonic submission
timestamp, interval, requested interval, jitter, recreation/persistent update/
watchdog counters, and bounded-buffer accounting. Session metadata also records
the actuator count and output strategy. Existing product version and build
commit metadata identify the session.

The observer uses fixed storage (256 records), never blocks or writes files in
the force path, and reports overwritten records through `r05_dropped_samples`.
It is enabled by the existing developer telemetry switch and remains disabled
by default.

## Correlation limits and future research

Existing Road, Surface, Bump, AER and composer fields can be aligned with the
monotonic DirectInput submission timestamp. Existing audio-sync observations
can be correlated offline, but R05 adds no audio hook. Surface-transition and
material meanings remain candidates unless separately validated; this work
does not claim grass, sand, asphalt, cobblestone, or kerb identity.

Physical UAT must confirm only that the new observations match real sessions.
It must not treat an accepted HRESULT as evidence of torque magnitude or
arcade-hardware fidelity.
