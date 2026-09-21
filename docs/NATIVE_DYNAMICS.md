# Native OutRun dynamics

This document records what the project currently knows about OutRun's native
vehicle state. The names are intentionally confidence-aware. A useful
relationship in telemetry is not automatically a physical quantity with known
units.

## Steering-response state

The player car contains a related state cluster around `EVWORK_CAR + 0xD38`
through `+0xD48`, along with the signed-angle reference at `field_32`. Static
lineage work and controlled captures support the following interpretation:

| Value | Current interpretation | Confidence |
| --- | --- | --- |
| `field_32` | Steering/reference angle in signed native-angle representation | Confirmed representation; semantic role strongly supported |
| `D38` | Response authority | Strongly supported |
| `D3C` | Overshoot/transition attenuation | Strongly supported |
| `D40` | Physics-direction state used in the response calculation | Strongly supported; exact physical meaning unresolved |
| `D44` | Correction applied to the reference for a physics-facing combined angle | Strongly supported |
| `D46` | Accumulated vehicle-response angle | Strongly supported |
| `D48` | Per-update response increment | Strongly supported |

The signed angle fields use `pi / 32768` radians per integer unit with 16-bit
wrap behavior. The semantic interpreter exposes reference angle, response
angle, wrapped reference/response error, corrected reference, response rate,
authority, and attenuation. `field_283` marks a native transition window;
dynamic interpretation is suppressed while that counter is nonzero rather
than silently substituting retained state.

The response angle is vehicle information, not a desired wheel angle. Response
rate and reference/response error help identify divergence and vehicle-led
reconvergence, but neither directly creates steering torque.

Early research treated the D38-D48 fields as neutral steering-response
candidates. Their roles were narrowed through static writer/reader tracing,
stationary steering captures, moving captures, and temporal comparison. No
field was renamed to Howard Casto's historical X-Force because the evidence
does not establish that identity.

## Native four-corner structure

The game's physics context contains a four-entry pointer table at `+0x248` and
four inline corner blocks beginning at `+0x258`, each `0xF4` bytes apart. The
observer accepts a frame only when every pointer matches its expected inline
block and all recorded floats are finite.

Controlled known-side surface testing established the order:

```text
0 = front left    1 = front right
2 = rear left     3 = rear right
```

The ordering was not assumed from memory layout. Initial telemetry kept
neutral corner numbers. Front/rear pairing emerged first, then same-side
correlation and a controlled left-side surface transition established the
left/right order.

## Corner values

| Offset | Supported description | Evidence boundary |
| --- | --- | --- |
| `+0x14` | Per-corner surface/contact classification | Raw category only; material names are not established |
| `+0x28` | Reference-relative corner displacement/loading context | Literal suspension travel, physical distance, normal load, and tire load remain unproven |
| `+0xAC` | Native lateral contact-plane response candidate | Not established as lateral tire force or self-aligning torque |
| `+0xB0` | Native longitudinal contact-plane response candidate | Not established as longitudinal tire force |
| `sqrt(AC^2 + B0^2)` | Exact observed combined-response relationship | Not grip percentage or friction-circle utilization |

`+0x28` is the clearest example of why the terminology changed. It began as a
possible suspension-related value. Runtime evidence showed a stable
reference-relative corner context that follows loading and displacement
changes, but it did not establish literal suspension travel. The current name
preserves the useful observation without promoting the original hypothesis to
fact.

The `+0xAC` channel contains an alternating stationary component. The semantic
context suppresses that component with the mean of the current and previous
sample. Research normalization uses separate front and rear scales because the
rear channel has a larger observed operating range:

| Channel | Front scale | Rear scale |
| --- | ---: | ---: |
| Lateral `+0xAC` | 1400 | 2000 |
| Longitudinal `+0xB0` | 2500 | 2200 |

These are descriptive research scales, not physical units or utilization
percentages. Signed channels are bounded to `[-1, 1]`; combined normalized
magnitude is bounded to `[0, 1]`.

## Aggregated context

The `FourCornerContext` layer keeps FL, FR, RL, and RR values and derives
front/rear, left/right, and whole-vehicle averages. It also records
front/rear displacement bias, left/right displacement bias, normalized lateral
and longitudinal biases, and whether surface classifications agree across the
car.

Later M5 work added descriptive lateral, longitudinal, chassis, recovery, and
confidence states. These labels describe the distribution and quality of
observed response. They do not classify understeer, oversteer, axle
saturation, tire grip, braking force, or throttle state.

M4 remains the authority for the LOAD, RELEASE, FREE, BITE, and returned-load
envelope. M5 uses four-corner state only to characterize an already-existing
directional request during narrowly eligible RELEASE frames. Rear response
never supplies steering direction.

## Surface state and validity

`surface_0..surface_3` are the raw per-corner classifications corresponding to
FL, FR, RL, and RR. The interpreter reports equality and asymmetry but does not
assign material names or treat a particular value as airborne. Surface
asymmetry currently contaminates M5 confidence and prevents the active M5
contribution; it does not manufacture a new surface effect.

Native steering state and four-corner state use separate validity checks. The
steering interpreter distinguishes valid, transition-suppressed, and
unavailable state. The corner observer requires the exact pointer topology and
finite values. Invalid state cannot advance BITE restoration or M5 output.

## What remains unknown

Physical units for the corner channels are unknown. The project has not proven
tire forces, suspension travel, normal load, slip angle, SAT, grip percentage,
or universal cross-car normalization. The mapping between the anonymous fields
used by OutRun's restored Xbox vibration calculation and Howard Casto's
historical X-Force observations also remains unresolved.

The next native-surface fidelity target is the horizontal-striped runoff near
the first beach ball at Sunny Beach. A future controlled study should inspect
corner `+0x28`, raw surface classification, `+0xAC`/`+0xB0`, front/rear timing,
the road contribution, and output slew. The visual stripes alone are not proof
that the game models repeated physical bumps.
