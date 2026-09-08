# OutRun 2006: Multi-Device Input

A fork of [OutRun2006Tweaks](https://github.com/emoose/OutRun2006Tweaks) built for modern driving hardware in **OutRun 2006: Coast 2 Coast**.

Connect a wheel base, steering wheel, pedals, shifter, button box, and gamepad at the same time. Configure and test everything inside the game without vJoy, an external input mapper, or manual controller configuration.

> [!IMPORTANT]
> This project is under active development. Multi device input and native force feedback are functional. More hardware testing and tuning are welcome.

## Why this fork exists

The PC release expects a much simpler controller arrangement than modern driving setups provide. A wheel may expose several Windows interfaces, while its pedals and shifter may each be separate USB devices. That can make otherwise capable hardware difficult or impossible to configure in the original game without extra software.

This fork treats setup as part of the game experience. Each connected controller is detected independently, several devices can control one player, and setup is handled through a guided interface. Compatibility problems are logged automatically, while advanced tuning stays out of the way until it is needed.

## Features added by this fork

### Multi-device controls

Use axes, buttons, and hats from multiple USB devices simultaneously. A wheel can be combined with separate pedals, a shifter, button boxes, and a gamepad. Bindings remain attached to the correct physical device across restarts, including devices that expose identical or duplicated interfaces.

The **Controllers** tab shows live activity and provides in game calibration. Individual bindings can be added, removed, or inverted without editing a file.

### Guided Quick Setup

Quick Setup walks through steering, throttle, brake, shifting, and menu controls. Each prompt provides a six second capture period, displays the detected input, and asks for confirmation. A step can be retried without restarting the setup process.

### Native force feedback

Native force feedback uses DirectInput, so no vJoy or separate FFB application is required. Compatible wheel interfaces are listed by name. When a wheel exposes separate input and force output endpoints, the game automatically finds the usable endpoint.

The **Force Feedback** tab includes safe left and right tests, master strength, centering, damping, road detail, grip loss, collision feedback, and force direction controls. Forces build gradually and stop safely if game updates pause. Device capabilities and failures are written to `OutRun2006Tweaks.log` for troubleshooting.

The live driving model provides speed based centering, steering damping, surface texture, grip loss during slides, and impact feedback.

## Quick start

### 1. Install

Download the newest successful Windows build from this repository's [Actions page](https://github.com/hyp36rmax/multi-device-input/actions). Open the build, scroll to **Artifacts**, and download `outrun2006tweaks-...`.

Extract its contents into the **OutRun 2006: Coast 2 Coast** folder containing `OR2006C2C.EXE`, replacing files when prompted.

Install the latest [Microsoft Visual C++ x86 Redistributable](https://aka.ms/vs/17/release/vc_redist.x86.exe), even if a different Visual C++ package is already installed.

### 2. Connect your hardware

Connect and power on the wheel base, pedals, shifter, button boxes, and any gamepads before launching the game. Multiple interfaces with the same wheel name can be normal, particularly with Fanatec hardware.

### 3. Configure controls in the game

1. Launch `OR2006C2C.EXE`.
2. Open the game's **Options** menu.
3. Select **Controller**. The controller setup overlay will open automatically.
4. Select **Quick Setup**.
5. Perform and confirm each requested input.
6. Open **Controllers** to verify live movement from every device.
7. Select **Save bindings**.

### 4. Enable force feedback

1. Open the **Force Feedback** tab in the controller setup overlay.
2. Enable force feedback and choose the wheel.
3. Set a conservative Strength; **50%** is a sensible starting point.
4. Use **Test left** and **Test right** before entering a race.
5. Drive a race and adjust Strength to taste.

If the live force pulls away from center, open **Advanced** and enable **Invert force direction**. Adjust Centering or Damping only if the default feel needs refinement.

> [!CAUTION]
> Direct-drive wheels can produce substantial torque. Begin with a low hardware torque limit and a modest in-game strength. Keep hands clear during direction tests if you are unsure how the wheel will respond.

## Tested hardware

- Xbox One controller
- Fanatec Podium Wheel Base DD2, including its separate input and force-output interfaces
- Separate USB driving controls used together through the multi-device binding system

Other DirectInput wheels are intended to work, but broader community testing is still needed. Reports for Fanatec, Logitech, MOZA, Simagic, Thrustmaster, and other manufacturers are welcome.

## Troubleshooting and compatibility reports

If a device is missing or FFB does not work:

1. Open **Controllers** and verify whether the device and its live inputs appear.
2. Open **Force Feedback**, choose the wheel, and try both direction tests.
3. Select **Refresh connected wheels** under Advanced if hardware was connected after startup.
4. Close the game normally so the latest log is complete.
5. Open a [GitHub issue](https://github.com/hyp36rmax/multi-device-input/issues) and attach `OutRun2006Tweaks.log`.

Please include the wheel base, rim, pedals, and shifter models, along with driver and firmware versions. Tell us which compatibility mode you used, whether the direction tests worked, whether live driving force worked, and what you expected compared with what you observed.

Do not include unrelated personal information in uploaded logs or screenshots.

## Original OutRun2006Tweaks features

This fork retains the fixes and enhancements provided by OutRun2006Tweaks, including framerate correction and interpolation, graphics improvements, shorter loading, restored online multiplayer support, overlay configuration, expanded audio support, and numerous game bug fixes.

For the upstream project overview, community, and original releases, visit [emoose/OutRun2006Tweaks](https://github.com/emoose/OutRun2006Tweaks).

Steam Deck and Linux users may need this launch option for the wrapper to load:

```text
WINEDLLOVERRIDES="dinput8=n,b" %command%
```

Native wheel FFB in this fork targets Windows DirectInput and may behave differently through Wine or Proton.

## Project direction

The next priorities are testing and refining compatibility across more wheel manufacturers, continuing to tune the driving feel, keeping all normal controller and FFB setup inside the game, and adding optional SimHub compatible telemetry.

## Building

Building requires Visual Studio 2022, CMake, and Git.

Clone this repository with its submodules, run `generate_vs2022.bat`, open `build\outrun2006tweaks-proj.sln`, and build the Release configuration for Win32.

Pushes and pull requests are also compiled by the Windows workflow under the repository's Actions tab.

## Credits

### Multi-device input and force-feedback project

Conceived, directed, and hardware-tested by [hyp36rmax](https://github.com/hyp36rmax).

Special thanks to **el julo** on Discord for early troubleshooting and for inspiring the intuitive approach behind this project.

### Original project

Based on [OutRun2006Tweaks](https://github.com/emoose/OutRun2006Tweaks), created by [emoose](https://github.com/emoose) with contributions from its community.

Thanks to [debugging.games](http://debugging.games) for hosting OutRun 2 SP debug symbols used by the original project.

## License

This fork retains the upstream project's MIT License and copyright notice. See [LICENSE.md](LICENSE.md).
