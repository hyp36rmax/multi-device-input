# HYP36R Information Amplification and Safety Envelope

> Historical S1 architecture. The consolidated current boundary is documented
> in [PRESENTATION_AND_SAFETY.md](PRESENTATION_AND_SAFETY.md).

S1 defines a future architecture. It does not add runtime components, change
Force, increase amplitude, alter DirectInput, or create device profiles.

The frozen references are:

- M4 Force 2.0 Foundation:
  `42118c029340db95e5f77539b8696bc6b824a08d`
- M5 Force 2.1 Research:
  `8c5d68d92b19a21a09b3840413b8a453520517cc`

`M4_ONLY` remains the default. `M5_LATERAL_ACTIVE` remains experimental.

## Current output architecture

The current active path selects a directional result from Legacy, M4, or the
eligible M5 experiment. It then combines that directional value with impact
and road, applies `tanh` and the existing output ramp, and passes one normalized
force value to `WheelForceFeedback::drive()`.

`drive()` applies inversion, multiplies by the user strength, clamps the result
to the DirectInput nominal range, and submits the constant-force request.

```text
vehicle observations
        |
        v
Legacy / M4 / experimental M5 directional selection
        + impact
        + road
        |
        v
tanh -> output ramp
        |
        v
drive()
        |
        v
user strength -> inversion -> DirectInput clamp
        |
        v
hardware
```

This path is intentionally compact, but presentation strength, exposure
management, and physical device mapping are not independent concepts within
it. The existing final clamp is an output boundary, not an information-aware
safety envelope.

## Proposed layered architecture

```text
VEHICLE INFORMATION
        |
        v
FORCE MODEL
        |
        v
INFORMATION PRESENTATION
        |
        v
SAFETY / HEADROOM ENVELOPE
        |
        v
DEVICE CALIBRATION
        |
        v
EXISTING OUTPUT CONDITIONING
        |
        v
HARDWARE
```

The interfaces between these layers should use normalized, device-independent
values. Each layer should expose what it changed and why. No downstream layer
may rewrite upstream vehicle meaning.

## Layer responsibilities

### Vehicle information

This layer identifies what the car is communicating. Current examples include
the M4 grip envelope, M5 four-corner context, surface classification, and any
future validated native state.

It owns source validity, semantic confidence, and evidence boundaries. It must
not contain presentation gain, hardware-specific scaling, torque claims, or
device protection logic.

### Force model

This layer decides how validated information is represented as Force. It owns
directional load, RELEASE, FREE, BITE, restoration, road, impact, and vibration
semantics.

M4 remains authoritative. M5 may remain subordinate to M4 only within its
validated gate. The Force model must not contain wheel profiles, physical
torque mapping, or protection thresholds.

### Information presentation

This future layer decides how clearly a validated Force relationship is
communicated. A Reference presentation, a future Arcade presentation, and a
simple user clarity or strength preference belong here.

Presentation may change perceptual amplitude or contrast while retaining the
Force model's sign, phase, relative ordering, and authority. It may not create
vehicle information, redefine RELEASE or BITE, turn M5 into a steering source,
or bypass safety.

### Safety and headroom envelope

This future layer protects dynamic range and output integrity. It observes the
presented channels and their recent exposure before deciding whether the
requested presentation fits within the available normalized envelope.

It should preserve relationships before reducing everything equally. It must
report when reserve is low, when intervention occurs, and whether the pressure
came from peaks, sustained load, or repeated saturation. S1 assigns no
thresholds and implements no limiting.

### Device calibration

This future layer maps safe normalized output to a physical device class or a
validated device profile. Possible classes include high-torque direct drive,
mid-range direct drive, entry direct drive, and belt/gear wheels.

Calibration may describe verified capability, response, and safe mapping. It
must not alter vehicle semantics or use one DD2 observation as a universal
rule. Device profiles are not created in S1.

### Existing output conditioning

The existing output ramp, user master strength, inversion, DirectInput nominal
clamp, focus/device guards, and effect submission remain the final operational
boundary. Future layers must integrate before this boundary without silently
replacing its protections.

## Safety-zone model

The zones are conceptual states, not thresholds.

### Normal information zone

