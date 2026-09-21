# HYP36R Force

HYP36R Force is this fork's independent force-feedback interpretation for
OutRun 2006. It reads observed game state, builds a bounded directional-force
request, adds the existing road and impact channels, and sends the normalized
result through the DirectInput backend. It is not Howard Casto's historical
X-Force and it does not claim to reproduce steering-rack torque or tire forces.

The current research lineage has three useful baselines:

| Baseline | Commit | Meaning |
| --- | --- | --- |
| Force 2.0 Foundation | `42118c029340db95e5f77539b8696bc6b824a08d` | Validated M4 grip-envelope model |
| Force 2.1 Research Reference | `8c5d68d92b19a21a09b3840413b8a453520517cc` | M4 plus frozen M5 lateral-context research |
| S-series graduation | `7036d3d8b6cfd4af6454636ce3e0c1c1bed2b35b` | Validated Reference and experimental Reference+ presentation foundation |

## Current path

```text
steering input + speed + derived motion context
                    |
                    v
        Legacy center-out directional model
                    |
                    v
native steering reference and response state
                    |
                    v
       M4 grip-authority envelope
  LOAD -> RELEASE -> FREE -> BITE -> LOAD
                    |
                    v
      optional eligible M5 characterization
                    |
                    v
        Reference / Reference+ presentation
                    |
                    + road + impact
                    |
                    v
          tanh -> output ramp -> drive()
                    |
                    v
      strength -> inversion -> clamp -> DirectInput
```

The layers matter. Native observations describe what OutRun is doing. The
semantic interpreters make those values usable. M4 and M5 decide which
information belongs in the directional request. Presentation controls how
that information is expressed. The device layer maps the normalized request
to a wheel. A later layer cannot be used as evidence that an earlier semantic
claim is correct.

## Legacy foundation

The original HYP36R center-out model remains the directional backbone. It uses
normalized steering input, native vehicle motion magnitude, and a lateral
projection derived from the car orientation matrix. From those it builds a
speed-scaled spring, steering-input damper, and a derived grip-loss reduction.
The grip estimate is synthetic context, not native tire telemetry.

OutRun's restored Xbox vibration calculation supplies the source envelopes for
road texture and collision onset. HYP36R turns those envelopes into a bounded
6 Hz road carrier and a decaying impact kick. Those wheel effects are
synthetic, even though their triggers originate in game state.

## M4: the Force 2.0 foundation

M4 introduced native vehicle-response information without turning it into a
steering target. It interprets the difference between the player's steering
reference and the vehicle response as changing directional authority:

- **LOAD:** normal directional authority.
- **RELEASE:** reference and vehicle response diverge; up to 25 percent of the
  existing directional load is removed progressively.
- **FREE:** meaningful vehicle displacement remains while directional
  authority is reduced. The wheel does not steer the slide for the driver.
- **BITE:** credible vehicle-led reconvergence allows part of the removed load
  to return progressively.
- **RETURNED LOAD:** the response and dynamic context have settled back into
  normal authority.

BITE detection deliberately separates vehicle convergence from the driver
merely moving the steering reference toward the response. If recovery fails,
the restoration allowance returns toward the M4C unloading trajectory at a
bounded rate. No phase adds a pulse, snap, or manufactured state effect.

M4 never exceeds the Legacy directional magnitude, creates force from zero,
reverses the existing direction, or uses the native response angle to command
the wheel. Road, impact, output ramp, user strength, inversion, and the final
DirectInput clamp remain outside the grip-envelope modulation.

## M5: four-corner context

M5 added observed FL/FR/RL/RR vehicle context. The active research candidate
uses only the lateral distribution during eligible developing RELEASE frames.
It can subtly qualify the existing M4 directional value; it cannot invent a
direction or replace M4's authority.

The provisional M5I equation compares front and rear lateral-response
magnitudes, applies activity and confidence gates, and caps modulation at four
percent before the Legacy magnitude boundary. BITE, FREE, surface
contamination, low activity, mixed response, and non-developing phases veto
the contribution. The current evidence is a Ferrari Dino baseline. Cross-car
validation is still required before M5 can become a universal or production
default.

## Presentation modes

`REFERENCE` is the validated permanent comparison and fail-safe. It selects
M4-only directional information at Presence 1.00 with no M5 secondary input.

`REFERENCE_PLUS_EXPERIMENTAL` is the validated experimental presentation
foundation. It selects:

```text
M4 primary
  + eligible M5 delta * Contrast 4
  -> five-percent primary-relative secondary budget
  -> zero and sign preservation
  -> Legacy directional-authority boundary
  -> Presence 1.20
```

Reference+ produced clearly greater steering presence in physical S9 testing
while retaining natural behavior. It remains experimental because final
intensity, cross-car behavior, device calibration, and a physical hardware
safety envelope are unresolved. Road, impact, and vibration use the same
expression as Reference.

## Output boundary

The selected directional value is combined with road and impact, passed
through `tanh`, and multiplied by the restart-safe output ramp. The wheel
backend optionally inverts the result, applies the user's master strength, and
clamps the DirectInput request to `[-10000, 10000]`.

Telemetry's `ffb_raw` is the normalized value passed to `drive()`.
`ffb_final` is the final software request after strength, inversion, and clamp.
Neither is measured wheel torque or proof that a driver accepted the update.

## Known limits

The current model does not establish self-aligning torque, steering-rack
torque, literal suspension travel, tire normal load, tire force, physical slip
angle, grip percentage, or friction-circle utilization. Native four-corner
units remain unknown. M5 normalization is not validated across cars. Software
headroom is measured; hardware safety headroom is not.

The native evidence and its confidence boundaries are maintained in
[NATIVE_DYNAMICS.md](NATIVE_DYNAMICS.md). Presentation and hardware boundaries
are maintained in
[PRESENTATION_AND_SAFETY.md](PRESENTATION_AND_SAFETY.md). The development path,
including the failed assumptions that produced today's routing fields and
fallbacks, is preserved in [DEVELOPMENT_HISTORY.md](DEVELOPMENT_HISTORY.md).
