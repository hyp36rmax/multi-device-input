# Roadmap

## Completed foundations

- In-game multi-device discovery, binding, calibration, Quick Setup, and
  persistence.
- DirectInput wheel selection, compatibility testing, diagnostics, and force
  output without vJoy.
- M4 Force 2.0 LOAD → RELEASE → FREE → BITE → returned-load foundation.
- Native FL/FR/RL/RR observation and M5 lateral-context research path.
- Authoritative telemetry from native state through the DirectInput request.
- S-series presentation architecture and software-headroom research.

## Current validated modes

- **Reference:** validated permanent comparison and fail-safe; M4-only at
  Presence 1.00.
- **Reference+:** the fresh-install presentation default on the physically
  validated #78 lineage; M4 plus eligible bounded M5, Presence 1.44,
  Contrast 4, and a five-percent linear secondary budget.

Reference+ is the current player-facing default, but cross-wheel calibration
and a physical torque-safety envelope remain open research questions.

## Deferred research

- Cross-car M5 validation before universal normalization or defaults.
- Final presentation intensity and comparison of greater Presence and/or
  Information Contrast.
- A physical hardware safety envelope based on more than normalized software
  output.
- Device and device-class calibration.

## Targeted fidelity investigation

- **Sunny Beach striped runoff:** before the first beach ball, compare the
  left-then-right horizontal runoff crossing against corner `+0x28`, surface
  classifications, `+0xAC`/`+0xB0`, front/rear timing, road contribution, and
  output slew. Do not assume the visible stripes are modeled bumps.

## AER

- **Target:** v2.0 RC. **Status:** experimental research and development;
  release readiness is not established.
- Use the stable v1.5 Multi-Device Input and HYP36rforce FFB capabilities as
  the foundation without placing AER in the v1.5 release path.
- Continue the opt-in Arcade Experience profile, steering centering and
  resistance research, arcade-specific feedback interpretation, telemetry,
  and controlled validation.
- Keep AER downstream of verified native vehicle evidence and distinguish
  confirmed arcade behavior, observation, engineering interpretation,
  experimental implementation, and unverified hypotheses.
- Latest wheel-direction controls still require hardware validation. Do not
  describe the profile as arcade-accurate until evidence supports that claim.
- Surface 2.0 remains an independent research track and is not a required v2.0
  RC feature.

## Known blockers

- Additional cars are locked on the current research profile, limiting
  controlled cross-car work until progression is completed.
- Native corner units and physical force meanings remain unknown.
- Current software exposure does not establish device torque or safety.

## Watch list, not confirmed bugs

- Two standard gamepads may contend for the generic primary-gamepad bindings
  because the first detected pad is used for the single-player input stream.
  No P1/P2 problem has been reproduced, and the current multi-device setup
  remains unchanged. Investigate only if a real restart/reconnect report shows
  the wrong controller taking over. See
  [Hotfix R1 controller audit](HOTFIX_R1_CONTROLLER_MENU_AUDIT.md).
