# HYP36Rforce Road 2.0 R4.2H-G1 — Development Gain Calibration

The first DD2 evaluation of Active Road 2.0 V0 was inconclusive because the
Road contribution was not perceptible. G1 does not redesign Road 2.0. It adds a
development-only gain sweep after the existing Road Detail amount and before
final Force composition.

## Fixed architecture

The 120 Hz xorshift generator, `0x48595036` seed, filters, DC blocker, native
authority, four-corner occupancy, transition conditioning, material policy and
single ConstantForce topology are unchanged from `c12ce95`.

The development path is now:

```text
native authority
-> occupancy and transition conditioning
-> Road 2.0 V0 aperiodic presentation
-> existing Road Detail amount
-> development gain (1x / 2x / 4x / 8x)
-> +/-0.25 Road-channel clamp and 0.90/s slew protection
-> existing composition
```

The selector is in Debug/Research rather than the normal Force Feedback menu.
An invalid stored value resolves to 1x. At 1x the added stage returns the
existing c12ce95 Road value directly; this preserves exact V0 behavior. Higher
gains pass through the new final Road-channel slew limiter. Loss of native
authorization clears the stage immediately, so no amplified tail leaks onto
normal road.

Road Detail remains the same 0–100% amount control. It does not change the
generator, seed, rate, bandwidth, density, transitions or surface semantics.

## Accepted-capture replay

Values are normalized software Road contributions after the gain stage, at the
Reference+ Road Detail amount. No accepted capture reached the `0.25` hard
ceiling. Higher-gain peaks can be below exact multiplication where the final
slew limiter intervened.

| Capture | 1x RMS / max | 2x RMS / max | 4x RMS / max | 8x RMS / max |
|---|---:|---:|---:|---:|
| CST01 normal | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 |
| CST02 cobblestone | .000236 / .001075 | .000471 / .002150 | .000943 / .004300 | .001885 / .008600 |
| CST03 normal return | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 |
| B01 asphalt | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 |
| B02 runoff partial | .000344 / .002523 | .000689 / .005046 | .001377 / .010093 | .002704 / .020185 |
| B03 runoff broad | .000278 / .002482 | .000556 / .004964 | .001112 / .009928 | .002181 / .019857 |
| B04 rough/sand partial | .000651 / .003100 | .001303 / .006200 | .002593 / .012401 | .005014 / .018784 |
| B05 rough/sand broad | .001299 / .007582 | .002596 / .015164 | .004873 / .030327 | .008227 / .042114 |
| B06 re-entry | .000726 / .004640 | .001453 / .009280 | .002878 / .016145 | .005106 / .032290 |

Normal-road output remained exactly zero at all four gains. Mean DC remained
between `-0.0000462` and `0.0003182`; the largest value was the 8x B05 result.
The final slew limiter engaged only where amplified samples needed it and no
output exceeded the Road-channel bound. Repeated replay remains deterministic.

## Physical gate

Use the same DD2 configuration, Road Detail setting and Tulip Garden section.
Drive 1x, 2x, 4x and 8x without changing anything else. Record the first gain
that becomes clearly perceptible, then decide whether its existing texture is
useful Road information or artificial sensation. No character tuning belongs
in this sweep.
