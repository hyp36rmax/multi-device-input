# HYP36R Debug Control Inventory

This inventory describes the Debug tab on `feature/aer-arcade-profile`. It separates visible controls from configuration-only state and preserves existing keys and behavior.

## Device & Input

| Display | Configuration/source | Default / values | Runtime behavior | Status |
| --- | --- | --- | --- | --- |
| Active Backend | `Controls.InputBackend` plus existing startup policy | `0`; Automatic, RawInput, DirectInput, XInput | Read-only description of the backend selected at startup | Visible |
| Requested Backend | `Controls.InputBackend` | Same values | Read-only; changes remain owned by the existing Controls workflow | Visible |
| Input Devices | `InputManager::devices` | Runtime count | Registered SDL devices available to bindings | Visible |
| FFB Devices | `WheelForceFeedback::devices()` | Runtime count | Native DirectInput FFB endpoints; independent of SDL input count | Visible |
| Discovery Status | `InputManager::devices` | No devices / registered | Read-only current-session summary | Visible |
| Discovery Details | SDL device registry | Device names | Read-only; collapsed to keep the workspace compact | Collapsed |

INPUT-COMPAT-U03 is not forward-ported here. The v2 branch retains its existing startup behavior until the v1.5.1 resolver work receives physical validation.

## FFB Diagnostics

| Display/control | Configuration/source | Default / values | Runtime behavior | Status |
| --- | --- | --- | --- | --- |
| Selected FFB Endpoint | `WheelForceFeedback::active_device_id()` | Runtime | Resolves the active ID through the enumerated endpoint list | Visible |
| FFB Readiness | `WheelForceFeedback::ready/status` | Runtime | Read-only current native FFB state | Visible |
| Actuator Axes | DirectInput enumerated actuators | Runtime count | Read-only; reports the axes used by the current endpoint | Visible |
| Current Output Path | Active DirectInput effect | Inactive, Stopped, two-axis constant force, one-axis fallback | Read-only; does not alter delivery | Visible |
| Diagnostic Logging | `Controls.WheelFFBDiagnosticLog` | `false`; boolean | Persists existing diagnostic logging setting | Visible |
| Re-detect FFB Devices | Existing `WheelForceFeedback::refresh()` | Action | Stops output safely and re-enumerates through the existing implementation | Visible |
| Latest API status | Existing FFB status producer | Runtime text | Read-only; no new error channel was invented | Visible |

Player Strength and Invert are intentionally not duplicated here.

## Telemetry & Capture

| Display/control | Configuration/source | Default / values | Runtime behavior | Status |
| --- | --- | --- | --- | --- |
| Enable Telemetry | `Developer.TelemetryEnabled` | Existing default; boolean | Existing persistence and sampling gate | Visible |
| Scenario | `Developer.TelemetryTestScenario` | Existing string | Existing session metadata | Visible |
| Notes | `Developer.TelemetryNotes` | Existing string | Existing session metadata | Visible |
| Start/Stop Capture | `TelemetryProbe` | Action | Uses the existing capture service and output path | Visible |
| Capture Status / Output | `TelemetryProbe::snapshot()` | Runtime | Reports recording state and current filename | Visible |
| Compact overlay | Existing automatic telemetry overlay | Automatic | Appears while capturing; no duplicate visibility setting | Documented |
| Guided UAT launchers | Existing `GuidedUat` requests | Road and Surface protocols | Existing workflows unchanged | Visible |
| SimHub Telemetry | `SimHubLive::diagnostics()` | Runtime | Existing local UDP diagnostic state | Collapsed |
| FFB Telemetry | `TelemetryProbe::snapshot()` | Runtime | Detailed research values; only shown when telemetry is enabled | Collapsed |
| Advanced Output Telemetry | Existing FFB sample | Unavailable, Zero, Active | Owns Final Force presentation without fabricating a zero | Collapsed |

## Engineering

Engineering is collapsed by default. Its children remain functional and retain their existing persistence.

### Surface Tuning

| Control | Key/source | Default / valid values | Status |
| --- | --- | --- | --- |
| Road Detail Authority | `Developer.Road2ArcadeAuthority` | Existing boolean | Experimental, visible |
| Authority Multiplier | `Developer.Road2DebugAuthorityGain` | ×8, ×10, ×15, ×20, ×25, ×30 | Experimental, conditional |
| Texture Ceiling | `Developer.SurfaceAmplitudeCeiling` and override | 12%, 18%, 25% in this branch | Experimental, visible |
| Waveform | `Developer.SurfaceWaveform` | Sine, Triangle, Square | Experimental, visible |
| Frequency Profile | `Developer.SurfaceFrequencyProfile` | Low, Reference, Medium, High | Experimental, visible |
| Surface capability/request | Native FFB surface status | Runtime | Read-only |
| Research reset | Existing FFB configuration reset | Confirmed action | Visible; research settings only |

Player Surface Strength and renderer selection remain player-facing owners and are not duplicated as new Debug controls. Existing bump settings are configuration-compatible but hidden because the release model derives Bump from player Surface; no retired A/B control is restored.

### Road Research

The native sound-request ownership capture remains implemented and functional. Asphalt, grass, sand, cobblestones, curbs, and transitions remain research candidates; this interface does not claim that raw identifiers have proven material semantics.

### Audio Sync

The stationary audio/telemetry synchronization workflow remains implemented. It observes timing and sidecar markers only. It does not generate force from audio and adds no new audio hook.

## Legacy and configuration-only research

Force2 interpretation, M5 lateral work, older Road calibration, saved Surface preferences, and historical Debug overrides remain in code, settings, telemetry, and documentation where still referenced. This milestone does not delete keys or reset values. Superseded compatibility settings remain hidden rather than presented as active controls.
