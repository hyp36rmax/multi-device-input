# TP-01 telemetry probe

TP-01 adds a developer-only observation path. It does not change the force
model, input handling or DirectInput output.

Add the following to `OutRun2006Tweaks.user.ini` to record a session:

```ini
[Developer]
TelemetryEnabled = true
```

During gameplay, one sample is taken from the existing player-car update call.
This normally follows OutRun's 60 Hz simulation cadence; no polling thread is
created. Files are written beside `dinput8.dll` as
`telemetry_YYYYMMDD_HHMMSS.csv`. Rows are buffered and flushed every 120
samples, then flushed again during normal shutdown.

The game hook supplies speed, the normalized input-system steering value and
the four untouched `water_flag_24C` entries. Their physical wheel order is not
known, so they remain `surface_0` through `surface_3`.

The wheel output boundary observes the force received from the current model,
the final requested DirectInput level after inversion, master scaling and
clamping, and the configured master multiplier. `ffb_final` is a requested
software output, not measured wheel torque and not proof that a driver accepted
that frame's update.

`xforce` is left empty. No field in the current codebase is confirmed to carry
that meaning.

When telemetry is enabled, the existing Debug tab also shows the latest sample
under **FFB Telemetry**.

## Native force candidates

TP-01B appends seven anonymous `EVWORK_CAR` values that are already consumed by
OutRun's restored Xbox `CalcVibrationValues` routine. Their meanings are
currently unknown. They are recorded as `native_1D0`, `native_1D4`,
`native_1DC`, `native_1E0`, `native_1E4`, `native_264` and `native_268` so they
can be correlated against controlled driving scenarios without assigning
premature semantics.

The complete TP-01B CSV header is:

```csv
timestamp,frame,elapsed_time,speed,steering_input,xforce,surface_0,surface_1,surface_2,surface_3,ffb_raw,ffb_final,ffb_master,native_1D0,native_1D4,native_1DC,native_1E0,native_1E4,native_264,native_268
```

## TP-02C steering-response candidates

Static analysis in TP-02B identified a compact steering/handling cluster at
`EVWORK_CAR + 0xD34` through `+0xD48`. TP-02C appends the six primary raw
observations selected for physical classification:

```text
candidate_D38  float
candidate_D3C  float
candidate_D40  float
candidate_D44  signed 16-bit
candidate_D46  signed 16-bit
candidate_D48  signed 16-bit
```

Current working hypotheses describe `D38` as response/authority-like, `D3C` as
a filtered response/transition state, `D40` as previous-frame directional
state, `D44` as a calculated response consumed by physics-vector generation,
`D46` as bounded signed accumulated steering response, and `D48` as a signed
increment/rate/correction-like state. These are hypotheses, not established
physical meanings.

The TP-02C CSV appends these fields after all existing TP-01 columns. No
candidate feeds HYP36R Force. No native steering torque or Howard X-Force
signal has been proven.

| Offset | Current C/C++ field | Recorded type | Use in the restored Xbox routine |
| --- | --- | --- | --- |
| `0x1D0` | `EVWORK_CAR::field_1D0` | signed IEEE-754 `float` | Preserves the original value and also calculates its absolute value with `Fabsf`; both participate in later threshold and sign comparisons. |
| `0x1D4` | `EVWORK_CAR::field_1D4` | signed IEEE-754 `float` | Compared directly with zero and with the absolute/raw `0x1D0` working values. |
| `0x1DC` | `EVWORK_CAR::field_1DC` | signed IEEE-754 `float` | Compared with zero using an unordered floating-point comparison; no arithmetic transform is applied first. |
| `0x1E0` | `EVWORK_CAR::field_1E0` | signed IEEE-754 `float` | Compared directly with zero; no arithmetic transform is applied first. |
| `0x1E4` | `EVWORK_CAR::dword1E4` | declared unsigned `uint32_t`; consumed and recorded as its IEEE-754 `float` bit interpretation | The assembly performs a floating-point comparison directly against the four bytes at `0x1E4`. TP-01B uses `std::bit_cast<float>` to expose exactly that interpretation without numeric conversion. |
| `0x264` | `EVWORK_CAR::field_264` | signed IEEE-754 `float` | When applicable, subtracts `-0.1`, caps the result at `2.0`, multiplies by native speed and `0.01`, then adds it to the vibration working value. |
| `0x268` | `EVWORK_CAR::field_268` | signed IEEE-754 `float` | Compared with `0.1`; one branch negates it and subtracts `-0.1`, then shares the capped, speed-scaled vibration transform used by `0x264`. |

These values remain neutral observations. Their possible relationship to
Howard Casto's historical OutRun force observations, including the signal he
described as X-Force, is a research question rather than a conclusion.

