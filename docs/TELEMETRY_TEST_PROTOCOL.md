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

## M5B four-corner validation

M5B records raw native candidates only. Keep the labels `corner0` through
`corner3`: corners 0/1 are the confirmed steered/front pair and corners 2/3
are the other/rear pair, but left/right ordering is not yet established.
`displacement_candidate`, `directional_ac`, and `directional_b0` are neutral
names and must not be treated as suspension, lateral force, longitudinal
force, grip, or load during capture.

These maneuvers can be recorded in one continuous session or a small number
of sessions. Use **Start New Capture** when a separate scenario file is useful;
restarting the game is not required. Keep exactly one `[Developer]` section in
the effective configuration.

### T01 — Stationary baseline

Hold the stopped vehicle with centered steering for about ten seconds. This
establishes candidate offsets and idle noise.

### T02 — Stationary steering sweep

While stopped, slowly sweep center to partial and full lock in both directions,
pausing briefly at each position. This checks for steering contamination in
the three candidate groups; the established orientation fields are not added
to the M5B CSV.

### T03 — Straight acceleration

Accelerate smoothly in a straight line through low, moderate, and high speed.
This is intended to distinguish the AC/B0 component most responsive to
longitudinal motion and reveal any front/rear or driven-corner difference.

### T04 — Straight braking

From a safe moderate speed, brake progressively while holding the wheel near
center. Compare front/rear displacement redistribution and AC/B0 response.

### T05L / T05R — Steady cornering

Capture one steady left turn and one steady right turn at similar speed and
radius. Compare inside/outside candidate behavior to establish left/right
ordering without encoding that ordering in software first.

### T06 — Progressive front push

Build steering demand gradually until a safe understeer-like/front-push event
occurs. Compare the front-pair candidates with the recorded M4 RELEASE state.

### T07 — Rear breakaway

Capture a controlled rear-rotation event. Compare the rear pair against the
front pair and the recorded M4 FREE state.

### T08 — Power-on slide

Capture a controlled power-on slide to observe combined directional demand.

### T09 — One-side curb or surface transition

Place only one side of the car on a curb or different surface where practical.
Use the existing `surface_0` through `surface_3` columns with the new candidates
to resolve corner ordering and response lineage.

### T10 — Safe crest or naturally light contact

If a safe, natural crest is available, capture it to test whether displacement
and surface state identify reduced contact. Do not create a dangerous or highly
artificial maneuver solely to perform this test.
