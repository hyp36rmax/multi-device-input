# S10 presentation foundation consolidation

S10 closes the S-series without changing force behavior. The active code remains
the S9 baseline at `d8cf776d1766cee507a71b2fcd3115a3ac13c860`, built successfully
by Windows CI run 76. The earlier architectural baselines remain Force 2.0 at
`42118c029340db95e5f77539b8696bc6b824a08d` and the Force 2.1 research
reference at `8c5d68d92b19a21a09b3840413b8a453520517cc`.

## What graduated

The S-series separated a problem that had initially looked like one gain control
into four different concerns: force information, its presentation, software
headroom, and device calibration. S1 established that separation. S2 measured
the existing software output. S3 replayed possible amplification against the
available headroom. S4 separated Global Presence from Information Contrast,
and S5 proved that distinction in replay. S6 defined which layer has authority
to present secondary information. S7 compared near-zero budget curves and kept
the simpler linear rule. S8 put the policy into the runtime as a passive
request. S9 activated the conservative Reference+ candidate and validated it on
the wheel.

That work supports the following graduation decisions:

| Area | S10 status |
| --- | --- |
| HYP36R presentation architecture | Foundation complete |
| Software headroom research | Foundation complete |
| Reference presentation | Validated |
| Reference+ | Validated experimental foundation |
| Final presentation intensity | Deferred |
| Physical hardware safety envelope | Deferred |
| Device calibration | Deferred |
| Cross-car M5 validation | Deferred |
| AER / Arcade presentation | Deferred |

Reference+ is not being described as production-ready. It is a stable research
foundation with a successful physical test and known software exposure, not a
certified hardware envelope.

## Preserved presentation modes

`REFERENCE` remains the permanent comparison and safe fallback. It selects the
validated M4-only directional result with Presence 1.00. It does not introduce
M5 information.

`REFERENCE_PLUS_EXPERIMENTAL` remains the validated experimental foundation. It
uses the M4 primary plus eligible policy-permitted M5 information, Presence
1.20, Contrast 4, and a linear secondary ceiling of five percent of the M4
primary magnitude. Legacy directional authority remains the boundary for the
secondary contribution. Road, impact, vibration, output conditioning, user
strength, inversion, and DirectInput conditioning remain on their Reference
paths.

The S9 physical comparison found clearly greater steering presence. The change
was perceptible, remained natural, and felt like a useful foundation. More
intensity may be possible, but this observation is deliberately not being
turned into another gain change. The earlier S3 replay placed approximately
1.00 to 1.30 in the observed software comfort territory and approximately 1.40
to 1.60 in headroom-aware territory. Those ranges remain research observations,
not device recommendations.

## S9 exposure record

The physical capture `telemetry_20260921_075311 (S9).csv` contains 19,931
frames over 331.95 seconds. It records a Ferrari Dino 246 GTS at Sunny Beach on
a Fanatec DD2 with hardware strength at 50 percent and game strength at 100
percent.

| Software exposure measure | Result |
| --- | ---: |
| Instantaneous magnitude P95 | 0.378874 |
| Instantaneous magnitude P99 | 0.468083 |
| Instantaneous magnitude maximum | 0.588180 |
| One-second RMS P95 | 0.327202 |
| One-second RMS maximum | 0.446214 |
| Three-second RMS P95 | 0.279331 |
| Three-second RMS maximum | 0.356084 |
| Frames with three-second occupancy at or above 0.75 | 0 |
| Frames with three-second occupancy at or above 0.90 | 0 |
| Frames with three-second occupancy at or above 0.98 | 0 |
| Frames with M5 secondary information active | 1,347 |

Every frame records `REFERENCE_PLUS_EXPERIMENTAL`, Presence 1.20, Contrast 4,
the software comfort region, and no presentation fallback. Replay of the
recorded path found no nonfinite value, primary sign reversal, force created
from a zero primary, secondary-budget violation, or Legacy-authority violation.
No secondary information survived a FREE or active BITE veto. The recorded S9
hardware-selected directional value equals the post-Presence request on every
frame, and the recorded composer input equals that selection plus the unchanged
road and impact requests.

These values describe normalized software requests. They do not measure wheel
torque and do not certify hardware safety, comfort, or fatigue behavior.

## Deferred investigations

### Presentation calibration

Keep the driver observation with the backlog: current Reference+ has more
presence and feels like a good foundation; additional intensity appears
possible. A later controlled calibration may compare greater Presence,
Information Contrast, or both. It must keep device output and software
presentation separate and should not change today's values without new replay
and physical evidence.

### Sunny Beach runoff fidelity

The section before the first beach ball has horizontal-striped runoff reached
on the left and then the right around the corner. It looks and feels as though
it may contain repeated ridge or bump behavior, but the current steering
presentation does not clearly communicate an oscillatory disturbance.

A targeted study should compare corner `+0x28` displacement/loading context,
surface classification, the road contribution, AC/B0 response, front/rear
timing, and output slew through that section. The visual stripes are not proof
of physical bumps. Reference should expose an effect only if native game state
supports it.

### Gameplay: Unlock All Content

The preferred direction is a reversible managed save/profile clone. The
player's legitimate progress must remain untouched, the unlocked profile must
stay isolated, and returning to the original profile must be straightforward.
There should be no silent merge of artificial progress into a real save. This
would also make cross-car research practical without progression grinding.

## Documentation handoff

The next repository-wide documentation pass should consolidate the durable
engineering record around `HYP36R_FORCE.md`, `NATIVE_DYNAMICS.md`,
`TELEMETRY.md`, `DEVELOPMENT_HISTORY.md`, `PRESENTATION_AND_SAFETY.md`, and
`ROADMAP.md`. Existing milestone notes can remain as provenance.

The permanent record should preserve hypotheses, experiments, measurements,
uncertainty, failed CI and UAT, root causes, corrections, rejected approaches,
and the reasons decisions changed. It should remove repeated prompt scaffolding,
duplicated conclusions, and mechanical milestone templates without making the
work appear more linear than it was.

That documentation preservation pass is the recommended first post-S
milestone. It should organize the evidence already earned before any new force,
road, gameplay, calibration, or AER work begins.

## Regression audit

S10 made no source or configuration change. Inspection of the S9 baseline and
the physical capture confirms the existing routing: Reference stays exact M4,
Reference+ stays Presence 1.20 and Contrast 4 with its five-percent linear
secondary budget, the Legacy boundary remains authoritative, FREE and BITE
continue to veto M5, and road, impact, vibration, output conditioning, and
hardware strength remain unchanged.