Controlled captures are intended to cover stationary baseline, stationary and
moving steering sweeps, straight-line speed changes, surface transitions,
normal cornering, progressive understeer, drift/rear rotation, collisions and
gear shifts. TP-01B does not detect or label those tests automatically.

The observation boundary remains deliberately separate from later
interpretation, derived HYP36R Force state and force composition. No candidate
feeds back into the force model.

## Controlled captures

Optional scenario metadata can be added to `OutRun2006Tweaks.user.ini`:

```ini
[Developer]
TelemetryEnabled = true
TelemetryTestScenario = T01_stationary_baseline
TelemetryNotes = DD2 60%, wheel centered, Ferrari F430
```

`TelemetryTestScenario` and `TelemetryNotes` may be left blank. Each new CSV
records their values once in `# test_scenario=` and `# notes=` comment lines;
they do not alter the per-frame schema.

While telemetry is enabled, the Debug tab provides **Start New Capture** and
**Stop Capture** controls. Starting a new capture flushes and closes the current
file, then immediately opens a fresh timestamped CSV. Stopping flushes and
closes the current file and prevents another file from opening until **Start
New Capture** is selected. The overlay shows whether recording is active, the
configured scenario, sample count and current filename.

One CSV per controlled scenario is recommended. The first capture set is:

- **T01 stationary baseline:** hold the stationary vehicle with centered
  steering for approximately ten seconds.
- **T02 steering sweep:** capture centered, partial and full steering in both
  directions, once stationary and once at a stable moderate speed.
- **T03 speed sweep:** keep steering near center while progressing through low,
  moderate and high straight-line speed.

The seven native candidates retain neutral names and unknown semantics. Capture
management does not classify scenarios or interpret their values.

If the capture controls are missing while telemetry should be enabled, first
check the effective INI configuration for duplicate `[Developer]` sections or
duplicate `TelemetryEnabled`, `TelemetryTestScenario` or `TelemetryNotes`
entries. The working configuration contains exactly one of each. This
configuration duplication has previously explained missing controls; UI and
capture-backend investigation should wait until it has been ruled out.

## M3B passive vehicle-state interpreter

M2 Native Signal Discovery is complete. M3 begins with a passive semantic
interpreter between the validated native steering-response fields and future
HYP36R Force development. M3B observes the existing TP-02C values, converts
the confirmed signed-angle representations to radians, derives the wrapped
reference/response separation, and explicitly identifies the native 30-update
transition-suppression period.

The interpreter uses the actual elapsed time between player-car update calls
for D48's diagnostic angular-rate conversion. The first sample, intervals
shorter than 1/240 second, and intervals longer than 100 milliseconds are
marked timing-invalid instead of producing an extreme rate.

M3B appends these semantic diagnostic columns after the unchanged TP-02C raw
candidate columns:

```text
state_validity
steering_reference_rad
response_angle_rad
response_rate_rad_s
response_rate_valid
reference_response_error_rad
corrected_reference_rad
response_authority
overshoot_attenuation
transition_frames_remaining
last_valid_state_age_s
response_rate_utilization
response_rate_utilization_valid
```

Suppressed or unavailable dynamic values are emitted as empty CSV cells. The
current state remains current; the retained last-valid state is not silently
substituted. The vehicle-specific D48 limit is used only for the optional
diagnostic utilization value and does not normalize vehicle behavior.

The implementation preserves three distinct layers:

```text
native observed state
!= derived semantic state
!= synthetic force
```

No semantic value feeds the force composer in M3B. No field is claimed as
X-Force, steering torque, self-aligning torque, tire force, or grip percentage.

## M3E synchronized native vs synthetic telemetry

M3E appends the existing production HYP36R synthetic vehicle-state values to
the same CSV row as the raw and semantic native state:

```text
synthetic_lateral_speed
synthetic_slip_ratio
synthetic_grip_loss
```

These values are not recomputed by the telemetry probe. The player-car update
passes the exact `lateralSpeed`, `slipRatio`, and `gripLoss` locals already used
by the current force calculation. The semantic observation and telemetry call
occur in that same hooked update, after the force request and before the
original game update is resumed, so instrumentation introduces no one-frame
lag between native, synthetic, and force observations.

M3E exists only to allow direct offline comparison of validated native semantic
state against the existing production HYP36R synthetic vehicle-state
calculations. It does not change force behavior.

## M4B passive HYP36R Force 2.0 shadow composer

M4A architecture is approved. M4B introduces a passive Force 2.0 shadow
composer that assesses native availability and event context, creates normalized
force intent, applies provisional shadow budgets, and records the prospective
result for offline comparison.

Shadow output is never sent to hardware. The existing legacy `force` remains
the sole value passed to `WheelForceFeedback::drive`, so M4B does not change
wheel feel. All divergence normalization, persistence, native-weight slew,
unloading, component budgets, and output slew values are conservative shadow
research constants rather than production tuning.

