# OutRun 2006 C2C Multi Input v1.5

Modern controller support and HYP36rforce FFB for OutRun 2006: Coast 2 Coast
on PC.

## What v1.5 brings

- Use a wheel, separate pedals, shifter, button box, and gamepad together with
  less dependence on Windows controller priority, vJoy, or external controller
  utilities.
- Configure controls in the game through Quick Setup or manual Bindings, then
  keep device assignments across restarts.
- Let Multi Input resolve the force-output endpoint when a wheel exposes more
  than one DirectInput interface.
- Drive with HYP36rforce FFB and its recommended Reference+ profile.
- Choose Classic or Enhanced Road presentation while retaining the shared
  Surface channel.
- Adjust Strength, Steering Load, Road Detail, Impact, and Surface without
  exposing the engineering controls behind the experience.
- Feel Surface Texture and automatic Bump cues through one player-facing
  Surface control.
- Use short Left/Right tests, Invert Wheel, and Re-detect Wheel directly in the
  controller overlay.
- Record clearer General Telemetry captures while keeping Guided UAT research
  sessions organized separately.

## Setup and drive

Install OutRun 2006: Coast 2 Coast on PC, then extract the complete Multi Input
package into the game's main folder. Launch the included `OR2006C2C.exe`, open
**Options → Controller**, and the controller overlay will open automatically.
Run **Quick Setup** or use **Bindings** for manual assignment, verify each device
under **Controllers**, and save your bindings.

Wheel users can then open **Force Feedback**. The intended starting point is
Reference+, Strength 100%, and a Connected wheel. Begin with a conservative
wheel-side torque limit, use **Test Left** and **Test Right**, and enable
**Invert Wheel** if the live force pulls away from center.

The [README](https://github.com/hyp36rmax/multi-device-input/blob/multi-device-input/README.md)
contains complete setup, upgrade, hardware, telemetry, safety, and
troubleshooting guidance.

## Upgrading

The package does not include `OutRun2006Tweaks.user.ini`. Existing player
preferences, controller bindings, and device assignments remain compatible.
v1.5 normalizes obsolete development-only HYP36rforce research overrides where
required while preserving normal player preferences. **Reset to default**
restores the normal FFB setup without resetting controller bindings.

Install or repair the latest Microsoft Visual C++ 2015–2022 Redistributables if
the game reports startup error `0xc0000142`. The x86 package is required because
the game is 32-bit; repairing both x86 and x64 resolved the reproduced case.

## Project lineage

Multi Input and HYP36rforce FFB are developed and hardware-tested by
[hyp36rmax](https://github.com/hyp36rmax).

**Based on [OutRun2006Tweaks v0.6.1.0 by emoose](https://github.com/emoose/OutRun2006Tweaks).**
The upstream foundation, license, and attribution remain included. Thanks to el
julo for early troubleshooting and for inspiring a more intuitive solution.
