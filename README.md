# OutRun 2006 C2C Multi Input

**Modern controller support and physics-informed force feedback for OutRun 2006: Coast 2 Coast.**

OutRun 2006 C2C Multi Input began with a simple frustration: getting modern
controllers, wheels, pedals, and multi-device setups working well with OutRun
2006 on PC often meant relying on **vJoy, external utilities, and additional
third-party applications**.

They could solve the problem, but they also added another layer between the
player and the game.

The original goal of Multi Input was straightforward:

> **Bring that functionality into the experience itself.**

Connect your hardware, configure it in-game, and drive without requiring a
collection of external controller tools just to make a modern setup work.

That work eventually led to a second question:

> **Could we rethink what OutRun communicates through the steering wheel itself?**

Those two ideas became the foundation of the project:

- **Multi Input** provides an integrated controller experience designed around
  modern gamepads, arcade controls, wheels, pedals, and multi-device setups,
  with the goal of reducing reliance on external controller software.
- **HYP36R Force** is a vehicle-informed force-feedback system developed to
  communicate more of what the car is doing instead of simply increasing or
  reshaping the effects already exposed by the original PC force-feedback
  implementation.

The goal is simple: preserve what makes OutRun 2006 special while removing
unnecessary friction between the player, their hardware, and the car.

> [!IMPORTANT]
> This project is under active development. Multi-device input and native force feedback are functional. More hardware testing and tuning are welcome.

## Multi Input

Multi Input exists to solve one of the more frustrating parts of running
OutRun 2006 on a modern PC.

The game comes from an era when today's combinations of USB wheels, separate
pedals, gamepads, arcade controls, and multiple simultaneous input devices were
not the norm.

Community solutions have made many of these configurations possible, often
through tools such as vJoy and other external input utilities. Multi Input takes
a different approach:

> **Make the functionality part of OutRun itself.**

The project expands OutRun 2006's PC input experience for modern and mixed
hardware configurations without asking the player to understand virtual
joystick plumbing, Windows device ordering, or chains of third-party
applications.

Multi Input is designed around a simple principle:

> **Connect your controls and drive.**

The objective is not to expose more configuration.

> **It is to require less of it.**

## HYP36R Force

As Multi Input evolved, force feedback became a much larger part of the
project.

OutRun 2006 already contains force-feedback and rumble-style effects. Existing
PC solutions can expose, modify, or strengthen those signals.

We wanted to investigate something different.

Instead of beginning with:

> **How can we make the existing effects stronger?**

we started asking:

> **What does the running game know about the vehicle, and can that information produce a more physics-derived steering experience?**

That question became **HYP36R Force**.

HYP36R Force uses information available from the running vehicle simulation to
construct the primary steering presentation. The system interprets vehicle
behavior to communicate changes in steering load, cornering response, grip
transition, release, and recovery while preserving road and collision
information as distinct parts of the experience.

This required more than tuning force strength by feel.

Development involved vehicle-state investigation, telemetry capture, controlled
driving scenarios, deterministic replay, force-channel separation,
software-headroom analysis, hypothesis testing, and physical wheel testing.

Some ideas worked.

Others did not.

> **When the evidence contradicted an interpretation, we changed it.**

That process became as important to HYP36R Force as the force model itself.

## A Physics-Derived Approach

The purpose of HYP36R Force is not to claim that OutRun exposes real-world
steering torque or that its internal values correspond directly to physical
units.

This is also not an attempt to turn OutRun into a modern simulation.

> **OutRun remains OutRun.**

The goal is to use the vehicle behavior already present in the game to create a
more informative steering experience.

Traditional effect-based force feedback generally begins with an event or
effect produced by the game and presents that information through the wheel.

HYP36R Force takes a different path:

> **Vehicle information → force interpretation → presentation → output conditioning → wheel**

Road and collision effects still have an important role. They simply do not
have to represent the entire steering experience.

## Reference+

**Reference+** is the recommended HYP36R Force profile.

It represents the current result of the project's vehicle-state research,
telemetry analysis, force-model development, replay validation, and physical
testing.

Reference+ focuses on:

- progressive steering and cornering load
- clearer changes in vehicle loading and unloading
- communication through grip release and recovery
- preservation of road and collision information
- useful detail without intentionally creating vehicle behavior unsupported by
  the available information

For most players, the intended setup is simple:

**Profile:** Reference+

**Strength:** 100%

**Wheel:** Connected

> **Then drive.**

Players who want to adjust the experience can independently reduce
**Steering Load**, **Road Detail**, and **Impact** while retaining the
underlying Reference+ behavior.

## Why This Is Different

Multi Input and HYP36R Force started by solving different problems, but they
share the same purpose.

For controls, that meant reducing the number of things standing between the
player's hardware and the game.

For force feedback, that meant looking deeper into the information already
available from the vehicle instead of relying entirely on effect strength to
communicate what the car is doing.

Neither goal is about making OutRun more complicated.

It is the opposite.

> **Less setup between you and the game.**

> **More information between the car and the wheel.**

## Project Lineage

OutRun 2006 C2C Multi Input is built on **[OutRun2006Tweaks by emoose](https://github.com/emoose/OutRun2006Tweaks)**.

That project provides the foundation that made this work possible.

Multi Input and HYP36R Force are developed by **[hyp36rmax](https://github.com/hyp36rmax)**.

The project preserves and credits its upstream foundation while exploring a new
direction for how OutRun 2006 can work and feel on modern hardware.

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

## Controls and configuration

Use axes, buttons, and hats from multiple USB devices at once. A wheel can be
combined with separate pedals, a shifter, button boxes, and a gamepad. Bindings
remain attached to the correct physical device across restarts, including
devices that expose identical or duplicated interfaces. The **Controllers**
tab shows live activity and provides in-game calibration; bindings can be
added, removed, or inverted without editing a file.

**Quick Setup** walks through steering, throttle, brake, shifting, and menu
controls. Each prompt gives you a six-second capture period, shows the detected
input, and asks for confirmation. You can retry a step without starting over.

HYP36R Force outputs through DirectInput, without vJoy or another FFB app. When
a wheel exposes separate input and force-output endpoints, Multi Input checks
which endpoint can create and start an effect rather than trusting only its
name or advertised capability. The **Force Feedback** tab shows one resolved
wheel, Reference+, and Strength. Advanced controls include Steering Load,
Road Detail, Impact, Invert Wheel, bounded Left/Right tests, and Re-detect
Wheel. Forces build gradually and stop if game updates pause. Device failures
are written to `OutRun2006Tweaks.log`.

Strength and the three Force Character controls run from 0–100%. At 100%, each
Force Character channel expresses the intended Reference+ balance; lowering it
reduces that channel. Above-Reference amplification is not a player control.
The Left/Right tests send a brief 20% diagnostic request independently of the
Strength slider. The model combines observed vehicle state with derived and
synthetic force components; its native values do not have proven physical
units. The [force architecture](docs/HYP36R_FORCE.md) explains those layers,
and the [V1 issue matrix](docs/V1_RELEASE_ISSUES.md) tracks checks still needed
before a release candidate.

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
setup experience inside the game.

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