The current research constants use 0.50 rad as full-scale divergence, require
three persistent updates above 10% normalized divergence for `emerging`, and
classify the established synthetic state at 0.08 slip ratio or nonzero derived
grip loss. Native confidence and prospective output slew by 1/30 per update;
native unloading is capped at 25%. Directional, texture, impact, and total
shadow budgets are respectively 1.0, 0.25, 0.55, and 1.0 normalized units.
These values exist to make shadow captures deterministic and are not approved
production gains or timings.

M4B appends these fields:

```text
composer_mode
composer_native_availability
composer_native_weight
composer_event_phase
composer_recovering
intent_directional
intent_unloading
intent_motion
intent_road
intent_impact
legacy_directional_component
force2_shadow_directional
shadow_texture_component
shadow_impact_component
shadow_pre_budget
shadow_post_budget
shadow_rate_limit_active
shadow_headroom_limit_active
shadow_pre_master
force2_shadow_output
shadow_minus_legacy
```

The existing `ffb_final` column is the same-cycle legacy hardware request and
therefore serves as `legacy_output` without adding a duplicate column.

The active experiment planned from M4B was M4C divergence unloading.
Response-angle target movement remains postponed until after M4D, and
response-rate force remains postponed until M4F. In M4B response angle and
bounded response rate are diagnostic context only.

## M4C first active experiment

M4C makes one validated Force 2.0 behavior available for controlled physical
testing: early divergence-based unloading of the existing directional force.
It does not add torque. The native reference/response error is normalized at
the provisional 0.50 rad research scale, blended through the existing M4B
native-validity weight, and can remove at most 25% of the legacy directional
subtotal. The operation cannot increase its magnitude, reverse its sign, or
create directional force from zero.

Select the restart-required developer mode in `OutRun2006Tweaks.ini`:

```ini
[Developer]
Force2Mode = Legacy
```

Accepted values are `Legacy`, `Shadow`, and `Active` (case-insensitive).
Missing or invalid values safely select `Legacy`.

- `Legacy`: the exact existing directional + impact + road calculation reaches
  the existing tanh, output ramp, master strength, inversion, and DirectInput
  safety path.
- `Shadow`: hardware still receives that exact legacy calculation while the
  complete M4B shadow composer is recorded separately.
- `Active`: only the directional subtotal is replaced with its bounded unloaded
  value. The existing impact and road values are then added unchanged before
  the same tanh, ramp, master, inversion, and DirectInput safety path.

Response angle remains diagnostic and does not move the steering target.
Response rate remains diagnostic and generates no force. Event phase,
authority, and overshoot remain context only. Road texture, tire-slip
vibration, and impact behavior are unchanged by the unloading calculation.
The 0.50 rad scale and 25% ceiling remain provisional M4C test values.

M4C appends three columns to the preserved M4B schema:

```text
legacy_force_output
active_directional_component
active_unloading_applied
```

`composer_mode` identifies the selected mode. `ffb_raw` is the exact normalized
force passed into the existing wheel output boundary after mode selection;
`ffb_final` is the hardware-bound request after master strength, inversion, and
final clamping. `legacy_force_output` is the counterfactual legacy request after
those same final stages, even in Active mode. Together these fields distinguish
what legacy would have produced, the directional change, the unloading amount,
and what was requested from DirectInput without duplicating the hardware trace.

## M4E passive BITE detector

M4D established an observable BITE/load-reacquisition region: returning
directional load capability while meaningful dynamic state remains. M4E adds a
passive semantic detector for that region. BITE is not grip percentage, a
centering command, a request to steer toward `responseAngle`, or an added force.
M4C hardware behavior and its 0.50-rad/25% unloading experiment are unchanged.

The detector uses a nine-valid-sample moving average, approximately 150 ms at
60 Hz, for absolute reference/response error, reference rate, and response
rate. A candidate must then remain qualified for eight valid updates,
approximately 133 ms, before becoming BITE. All values below are provisional
M4E research thresholds.

A candidate requires:

- valid native error and response-rate state;
- filtered absolute error of at least 0.10 rad;
- at least one existing dynamic indicator: 0.15 lateral speed, 0.08 slip ratio,
  or 0.01 derived grip loss;
- error closing at least 0.08 rad/s;
- vehicle convergence of at least 0.03 rad/s; and
- vehicle convergence greater than driver convergence.

With `error = reference - response`, positive closing contributions are:

```text
vehicleConvergence = sign(error) * responseAngularRate
driverConvergence  = -sign(error) * referenceAngularRate
```

This prevents decreasing error caused only by steering/reference movement from
being identified as BITE. Driver, vehicle, and combined convergence are also
classified for offline auditing.