The requested presentation fits comfortably. The full intended relationships
and transient contrast remain available.

### Headroom awareness zone

Output remains valid, but peak or sustained reserve is decreasing. Monitoring
should distinguish a brief intentional transient from accumulating sustained
load. No intervention is implied merely by entering this zone.

### Soft protection zone

A future system may reduce sustained presentation pressure while preserving
short information-bearing peaks and relative channel differences. The goal is
to retain communication, not hide clipping with a lower but equally flattened
signal.

### Hard output boundary

The existing normalized and DirectInput output limits remain the final
boundary. Future safety work must avoid treating repeated contact with this
boundary as normal operation.

## Headroom model

Future observation should distinguish these measurements:

- **Current magnitude:** absolute value of the current composed request.
- **Recent peak:** highest magnitude within a short, defined observation
  window, with the window reported in telemetry.
- **Sustained exposure:** an RMS-like or energy-like rolling measure over a
  longer, defined window.
- **Saturation occupancy:** fraction of samples within a documented distance
  of the output boundary.
- **Limiter occupancy:** fraction of samples affected by a future soft or hard
  intervention, separated by intervention type.
- **Transient headroom:** reserve between the current sustained presentation
  and the peak allowance available for short events.
- **Slew exposure:** frequency and size of rapid output changes, kept separate
  from absolute magnitude.

The selected windows, thresholds, weighting, and release behavior remain open
research questions. Raw channel values and the final composed value must remain
observable so that a healthy-looking aggregate cannot conceal one saturated
channel.

## Transient and sustained output

A short collision impulse and a sustained directional load are different
events. Future evaluation should label channel origin and measure both short
peak behavior and longer exposure.

The preferred strategy is to reserve room for bounded, information-bearing
transients while controlling sustained presentation pressure gradually. A
future envelope should avoid pumping, sudden gain steps, or a recovery ramp
that could feel like a manufactured force event.

No transient automatically receives permission to exceed the hard output
boundary. No sustained-load rule may erase M4's RELEASE, FREE, BITE, or
returned-load relationships.

## Channel-priority principles

Priority means preservation of information structure, not an unconditional
gain hierarchy.

1. **M4 directional semantics remain authoritative.** Safety may scale a
   presented result but may not change its phase, sign, or steering meaning.
2. **M5 remains subordinate.** Future M5 amplification cannot dominate the M4
   directional value, survive a BITE or FREE veto, create force from zero, or
   exceed the governing magnitude boundary.
3. **Impact is a bounded transient.** It may use reserved transient headroom,
   but cannot bypass the envelope.
4. **Road and tire-slip vibration carry detail.** Sustained-load management
   should avoid erasing them merely because the directional channel consumes
   most of the range.
5. **Composition remains inspectable.** Any future allocation must expose
   channel input, granted contribution, and reason for reduction.

A possible future allocator could protect a directional baseline, reserve a
bounded transient budget, and preserve a small detail budget. S1 does not
choose budgets or implement channel scaling.

## Device-calibration architecture

A calibration record should eventually describe verified facts rather than
brand assumptions. Candidate inputs include device identity, class, known
physical capability when authoritative, usable normalized range, tested
response behavior, and provenance/version.

The output of calibration should be a device mapping applied after the safety
envelope. Presentation and safety should operate in normalized HYP36R space;
calibration translates the safe result to the device.

An unknown device should use a conservative generic mapping. A device-specific
profile must never raise the universal Force model's authority or silently
change M4/M5 semantics.

## Fail-safe model

Future failure handling should be explicit:

| Condition | Intended fallback |
| --- | --- |
| Invalid calibration | Conservative generic mapping; report invalid profile |
| Unknown wheel | Conservative generic device class |
| Missing profile | Generic mapping with profile state visible |
| Non-finite input | Reject or sanitize the affected sample before output; log the fault |
| Configuration error | Fall back to validated defaults and identify the rejected value |

Where technically safe, conservative generic FFB is preferable to silently
removing all feedback. Existing focus loss, device loss, race exit, and effect
failure protections remain authoritative. The exact conditions requiring a
zero-force stop belong to a later failure-mode review.

## Minimum future telemetry schema

S1 adds no fields. A later passive observer needs only enough data to prove the
layer boundaries:

