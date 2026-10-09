# OutRun 2006 C2C Multi Input

Modern multi-device controls and HYP36rforce FFB
for OutRun 2006: Coast 2 Coast on PC.

OutRun 2006 C2C Multi Input by hyp36rmax is a Windows PC extension built on
OutRun2006Tweaks v0.6.1.0 by emoose that lets OutRun 2006: Coast 2 Coast use
modern steering wheels, separate USB pedals and shifters, and HYP36rforce FFB
without virtual-controller or controller-order workarounds.

This README describes the **1.5.0** release candidate. The latest public release
remains **v1.0.0** until the validated v1.5 candidate is explicitly approved,
tagged, and published.

> [!IMPORTANT]
> **OutRun2006Tweaks v0.6.1.0 by emoose is already integrated into Multi Input. You do not need to download or install OutRun2006Tweaks separately.**

## How We Got Here

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
- **HYP36rforce FFB** is a vehicle-informed force-feedback system developed to
  communicate more of what the car is doing instead of simply increasing or
  reshaping the effects already exposed by the original PC force-feedback
  implementation.

The goal is simple: preserve what makes OutRun 2006 special while removing
unnecessary friction between the player, their hardware, and the car.

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

## HYP36rforce FFB

As Multi Input evolved, force feedback became a much larger part of the
project.

OutRun 2006 already contains force-feedback and rumble-style effects. Existing
PC solutions can expose, modify, or strengthen those signals.

We wanted to investigate something different.

Instead of beginning with:

> **How can we make the existing effects stronger?**

we started asking:

> **What does the running game know about the vehicle, and can that information produce a more physics-derived steering experience?**

That question became **HYP36rforce FFB**.

HYP36rforce FFB uses information available from the running vehicle simulation to
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

That process became as important to HYP36rforce FFB as the force model itself.

## A Physics-Derived Approach

The purpose of HYP36rforce FFB is not to claim that OutRun exposes real-world
steering torque or that its internal values correspond directly to physical
units.

This is also not an attempt to turn OutRun into a modern simulation.

> **OutRun remains OutRun.**

The goal is to use the vehicle behavior already present in the game to create a
more informative steering experience.

Traditional effect-based force feedback generally begins with an event or
effect produced by the game and presents that information through the wheel.

HYP36rforce FFB takes a different path:

> **Vehicle information → force interpretation → presentation → output conditioning → wheel**

Road and collision effects still have an important role. They simply do not
have to represent the entire steering experience.

## Reference+

**HYP36rforce FFB Reference+** is the recommended force profile.

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

## Road Detail 2.0

Road Detail 2.0 is one of the largest changes in v1.5. It expands what the
wheel can communicate beyond steering resistance, bringing forward kerbs,
road edges, bumps, and changes between driving surfaces when the game's native
signals provide evidence for them.

The goal is not simply stronger vibration. It is to make more of the road
perceptible without overwhelming steering information or losing OutRun's
arcade character. Development used controlled routes, synchronized telemetry,
replay analysis, and physical wheel testing. Not every visible texture creates
a distinct native feedback event, and the project does not present one where
the evidence does not support it.

## Classic and Enhanced Road

**Classic** keeps the established directional Road presentation. **Enhanced**
uses the Road Detail 2.0 presentation, with more pronounced road and kerb
information. Both modes use the same stable Surface channel, so changing Road
mode does not select the separate experimental Surface 2.0 research renderer.

Enhanced is the recommended starting mode. Classic remains available for
players who prefer the earlier road character or want a direct comparison.

## Stable Surface feedback

The player-facing **Surface** control adjusts the stable v1.5 texture and bump
presentation shared by Classic and Enhanced Road. It communicates supported
road texture, bump activity, and surface transitions through a bounded wheel
effect. Surface strength is adjustable independently from Steering Load, Road
Detail, and Impact.

The experimental Surface 2.0 renderer belongs to future research and is not
included in the v1.5 release candidate.

## Why This Is Different

Multi Input and HYP36rforce FFB started by solving different problems, but they
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

## Project Lineage & Attribution

