# Road 2.0 R4.2F-T1.2 — Tulip Garden feasibility UAT

This development/research build answers one question: can the known Tulip
Garden cobblestone section support a later controlled spatial-timing campaign?
It does not test Road 2.0 feel, infer a wavelength, or send Road 2.0 output to
the wheel. Current Road and Reference+ remain authoritative.

## Fixed waypoint

Use **OutRun 2 → Tulip Garden → cobblestone immediately after the first
corner**. Use the same line for all three passes. Earlier evidence observed all
four surface channels at `0x100000` here; every new capture must verify that
rather than assume it.

## Install

Copy the six files from the artifact package beside the game executable and
launch normally. This is a `1.0.0-dev+<shortSHA>` research build, not a public
release. Do not change Force settings and do not evaluate Road 2.0 feel.

## Capture sequence

Open **F11 → Debug → FFB Telemetry**. For each scenario select **Start Test**,
follow the 3–2–1 countdown, drive the target section, and wait for the automatic
stop at 10 seconds. Select **Accept** to advance or **Retry** if the pass clearly
missed the target. **Cancel Test** remains available.

1. `R4_2FT12_LOW`: cross at a deliberately low but stable usable speed. Maximize
   clean time without making vehicle control artificial.
2. `R4_2FT12_MEDIUM`: cross the same line at a natural medium speed similar to
   the earlier CST02 pass.
3. `R4_2FT12_HIGH`: cross the same line at a materially higher but controlled
   speed. Do not sacrifice surface occupancy or control for speed.

For every pass, approach cleanly, hold speed as steadily as practical, use
minimal steering, avoid drift and collisions, avoid shifting on the target when
practical, and remain on the same cobblestone line. No mph/kph target is used;
LOW, MEDIUM and HIGH are relative tiers.

## Acceptance and analysis plan

Analysis selects the longest continuous interval where all four surface states
are `0x100000`. Mixed-surface approach and exit frames are excluded. For that
interval report duration, samples, relative distance, median/P05/P95 speed,
speed coefficient of variation, sustained drift, steering activity, gear and
state transitions, Impact/event activity, timing gaps, and surface transitions.

Distance is reconstructed only in game-relative units:

`distance += max(speed, 0) * valid_dt`

The frozen stability target is approximately ±5% around the pass median;
sustained drift beyond approximately 10% is a concern. Each tier is checked
against the 7-second clean-evidence floor and honestly compared with the
preferred 12–15 seconds. After all captures, LOW/MEDIUM/HIGH must be materially
separated enough to support a future speed-scaling test.

Because the recurrence interval is still unknown, this probe cannot prove five
cycles. It classifies whether duration and distance make five resolved cycles
**PLAUSIBLE**, **MARGINAL**, or **UNLIKELY** if a meaningful recurrence exists.

The final gate is one of: **A** supports full T2; **B** supports T2 with stated
caveats; **C** too short/unstable; or **D** inconclusive and requires one focused
repeat. T1.2 performs no autocorrelation, spectral selection, wavelength
inference, time-vs-distance decision, or product-timing selection.

## Evidence boundary

The validated 222-column schema is unchanged and already provides time/frame,
speed, four surface states and transitions, native L/R activity, gear, Impact
and event context, steering, load/grip state, and timing gaps. No periodic
request, phase, or passive-periodic shadow fields were added. ALPHA/Force
Character, R5/AER, and Event 2.0 remain separate. GATS/THP32 material remains
external reference only and is not integrated.
