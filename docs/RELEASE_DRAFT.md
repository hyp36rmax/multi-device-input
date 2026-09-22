# OutRun 2006 C2C Multi Input

**DRAFT — PRE-RC. Not release notes yet.** Final public version and complete
combined V1 hardware UAT are still pending. Do not publish this text unchanged
until the release candidate has been checked on the intended package.

## Highlights

- Connect a wheel, separate pedals, shifter, button box, and gamepad without an
  external input mapper.
- Configure bindings, calibration, and HYP36R Force inside the game.
- Use Reference+, the recommended vehicle-informed force-feedback profile.
- Adjust Steering Load, Road Detail, and Impact independently, with 100% as
  the intended Reference+ balance.
- Let Multi Input validate a usable force-output endpoint when a wheel exposes
  more than one interface.
- Check direction with bounded Left/Right tests; use Invert Wheel if needed.
- Built on OutRun2006Tweaks by emoose.

## Why this exists

This started with a practical problem: OutRun 2006's PC controls were not made
for a modern collection of USB driving devices. A wheel base, pedals, shifter,
and buttons may each appear separately, while some bases expose duplicate
interfaces. Multi Input brings their setup into the game. The goal is simple:
connect your controls and drive, without vJoy or hand-editing a controller
configuration file. Generic capability checks reduce friction, but we are not
claiming universal wheel compatibility.

Working on the controls opened a second question: what could the wheel
communicate about OutRun's driving state? HYP36R Force takes a different
approach from an effect-only presentation. It uses observed vehicle state,
bounded derived behavior, and separate road and impact channels to convey
progressive cornering load, unloading, grip transition, release, and recovery.
It does not measure real steering-rack torque, establish physical units for
OutRun's internal values, or recreate the original arcade hardware exactly.
This is not an attempt to turn OutRun into a modern simulation. OutRun remains
OutRun.

The force work progressed through native vehicle-state investigation,
controlled telemetry, replay analysis, channel separation, software-headroom
checks, and physical wheel testing. Several hypotheses and implementations
were revised or discarded when their evidence did not hold. The
[engineering history](DEVELOPMENT_HISTORY.md) and
[research preservation index](../research/README.md) retain those details.

## Setup and drive

Install the complete package beside the supported game executable, then open
the game's **Options → Controller** menu. The in-game controller overlay opens
automatically. Run **Quick Setup**, confirm each input, and save the bindings.
In **Force Feedback**, leave the profile at **Reference+**, confirm the resolved
wheel, and start with **Strength 100%** only if your wheel-side torque setting
is conservative. Use the short Left/Right tests before driving. Advanced users
can attenuate Steering Load, Road Detail, and Impact; 100% is the intended
Reference+ expression for each channel.

Direct-drive wheels can generate substantial torque. Start with a low
wheel-side limit and reduce game Strength if needed. Stop testing if the wheel
behaves unexpectedly. The [README](../README.md) carries the full install and
troubleshooting guide.

## Validation still required before publication

Reference+ 1.44 has been checked through offline replay, and initial physical
driving feedback has been strongly positive. That is not the same as a
completed combined V1 hardware UAT. Cross-wheel behavior, device-specific
safety, race/start/exit stability, and the final clean-package pass still need
their release-candidate checks. The [V1 issue matrix](V1_RELEASE_ISSUES.md)
tracks those checks. Current evidence does not justify above-Reference
player-facing Force Character ceilings.

Multi Input and HYP36R Force are developed by
[hyp36rmax](https://github.com/hyp36rmax). This project is based on
[OutRun2006Tweaks by emoose](https://github.com/emoose/OutRun2006Tweaks) and
retains its upstream license and attribution. Thanks to el julo for early
troubleshooting and inspiration toward a more intuitive setup.
