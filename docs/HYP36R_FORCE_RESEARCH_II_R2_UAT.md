# HYP36R Force Research II — R2 controlled capture campaign

This is a research build, not a product release. It records controlled
Reference+ 1.44 evidence without changing Force behavior or testing amplified
gains.

## Install once

1. Open the artifact's `UAT/` folder.
2. Copy everything inside `UAT/` into the main OutRun 2006 directory.
3. Launch normally and confirm Reference+ feels normal.

All captures use Strength, Steering Load, Road Detail, and Impact at 100%.
Keep the wheel's required Invert setting. Do not change Force settings between
scenarios.

## Capture each scenario

Open **F11 → Debug → FFB Telemetry**. Choose the scenario from **R2 scenario**,
then select **Start New Capture**. Perform only that scenario and select
**Stop Capture** when finished.

Do not open Controller, Force Feedback, or other configuration screens while a
capture is active. If a run is interrupted or performed incorrectly, stop it
and repeat it as a new capture. Earlier attempts are never overwritten.

The three sessions can be completed separately.

## Session A — Vehicle and Force baseline

| Scenario | What to do | Avoid | Useful duration | Purpose |
| --- | --- | --- | --- | --- |
| `R2_A01_STRAIGHT_BASELINE` | Hold a clean straight at steady speed with minimal steering. | Surface crossings and impacts. | 15–30 seconds | Establish the vehicle, surface, Road, per-corner, and output baseline. |
| `R2_A02_PROGRESSIVE_LEFT` | Enter a predictable left corner cleanly, build steering progressively, hold briefly, and exit normally. | Drift, curb, and runoff. | One clean corner | Record progressive left directional growth. |
| `R2_A03_PROGRESSIVE_RIGHT` | Repeat the progressive test in a predictable right corner. | Drift, curb, and runoff. | One clean corner | Record the opposite direction without assuming corner ordering. |
| `R2_A04_SUSTAINED_HIGH_LOAD_CORNER` | Hold significant controlled load through the best available long corner. | Intentional drift or leaving the road. | 5–15 useful seconds | Capture natural sustained Steering activity and headroom. |
| `R2_A05_DRIFT_INITIATION` | Record one clean transition from grip into an early drift. | Maximizing drift angle. | One transition | Observe LOAD to RELEASE to early FREE. |
| `R2_A06_SUSTAINED_DRIFT` | Maintain a stable drift for several seconds where practical. | Impacts and surface changes. | 3–8 useful seconds | Observe sustained FREE and M5 behavior. |
| `R2_A07_RELEASE_RECOVERY` | Transition from drift or slip back into grip; prioritize recovery. | Extending the drift unnecessarily. | One clean recovery | Observe FREE through recovery and BITE back to LOAD. |

## Session B — Surface campaign

Use one repeatable local area where practical. Prefer the sequence normal
asphalt → target surface → normal asphalt. These labels describe the operator's
target, not a conclusion about any raw telemetry value.

| Scenario | What to do | Avoid | Useful duration | Purpose |
| --- | --- | --- | --- | --- |
| `R2_B01_LOCAL_ASPHALT_CONTROL` | Drive the same local test area while staying fully on normal asphalt. | Target surfaces, drift, and impacts. | 15–20 seconds | Establish the local control. |
| `R2_B02_STRIPED_RUNOFF_PARTIAL` | Cross striped runoff with one side of the car, then return fully to asphalt. | Drifting or a full-width crossing. | One repeatable pass | Test partial spatial surface observation. |
| `R2_B03_STRIPED_RUNOFF_FULL` | Move more or all of the car onto the same runoff, then return. | Impacts and drift. | One repeatable pass | Compare partial and broad occupancy. |
| `R2_B04_ROUGH_OR_SAND_PARTIAL` | Put one side onto a repeatable rough shoulder, sand, or off-track target, then return. | Full-width entry and impacts. | One repeatable pass | Record a partial rough-surface transition. |
| `R2_B05_ROUGH_OR_SAND_FULL` | Move more or all of the car onto the same target, then return. | Impacts and drift. | One repeatable pass | Compare partial and broad occupancy. |
| `R2_B06_SURFACE_REENTRY` | Begin off-surface and focus on the transition back to asphalt and grip. | Additional unrelated maneuvers. | One clean re-entry | Separate surface re-entry from load recovery. |

## Session C — Transients

| Scenario | What to do | Avoid | Useful duration | Purpose |
| --- | --- | --- | --- | --- |
| `R2_C01_GEAR_SHIFTS` | Make several ordinary upshifts and downshifts during otherwise clean driving. | Surface changes and collisions around each shift. | 20–30 seconds | Isolate gear, restored vibration, Impact, and final output. |
| `R2_C02_CONTROLLED_IMPACT` | Produce one or more clearly separated, mild, repeatable impacts. | An unnecessarily violent wheel test. | 10–20 seconds | Record natural high Impact activity for later replay. |

## Minimal car and course coverage

Use at least two available cars with meaningfully different handling where
useful: one stable/grippy context and one more drift-capable context. Use at
least two course contexts: one with repeatable straights/corners and one with
repeatable target surfaces. Do not unlock or require specific content, and do
not repeat every scenario with every car.

## Return the evidence

After each session, return its scenario folders from `HYP36R/Research/`, or
return the complete `HYP36R/Research/` directory after all sessions. General
logs and unrelated game files are not needed unless the runtime reports an
error.
