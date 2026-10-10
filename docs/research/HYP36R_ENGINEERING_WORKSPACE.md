# HYP36R Engineering Workspace

The Debug tab is organized around four jobs:

1. **Device & Input** — inspect the active startup input path and registered devices.
2. **FFB Diagnostics** — inspect the native FFB endpoint, readiness, actuator axes, output path, logging, and re-detection.
3. **Telemetry & Capture** — manage the existing capture service, metadata, Guided UAT, SimHub diagnostics, detailed FFB telemetry, and advanced output telemetry.
4. **Engineering** — access Surface Tuning, Road Research, and Audio Sync without crowding normal diagnostics.

The first three sections open by default. Engineering and detailed telemetry are collapsed by default.

## Final Force ownership

Final Force uses the existing `TelemetryProbe` sample. It was removed from the compact player telemetry presentation and placed under **Telemetry & Capture → Advanced Output Telemetry**. The view reports:

- **Unavailable** when no force sample is available;
- **Zero** when a valid published sample is exactly zero; and
- **Active** when a valid nonzero sample is present.

The UI does not manufacture a value or modify the producer to populate it. `Stopped` and `Inactive` are used by FFB output-path diagnostics, where those states are known from the DirectInput effect lifecycle.

## Configuration compatibility

All existing setting keys, defaults, ranges, and write paths are preserved. The reorganization calls existing services for capture, FFB refresh, Guided UAT, Road authority, Surface waveform/frequency, and research reset. No force equation, gain, limiter, transport, telemetry schema, or migration rule changes.

## Input compatibility dependency

The universal startup resolver remains isolated on `dev/v1.5.1-input-compatibility`. It has not been copied into the v2 branch because physical Sora and DD2 validation is still pending. The workspace reports the v2 branch's existing backend state and explicitly identifies that dependency.

## Research boundaries

- **Implemented:** current device/FFB diagnostics, capture controls, Guided UAT, Surface tuning, sound-request capture, and audio synchronization.
- **Experimental:** AER profile, Road authority, Surface waveform/frequency/ceiling, and the active Road research presentation.
- **Research candidates:** material classification and correlations involving asphalt, grass, sand, cobblestones, curbs, surface transitions, audio cues, and impacts.
- **Archived/hidden:** superseded compatibility controls whose keys or readers remain for history and migration.

The workspace does not create placeholder controls for unimplemented research.
