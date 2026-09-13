# TP-01C controlled telemetry test protocol

Use one capture for each scenario. Before starting, set the scenario and notes
in `OutRun2006Tweaks.user.ini`, launch the game, enter gameplay, and use **Start
New Capture** in the Debug tab. Select **Stop Capture** immediately after the
test so the CSV is flushed and closed cleanly.

Recommended notes include the car, stage or location, wheel hardware, hardware
FFB setting and in-game FFB strength.

The seven `native_*` candidates have unknown meanings. These tests collect
observations only; they do not identify or classify any signal.

## T01 — Stationary baseline

Scenario:

```text
T01_stationary_baseline
```

Start a race or gameplay session. Keep the vehicle stationary if practical,
hold the steering centered, make no deliberate steering input, and avoid all
collisions. Record approximately ten seconds.

Goal: establish idle values and identify candidates that remain constant,
oscillate, sit near zero or react without deliberate vehicle input.

## T02A — Stationary steering sweep

Scenario:

```text
T02A_stationary_steering_sweep
```

Keep the vehicle stationary. Move the wheel slowly through this sequence,
pausing approximately one to two seconds at every position:

```text
center
↓
~25% left
↓
~50% left
↓
full left
↓
center
↓
~25% right
↓
~50% right
↓
full right
↓
center
```

Goal: observe which candidates correlate with steering position independently
of vehicle speed. Do not classify them during the test.

## T03 — Straight-line speed sweep

Scenario:

```text
T03_straight_speed_sweep
```

Keep the steering as close to center as practical and perform one smooth run:

```text
stationary
↓
low speed
↓
moderate speed
↓
high speed
```

Avoid deliberate drifting, collisions, curbs, off-road surfaces and aggressive
steering.

Goal: observe which candidates correlate primarily with vehicle speed.

## Questions for later analysis

Ask these questions independently for every native candidate.

### Baseline

- Does it remain constant or oscillate?
- Does it sit near zero or hold a nonzero baseline?

### Steering

- Does it change with steering position or magnitude?
- Does it change sign between left and right?
- Does it respond while the vehicle is stationary?

### Speed

- Does it increase or decrease with speed?
- Does it approach zero with speed?
- Does it remain independent of speed?

### Interaction

- Does its steering behavior change once the vehicle is moving?

Howard Casto historically described his X-Force observation as showing
different apparent magnitude behavior at low and higher speeds. That is a
comparison target only. No candidate should be identified as X-Force from
resemblance or from a single run.

Assigning meaning later requires repeatability, isolation, a consistent
directional relationship and multiple controlled captures.
