# HYP36Rforce Road 2.0 R4.2H — Active V0

R4.2H is the first Road 2.0 implementation permitted to reach the wheel. It is
an experimental development selection and does not change the public default,
the product version, or Reference+.

## Signal ownership

### Native inputs

- OutRun's four raw corner-surface states establish reference, partial and
  broad occupancy.
- The existing native left/right motor evidence supplies an unsigned material
  and speed-sensitive Road authority.
- Native gear, collision-candidate and unknown-event classifications exclude
  samples that are not defensibly continuous Road information.

These inputs establish when Road is authorized and how much observed authority
is available. They do not establish a signed waveform, phase, wavelength,
frequency, suspension travel, tire load, or steering direction.

### Derived semantics

`HYP36R_ROAD_POLICY_V1` establishes reference-relative occupancy, exclusions
and transitions. The validated Direct `HYP36R_ROAD_PRESENTATION_V1_SHADOW`
candidate converts the two unsigned native motor channels into a normalized
RMS authority. R4.2H preserves the four corners only as a 0/4 through 4/4
occupancy confidence and uses bounded attack/release conditioning at changes.

### Synthetic presentation

`HYP36R_ROAD2_ACTIVE_V0` presents the authorized information with a synthetic,
deterministic, aperiodic texture. It uses a fixed `0x48595036` xorshift state at
a 120 Hz internal step. Two one-pole filters (`0.34` and `0.055`) form a
band-limited difference signal. A `0.992` DC blocker removes persistent bias.
No wall clock, system time, `random_device`, named material frequency or
external implementation is used.

The contribution is:

```text
native RMS authority
× reference-relative occupancy envelope
× conditioned zero-mean texture
× 0.06 internal development ceiling
```

It is hard-clamped to `±0.06` before the existing Road Detail gain and has a
maximum slew of `0.90` normalized units per second. Invalid or nonfinite state
resets deterministically and outputs zero. Zero native authority always outputs
exactly zero.

## Output topology

V0 uses the existing single DirectInput ConstantForce request. This is the
smallest available topology and avoids a second effect slot, Spring/Damper use,
or unsupported periodic assignments. Isolation is enforced before final
composition: `Road Presentation = Reference+` selects the unchanged legacy
Road scalar; `Road 2.0 Experimental` selects the V0 scalar. They are never
summed. Force Character then scales the three independent Directional, Road and
Impact values, after which the existing `tanh`, output ramp and one `drive()`
request remain authoritative.

This topology does not provide a separate hardware effect slot for Road. The
Road scalar shares the final ConstantForce request after logical channel
isolation. That is an explicit V0 limitation to validate on the DD2.

## Player and developer surface

Advanced Force Feedback contains the development A/B selector. Reference+ is
the default and invalid configuration falls back to it. The existing Road
Detail control remains the only player-facing Road level. No frequency,
roughness, seed, attack or release controls are exposed.

The Debug tab reports mode, native authority, four-corner tuple, occupancy,
transition phase, raw generator state, conditioned texture, final contribution
and safety/clamp/slew state.

## Accepted-capture replay

The offline replay uses reference raw surface `2`, the same policy gates and
the same deterministic generator. All results are pre-Road-Detail normalized
software contributions.

| Capture | Authorized rows | Mean DC | RMS | Maximum | Normal false positives |
|---|---:|---:|---:|---:|---:|
| CST01 normal | 0 | 0 | 0 | 0 | 0 |
| CST02 cobblestone | 451 | -0.00000124 | 0.00023568 | 0.00107504 | 0 |
| CST03 normal return | 0 | 0 | 0 | 0 | 0 |
| B01 asphalt | 0 | 0 | 0 | 0 | 0 |
| B02 striped runoff partial | 173 | 0.00000005 | 0.00034428 | 0.00252319 | 0 |
| B03 striped runoff broad | 175 | -0.00000092 | 0.00027794 | 0.00248210 | 0 |
| B04 rough/sand partial | 943 | -0.00000290 | 0.00065148 | 0.00310016 | 0 |
| B05 rough/sand broad | 467 | 0.00003657 | 0.00129858 | 0.00758176 | 0 |
| B06 re-entry | 488 | -0.00000904 | 0.00072648 | 0.00464000 | 0 |

No replay exceeded the ceiling or slew bound. Gear/impact-like samples were
excluded, every normal-road output was zero, and repeated CST02 replay produced
the same SHA-256 output digest. These results establish software invariants,
not physical torque, perceptibility or comfort.

## Boundaries and next gate

Road 2.0 V0 is not a recovered Sega waveform and makes no claim about native
frequency or original arcade/PS2 presentation. THP32 material remains
**EXTERNAL REFERENCE — NOT INTEGRATED**; no code, constants, frequencies,
effect parameters or equations were used.

The next gate is one focused DD2 A/B UAT on normal road, Tulip Garden
cobblestone, rough/off-road and convenient runoff. It must establish whether
the conservative output is perceptible and useful, whether transitions feel
natural, and whether it leaves steering authority clean before any tuning.
