# Device Diagnostics changelog

## 0.2.0-dev.6

- Stores Quick Setup input assignments with stable SDL identity evidence and restores the assigned steering axis after SDL re-enumeration.
- Fails closed when the assigned device is missing or an identity is ambiguous; duplicate names never authorize silent substitution.
- Reports missing-device, ambiguous-identity, invalid-axis, and unavailable-axis states separately in Loaded Steering Response.
- Preserves the physically verified dev.5 DirectInput FFB descriptors, Re-detect ordering, force behavior, and safety limits.

## 0.2.0-dev.5

- Corrected Re-detect so SDL DirectInput refresh completes before the exact saved native FFB endpoint is reacquired and its actuators are enabled.
- Added fail-closed endpoint restoration and readiness checks for data format, cooperative level, acquisition, and actuator enablement.
- Reacquires and enables actuators immediately before an explicitly authorized Left, Right, or Shake request without changing force descriptors, magnitude, inversion, or safety limits.
- Preserved report schema `HYP36R_DEVICE_DIAGNOSTICS_V1`.

## 0.2.0-dev.4-rollback

- Restored the physically validated 0.2.0-dev.1 diagnostic implementation from commit `1518f82`.
- Removed the active dev.2 M10 compatibility experiment and the dev.3 DD2 fallback experiment while preserving both commits in Git history.

## 0.2.0-dev.1

- Added independent Device Diagnostics semantic version and compiled source identity.
- Added build identity to the application header, About panel, and TXT/JSON reports.
- Preserved report schema `HYP36R_DEVICE_DIAGNOSTICS_V1` independently from the application version.
- Includes the reviewed device, input, Quick Setup, FFB delivery, and Loaded Steering diagnostic workflows through P11-E.
