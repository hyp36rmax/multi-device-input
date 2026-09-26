# HYP36R Force 2.0 R4.2P passive Road presentation

R4.2P evaluates how the passive Road policy could be presented without making
Road a second steering model. `HYP36R_ROAD_PRESENTATION_V1_SHADOW` is offline
and replay-only. It is not included or evaluated by the runtime force hook.

## Contract and authority boundary

The prototype consumes only `HYP36R_ROAD_POLICY_V1`. It preserves three
separate outputs:

- a continuous activity envelope;
- neutral group-context imbalance;
- an unchanged transition intent.

It does not combine them into torque. It has no gain, frequency, Nm scale,
DirectInput request or conversion to a scalar force. Reference+ remains the
sole owner of steering load, centering, grip release/recovery and
LOAD/RELEASE/FREE/BITE behavior.

E8, EC and EE are not used. The presentation does not read Signal State or game
memory directly. The public telemetry schema remains unchanged.

## Candidate models

### Candidate A — direct evidence

The continuous envelope is the bounded RMS of the independently preserved
native L/R magnitudes:

```text
continuous = clamp01(sqrt((clamp01(abs(L))^2 + clamp01(abs(R))^2) / 2))
```

It is nonzero only when Road policy identifies a continuous candidate. Native
channel contrast `(abs(L)-abs(R))/(abs(L)+abs(R))` remains separate and is not
treated as physical handedness.

Each neutral surface group receives a context value from the fraction of its
two channels differing from the established reference. The neutral group axis
is:

```text
group_axis = clamp(group02_context - group13_context, -1, 1)
```

This is an information axis, not torque direction. A separate categorical flag
preserves group-to-group raw-state differences when both groups have equal
non-reference coverage and the numeric axis is therefore zero.

Transitions copy the policy's changed mask, changed count, topology and
coverage. They never create an impulse.

### Candidate B — three-sample conditioned evidence

Candidate B applies a trailing three-sample mean independently to the direct
continuous envelope and neutral group axis. It leaves transition identity
unchanged. At the approximately 60 Hz R2 rate this spans at most roughly 33 ms
of prior evidence and introduces one-to-two frames of trailing persistence.

This candidate exists only to measure whether minimal conditioning provides a
clear information benefit. It is not an approved feel filter.

## Bounds

Continuous activity and both group-context values are bounded to `[0,1]`.
Neutral group axis and native-channel contrast are bounded to `[-1,1]`.
Transition counts and masks remain discrete. These values are normalized
presentation intents, not torque and not Nm.

Uniform broad occupancy does not imply a larger envelope than partial
occupancy. Material IDs never alter magnitude.

## Accepted R2-B replay

All 5,860 accepted R2-B samples were replayed. `A` and `B` below refer to the
two candidate models. Correlation uses Candidate A against absolute v1 Road.

| Scenario | A coverage | A P95 / P99 / max | A 1 s RMS P95 | B coverage / P95 / max | Spatial coverage | Transitions | Correlation with v1 Road |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| B01 local asphalt control | 0% | 0 / 0 / 0 | 0 | 0% / 0 / 0 | 0% | 0 | n/a |
| B02 striped partial | 29.8% | .0821 / .0990 / .0990 | .0770 | 31.0% / .0819 / .0990 | 29.8% | 24 | .641 |
| B03 striped full | 16.2% | .0982 / .0990 / .1227 | .0692 | 17.6% / .0812 / .1069 | 7.7% | 24 | .555 |
| B04 rough/sand partial | 87.3% | .0910 / .1218 / .1233 | .0865 | 87.7% / .0908 / .1232 | 21.4% | 42 | .654 |
| B05 rough/sand full | 43.2% | .1711 / .3736 / .3739 | .2139 | 44.5% / .1714 / .3736 | 6.0% | 33 | .761 |
| B06 surface re-entry | 21.5% | .1506 / .1585 / .1593 | .1336 | 23.6% / .1500 / .1592 | 9.6% | 28 | .517 |

B04 remains the clearest sustained-activity case. B02 retains strong partial
spatial information. B03 is not automatically stronger than B02. B06 keeps its
28 surface-transition rows separate from later vehicle load/recovery.

Candidate B reduced continuous-envelope total variation by approximately 20%
to 42% in B02–B06, but increased activity coverage through trailing persistence
and delayed the representation of current evidence. R2-B does not demonstrate
noise that requires this trade. Candidate A is therefore selected as the
passive presentation baseline.

## Information beyond v1 Road

There are 409 accepted R2-B rows where v1 Road is effectively zero but the
prototype still retains continuous, spatial or transition information. These
include 301 rows with spatial context and 44 transition rows. This is the main
presentation result: the extra information is not a louder version of the v1
scalar.

The surface campaigns also show similar scalar Road values alongside different
raw tuples, group occupancy and L/R evidence. The shadow contract retains those
dimensions without claiming that they should all become wheel torque.

## False-positive controls

The seven accepted R2-A captures contain 2,288 samples where the surface is at
its established reference while the native directional phase is non-normal.
The prototype produces zero continuous activity and zero spatial imbalance on
all of those samples. Steering, high load, drift, release and recovery do not
become Road.

R2-C C01 contains 11 gear/native-transient rows on a stable surface; none
becomes continuous Road or spatial imbalance. C02 contains 12 classified event
rows; none becomes continuous Road. C02 has real recorded surface variation in
other rows, which remains valid context rather than being erased merely because
a collision occurred nearby.

## Selection

Candidate A is selected for future passive/active design work because it:

- reflects same-cycle evidence without an unsupported filter;
- preserves transitions separately;
- is bounded and deterministic;
- creates no activity on B01 or stable-surface directional controls;
- retains spatial and L/R information unavailable in v1;
- does not imply material strength or steering direction.

Candidate B remains a documented comparison, not the selected baseline.

## Runtime and regression boundary

The presentation files are linked only into their deterministic test target.
The runtime hook contains no presentation include, instance, call or read. Tests
require exact Reference+ equality for Directional, Road, Impact, composition,
tanh, pre-drive and final request before and after offline evaluation.

The contract is fixed-size, trivially copyable and allocation-free. It performs
no file I/O or string processing. A 200,000-update policy-and-presentation smoke
test runs in CI.

## Empirical gates

Physical handedness of groups 0/2 and 1/3 remains unresolved. The neutral group
axis must not become signed wheel torque until a controlled test independently
establishes the mapping. This does not block a future symmetric continuous
prototype, but it blocks directional spatial presentation.

The user's cobblestone observation is a useful unresolved question. R2-B does
not include a stable identified cobblestone interval, so one targeted future
capture is recommended before active Road work:

```text
local normal-road control
-> stable cobblestone section
-> normal-road return
```

Use relatively steady speed and minimal steering. The exact hypothesis is:

> Stable cobblestone occupancy produces repeating native L/R activity while
> canonical surface identity remains stable, and Candidate A preserves that
> activity without manufacturing Directional or transition intent.

Do not add a partial-occupancy pass unless the first capture supports the
hypothesis and track geometry makes it controlled. No UAT is created by R4.2P.

After that evidence gate, the first active prototype should be symmetric and
strictly bounded, use Candidate A's continuous envelope only, retain v1 Road as
an immediate fallback, and keep spatial and transition presentation inactive.
