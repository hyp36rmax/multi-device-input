# S2 passive output exposure and headroom telemetry

S2 observes the existing output path without changing it. It measures normalized
software-output exposure. It does not measure physical wheel torque, motor
current, motor temperature, PSU load, thermal headroom, or hardware protection
margin.

The starting baseline is S1 commit
`4834f1c0bd3bd8bb7f4946e744fa36bd2c6fe150`.

## Measurement points

The observer runs after M4/M5 directional selection and immediately before the
unchanged `WheelForceFeedback::drive(hardwareForce)` call.

```text
s2_composer_input = hardware-selected directional + impact + road
s2_post_tanh = tanh(s2_composer_input)
ffb_raw = s2_post_tanh * existing output ramp
ffb_final = existing master, inversion, and DirectInput clamp result
```

`ffb_raw` and `ffb_final` remain the authoritative existing fields and are not
duplicated.

## Observation windows

The player-car Force path updates at a nominal 60 Hz in validated captures. S2
uses fixed sample windows to keep the real-time observer deterministic:

- recent peak: 30 samples, approximately 500 ms;
- sustained RMS: 60 samples, approximately 1 second;
- sustained RMS and occupancy: 180 samples, approximately 3 seconds.

These are provisional research windows, not safety thresholds. Startup windows
use the samples available so far. A pause longer than 500 ms resets the observer
with the existing Force-state reset.

## Metric definitions

All magnitude metrics use the authoritative pre-drive value.

```text
instantaneous magnitude = abs(ffb_raw)
normalized headroom = clamp(1 - abs(ffb_raw), 0, 1)
recent peak = max(abs(ffb_raw)) over 30 samples
sustained RMS = sqrt(mean(ffb_raw^2)) over 60 or 180 samples
occupancy_N = fraction of the 180-sample window with abs(ffb_raw) >= N
output slew = abs(current ffb_raw - previous ffb_raw) / valid dt
```

The occupancy bands 0.50, 0.75, 0.90, and 0.98 are observation bands only.
They trigger no behavior. Slew is normalized output change per second, not
wheel angular velocity or motor torque slew. Non-finite inputs are observed as
zero; invalid, non-positive, or greater-than-500-ms `dt` produces zero slew.

`s2_pre_tanh_over_unity` records a composer request above unit magnitude before
nonlinear compression. `s2_near_boundary` records pre-drive magnitude at or
above 0.98. `s2_directinput_clamp_active` records whether the existing scaled
request exceeds unit magnitude before the existing DirectInput clamp.

## CSV fields

S2 appends:

```text
s2_composer_input
s2_post_tanh
s2_instantaneous_magnitude
s2_normalized_headroom
s2_recent_peak_500ms
s2_sustained_rms_1s
s2_sustained_rms_3s
s2_occupancy_50_3s
s2_occupancy_75_3s
s2_occupancy_90_3s
s2_occupancy_98_3s
s2_output_slew_per_s
s2_pre_tanh_over_unity
s2_near_boundary
s2_directinput_clamp_active
```

Existing M4 phase, unloading, BITE/restoration, M5 mode and modulation, road,
impact, `ffb_raw`, and `ffb_final` fields remain synchronized in the same row.

## Runtime cost

The observer owns one fixed 180-float ring buffer and a small frame structure.
It performs bounded scans of at most 180 samples per update. It allocates no
memory, performs no I/O, blocks no thread, and changes no Force value. CSV I/O
continues through the existing telemetry buffer only when telemetry is enabled.

## Passivity boundary

The S2 frame is written by the observer and read only by telemetry. No S2 field
is read by M4, M5, spring, damper, road, impact, output ramp, `drive()`, or
DirectInput. The exact pre-S2 `hardwareForce` variable is still passed to
`drive()` without replacement or modification.

## Synthetic validation

The fixed observer was exercised with deterministic 60 Hz traces:

- zero output;
- steady 0.25, 0.75, and 1.0;
- one-frame unit transient;
- short high-output burst;
- sustained high load;
- alternating positive/negative output;
- linear ramp up and down;
- pre-tanh input above unity;
- zero, negative, excessive, and non-finite `dt`;
- non-finite Force inputs.

Validation requires finite outputs, headroom and occupancy within 0..1, full
headroom at zero, sign-symmetric magnitude metrics, a captured one-frame peak,
lower sustained RMS for a short transient than a steady high load, correct
observation flags, and zero slew for invalid timing.

## Runtime capture procedure

Use the S2 controlled build with the established research hardware baseline:

```ini
[Developer]
Force2Mode = Active
M5LateralMode = M4_ONLY
TelemetryEnabled = true
TelemetryTestScenario = S2_DINO_M4_OUTPUT_EXPOSURE
```

For the current DD2 baseline, retain hardware FFB at 50% and game FFB at 100%.
These settings identify the test configuration; normalized output must not be
interpreted as physical torque or protection margin.

Run one continuous session containing:

1. stationary baseline;
2. normal low and medium-load driving;
3. a sustained high-speed directional-load interval;
4. LOAD through RELEASE and FREE;
5. BITE and returned load;
6. one mild impact and ordinary road detail;
7. left and right turns;
8. a return to low output before stopping capture.

Repeat with `M5LateralMode = M5_LATERAL_ACTIVE` and scenario
`S2_DINO_M5_OUTPUT_EXPOSURE`. Keep the route, car, hardware, and strength fixed.
The comparison asks whether current M5 materially changes exposure; it does not
authorize amplitude changes.

## S3 decision criteria

Proceed to an S3 passive analysis milestone only after captures show that:

- every S2 field is finite and internally consistent;
- `s2_instantaneous_magnitude` equals `abs(ffb_raw)` within CSV precision;
- headroom, RMS windows, occupancy, peak retention, slew, and clamp flags
  replay from the raw trace;
- transients and sustained load separate meaningfully under the provisional
  windows;
- M4/M5 routing and final DirectInput requests remain unchanged;
- the M4-only and M5-active comparison is repeatable enough to assess whether
  M5 changes exposure.

S3 should analyze and validate these measurements. It should not amplify,
limit, allocate channels, create profiles, or change hardware output.

