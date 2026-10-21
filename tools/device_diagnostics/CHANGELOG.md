# Device Diagnostics changelog

## 0.2.0-dev.4-rollback

- Restored the physically validated 0.2.0-dev.1 diagnostic implementation from commit `1518f82`.
- Removed the active dev.2 M10 compatibility experiment and the dev.3 DD2 fallback experiment while preserving both commits in Git history.

## 0.2.0-dev.1

- Added independent Device Diagnostics semantic version and compiled source identity.
- Added build identity to the application header, About panel, and TXT/JSON reports.
- Preserved report schema `HYP36R_DEVICE_DIAGNOSTICS_V1` independently from the application version.
- Includes the reviewed device, input, Quick Setup, FFB delivery, and Loaded Steering diagnostic workflows through P11-E.
