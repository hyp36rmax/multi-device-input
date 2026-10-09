# Arcade Experience (AER) experimental profile

## Research authority and versioned sync

Original Sega game-side evidence is mirrored at [Current AER Research](research/aer/outrun-2-sp/current/README.md), pinned to LinuxLoader source commit `063bfbcbd0`; the [AER profile blueprint](research/aer/outrun-2-sp/current/AER_PROFILE_IMPLEMENTATION_BLUEPRINT.md) distinguishes verified arcade requests from our modern force synthesis. This document describes **experimental implementation in this development branch**, not shipped v1.5 functionality or confirmed Sega motor torque.

## Status

- **Target:** v2.0 RC
- **Current status:** experimental research and development
- **Hardware validation:** pending for the latest wheel-direction controls
- **Research maturity:** ongoing
- **Release readiness:** not established

Arcade Experience Reconstruction is an opt-in, experimental HYP36rforce FFB
profile on `feature/aer-arcade-profile`. It is not part of v1.5. Reference+
remains the default and fallback, and selecting AER does not rewrite the
player's Reference+ Force Character settings.

This is an independent modern interpretation informed by the original Sega
game-side research preserved under `docs/research/aer/outrun-2-sp/`. It is not
a reproduction of drive-board firmware, cabinet torque, or original waveforms.
The LinuxLoader Arcade Experience Framework remains an independent research
project: its observations can inform this work, but there is no runtime
dependency and implementations are not copied between projects.

## Verified OutRun 2006 evidence mapping

| AER input | OutRun 2006 source | Use | Limit |
|---|---|---|---|
| Steering and normalized speed | Existing player input and vehicle speed observation | Direction and speed authority | Steering is player input, not native torque |
| Response angle and authority | `HYP36RVehicleState` via `HYP36RSignalState` | Continuous steering response | Derived from verified PC state; not Sega `EVWORK_CAR+0x054` |
| Surface values and transitions | Four current `water_flag_24C` values in `HYP36RSignalState` | Short transition events | Values are not assigned Sega material ordinals |
| Mixed contact | Existing four-corner occupancy classification | Contact attenuation/asymmetry event | Wheel/corner ownership remains unknown |
| Road activity | Existing passive Road intent | Road evidence context | Does not imply a specific material |
| Impact evidence | Existing native vibration rise and established impact contribution | Short directional impact | Does not identify wall versus traffic |

No OutRun 2 SP addresses, command bytes, or decoded magnitude values are used
as OutRun 2006 runtime inputs.

## Architecture and behavior

`AerEvidenceFrame` adapts verified PC evidence. `AerContinuousInterpreter`
applies bounded nonlinear steering shaping, speed/response authority, mixed-
contact attenuation, and smoothing. `AerEventInterpreter` produces short
surface-transition, contact-asymmetry, and impact events with impact priority,
duration, retrigger protection, and cooldown. `AerComposer` applies a soft
limit and slew protection. `AerSafetyGate` requires explicit profile selection,
gameplay, enabled FFB, a ready device, finite data, and fresh evidence.

`AerTelemetry` exposes evidence validity, continuous/event requests, event
class, pre-limit/final requests, limiting, safety state, and the two AER player
settings under the versioned `HYP36R_AER_PROFILE_V1` contract.

The active profile reuses the existing wheel backend and its master strength,
focus-loss stop, disconnect handling, watchdog, clamp, inversion, and
DirectInput effect ownership. Shadow mode calculates the same proposal but is
never selected for hardware output.

## Player controls

- Force Profile: `Reference+` (default) or `Arcade Experience (Experimental)`.
- AER Strength: 0–100%.
- AER Road Detail: 0–100%.
- Device: Invert Wheel, Test Left, Test Right, Re-detect Wheel (shared wheel controls).

Advanced interpretation constants remain internal while the profile is under
research. AER Strength is profile-local presentation authority; the established
master wheel strength remains the final device-level control.

## Wheel direction and safe test

The Arcade Experience Force Feedback panel now exposes the same connected-wheel **Invert Wheel**, **Test Left**, **Test Right**, and **Re-detect Wheel** controls already used by Reference+. There is no separate arcade inversion layer or additional wheel backend: inversion is a device-level orientation setting and applies consistently to both profiles, while the *force-character* settings remain profile-independent.

