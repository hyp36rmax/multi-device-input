# Arcade Experience (AER) experimental profile

## Status

Arcade Experience is an opt-in, experimental HYP36rforce profile. Reference+
remains the default and fallback. Selecting AER does not rewrite the player's
Reference+ Force Character settings.

This is an independent modern interpretation informed by the original Sega
game-side research preserved under `docs/research/aer/outrun-2-sp/`. It is not
a reproduction of drive-board firmware, cabinet torque, or original waveforms.

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

Advanced interpretation constants remain internal while the profile is under
research. AER Strength is profile-local presentation authority; the established
master wheel strength remains the final device-level control.

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
