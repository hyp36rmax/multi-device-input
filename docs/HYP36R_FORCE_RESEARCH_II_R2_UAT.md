# HYP36R Force Research II — R2-A scenario-runner UAT

This is a research build, not a product release. It records the controlled
R2-A vehicle and force baseline without changing Reference+ or any Force
behavior. R2-B has not started.

## Install once

1. Open the artifact's `UAT/` folder.
2. Copy everything inside `UAT/` into the main OutRun 2006 directory.
3. Launch normally and confirm Reference+ feels normal.

Leave Strength, Steering Load, Road Detail, and Impact at 100%. Keep the
wheel's required Invert setting. Do not change Force settings during the
campaign.

## Run the campaign

1. Open **F11 → Debug → FFB Telemetry**.
2. Confirm the campaign starts at **A01 Straight Baseline — READY**.
3. Select **Start Test**.
4. Drive only the instruction shown after the 3, 2, 1 countdown.
5. The runner stops and saves the capture automatically.
6. Reopen F11 if needed, then choose **Accept** or **Retry**.
7. Continue until A07 is accepted and the campaign reports complete.

Retry never overwrites the earlier attempt. Cancel during a recording safely
stops and flushes that attempt without advancing. Cancel during the countdown
returns the same scenario to READY.

## R2-A sequence

| Scenario | Target | Instruction | Avoid |
| --- | ---: | --- | --- |
| A01 Straight Baseline | 20 s | Drive straight on clean asphalt at a steady speed. | Intentional steering or surface transitions. |
| A02 Progressive Left | 18 s | Progressively load a clean left-hand corner. | Leaving asphalt or intentional drift. |
| A03 Progressive Right | 18 s | Progressively load a clean right-hand corner. | Leaving asphalt or intentional drift. |
| A04 Sustained High-Load Corner | 20 s | Maintain steering load through a long clean corner. | Runoff or breaking grip. |
| A05 Drift Initiation | 15 s | Transition cleanly from grip into drift. | Extending the drift instead of capturing entry. |
| A06 Sustained Drift | 18 s | Establish and hold a stable drift. | Impacts where practical. |
| A07 Release / Recovery | 15 s | Recover from slip or drift back into grip. | Extending the drift unnecessarily. |

## Output and review

Every attempt produces a uniquely named telemetry CSV and session file under:

`HYP36R/Research/<scenario>/`

The session file records the campaign, canonical scenario, attempt, target and
actual duration, capture outcome, and later Accept or Retry decision. The CSV
schema remains the validated 222-column Research II schema.

After A07, return the complete `HYP36R/Research/` directory. General logs and
unrelated game files are not needed unless the runner reports an error.
