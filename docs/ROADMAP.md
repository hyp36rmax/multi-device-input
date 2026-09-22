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
  validated #78 lineage; M4 plus eligible bounded M5, Presence 1.20,
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

## Experience backlog

- **Gameplay → Unlock All Content:** create a reversible managed profile clone.
  Keep legitimate progression untouched, isolate artificial unlocks, allow
  immediate restoration, and never merge artificial progress silently.

## Targeted fidelity investigation

- **Sunny Beach striped runoff:** before the first beach ball, compare the
  left-then-right horizontal runoff crossing against corner `+0x28`, surface
  classifications, `+0xAC`/`+0xB0`, front/rear timing, road contribution, and
  output slew. Do not assume the visible stripes are modeled bumps.

## AER

- Build an Arcade Experience Reference only from validated original arcade
  force communication and hardware behavior.
- Keep AER downstream of native vehicle semantics and the HYP36R force model.

## Known blockers

- Additional cars are locked on the current research profile, limiting
  controlled cross-car work until progression is completed or the managed
  profile feature exists.
- Native corner units and physical force meanings remain unknown.
- Current software exposure does not establish device torque or safety.

## Watch list, not confirmed bugs

- Two standard gamepads may contend for the generic primary-gamepad bindings
  because the first detected pad is used for the single-player input stream.
  No P1/P2 problem has been reproduced, and the current multi-device setup
  remains unchanged. Investigate only if a real restart/reconnect report shows
  the wrong controller taking over. See
  [Hotfix R1 controller audit](HOTFIX_R1_CONTROLLER_MENU_AUDIT.md).
