# Device Diagnostics changelog

## 0.2.0-dev.2

- Separated Quick Setup workflow completion from assigned, skipped, unresolved, and cancelled binding outcomes.
- Added noise-aware, rotation-independent steering-axis detection with ambiguity rejection and explicit confirmation.
- Added stable diagnostic input identity across reconnect and re-enumeration.
- Added a bounded two-axis-polar versus one-axis-Cartesian directional FFB comparison for M10 validation.
- Advanced paired TXT/JSON exports to `HYP36R_DEVICE_DIAGNOSTICS_V2`.

## 0.2.0-dev.1

- Added independent Device Diagnostics semantic version and compiled source identity.
- Added build identity to the application header, About panel, and TXT/JSON reports.
- Preserved report schema `HYP36R_DEVICE_DIAGNOSTICS_V1` independently from the application version.
- Includes the reviewed device, input, Quick Setup, FFB delivery, and Loaded Steering diagnostic workflows through P11-E.
