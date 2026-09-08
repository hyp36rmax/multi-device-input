# OutRun 2006: Multi-Device Input

A driving-hardware-focused fork of [OutRun2006Tweaks](https://github.com/emoose/OutRun2006Tweaks) for **OutRun 2006: Coast 2 Coast**.

Connect a wheel base, steering wheel, pedals, shifter, button box, and gamepad at the same time. Configure and test them inside the game—without vJoy, an external input mapper, or hand-editing controller configuration files.

> [!IMPORTANT]
> This project is under active development. Multi-device input is functional. Native force feedback currently includes wheel discovery, automatic interface selection, safe direction tests, and an initial live-driving centering model. More road and vehicle effects are planned.

## Why this fork exists

The PC release expects a much simpler controller arrangement than modern driving setups provide. A wheel may expose several Windows interfaces, while its pedals and shifter may each be separate USB devices. That can make otherwise capable hardware difficult or impossible to configure in the original game without extra software.

This fork treats setup as part of the game experience:

- every connected device is detected independently;
- inputs from several devices can control one player;
- setup is guided and visible;
- wheel compatibility problems are logged automatically; and
- advanced tuning stays out of the way until it is needed.

## Features added by this fork

### Multi-device controls

- Use axes, buttons, and hats from multiple USB devices simultaneously.
- Mix a wheel, separate pedals, shifter, button box, and Xbox controller.
- Keep bindings attached to the correct physical device across restarts.
- Distinguish identical or duplicated device interfaces.
- View live axis, button, and hat activity on the **Controllers** tab.
- Calibrate steering and pedals from inside the game.
- Add, remove, or invert individual bindings without editing a file.

### Guided Quick Setup

- Walks through steering, throttle, brake, shifting, and menu controls.
- Allows six seconds for each requested input.
- Shows the detected input before accepting it.
- Requires confirmation for every selection, preventing accidental skipped steps.
- Allows retrying a step without restarting setup.

### Native force feedback

- Uses DirectInput directly—no vJoy or separate FFB application required.
- Lists force-feedback-capable wheel interfaces by name.
- Automatically falls back to a usable force-output interface when a wheel exposes separate input and FFB endpoints.
- Provides gentle **Test left** and **Test right** controls.
- Provides master Strength with optional Centering, Damping, and direction inversion controls.
- Starts the live force gradually and stops stale force automatically if game updates pause.
- Writes device capabilities and failures to `OutRun2006Tweaks.log` for troubleshooting.

The current live-driving model supplies speed-scaled centering and steering damping. Road texture, grip loss, impacts, and other vehicle effects are future work.

## Quick start

### 1. Install

Download the newest successful Windows build from this repository's [Actions page](https://github.com/hyp36rmax/multi-device-input/actions). Open the build, scroll to **Artifacts**, and download `outrun2006tweaks-...`.

Extract its contents into the **OutRun 2006: Coast 2 Coast** folder containing `OR2006C2C.EXE`, replacing files when prompted.

Install the latest [Microsoft Visual C++ x86 Redistributable](https://aka.ms/vs/17/release/vc_redist.x86.exe), even if a different Visual C++ package is already installed.

### 2. Connect your hardware

Connect and power on the wheel base, pedals, shifter, button boxes, and any gamepads before launching the game. Multiple interfaces with the same wheel name can be normal, particularly with Fanatec hardware.

### 3. Configure controls in the game

1. Launch `OR2006C2C.EXE`.
2. Press **F11** to open OutRun2006Tweaks.
3. Open **Controls** and choose **Configure Input Bindings**.
4. Select **Quick Setup**.
5. Perform and confirm each requested input.
6. Open **Controllers** to verify live movement from every device.
7. Select **Save bindings**.

### 4. Enable force feedback

1. Open the **Force Feedback** tab in the same controller window.
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

Other DirectInput wheels are intended to work, but need broader community testing. Reports for Fanatec, Logitech, MOZA, Simagic, Thrustmaster, and other manufacturers are welcome.

## Troubleshooting and compatibility reports

If a device is missing or FFB does not work:

1. Open **Controllers** and verify whether the device and its live inputs appear.
2. Open **Force Feedback**, choose the wheel, and try both direction tests.
3. Select **Refresh connected wheels** under Advanced if hardware was connected after startup.
4. Close the game normally so the latest log is complete.
5. Open a [GitHub issue](https://github.com/hyp36rmax/multi-device-input/issues) and attach `OutRun2006Tweaks.log`.

Please include:

- wheel base, rim, pedals, and shifter models;
- wheel driver and firmware versions;
- the selected compatibility or operating mode;
- whether left/right tests work;
- whether live driving force works; and
- the exact behavior you expected and observed.

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

Near-term priorities are:

- expand the live-driving force model with validated road, grip, and impact signals;
- test and refine compatibility across major wheel manufacturers;
- keep all ordinary controller and FFB setup inside the game; and
- add optional SimHub-compatible telemetry after the driving controls are stable.

## Building

Building requires Visual Studio 2022, CMake, and Git.

Clone this repository with its submodules, run `generate_vs2022.bat`, open `build\outrun2006tweaks-proj.sln`, and build the Release configuration for Win32.

Pushes and pull requests are also compiled by the Windows workflow under the repository's Actions tab.

## Credits

### Multi-device input and force-feedback project

Conceived, directed, and hardware-tested by [hyp36rmax](https://github.com/hyp36rmax).

### Original project

Based on [OutRun2006Tweaks](https://github.com/emoose/OutRun2006Tweaks), created by [emoose](https://github.com/emoose) with contributions from its community.

Thanks to [debugging.games](http://debugging.games) for hosting OutRun 2 SP debug symbols used by the original project.

## License

This fork retains the upstream project's MIT License and copyright notice. See [LICENSE.md](LICENSE.md).
