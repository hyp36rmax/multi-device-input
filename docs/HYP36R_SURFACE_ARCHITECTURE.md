# HYP36rforce FFB 2.0 Surface Architecture

Surface is a normal HYP36rforce FFB player feature in both Classic and Enhanced
Road modes, backed by engineering controls for Texture presentation and an
internal Bump component.

## Branch lineage

Before this milestone, Surface consumed `GainFrame::finalRoad`:

```text
game-derived Road source
→ player Road Detail
→ Enhanced calibration
→ Directional Road ±0.25 ceiling
→ Directional Road 0.90/second slew
→ player Surface scaling
→ Texture ceiling
→ DirectInput periodic effect
```

Surface therefore inherited two safety stages owned by the directional
constant-force channel.

The independent branch now keeps HYP36R native Surface observation active in
both Road modes and resolves one shared Surface source before Road presentation
selection:

```text
game-derived HYP36R surface observation
→ player Road Detail → shared Surface calibration
                       ├─ Texture
                       │  → player Surface scaling
                       │  → Texture ceiling
                       │  → periodic effect
                       └─ Bump
                          → frame delta threshold
                          → cooldown
                          → 25% pulse bound
                          → finite constant-force pulse

Road Mode independently selects:
Classic Road, unchanged
or Enhanced Road → calibration → ±0.25 ceiling → 0.90/second slew
→ composer / tanh / constant force
```

No game physics are re-derived. Classic and Enhanced consume the exact same
Surface source. Road Mode only selects the separate directional Road
presentation. Classic Road itself is not rerouted or modified. Each output
mechanism owns its safety conditioning.

## Player model

The player sees one `Surface` control from 0–100%, default 50%. It scales both
components linearly inside their independent engineering bounds:

- Texture request uses `player Surface / 100` as its strength.
- Bump uses `25% nominal × player Surface / 100` as its strength.

At 0%, both Texture and Bump are disabled. At 100%, neither can exceed its
engineering limit. Road Detail remains independent and continues through the
Directional Road path.

## Engineering defaults

- Normal Enhanced Road calibration: 30x
- Debug Road override: off; stored research multiplier 10x
- Surface player control: 50%
- Normal Texture ceiling in both modes: 18%
- Texture research override: off; choices 12/18/25%
- Waveform: Sine
- Frequency profile: Reference (18–42 Hz)
- Surface Bump: on; Debug exposes only an A/B isolation switch
- Bump threshold: 0.020
- Bump strength: 25%
- Bump duration: 60 ms
- Bump cooldown: 120 ms (fixed prototype protection)

Texture engineering choices are 12, 18 and 25%. With the research override
off, both Road modes use the 18% reference without changing player Surface.
Bump uses a separate 25% nominal safety bound and its source, threshold,
scaling, duration, cooldown and cap are identical in both Road modes.

`Reset HYP36R Debug Settings` restores only these research settings and clears
active Surface effects. It does not reset normal player Force Feedback values.

## Configuration migration

Existing explicit Road Mode choices remain intact: Classic stays Classic and
Enhanced stays Enhanced. The centralized Enhanced resolver now returns 30x
when the Debug override is off, regardless of an older stored research
multiplier. If the Debug override is on, its valid 8/10/15/20/25/30 selection
remains intentional and continues to override the baseline.

Missing player Surface configuration resolves to 50%. Older experimental
Texture ceilings do not affect normal operation because the new research
override defaults off. If the override is intentionally enabled, values outside
12/18/25 safely resolve to 18%. Existing waveform, frequency and Bump A/B
selections remain persisted. Historical telemetry retains the schema and
multiplier recorded at capture time and is not reinterpreted.

## Bump qualification

The detector observes the absolute frame-to-frame delta of the shared
calibrated Surface source. A candidate must exceed the configured threshold and
the fixed cooldown must have expired. Sustained stable magnitude therefore does
not repeatedly trigger pulses. Each pulse is finite, independently bounded and
rendered as a separate one-axis DirectInput constant-force effect; it is not
added to Steering, Impact, Directional Road or the main composer.