After BITE, returned load requires 15 consecutive valid updates with error
below 0.05 rad, lateral speed below 0.15, slip below 0.08, and grip loss below
0.01. The returned state is retained for 30 updates for visibility. Candidate
state is cancelled by an invalid native sample. Active BITE confidence decays
during invalid native state and the entire detector resets after eight invalid
updates. Invalid samples never prime or activate the detector.

Confidence is a bounded 0..1 diagnostic combination of error magnitude,
closing rate, vehicle convergence, vehicle-over-driver dominance, dynamic
context, and candidate persistence. It is never mapped to force.

M4E appends:

```text
bite_state
bite_candidate
bite_active
bite_confidence
bite_error_magnitude
bite_error_closing_rate
bite_vehicle_convergence
bite_driver_convergence
bite_age_s
bite_dynamic_context
bite_convergence_source
```

The passivity boundary is one-way:

```text
native + synthetic observations
→ passive BITE detector
→ telemetry only
```

No detector value is accepted by `ForceIntent`, unloading, spring, damper,
road, impact, `activeDirectional`, `WheelForceFeedback::drive`, or DirectInput.

## M4F passive BITE-informed restoration shadow

M4F calculates an alternative unloading trajectory for research only. Before
BITE, shadow unloading exactly equals current M4C unloading. During a valid,
credible BITE, the model may progressively release part of the unloading that
M4C still applies, including while raw divergence remains above M4C's 0.50-rad
saturation region. It never adds force beyond the Legacy directional value.

The provisional shadow model advances only when confidence is at least 0.20,
error is closing, and vehicle convergence exceeds positive driver convergence.
Confidence above 0.20 permits a proportional build rate up to 0.08 unloading
units per second. Early restoration is capped at 60% of current M4C unloading,
so meaningful unloading remains during BITE/FREE even at maximum confidence.

If BITE remains active but immediate evidence weakens without renewed
separation, restoration holds. If error begins separating faster than 0.08
rad/s, BITE ends without RETURNED state, or native state becomes invalid, the
allowance returns toward M4C at 0.04 unloading units per second. RETURNED state
converges the allowance at 0.08 units per second. None of these shadow research
constants affect hardware.

M4F appends:

```text
bite_shadow_phase
bite_shadow_active
bite_shadow_current_m4c_unloading
bite_shadow_unloading
bite_shadow_load_restoration
bite_shadow_directional
bite_shadow_restoration_rate
bite_shadow_limiter_active
bite_shadow_abort_active
```

`ffb_raw` and `ffb_final` remain the unmodified M4C hardware path. The M4F
fields are calculated after M4C composition and are forwarded only to the
telemetry probe.

## M4G active BITE-informed load restoration

M4G is the first complete active HYP36R grip-envelope candidate. It changes
only Active mode: the directional subtotal now uses the exact M4F
`bite_shadow_directional` result instead of the earlier M4C-only directional
result. No restoration algorithm or constant is recreated or retuned.

The active directional path is:

```text
M4C unloading
-> M4F BITE restoration allowance
-> final unloading = M4C unloading - allowed restoration
-> active directional = Legacy directional * (1 - final unloading)
```

The early-restoration limit remains 60% of current M4C unloading. At the M4C
0.25 ceiling, at least 0.10 unloading therefore remains during early BITE.
Aborted recovery, invalid-state suppression, and RETURNED convergence are the
same validated M4F stateful behavior. Native vehicle direction changes load
capability only; it is never used as a steering target.

The preserved telemetry schema already reconstructs why wheel load changed:

- `bite_shadow_current_m4c_unloading` is the M4C RELEASE amount;
- `bite_shadow_load_restoration` is the allowed M4F restoration;
- `bite_shadow_unloading` and `bite_shadow_directional` are the final Active
  unloading and directional subtotal;
- `ffb_raw` is the actual force passed to `drive()` after unchanged road,
  impact, `tanh`, and output ramp;
- `ffb_final` is the actual master/inversion/clamp result requested from
  DirectInput; and
- `legacy_force_output` is the counterfactual Legacy result after those same
  final output stages.

No duplicate columns are added. The probe version is M4G; all M4F columns and
their meanings remain intact.

Legacy and Shadow still send the exact Legacy hardware result. Active alone
uses M4F's restored directional subtotal, then adds the unchanged road and
impact channels before the existing `tanh`, output ramp, master strength,
inversion, clamp, and DirectInput stages. Tire-slip vibration remains on its
existing independent path.

M4G is **more information, not more torque**: active directional magnitude
cannot exceed Legacy or reverse its sign. It provides continuous vehicle
communication rather than a manufactured BITE effect. HYP36R Force
communicates the inertia of grip; it does not fight the slide.

The HYP36R Dynamics Reference Model remains interpretive. M4G does not add
tire physics, AER presentation behavior, profiles, explicit four-corner tire
forces, or suspension geometry. Observed OutRun behavior remains authoritative.
