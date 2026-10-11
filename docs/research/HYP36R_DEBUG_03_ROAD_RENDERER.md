# HYP36R-DEBUG-03 — Road Renderer Selection

The Engineering Workspace exposes the existing `Developer/RoadRenderer`
setting under **Engineering → Surface Tuning**. Accepted persisted values are
`DIRECTIONAL` and `SURFACE`; legacy `SURFACE_EXPERIMENTAL` remains accepted by
the existing parser. Invalid values resolve safely to Directional.

The established v2 configuration baseline remains Surface. A selection is
persisted immediately and is consumed by the force loop on the next frame.
Every real renderer transition first stops the periodic Surface transport;
selecting Surface then recreates it through the existing guarded DirectInput
path when gameplay, focus, device, waveform, and FFB safety conditions permit.

No Road signal, waveform, frequency, amplitude, force-composition, or
DirectInput delivery calculation is changed. The existing 294-column research
schema remains unchanged: `road_renderer` records the active selection,
`surface_effect_active` exposes periodic-effect state, and
`r05_output_strategy` records the observed output delivery strategy.
