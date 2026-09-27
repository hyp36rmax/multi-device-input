# HYP36Rforce FFB v1 development — Advanced validated ranges

**PART I — ENGINEERING RECORD**

## Boundary

This milestone promotes the physically exercised Alpha / Force Character
ceilings into the normal Advanced FFB menu of development builds. It changes
the UI mapping, not the force equations. Road2, Event2 and AER remain separate
research lanes and are not integrated.

## One canonical value

Each channel retains its existing `[Controls]` integer multiplier:

| Setting | Reference+ | Development ceiling |
| --- | ---: | ---: |
| `WheelFFBSteeringLoad` | `100` / 1.00x | `130` / 1.30x |
| `WheelFFBRoadDetail` | `100` / 1.00x | `200` / 2.00x |
| `WheelFFBImpactLevel` | `100` / 1.00x | `150` / 1.50x |

The player-facing percentage is calculated for display and is never persisted
separately. Existing `100/100/100` files therefore remain exact Reference+ and
are not reinterpreted as the new ceilings. Missing settings retain the same
defaults. Numeric settings clamp on load and the Force Character composer
clamps again at use, so manual INI edits cannot bypass the ceilings.

## Player mapping

| Channel | 0% | Recommended | 100% |
| --- | ---: | ---: | ---: |
| Steering Load | 0.00x | 77% -> exact 1.00x | 1.30x |
| Road Detail | 0.00x | 50% -> exact 1.00x | 2.00x |
| Impact | 0.00x | 67% -> exact 1.00x | 1.50x |

The rounded Steering and Impact Recommended positions are explicit snap
points. Both the marker action and moving the slider to that position write
the canonical value `100`; they do not reconstruct an approximate multiplier.
Debug and telemetry continue reading the same canonical setting and therefore
show actual engineering multipliers.

`Reset to Reference+` writes only `100/100/100`. It does not alter Strength,
inversion, bindings, wheel selection or unrelated settings. Successfully
finishing Quick Setup selects the same canonical Reference+ Force Character;
cancelling Quick Setup leaves the current values alone.

## Force-equivalence proof

The gains remain after the existing Reference+ selection and before the
existing composition:

```text
resolved Directional / current Road / current Impact
    -> per-channel canonical gain
    -> existing sum
    -> existing tanh
    -> existing output ramp
    -> existing inversion and master Strength
    -> DirectInput
```

At `100/100/100`, every multiplication is exactly `1.0`, so the input channels
and composed `tanh` result are identical to the validated Reference+ path. No
M4, M5, LOAD, RELEASE, FREE, BITE, road/impact generation, gear behavior,
output conditioning, controller or device-resolution equation changed.

The deterministic test covers endpoints, exact Recommended snaps, identity,
one-channel removal, intermediate values, ceiling clamping, non-finite channel
inputs and repeated canonical serialization cycles. A separate generated-header
test proves the development build identity contains `1.0.0-dev+` followed by
the first seven characters of the actual configured commit.

## Development identity

Development builds identify as `1.0.0-dev+<shortSHA>` in startup diagnostics,
Debug/Research, About, notifications and telemetry/session metadata. CI names
the artifact `OutRun-2006-C2C-Multi-Input-HYP36R-1.0.0-dev-<shortSHA>` and
validates that form before upload. No `2.0.0-dev` identity is introduced.

Public tag `v1.0.0` and its release assets remain unchanged. This milestone
does not create a stable release or physical UAT package.

## Part II — Development Journey Recap

This Alpha / Force Character milestone exposed validated engineering ceilings
through a simple 0–100% player scale while keeping exact Reference+ identity at
the Recommended points. It also made development builds traceable as
`1.0.0-dev+<shortSHA>` without presenting them as HYP36Rforce FFB 2.0 releases.

The higher endpoints are available ranges, not new defaults or preferred
tuning. Road 2.0, R5/AER, Event 2.0, and 2.0 integration were not included.
