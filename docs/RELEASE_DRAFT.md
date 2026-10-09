# OutRun 2006 C2C Multi Input v1.5.0

Modern controller support, HYP36rforce FFB, and enhanced road feedback for **OutRun 2006: Coast 2 Coast** on PC.

> [!IMPORTANT]
> **OutRun2006Tweaks by emoose is already integrated into Multi Input. You do not need to download or install OutRun2006Tweaks separately.**

## Highlights

- **Modern Multi-Device Input:** Use a steering wheel, separate pedals, shifter, button box, and gamepad together. No vJoy installation required.
- **HYP36rforce FFB:** Experience physics-derived steering feedback using native vehicle information, with Reference+ as the recommended force profile.
- **Road Detail 2.0:** Enhanced road and kerb feedback brings more of OutRun's road surfaces, bumps, and transitions to your steering wheel.
- **Classic & Enhanced Road:** Choose between the established road presentation and enhanced road feedback.
- **Surface Feedback:** Feel additional road texture and bump detail, with adjustable intensity.
- **Force Character:** Independently adjust Steering Load, Road Detail, and Impact without changing the underlying Reference+ experience.
- **Modern Wheel Compatibility:** Automatic FFB device detection, wheel re-detection, inversion, and controlled Left/Right force tests.
- **270° Steering Recommendation:** A recommended starting point for responsive arcade-style steering.
- **Integrated Telemetry:** Record driving sessions, inspect FFB behavior, and share captures for troubleshooting and community validation.
- **OutRun2006Tweaks Included:** Retains the original fixes, graphics improvements, and enhancements developed by emoose.

## How We Got Here

Multi Input started with a familiar challenge: getting modern USB wheels, separate pedals, shifters, and other controllers working together in OutRun 2006 without relying on multiple external utilities.

The goal was simple:

> **Less setup between you and the game.**

Once multi-device controls were working, we turned our attention to force feedback.

Rather than simply amplifying the game's existing vibration effects, we began investigating the vehicle information available during gameplay.

That research became **HYP36rforce FFB**, a physics-derived interpretation of steering load, vehicle response, road activity, and impacts.

With v1.5, we've continued refining how that information reaches the wheel.

### Road Detail 2.0

One of the biggest improvements in v1.5 is **Road Detail 2.0**, developed through extensive telemetry research and community testing.

Our focus expanded beyond steering resistance to the smaller details that make the road feel alive.

Kerbs, road edges, bumps, and surface transitions can now produce more pronounced feedback where the game's native signals support them.

Players can choose between **Classic** and **Enhanced** road presentation, while the Surface control adjusts additional texture and bump sensations.

The goal isn't simply stronger vibration.

> **It's feeling more of the road without losing OutRun's arcade character.**

Development has been guided by controlled driving scenarios, telemetry analysis, and physical testing across different wheelbases.

Not every visible surface produces a dedicated feedback event, and we continue to distinguish verified vehicle information from interpreted effects.

## Setup and Drive

1. Install **OutRun 2006: Coast 2 Coast** on PC.
2. Download the complete Multi Input v1.5 release and extract it into the game's main directory.
3. Install the [Microsoft Visual C++ x86 Redistributable](https://aka.ms/vs/17/release/vc_redist.x86.exe).
4. Launch the included `OR2006C2C.exe`.
5. Open **Options → Controller** to access Multi Input.
6. Run **Quick Setup** to configure and confirm your devices.
7. Set your wheel rotation to **270°** as a recommended starting point.
8. Open **Force Feedback**, confirm your wheel is connected, and select **Reference+**.
9. Test force direction using **Test Left** and **Test Right**, then drive.

### Recommended Force Feedback Settings

Reference+ is the recommended starting experience.

| Setting | Recommended |
|---|---|
| Force Profile | Reference+ |
| Overall Strength | 100% |
| Steering Load | 77% |
| Road Detail | 50% |
| Impact | 67% |
| Wheel Rotation | 270° |
| Road Mode | Enhanced |

These are starting recommendations, not hardware requirements.

Direct-drive wheels can produce substantial torque. Begin with conservative wheelbase strength and adjust the game's overall Strength if necessary.

### Optional HD Texture Packs

Multi Input retains OutRun2006Tweaks' integrated texture-replacement functionality, allowing compatible HD texture packs to enhance the game's visuals.

Install compatible texture packs according to their instructions, using the game's texture-replacement directory.

HD packs are optional and do not affect Multi Input controls or HYP36rforce FFB.

*The final release documentation will identify verified HD pack installation paths and sources.*

### Integrated Telemetry

v1.5 includes telemetry capture tools for troubleshooting, community testing, and HYP36rforce development.

**General Capture** records gameplay and FFB information, including vehicle and stage identification, force output, road activity, and other diagnostic signals.

Captures are automatically organized under:

```text
Telemetry/
└── General Capture/
```

Recording filenames include the car, starting stage, date, and time.

Advanced telemetry scenarios and guided UAT remain available for research and controlled testing.

Telemetry is optional and does not need to be enabled for normal gameplay.

## Validated Hardware

HYP36rforce FFB is designed for broad compatibility with modern steering wheels, rather than a fixed list of supported manufacturers.

**Primary development hardware:** Fanatec Podium DD2.

Community validation includes hardware from Fanatec, Logitech, Thrustmaster, Simagic, and Simucube.

Hardware behavior can vary depending on drivers, firmware, wheelbase strength, and configuration.

The project prioritizes a universal FFB foundation. Changes to the force model are guided by repeatable evidence and cross-hardware testing rather than individual wheel preferences.

## What's Next

v1.5 establishes our stable Multi-Device Input and HYP36rforce FFB experience.

Our ongoing v2.0 research explores:

- **Arcade Experience Reconstruction (AER):** A Lindbergh-inspired interpretation of original OutRun 2 SP arcade steering feedback.
- **Surface 2.0:** Experimental road texture and bump rendering.
- **SimHub Integration:** Optional telemetry for bass shakers, dashboards, wind simulation, and other external hardware.

These experimental features are not included in v1.5.

## Project Attribution

Multi Input is developed by [hyp36rmax](https://github.com/hyp36rmax) and built upon [OutRun2006Tweaks by emoose](https://github.com/emoose/OutRun2006Tweaks).

OutRun2006Tweaks remains emoose's work and provides the integrated foundation of this project. Multi Input preserves its upstream attribution, fixes, and licensing.

Thanks to **el julo** for early troubleshooting and helping inspire a more intuitive multi-device experience, and to our community testers for their continued hardware feedback and telemetry contributions.