The direction tests use the existing DirectInput constant-force test path, capped at **20% nominal output for 350 ms**, regardless of Arcade Strength or Road Detail. They require a detected, ready wheel and enabled FFB; the backend also checks focus and stops regular drive output while the test is active. The buttons cannot be used when the wheel is unavailable. Begin with a conservative wheel-base torque limit and verify each direction before driving. If a requested left force pulls right, toggle Invert Wheel and repeat both tests. The setting is saved through the existing wheel-device preference; it does **not** reset Reference+ force-character values.

## Validation and remaining limits

Deterministic tests cover default-off behavior, profile selection, monotonic
continuous response, event priority and bounds, shadow isolation, disconnect,
stale/invalid evidence decay, finite output, and normalized limits. Reference+
continues through its unchanged selection and composition path whenever AER is
not selected.

Hardware UAT must still validate direction, continuous clarity, surface/event
separation, impact comfort, focus loss, disconnect, and shutdown on a
belt-driven wheel, a mid-torque direct-drive wheel, and a high-torque
direct-drive wheel. Passing UAT validates this presentation, not equivalence to
an original cabinet.

Surface 2.0 is tracked separately. It is not a prerequisite or automatically
included component of the v2.0 RC AER scope.

## Experiential reference: AER-BASELINE-01

- **Build:** `06738d3`
- **Hardware:** Fanatec Podium DD2
- **Configuration:** only the recorded project settings are authoritative;
  unrecorded wheel-base settings are intentionally not reconstructed.
- **Player observation:** strong arcade-style steering resistance, pronounced
  opposing force while drifting, dramatic contrast with physics-derived
  Reference+, a distinct nostalgic association with arcade steering, and a
  positive initial subjective response.

This is the preserved experiential baseline for future comparison. It does not
prove equivalence to Sega cabinet hardware and must not be replaced by later
tuning in historical documentation. AER-02 changes presentation and telemetry,
not the force equations, constants, polarity, ceilings, or interpretation.

## Force-direction observation: AER-UAT-02

On the same DD2, the preferred observed configuration used **Invert Wheel on**
for Reference+ and **Invert Wheel off** for Arcade Experience. This is not
classified as a defect. Device inversion is one shared final-output operation;
the two profiles produce independently interpreted force requests before that
operation. The contrast could therefore arise from profile response and
centering character rather than device polarity alone. Automatic inversion and
profile-specific sign changes remain out of scope pending additional hardware
evidence.

## Surface observation register

These entries are research targets, not confirmed force-feedback effects:

| ID | Context | Observation | Status / next comparison |
| --- | --- | --- | --- |
| AER-UAT-04 | Continuous-stage play | An icy or slippery-feeling texture retriggered; it was not observed in Time Attack | Unverified mode-dependent behavior; compare native state, transitions, classifications, interpreter state, renderer state, and output |
| AER-UAT-05 | National Park bridge | Visually distinctive bridge surface | Physical FFB unconfirmed; compare approach, crossing, and exit |
| AER-UAT-06 | Imperial Avenue | Paver-style roadway appears sustained through much of the course | Investigate sustained evidence without assuming original continuous texture output |
| AER-UAT-07 | Desert roadway | Visible dirt or loose-surface road; exact stage still requires confirmation | Compare classifications and runtime evidence; do not infer vibration from appearance |

### Surface research priority

1. **High:** Imperial Avenue, Tulip Garden, National Park, Sunny Beach, Snow
   Mountain, and the pending-confirmation Desert stage.
2. **Secondary:** Alpine, Lake, Ancient Ruins, and Cloudy Highland.
3. **Cross-mode:** repeat identifiable sections in continuous-stage play and
   Time Attack to isolate transition or lifecycle effects.

The canonical classification evidence remains
`docs/research/aer/outrun-2-sp/current/COURSE_CLASSIFICATION_MATRIX.md`. Tulip
Garden's `coli_cs_tuli_bin.gz` contains 6,208 main-course polygons and a
localized 39-polygon ordinal-20 run at indices 124–162, forming a `1 → 20 → 1`
sequence aligned with `re_CS_TULI_05_H_BLIDGE`. This strongly supports a
localized bridge-road classification; it does not establish a particular
continuous wheel vibration or material identity.
