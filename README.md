# OutRun 2006 C2C Multi Input

OutRun 2006 C2C Multi Input brings modern multi-device controller support and physics-informed force feedback to **OutRun 2006: Coast 2 Coast** on PC. Multi Input and HYP36R Force are developed by [hyp36rmax](https://github.com/hyp36rmax), building on [OutRun2006Tweaks by emoose](https://github.com/emoose/OutRun2006Tweaks).

Connect your controls and drive. A wheel base, steering wheel, pedals, shifter, button box, and gamepad can work together. Configure and test them inside the game without vJoy, an external input mapper, or manual controller configuration.

> [!IMPORTANT]
> This project is under active development. Multi device input and native force feedback are functional. More hardware testing and tuning are welcome.

## Why this fork exists

The PC release expects a much simpler controller arrangement than modern driving setups provide. A wheel may expose several Windows interfaces, while its pedals and shifter may each be separate USB devices. That can make otherwise capable hardware difficult or impossible to configure in the original game without extra software.

This fork treats setup as part of the game experience. Each connected controller is detected independently, several devices can control one player, and setup is handled through a guided interface. FFB endpoints are checked for usable effect output instead of relying only on a device name or capability flag. Compatibility problems are logged automatically, while advanced tuning stays out of the way until it is needed. This is generic, capability-based handling, not a claim that every wheel has been validated.

## Features added by this fork

### Multi-device controls

Use axes, buttons, and hats from multiple USB devices simultaneously. A wheel can be combined with separate pedals, a shifter, button boxes, and a gamepad. Bindings remain attached to the correct physical device across restarts, including devices that expose identical or duplicated interfaces.

The **Controllers** tab shows live activity and provides in game calibration. Individual bindings can be added, removed, or inverted without editing a file.

### Guided Quick Setup

Quick Setup walks through steering, throttle, brake, shifting, and menu controls. Each prompt provides a six second capture period, displays the detected input, and asks for confirmation. A step can be retried without restarting the setup process.

### HYP36R Force

HYP36R Force takes a vehicle-informed approach to OutRun force feedback. It uses information available from the running vehicle simulation to build progressive steering and cornering load, loading and unloading, grip transition, release and recovery. Road information and impacts remain distinct parts of the presentation. This is not simply a strength boost to existing effects, nor a claim of measured real-world steering torque or exact arcade-board reproduction.

This is not an attempt to turn OutRun into a modern simulation. OutRun remains OutRun. The aim is to communicate more of the vehicle behavior already available underneath the game while keeping its character and accessibility.

Force output uses DirectInput, so no vJoy or separate FFB application is required. When a wheel exposes separate input and force output endpoints, Multi Input checks which endpoint can actually create and start a force effect, then shows one resolved wheel identity.

The **Force Feedback** tab starts with Reference+, Strength, and the connected wheel. Advanced controls offer Steering Load, Road Detail, Impact, Invert Wheel, bounded left and right tests, and Re-detect Wheel. Forces build gradually and stop if game updates pause. Device capabilities and failures are written to `OutRun2006Tweaks.log` for troubleshooting.

Reference+ is the recommended HYP36R Force profile. Start with Reference+ and Strength at 100%, subject to a conservative wheel-side torque setting. The three Advanced Force Character controls run from 0–100%; 100% expresses the intended Reference+ balance and lower values attenuate their respective channels. Current evidence does not establish validated above-Reference player ceilings. The Left and Right tests send a short, gentle 20% diagnostic request independently of the Strength slider. The [V1 issue matrix](docs/V1_RELEASE_ISSUES.md) tracks hardware checks still needed before a release candidate.

The model combines observed vehicle state with derived and synthetic force components. Its native values do not have proven physical units; the [force architecture](docs/HYP36R_FORCE.md) distinguishes those layers.

## Quick start

### 1. Install

Download the newest package from this repository's [Releases page](https://github.com/hyp36rmax/multi-device-input/releases).

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
2. Confirm your wheel shows **Connected**. Reference+ is selected automatically.
3. Start with a conservative hardware torque limit; you can lower in-game **Strength** from its 100% default if needed.
4. Open **Advanced Force Feedback** and use **Test Left** and **Test Right** before entering a race.
5. Drive a race and adjust Strength or the three Force Character controls to taste.

If the live force pulls away from center, enable **Invert Wheel** under Advanced Force Feedback.

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
2. Open **Force Feedback**, confirm the wheel is connected, and try both direction tests.
3. Select **Re-detect Wheel** under Advanced Force Feedback if hardware was connected after startup.
4. Close the game normally so the latest log is complete.
5. Open a [GitHub issue](https://github.com/hyp36rmax/multi-device-input/issues) and attach `OutRun2006Tweaks.log`.

Please include the wheel base, rim, pedals, and shifter models, along with driver and firmware versions. Tell us which compatibility mode you used, whether the direction tests worked, whether live driving force worked, and what you expected compared with what you observed.

Do not include unrelated personal information in uploaded logs or screenshots.

### Application error 0xc0000142

`0xc0000142` is a Windows loader initialization failure, not an FFB error. It
can occur before the patch has started, so the patch cannot always create a new
log for that launch. Check `OutRun2006Tweaks.log` for this line:

```text
Startup diagnostic: OutRun2006Tweaks logger initialized successfully
```

If the line is present for the failed launch, attach the completed log to the
report. If the log was not updated or the line is absent, Windows failed before
our logger initialized. Include the Windows Event Viewer **Application Error**
entry instead, especially the faulting module name and exception code.

Download the latest supported [Microsoft Visual C++ Redistributables](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170#latest-supported-redistributable-version)
and install or repair both the x86 and x64 packages. The game and patch are
32-bit, so the x86 package is required even on 64-bit Windows. On the system
where we reproduced this problem, repairing both packages restored startup.

## Original OutRun2006Tweaks features

This fork retains the fixes and enhancements provided by OutRun2006Tweaks, including framerate correction and interpolation, graphics improvements, shorter loading, restored online multiplayer support, overlay configuration, expanded audio support, and numerous game bug fixes.

For the upstream project overview, community, and original releases, visit [emoose/OutRun2006Tweaks](https://github.com/emoose/OutRun2006Tweaks).

Steam Deck and Linux users may need this launch option for the wrapper to load:

```text
WINEDLLOVERRIDES="dinput8=n,b" %command%
```

Native wheel FFB in this fork targets Windows DirectInput and may behave differently through Wine or Proton.

## Current state and project direction

Multi-device input and the HYP36R Force foundation are functional. The current
validated Reference mode provides the stable comparison and fail-safe.
Reference+ is a validated experimental presentation with greater steering
presence; it is not yet a universal hardware preset or final production tune.

The next priorities are broader wheel and cross-car validation, device-aware
calibration, targeted surface fidelity research, and keeping the entire normal
setup experience inside the game. Optional SimHub-compatible telemetry and a
reversible managed profile for research access remain on the backlog.

Start with the [documentation map](docs/README.md). It links the current
[HYP36R Force architecture](docs/HYP36R_FORCE.md),
[native dynamics evidence](docs/NATIVE_DYNAMICS.md),
[telemetry reference](docs/TELEMETRY.md),
[presentation and safety boundary](docs/PRESENTATION_AND_SAFETY.md),
[development history](docs/DEVELOPMENT_HISTORY.md), and
[roadmap](docs/ROADMAP.md). The detailed milestone notes remain available as
the research record behind those summaries. The [research preservation index](research/README.md)
records which raw captures still exist outside Git and which findings survive
only in documentation.

## Building

Building requires Visual Studio 2022, CMake, and Git.

Clone this repository with its submodules, run `generate_vs2022.bat`, open `build\outrun2006tweaks-proj.sln`, and build the Release configuration for Win32.

Pushes and pull requests are also compiled by the Windows workflow under the repository's Actions tab.

## Credits

### Multi Input and HYP36R Force

Developed and hardware-tested by [hyp36rmax](https://github.com/hyp36rmax).

Special thanks to **el julo** on Discord for early troubleshooting and for inspiring the intuitive approach behind this project.

### Original project

Based on [OutRun2006Tweaks](https://github.com/emoose/OutRun2006Tweaks), created by [emoose](https://github.com/emoose) with contributions from its community.

Thanks to [debugging.games](http://debugging.games) for hosting OutRun 2 SP debug symbols used by the original project.

## License

This fork retains the upstream project's MIT License and copyright notice. See [LICENSE.md](LICENSE.md).
