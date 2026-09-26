# HYP36R Force Research II — R2-C transient and Impact UAT

This is a research build, not a product release. It records controlled gear
shifts and physical impacts without changing Impact or any other Force behavior.

## Install and safety

1. Open the artifact's `UAT/` folder.
2. Copy everything inside `UAT/` into the main OutRun 2006 directory.
3. Launch normally and confirm Reference+ and the correct Invert Wheel setting.

Leave Strength, Steering Load, Road Detail, and Impact at 100%. Use your normal
conservative wheel-side torque setting. Do not increase Force settings for this
test. If contact produces unexpectedly violent wheel behavior, cancel the test
and do not escalate impact severity.

## Run the campaign

1. Open **F11 → Debug → FFB Telemetry**.
2. Confirm **C01 Gear Shifts — READY**.
3. Select **Start Test**.
4. After the 3, 2, 1 countdown, follow the instruction in the research overlay.
5. The capture stops and saves automatically.
6. Reopen F11 if needed and choose **Accept** or **Retry**.
7. Continue through C02.

Use **Cancel Test** if a maneuver is unsafe or no longer representative.
Cancelled, retried, and accepted captures remain separate and are never
overwritten.

## Scenario sequence

| Scenario | Target | Instruction | Keep isolated from |
| --- | ---: | --- | --- |
| C01 Gear Shifts | 25 s | Drive cleanly on normal asphalt. Where practical, make 2–3 ordinary upshifts and 1–2 ordinary downshifts. Keep the driving natural. | Curbs, runoff, sand, rough shoulder, collisions, and intentional drift near shifts. |
| C02 Controlled Impact | 20 s | Drive normally, make mild contact, leave a clean interval, make moderate contact, then leave another clean interval. Use a small number of discrete events. | Maximum-severity impacts, continuous wall contact or scraping, and intentional rough/sand/runoff activity. |

Retry C01 only when there are no useful shifts or the shift windows are clearly
contaminated. Retry C02 only when there is no clear impact, scraping dominates,
surface activity dominates, or the event feels unsafe or unrepresentative. Do
not chase perfection.

## Output

Each attempt creates a unique CSV and session file under:

`HYP36R/Research/<scenario>/`

The runner records the campaign, canonical scenario, attempt, review status,
target duration, actual duration, and scenario Notes. The synchronized
222-column Research II R1 schema remains unchanged.

After C02, return the complete `HYP36R/Research/` directory. Do not amplify
Impact physically and do not rate or select an Impact ceiling during capture.