OutRun 2006 C2C Multi Input is built on and includes **[OutRun2006Tweaks v0.6.1.0 by emoose](https://github.com/emoose/OutRun2006Tweaks)**. You do not need to download or install OutRun2006Tweaks separately.

That upstream project remains emoose's work and provides the foundation that
made this project possible.

Multi Input and HYP36rforce FFB are developed by **[hyp36rmax](https://github.com/hyp36rmax)**.

The project preserves and credits its upstream foundation while exploring a new
direction for how OutRun 2006 can work and feel on modern hardware.

## Quick Start

1. Install **OutRun 2006: Coast 2 Coast** on PC.
2. Download the complete Multi Input package from this repository's
   [Releases page](https://github.com/hyp36rmax/multi-device-input/releases).
3. Extract the package into the game's main directory, alongside the game
   files, not into a new subfolder.
4. Install the latest [Microsoft Visual C++ x86 Redistributable](https://aka.ms/vs/17/release/vc_redist.x86.exe).
5. Launch the included `OR2006C2C.exe`.
6. Open **Options → Controller**.
7. Complete **Quick Setup**, confirm each detected input, and save the bindings.
8. Open **Force Feedback**, confirm the wheel is connected, and configure
   Reference+.
9. Use **Test Left** and **Test Right** to confirm force direction, then drive.

Connect and power on the wheel, pedals, shifter, button boxes, and gamepads
before launch. Multi-device support is integrated; no vJoy installation is
required. Recommended wheel rotation is **270°**. Set rotation through the
wheel's hardware, firmware, driver, or manufacturer software before Quick
Setup. Multi Input detects and binds Steering; it does not change the physical
wheel rotation automatically.

If the live force pulls away from center, enable **Invert Wheel** under Advanced Force Feedback.

> [!CAUTION]
> Direct-drive wheels can produce substantial torque. Begin with a low hardware torque limit and a modest in-game strength. Keep hands clear during direction tests if you are unsure how the wheel will respond.

## Recommended FFB Settings

| Setting | Recommended starting value |
| --- | --- |
| Force Profile | Reference+ |
| Overall Strength | 100% |
| Steering Load | 77% |
| Road Detail | 50% |
| Impact | 67% |
| Road Mode | Enhanced |
| Wheel Rotation | 270° |

These are recommended starting values, not mandatory hardware settings. Wheel
bases vary substantially in torque and response. Begin with a conservative
wheel-side torque limit and reduce Overall Strength if necessary; do not assume
that another player's hardware gain is safe for yours.

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

HYP36rforce FFB outputs through DirectInput, without vJoy or another FFB app. When
a wheel exposes separate input and force-output endpoints, Multi Input checks
which endpoint can create and start an effect rather than trusting only its
name or advertised capability. The **Force Feedback** tab shows one resolved
wheel, Reference+, and Strength. Player controls include Steering Load, Impact,
Road Mode, Road Detail, Surface, Invert Wheel, bounded Left/Right tests,
Re-detect Wheel, and Reset to default. Forces build gradually and stop if game
updates pause. Device failures are written to `OutRun2006Tweaks.log`.

Strength and the Force Character controls use a clear 0–100% player display.
The Recommended markers select the Reference+ balance: Steering Load 77%, Road
Detail 50%, and Impact 67%. **Reset to default** restores the complete normal
Reference+ setup without changing controller bindings.

**Classic** preserves the original/reference directional Road behavior and
adds the shared HYP36rforce Surface channel. **Enhanced** uses the HYP36rforce
Enhanced Road presentation with that same Surface channel. **Surface** adjusts
how strongly road textures, bumps, and changes in driving surface are felt
through the wheel. Texture and Bump work together behind this one player
control; their engineering settings are not required for normal setup.

The Left/Right tests send a brief 20% diagnostic request independently of the
Strength slider. The model combines observed vehicle state with derived and
synthetic force components; its native values do not have proven physical
units. The [force architecture](https://github.com/hyp36rmax/multi-device-input/blob/multi-device-input/docs/HYP36R_FORCE.md) explains those layers.

## Optional HD Texture Packs

HD texture packs are optional and are not included in Multi Input. The existing
OutRun2006Tweaks replacement system loads compatible files from
`textures/load/` by default. A pack may use subfolders matching its texture
packages; preserve the layout supplied by its author rather than flattening the
files. Scene and interface replacement are enabled separately by the existing
`SceneTextureReplacement` and `UITextureReplacement` settings.

For the interface, **Options → Graphics → Install HD Textures** downloads,
checks, and installs the currently verified
[OR2 HD GUI package](https://github.com/envido32/OR2006Sprites). The **HD
Interface** setting can then enable or disable those installed files without
downloading them again. Other community packs should be installed according to
their own instructions; no additional pack or download source is claimed as
validated here.

## Telemetry

Telemetry is optional and available from the F11 Debug/Telemetry tools; it is
not part of normal controller setup. **General Capture** writes ordinary
recordings under:

```text
Telemetry/
└── General Capture/
```

Use the compact telemetry overlay to start and stop a recording. It shows the
current car and starting stage while recording. Files are named automatically
using the car, starting stage, date, and time:

```text
General Capture - Car - Starting Stage - YYYY-MM-DD - HHMMSS
```

The capture retains FFB, road, and final-output diagnostics for reproducible
troubleshooting. Scenario and Notes are optional metadata. Guided UAT and other
controlled research captures remain separate under `Telemetry/Research/`.
Telemetry is optional; normal players do not need to record a session or
understand the engineering CSV schema. See the [telemetry reference](docs/TELEMETRY.md)
for field ownership and capture details.

## Upgrading to v1.5

Back up personal files before extracting an update, then install the complete
release package over the game folder. Personal controller bindings,
device assignments, and normal player preferences remain compatible. The
package does not include or replace `OutRun2006Tweaks.user.ini`.

On first v1.5 startup, Multi Input normalizes obsolete development-only
HYP36rforce research overrides where required while preserving normal player
preferences. Use **Reset to default** in Advanced Force Feedback if you want a
complete normal FFB reset; it does not reset controller bindings. Deleting the
user INI is not required.

## Validated Hardware

Validated means the hardware has been reported working with Multi Input and
HYP36rforce FFB. It represents real-world testing, not official manufacturer
certification or a guarantee for every driver, firmware, peripheral, or
operating-mode combination. Hardware not listed may still work: Multi Input
uses general device discovery and capability-based FFB rather than a wheel
whitelist.

### Primary development / validated

| Manufacturer | Hardware |
| --- | --- |
| Fanatec | Podium Wheel Base DD2 |

The DD2 is the project's primary development and extensive physical-UAT
platform.

### Community validated

| Manufacturer | Hardware |
| --- | --- |
| Fanatec | CSL DD |
| Logitech | G29 |
| Logitech | G Pro Wheel |
| Logitech | RS50 |
| Thrustmaster | T248 |
| Thrustmaster | T300 RS |
| Thrustmaster | T818 |
| Simagic | Alpha Evo |
| Simucube | Simucube 3 |

## Common Questions

### Will this work with my setup?

#### Does OutRun 2006 support modern steering wheels?

Multi Input adds modern Windows wheel and multi-device support to OutRun 2006:
Coast 2 Coast. It discovers device capabilities rather than using a fixed wheel
whitelist, but compatibility can still vary with drivers, firmware, and
operating modes.

#### Can I use a wheel and separate USB pedals together?

Yes. Steering, pedals, buttons, and other controls can come from different
physical USB devices and operate together as one player setup.

#### Can I use a separate shifter?

Yes. A separate USB shifter can be assigned alongside a wheel and pedals
through Quick Setup or manual Bindings.

#### Can I use multiple USB devices at the same time?

Yes. Combining controls from multiple physical devices is a core Multi Input
use case and avoids relying on the original game's controller-order behavior.

#### Do I need vJoy, Joystick Gremlin, x360ce, or another virtual controller?

Normally, no. Multi Input provides in-game multi-device bindings and
DirectInput wheel FFB without requiring those workarounds.

#### Does Multi Input support direct-drive wheels?

Yes. Several direct-drive wheelbases have been physically validated. Begin
with a conservative wheel-side torque limit and use Test Left and Test Right
before driving.

#### Which wheels have been validated?

See [Validated hardware](#validated-hardware). The table records completed
real-world validation; it is evidence, not a compatibility whitelist.

#### Why isn't my wheel listed?

It may simply not have a documented completed validation. Hardware not listed
may still work through Multi Input's device-discovery and capability-based FFB
paths.

### How do I configure it?

#### What wheel rotation should I use?

**270°** is the recommended starting point for OutRun 2006. Set it through the
wheel's hardware, firmware, driver, or manufacturer control software before
running Quick Setup.

#### Does Multi Input change my wheel rotation or steering ratio?

No. Multi Input reads the steering axis but does not change hardware rotation
or implement a software steering-ratio adjustment. Hardware profiles or
wheel-side macros may be used where supported.

#### What is Quick Setup?

Quick Setup walks through the primary controls, detects each input, shows what
it found, and asks you to confirm it.

#### Can I bind controls manually?

Yes. Quick Setup is optional. Open Bindings to add, remove, invert, or configure
individual assignments.

#### My wheel appears in Controllers, but Quick Setup doesn't detect steering. What should I check?

Set the wheel to the recommended 270°, confirm its steering axis moves under
Controllers, and make sure the wheel is initialized in Windows or its
manufacturer software. Restart the game or use Re-detect Wheel where
appropriate, then investigate its compatibility or input mode if steering is
still not detected.

### How does force feedback work?

#### Does OutRun 2006 have force feedback?

The original PC input implementation does not provide the complete modern
wheel FFB experience offered here. Multi Input adds a DirectInput wheel backend
and HYP36rforce FFB while retaining the upstream OutRun2006Tweaks foundation.

#### What is HYP36rforce FFB?

HYP36rforce FFB interprets vehicle behavior to communicate steering load,
cornering response, grip transitions, road texture and surface changes, and
impacts while preserving OutRun's arcade character. The fuller explanation is
in [HYP36rforce FFB](#hyp36rforce-ffb).

#### My wheel is detected, but FFB doesn't work. What should I check?

Confirm the wheel shows Connected under Force Feedback, try Test Left and Test
Right, and use Re-detect Wheel if it was connected after startup. Check that it
is initialized in Windows or its manufacturer control panel. If the problem
remains, close the game normally and attach `OutRun2006Tweaks.log` to a
compatibility report.

#### Why does my wheel pull away from center?

Enable Invert Wheel under Advanced Force Feedback, then repeat Test Left and
Test Right before driving.

#### How do I make road feedback easier to feel?

Start with the recommended Enhanced mode, Road Detail 50%, and Surface 50%.
Steering Load, Road Detail, Impact, and Surface are separate player controls,
so adjust Road Detail or Surface rather than raising every force. Not every
visible road texture produces a distinct native feedback event.

### Installation and upgrades

#### Does this replace OutRun2006Tweaks?

No. Multi Input is built on and includes **OutRun2006Tweaks v0.6.1.0 by
emoose**. That project remains the upstream foundation; Multi Input and
HYP36rforce FFB are additions developed by hyp36rmax.

#### Which OutRun 2006 executable or version is supported?

The complete release package includes the validated replacement
`OR2006C2C.exe` used by Multi Input's native integrations. Install the complete
package rather than mixing the DLL with an arbitrary game executable.

#### Can I use this with an existing OutRun 2006 installation?

Yes. Start with an installed Windows PC copy of OutRun 2006: Coast 2 Coast,
back up personal files, and extract the complete Multi Input package into the
game's main folder as described in [Quick start](#quick-start).

#### Will my settings and bindings survive an upgrade?

The release package does not include or replace `OutRun2006Tweaks.user.ini`.
Bindings, device assignments, and normal player preferences remain compatible;
v1.5 may normalize obsolete development-only HYP36rforce research overrides.

### Optional features

#### Are HD Interface textures included?

No. The assets are optional and are not bundled in the release package. The
Graphics menu can download, validate, and install the supported HD
Interface package.

#### What happens if HD Interface is installed but disabled?

The files remain installed, but the original game interface is used while HD
Interface is disabled. Re-enable it to use the installed files again without a
new download, provided the installation remains valid.

#### Is telemetry required to play?

No. Telemetry and Guided UAT are optional diagnostic and research tools. Normal
setup and gameplay do not require them; see [Telemetry](#telemetry) for General
Capture and research-capture details.

## Troubleshooting

If a device is missing or FFB does not work:

1. Open **Controllers** and verify whether the device and its live inputs appear.
2. Open **Force Feedback**, confirm the wheel is connected, and try both direction tests.
3. Select **Re-detect Wheel** under Advanced Force Feedback if hardware was connected after startup.
4. If Quick Setup or Add Binding does not open with several USB peripherals
   attached, temporarily disconnect the additional peripheral and retry. If
   that resolves it, report the exact device model and attach
   `OutRun2006Tweaks.log`.
5. If the force settings are confusing or were disabled by an older setting,
   use **Reset to default** in Advanced Force Feedback. Restart the game if
   the profile change requests it. This does not reset controller bindings.
6. If the force pulls the wrong way, enable **Invert Wheel** and repeat the
   short direction tests.
7. Close the game normally so the latest log is complete.
8. Open a [GitHub issue](https://github.com/hyp36rmax/multi-device-input/issues) and attach `OutRun2006Tweaks.log`.

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

### Missing or corrupt binkw32.dll

If an old installation reports a missing or corrupt `binkw32.dll`, start from
a clean legitimate game installation before applying the complete Multi Input
package. Reinstalling the underlying game resolved the reported case; the
missing game file was not created by Multi Input.

## Original OutRun2006Tweaks features

This fork retains the fixes and enhancements provided by OutRun2006Tweaks, including framerate correction and interpolation, graphics improvements, shorter loading, restored online multiplayer support, overlay configuration, expanded audio support, and numerous game bug fixes.

For the upstream project overview, community, and original releases, visit [OutRun2006Tweaks v0.6.1.0 by emoose](https://github.com/emoose/OutRun2006Tweaks).

Steam Deck and Linux users may need this launch option for the wrapper to load:

```text
WINEDLLOVERRIDES="dinput8=n,b" %command%
```

Native wheel FFB in this fork targets Windows DirectInput and may behave differently through Wine or Proton.

## Project Direction

Reference+ is the default HYP36rforce FFB profile. Reference remains available as
a comparison and fallback. Wheels and drivers differ, so a working result on
one device does not guarantee identical force or controls on every setup.

v1.5 is the stable Multi-Device Input, Reference+, Road Detail 2.0, and Surface
foundation described in this README. Separate v2.0 research includes Arcade
Experience Reconstruction (AER), Lindbergh-inspired arcade force feedback, an
experimental Surface 2.0 renderer, and optional SimHub integration. Those
research paths are not included in v1.5.

The next stable priorities are broader wheel and cross-car validation,
device-aware calibration, targeted surface fidelity research, and keeping the
normal setup experience inside the game.

Start with the online [documentation map](https://github.com/hyp36rmax/multi-device-input/blob/multi-device-input/docs/README.md).
It links the force architecture, native dynamics evidence, telemetry reference,
presentation and safety boundary, development history, and roadmap. The
detailed milestone notes remain available there as the research record behind
those summaries. The [research preservation index](https://github.com/hyp36rmax/multi-device-input/blob/multi-device-input/research/README.md)
records which raw captures still exist outside Git and which findings survive
only in documentation.

## Building

Building requires Visual Studio 2022, CMake, and Git.

Clone this repository with its submodules, run `generate_vs2022.bat`, open `build\outrun2006tweaks-proj.sln`, and build the Release configuration for Win32.

`cmake.toml` is the authoritative cmkr build definition. `CMakeLists.txt` is generated, so contributors should update `cmake.toml` rather than editing `CMakeLists.txt` directly.

Pushes and pull requests are also compiled by the Windows workflow under the repository's Actions tab.

## Credits

### Multi Input and HYP36rforce FFB

Developed and hardware-tested by [hyp36rmax](https://github.com/hyp36rmax).

Special thanks to **el julo** on Discord for early troubleshooting and for inspiring the intuitive approach behind this project.

Thanks to **GATS** for experiential and community feedback that surfaced useful
questions for controlled testing, and to **THP32** for additional PC/PS2 FFB
research and technical reference material that identified alternate paths worth
independent investigation. Their material was not copied or integrated into
HYP36rforce FFB; findings and implementation decisions documented here come from
this project's own research and validation unless a record explicitly says
otherwise.

### Original project

Based on [OutRun2006Tweaks v0.6.1.0](https://github.com/emoose/OutRun2006Tweaks) by [emoose](https://github.com/emoose), with contributions from its community.

Thanks to [debugging.games](http://debugging.games) for hosting OutRun 2 SP debug symbols used by the original project.

## License

This fork retains the upstream project's MIT License and copyright notice. See [LICENSE.md](LICENSE.md).
