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