| Field | Meaning |
| --- | --- |
| `presentation_mode` | Selected presentation, such as Reference or Arcade |
| `presentation_input` | Force-model result entering presentation |
| `presentation_output` | Presented normalized result before safety |
| `envelope_input` | Exact composed value entering the safety envelope |
| `envelope_output` | Exact safe normalized value leaving the envelope |
| `instantaneous_headroom` | Remaining current normalized reserve |
| `recent_peak_output` | Peak magnitude over the declared short window |
| `sustained_output` | Declared rolling sustained-exposure measure |
| `saturation_occupancy` | Boundary-near sample fraction over its declared window |
| `soft_limit_activity` | Whether and how much soft intervention occurred |
| `hard_limit_activity` | Whether the hard boundary changed the request |
| `device_profile` | Selected profile or generic class identifier |
| `calibration_state` | Valid, generic, missing, invalid, or unavailable |

Before implementation, the schema must also record window durations and units
in capture metadata. Channel-level input and granted output should be added
only when a passive channel-allocation study begins.

## Amplification research method

Future testing must keep five boundaries separate:

1. **Perceptual floor:** the lowest level at which information becomes useful.
2. **Preferred information range:** clear, natural communication.
3. **Manufactured-effect boundary:** where communication begins to feel
   exaggerated or artificial.
4. **Software headroom boundary:** where clipping, sustained saturation, or
   lost signal relationships appear.
5. **Device safety boundary:** hardware-specific evidence of undesirable
   behavior.

A safe study should begin with passive telemetry and replay. Physical testing
should then use progressive bounded steps, short controlled exposure, fixed
scenarios, explicit stop conditions, and monitoring of headroom and device
status. Each step should return to the validated baseline before proceeding.

Testing must stop well before known undesirable behavior. Increasing Force
until a device shuts down is not an acceptable method.

## DD2 evidence boundary

Historical testing on one Fanatec DD2 used hardware FFB around 50–60%, depending
on the test period. Increasing game/software Force beyond the normal research
baseline eventually produced a power or protection shutdown.

This establishes only that the tested DD2 and configuration encountered an
undesirable high-output condition. It does not establish a universal torque
ceiling, percentage, production limit, or safe threshold for another DD2 or
another wheel. S1 does not reproduce or quantify the shutdown boundary.

## AER relationship

AER means Arcade Experience Reference. A future validated Arcade presentation
belongs in the presentation layer:

```text
Force model
    -> Reference / Arcade presentation
    -> safety and headroom envelope
    -> device calibration
```

Arcade presentation can inform expression and contrast. It cannot bypass
safety, replace native OutRun physics, redefine HYP36R semantics, or rely on an
unvalidated interpreted third-party Force model.

## User experience

The normal player should not need to understand gain staging, RMS, clipping,
DirectInput magnitude, saturation occupancy, or device torque mapping.

HYP36R should eventually select a safe default from validated information and
calibration. The likely simple controls are `Force Strength` and a presentation
choice such as `Reference` or `Arcade`. Advanced calibration remains optional
and must not be required for normal setup. S1 does not define the final UI.

## Risks and unresolved questions

- Which observation windows represent peaks and sustained exposure without
  hiding short OutRun events?
- How much transient reserve preserves impact and detail without encouraging
  repeated boundary contact?
- How should sustained management release without pumping or manufacturing a
  load-return sensation?
- Which channel relationships must remain invariant under presentation and
  safety processing?
- What passive metric best reveals lost contrast before hard clipping occurs?
- Which calibration facts are authoritative and portable across firmware,
  driver modes, and device revisions?
- When must a fault command zero force instead of conservative generic output?
- How should user strength interact with presentation, safety, and device
  calibration without becoming a bypass?
- How should AER observations be normalized before comparison with Reference
  presentation?

## Recommended S2 milestone

**S2 — Passive Output Exposure and Headroom Telemetry Architecture.**

S2 should define, offline-test, and then add passive observation only for the
existing unchanged output path. It should validate measurement windows and
units for current magnitude, recent peak, sustained exposure, saturation
occupancy, and transient headroom. It must not amplify, limit, allocate
channels, add profiles, or change DirectInput output.
