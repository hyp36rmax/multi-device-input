# Multi-device driving controls

> Historical planning note. The current project overview is in the repository
> README, and current research priorities are in [ROADMAP.md](ROADMAP.md).

## Product promise

A player can connect a wheel, pedals, shifter, button box and gamepad, configure
them inside OutRun, verify every input visually, and start driving without an
external mapper or editing a configuration file.

Settings files remain an internal persistence detail. Every setting must be
discoverable, changeable, testable and reversible in the in-game UI.

## Delivery slices

1. **Discovery:** enumerate every SDL joystick at startup and during hot-plug,
   while retaining SDL's friendly gamepad path for standard controllers.
2. **Device-aware bindings:** store the physical device with each axis, button
   or hat binding and aggregate all bindings into OutRun's single-player state.
3. **Quick Setup:** ask the player to move the wheel, press each pedal and choose
   shift/menu buttons. Infer axis direction and range, then show a live test.
4. **Calibration and recovery:** support inversion, deadzone, saturation,
   combined pedals, missing-device recovery and a last-known-good profile.
5. **Wheel force feedback:** drive a selected wheel through a dedicated Windows
   force-feedback backend while keeping SDL responsible for input.

## Force-feedback model

The restored Xbox vibration code is useful for impact and surface cues, but its
two motor amplitudes are not steering torque. Proper wheel feedback combines:

- self-aligning torque derived from steering angle, speed and vehicle rotation;
- damping to control oscillation around centre;
- road texture and kerb detail;
- collision and landing impulses;
- reduced aligning force during loss of grip or airborne states.

The Windows backend should prefer DirectInput force-feedback effects for wheel
compatibility. It must provide a safety stop on pause, focus loss, device loss,
race exit and DLL shutdown. The in-game UI owns device selection, overall
strength, individual effect levels, inversion and a safe left/right test.

FFB ships incrementally: device capability detection and a bounded test effect
first, existing vibration-derived effects second, and physics-derived aligning
torque only after relevant car-state fields are identified and validated.

## First acceptance gate

With an Xbox One controller, wheel, USB pedals and shifter connected, the
Controllers page lists all four exactly once and shows live axes, buttons and
hats. Existing Xbox controls continue to operate unchanged.

## Future roadmap considerations

These are candidates for independent design and testing, not commitments for a
particular release:

- hardware periodic effects for clearer road and tire vibration;
- separate input polling and exclusive force-feedback connections;
- stronger recovery after focus or device loss;
- guaranteed force shutdown during every exit path;
- GUID-based identification of duplicate wheel interfaces;
- automatic fallback when a device rejects an effect type;
- selectable force profiles for controlled tuning and comparison.

Any work in these areas will be implemented and validated within this project,
without relying on third-party code or components.

### Experience backlog

- **Unlock All Content:** use a reversible managed save/profile clone. Keep the
  legitimate progression save untouched, isolate the unlocked profile, allow
  immediate restoration, and never merge artificial progress into the
  legitimate save. This may also enable full-car research without progression
  grinding.

### Force research and calibration backlog

- **HYP36R Information Amplification & Safety Envelope:** study useful
  perceptual amplification, clipping, headroom, limiter activity, sustained
  output, transient peaks, device protection, and safe device margins. A prior
  DD2 shutdown during high-strength testing is device-specific evidence, not a
  universal limit.
- **Cross-car M5 validation:** complete before universal normalization,
  production defaults, vehicle-independent claims, or final active deployment.
  Do not add per-car compensation before that evidence exists.
- **Arcade Experience Reference:** use validated original Sega/OutRun arcade
  force communication where available to study presentation. AER does not
  replace native OutRun physics or HYP36R Force semantics.
