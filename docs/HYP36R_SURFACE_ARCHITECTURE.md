# HYP36R Force 2.0 Surface Architecture

Surface is experimental research functionality. It is not part of the normal
player Force Feedback menu and does not change the Directional Road defaults.

## Branch lineage

Before this milestone, Surface consumed `GainFrame::finalRoad`:

```text
game-derived Road source
→ player Road Detail
→ Enhanced calibration
→ Directional Road ±0.25 ceiling
→ Directional Road 0.90/second slew
→ Surface Strength
→ Surface ceiling
→ DirectInput periodic effect
```

Surface therefore inherited two safety stages owned by the directional
constant-force channel.

The independent branch now uses the already-calculated
`GainFrame::postGainRoad` as its shared calibrated source:

```text
game-derived Road source → player Road Detail → Enhanced calibration
                                            ├─ Directional Road
                                            │  → ±0.25 ceiling
                                            │  → 0.90/second slew
                                            │  → composer / tanh / constant force
                                            └─ Surface
                                               ├─ Texture
                                               │  → Strength
                                               │  → Surface ceiling
                                               │  → periodic effect
                                               └─ Bump
                                                  → frame delta threshold
                                                  → cooldown
                                                  → 25% pulse bound
                                                  → finite constant-force pulse
```

No game physics are re-derived. Both branches share the same calibrated Road
information, while each output mechanism owns its safety conditioning.

## Research defaults

- Road Authority: off; stored research multiplier 10x
- Road Renderer: Directional
- Surface Strength: 100%
- Surface ceiling: 12%
- Waveform: Sine
- Frequency profile: Reference (18–42 Hz)
- Surface Bump: off
- Bump threshold: 0.020
- Bump strength: 25%
- Bump duration: 60 ms
- Bump cooldown: 120 ms (fixed prototype protection)

The temporary Surface boundary steps are 25, 50, 60, 70, 80, 90 and 100%.
Earlier amplitude-UAT values remain accepted so the existing protocol remains
functional. Bump uses a separate 25% nominal safety bound.

`Reset HYP36R Debug Settings` restores only these research settings and clears
active Surface effects. It does not reset normal player Force Feedback values.

## Bump qualification

The detector observes the absolute frame-to-frame delta of the shared
calibrated Surface source. A candidate must exceed the configured threshold and
the fixed cooldown must have expired. Sustained stable magnitude therefore does
not repeatedly trigger pulses. Each pulse is finite, independently bounded and
rendered as a separate one-axis DirectInput constant-force effect; it is not
added to Steering, Impact, Directional Road or the main composer.
