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
