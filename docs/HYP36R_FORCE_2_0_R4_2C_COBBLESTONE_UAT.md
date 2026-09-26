# HYP36R Force 2.0 R4.2C — cobblestone evidence UAT

This is a research build, not a Road 2.0 feel test. It records three short,
separate sections so stable cobblestone activity can be compared with nearby
normal road. Road 2.0 remains passive and current v1 Road remains authoritative.

## Install and safety

1. Open the artifact's `UAT/` folder.
2. Copy everything inside `UAT/` into the main OutRun 2006 directory.
3. Launch normally and confirm Reference+ and the correct Invert Wheel setting.

Leave Strength, Steering Load, Road Detail, and Impact at 100%. Keep your usual
conservative wheel-side torque setting.

## Run the campaign

1. Enter OutRun 2 mode and reach the area containing the cobblestone section.
2. Open **F11 → Debug → FFB Telemetry**.
3. Confirm **CST01 Normal Road Control — READY** and select **Start Test**.
4. Follow the instruction shown after the 3, 2, 1 countdown.
5. The 15-second capture stops and saves automatically.
6. Reopen F11 if needed and choose **Accept** or **Retry**.
7. Continue through CST02 and CST03. The runner labels every capture.

Use **Cancel Test** if the run becomes unsafe or clearly misses its target.
Cancelled, retried, and accepted attempts stay separate and are not overwritten.

## Scenario sequence

| Scenario | Target | Instruction | Retry when |
| --- | ---: | --- | --- |
| CST01 Normal Road Control | 15 s | On normal road near the cobblestone, hold a steady moderate speed, minimal steering, and a stable line. | Surface transitions, collision, or drift dominates; the local normal road is missed. |
| CST02 Cobblestone Stable | 15 s | Start after reaching cobblestone where practical. Remain on it at a steady moderate speed with minimal steering. | Cobblestone is missed; entry/exit dominates; collision or drift materially dominates; the car immediately leaves the section. |
| CST03 Normal Road Return | 15 s | After leaving cobblestone, use nearby normal road with driving similar to CST01. | Surface transitions, collision, or drift dominates; the local normal road is missed. |

Try to keep speed reasonably similar and use a stable gear, but do not chase
perfect matching. An ordinary shift does not invalidate an otherwise useful
stable interval. If convenient, note whether the repeating audible behavior was
present during CST02; no audio capture is required.

## Output and research boundary

Each attempt creates a unique CSV and session file under:

`HYP36R/Research/<scenario>/`

After CST03, return the complete `HYP36R/Research/` directory. The established
222-column Research II schema is unchanged: it already records the four surface
channels, native L/R and rise/change evidence, v1 Road, speed, gear,
Directional, and Impact. Candidate A is reconstructed offline from those
observations.

This build does not route Road2Policy or the passive presentation prototype to
Directional, Road, Impact, composition, `drive()`, or DirectInput. Do not judge
or tune Road 2.0 feel from this campaign.
