# HYP36R Force Research II — R2-B surface and Road UAT

This is a research build, not a product release. It records controlled surface
transitions without changing Road Detail or any other Force behavior.

## Install

1. Open the artifact's `UAT/` folder.
2. Copy everything inside `UAT/` into the main OutRun 2006 directory.
3. Launch normally and confirm Reference+ and the correct Invert Wheel setting.

Leave Strength, Steering Load, Road Detail, and Impact at 100%. Do not change
Force or controller settings during a capture.

## Choose two local test areas

Keep B01, B02, and B03 in the same local section where practical. Use one
repeatable striped-runoff boundary with adjacent normal asphalt.

Keep B04, B05, and B06 around the same repeatable rough, sand, shoulder, or
comparable off-track target where practical.

The runner records these pairing groups in each session's Notes. It does not
assign names or physical positions to raw surface values.

## Run the campaign

1. Open **F11 → Debug → FFB Telemetry**.
2. Confirm **B01 Local Asphalt Control — READY**.
3. Select **Start Test**.
4. After the 3, 2, 1 countdown, perform the instruction shown in the small
   research overlay.
5. The runner stops and saves automatically.
6. Reopen F11 if needed and choose **Accept** or **Retry**.
7. Continue through B06.

Use **Cancel Test** if the target is missed or the maneuver becomes unsafe.
Cancelled, retried, and accepted captures remain separate and are never
overwritten.

## Scenario sequence

| Scenario | Target | Instruction | Avoid |
| --- | ---: | --- | --- |
| B01 Local Asphalt Control | 20 s | Drive only on normal asphalt in the local B01/B02/B03 area. | Curbs, runoff, shoulder, impact, and drift. |
| B02 Striped Runoff Partial | 18 s | Start on asphalt, place one side on striped runoff, then return fully to asphalt. | Putting the full car on runoff, drift, and impact. |
| B03 Striped Runoff Full | 18 s | Start on asphalt, move most or all of the car onto the same runoff, remain briefly, then return. | Unrelated impacts or a different local section. |
| B04 Rough / Sand Partial | 18 s | Start on asphalt, place one side on the rough target, then return. | Putting the full car off-track or intentionally drifting. |
| B05 Rough / Sand Full | 18 s | Start on asphalt, move most or all of the car onto the B04 target, remain briefly, then return. | Unrelated impacts or a different target. |
| B06 Surface Re-entry | 18 s | Begin on the off-track target, cross back to asphalt, and regain normal loading. | Unnecessary impact. |

Prefer the sequence asphalt → target → asphalt within B02–B05. A capture is
acceptable when the intended partial or full transition is clear; geometrical
perfection is unnecessary. Retry when the wrong surface is crossed, both sides
cross during a partial test, the car spins, a collision dominates the event,
or the target cannot be identified confidently.

## Output

Each attempt creates a unique CSV and session file under:

`HYP36R/Research/<scenario>/`

The runner records campaign, canonical scenario, attempt, review status, target
and actual duration, and local-pairing Notes. The validated 222-column telemetry
schema remains unchanged.

After B06, return the complete `HYP36R/Research/` directory. Do not rate how a
surface felt during this objective capture campaign.
