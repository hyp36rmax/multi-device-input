# Device Diagnostics P06 — DirectInput update compatibility

## Historical source finding

At v1.0.0 commit `8d67c16792a386d5c3a03d42f2316115bb516abc`, `src/wheel_force_feedback.cpp` first attempts a two-axis polar constant-force descriptor. If that creation fails, it creates a one-axis Cartesian descriptor. Live output created through that fallback is replaced and restarted no more often than every 66 ms, reported as “15 Hz compatibility mode.” The selection condition is descriptor/actuator-layout fallback, not a demonstrated `SetParameters` rejection.

Current production code retains the same distinction: a successfully created two-axis live effect uses persistent `SetParameters`; a one-axis fallback uses replacement effects at the 66 ms boundary. P06 does not alter either path.

## Diagnostic comparison

The standalone diagnostic uses one deterministic bounded sequence for both strategies: zero, gradual positive force, return to zero, directional reversal, and final zero. The legacy strategy recreates and starts a one-axis constant-force effect for every sample. The dynamic strategy creates one effect and updates its type-specific force parameters. Both remain limited by the diagnostic 20% nominal ceiling and 1.5-second watchdog.

Software simulation validates lifecycle accounting without opening DirectInput hardware. Optional physical checks require separate authorization for each strategy and ask the user whether the force changed as expected. API success, simulated success, and physical observation remain distinct evidence classes.

## Interpretation boundary

One actuator axis does not prove legacy recreation is necessary. Two axes do not prove persistent updates are physically effective. P06 can establish whether the selected driver accepts each API path and can record a human observation, but it cannot measure wheel torque or safely generalize one result to every device.
