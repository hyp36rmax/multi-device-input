# OutRun 2006 C2C Multi Input v1.0.0

Modern controller support and HYP36R Force feedback for OutRun 2006: Coast 2
Coast on PC.

## Highlights

- Use a wheel, separate pedals, shifter, button box, and gamepad together, with
  less reliance on vJoy or external controller utilities.
- Set up and test controls inside the game.
- Drive with HYP36R Force and its recommended Reference+ profile.
- Adjust global Strength, then reduce Steering Load, Road Detail, or Impact
  independently if you prefer.
- Let Multi Input check which force-feedback endpoint can actually start an
  effect when a wheel exposes more than one interface.
- Use Re-detect Wheel after changing hardware, with generic device support when
  a driver does not expose a descriptive wheel name.
- Use Invert Wheel and short, bounded Left/Right tests to check force direction.
- Keep the fixes and improvements of OutRun2006Tweaks by emoose, the foundation
  of this project.

## How we got here

This started with the familiar problem of getting a modern collection of USB
controls working in OutRun 2006. A wheel base, pedals, shifter, and buttons can
all appear as separate devices. Community tools made setups like that
possible, but also put another layer between the player and the game. Multi
Input brings the setup into OutRun itself.

> **Less setup between you and the game.**

Once the controls worked, the question moved to the wheel. What did the
running game know about the car that its original PC force and rumble effects
did not fully communicate? HYP36R Force grew from that question. It interprets
available vehicle information for steering and cornering load, release, and
recovery, while keeping road and collision cues distinct. It does not claim to
measure real steering torque, assign physical units to OutRun's internal
values, or reproduce the arcade hardware exactly.

The work used controlled driving, telemetry, replay analysis, separate force
channels, software-headroom checks, and physical wheel tests. Ideas that did
not hold up were changed or discarded. The [engineering history](https://github.com/hyp36rmax/multi-device-input/blob/multi-device-input/docs/DEVELOPMENT_HISTORY.md)
retains that trail.

> **More information between the car and the wheel.**

## Setup and drive

Install OutRun 2006: Coast 2 Coast on PC, then extract the complete Multi Input
package into the game's main folder. Launch the included `OR2006C2C.exe` and
open **Options → Controller**. The controller overlay opens there. Run **Quick
Setup**, confirm the inputs, and save your bindings.

In **Force Feedback**, the intended starting point is **Reference+**, **Strength
100%**, and a **Connected** wheel. For direct-drive hardware, begin with a
conservative wheel-side torque limit and lower game Strength if needed. Use
**Test Left** and **Test Right** before driving. Advanced controls can reduce
Steering Load, Road Detail, and Impact independently; 100% preserves the
intended Reference+ balance for each.

> **Then drive.**

The [README](https://github.com/hyp36rmax/multi-device-input/blob/multi-device-input/README.md)
has the full setup, hardware notes, and troubleshooting steps. Wheel and driver
behavior can differ, so compatibility reports are welcome. Stop a direction
test or drive if the wheel behaves unexpectedly.

Multi Input and HYP36R Force are developed by
[hyp36rmax](https://github.com/hyp36rmax). This work is based on
[OutRun2006Tweaks by emoose](https://github.com/emoose/OutRun2006Tweaks) and
retains its upstream license and attribution. Thanks to el julo for early
troubleshooting and for inspiring a more intuitive solution.
