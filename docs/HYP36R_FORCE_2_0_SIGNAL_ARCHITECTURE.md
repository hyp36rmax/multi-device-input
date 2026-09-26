# HYP36R Force 2.0 signal-preservation architecture

## Decision

R4 defines an evidence-backed architecture for HYP36R Force 2.0. It does not
implement a new force model, change Reference+, or replace any validated v1
behavior.

The central rule is:

> Preserve signal identity until an explicit interpretation stage has enough
> evidence to classify it. Apply player gain only after that interpretation.

The recommended structure is a versioned internal **HYP36R Signal State**
between game-state acquisition and presentation. It carries raw observations,
validity, provenance, timing, and separately interpreted states. Reference+
remains the known-good fallback and can continue consuming the established v1
channels while 2.0 candidates are developed passively.

## Reference baseline

Treat these v1 behaviors as constraints, not disposable prototypes:

- vehicle-derived directional steering and current 1.00x presentation;
- known-good gear-shift feel;
- M4/M5 authority and BITE restoration boundaries;
- LOAD, RELEASE, FREE, and BITE foundation;
- channel composition, `tanh`, ramping, inversion, master Strength, and bounded
  DirectInput output;
- fail-closed behavior when optional native information is unavailable.

R4 changes where information is retained and interpreted. It does not claim
that every richer observation deserves a new wheel effect.

## Validated signal-class inventory

| Signal class | Earliest supported source | Confidence | Spatial resolution | Temporal behavior | v1 destination | Current loss point | Potential 2.0 consumers |
|---|---|---|---|---|---|---|---|
| Vehicle/directional state | steering, speed, native D38–D48 response state, M4/M5 | high for lineage and current interpretation | vehicle/global | continuous, frame-rate state | Reference+ directional | native detail is summarized into interpreted authority/unloading | directional wheel FFB, motion, telemetry, future AER comparison |
| Surface identity | `water_flag_24C[4]`; mirrored `field14` | high for four independent raw states; incomplete material names | four contact points | discrete state with persistent intervals | indirect input to restored effects; passive telemetry | `CalcVibrationValues()` combines it with other state | Road context, haptics, telemetry, future AER comparison |
| Spatial surface occupancy | combinations of four raw surface states | high for partial/broad distinction | four contact points plus derived occupancy | persistent and transitional | no independent v1 force channel | lost when four inputs become two motor composites | Road interpretation, spatial haptics, motion context |
| Continuous surface activity | restored right/left vibration during sustained surface travel | high that activity exists; moderate source specificity | two composite motor channels | continuous/enveloped | scalar Road from right motor | source/context already combined; right-only selection loses more | Road presentation, bass/seat haptics |
| Surface transition/event | per-corner changed flags plus synchronized vibration/event state | high for transition timing; moderate for physical meaning | per contact point plus vehicle/global event context | discrete edge with possible tail | no dedicated v1 channel; may influence Road/Impact | transition meaning is not retained after composite effect generation | Road/event interpretation, haptics, motion |
| Gear event | native gear mismatch and exact `.14/0` restored signature | high | vehicle/global | deterministic onset plus v1 decay | scalar Impact | `max(left,right)` and shared envelope remove event type | preserved wheel shift cue, pedal/seat haptic |
| Physical-collision event | controlled bilateral vibration/rise candidates without gear transition | moderate; four controlled candidates | currently vehicle/global with two-channel signature | discrete onset plus shared decay | scalar Impact | `max(left,right)` and shared envelope remove channel/source pattern | collision wheel cue, seat haptic, motion impulse |
| Grip/load state | D38–D48 lineage, M4 phases, BITE state/restoration | high for current state-machine behavior; bounded physical semantics | vehicle/global | continuous state plus transitions | directional authority/unloading | physical state is intentionally interpreted, not raw tire load | directional wheel FFB, motion, telemetry |
| Native per-corner dynamics | E8/EC/EE observations | E8 moderate context; EC/EE possible dynamic correlation | four contact points | continuous/discrete candidates | passive telemetry only | no active collapse because not yet consumed | future state refinement, motion, telemetry after validation |

“Potential consumer” means an architectural extension point. It does not
authorize a force or device behavior.

## Current v1 information-loss map

```text
GAME / NATIVE STATE
│
├─ steering + speed + D38–D48
│      ↓ interpreted M4/M5/BITE state
│      ↓ Reference+ directional presentation
│      └─ useful vehicle meaning retained, bounded by an explicit state model
│
├─ surface[4] + other car state
│      ↓ restored CalcVibrationValues()
│      ↓ left/right composite motor values              [spatial/source collapse]
│      ↓ right motor only × synthetic 6 Hz carrier      [further collapse]
│      ↓ scalar Road
│
└─ gear/collision/surface/other events
       ↓ restored left/right motor signature
       ↓ max(left,right)                                [channel/source collapse]
       ↓ rise threshold + common direction/decay        [event-type collapse]
       ↓ scalar Impact

directional + Road + Impact
       ↓ Force Character gain
       ↓ tanh / ramp / inversion / Strength / DirectInput
```

The directional path already uses an explicit interpretation model. Road and
Impact currently depend on composite effect carriers whose meaning is narrower
than the native evidence available upstream.

## Proposed 2.0 architecture

```text
Native Acquisition Snapshot
  raw vehicle state
  raw surface[4]
  raw left/right restored effects
  raw gear state
  raw native dynamic candidates
  timestamp/frame/executable identity
          ↓
HYP36R Signal State v1
  vehicle
  surface identity[4]
  spatial occupancy
  continuous surface activity
  surface transitions/events
  gear event
  collision candidate/event
  grip/load state
  validated native dynamics
  validity + confidence + provenance for every member
          ↓
Interpretation Policies
  Directional State Policy
  Road Meaning Policy
  Event Meaning Policy
  optional future AER policy
          ↓
Presentation Intents (pre-player gain)
  directional
  road activity
  gear event
  collision event
  other validated event
          ↓
Player Presentation Gain
          ↓
bounded composer / output conditioning
          ↓
wheel FFB

The same Signal State may also feed passive telemetry, offline replay,
active-pedal research, haptics, or motion without decoding final wheel torque.
```

Raw acquisition and interpretation are different structures. A raw value does
not acquire a physical name merely by entering the bus. Interpreted members
must identify their source revision, validity, confidence, and policy version.

## Road 2.0 responsibility

### Road Detail should own

- continuous, travel-related surface activity after its source is validated;
- optional presentation of supported spatial occupancy/context;
- a wheel-appropriate expression of a validated physical surface transient
  when that transient belongs in steering rather than the general event path;
- player gain applied to an already meaningful Road intent.

### Road Detail should not own

- raw material IDs converted directly into arbitrary intensity;
- all surface transitions by definition;
- vehicle grip, steering authority, or load transfer already owned by the
  directional/state model;
- gear changes or general collisions;
- gain used to hide missing spatial or source information;
- external curb/road algorithms imported without project evidence.

Surface identity is context, not automatically vibration. Spatial occupancy
can influence interpretation without every changed corner producing force.
Surface events should be classified before ownership is assigned: a texture
edge may belong to Road, while a discrete strike may belong to Event/Impact.
Grip response remains directional unless later evidence establishes a separate
physical surface contribution.

### Road information preservation

Road 2.0 should retain these inputs side by side through interpretation:

```text
surfaceIdentity[4]        raw discrete values
surfaceChanged[4]         per-corner edges
surfaceOccupancy          derived pattern/count, with raw values still present
restoredActivityLeft      composite observed carrier
restoredActivityRight     composite observed carrier
contactContext[4]         E8 only after validation
vehicleGripLoadState      read-only context from directional state model
```

An interpretation policy may produce separate `continuousActivity` and
`surfaceEvent` intents. It must record which inputs contributed and why. The
presentation stage can then choose wheel, seat, or other output treatment.

R3's scalar Road `2.00x` remains a valid **v1-architecture headroom result**.
It is not a 2.0 target because a new Road intent will have a different range,
duty cycle, and meaning. Road 2.0 must establish its own normalization before
any extended slider ceiling is validated.

## Event and Impact responsibility

The event layer should own short, discrete, validated events that are not the
continuous directional or Road-activity state. It should preserve event class,
source channels, onset time, direction evidence, confidence, and lifecycle.

Supported initial classes are deliberately small:

- `gear_shift`, high confidence;
- `physical_collision`, moderate controlled evidence;
- `surface_transient`, only when independently supported by surface timing and
  event evidence;
- `unknown_native_event`, an explicit unclassified observation, not a new
  semantic category.

The event layer should not turn every vibration rise into a collision or infer
event type from amplitude alone.

### Event-preserving flow

```text
gear mismatch ────────────────────────┐
surface transition/context ──────────┤
left/right restored effect levels ───┼─> Event Observation
left/right rise and timing ──────────┤      raw channels retained
vehicle context ─────────────────────┘
                                            ↓
                                      Event Classifier
                                      class + confidence
                                      onset + lifecycle
                                      no presentation gain
                                            ↓
                                      Event Intent Policy
                                      gear / collision /
                                      supported surface /
                                      unknown
                                            ↓
                                      per-class presentation
                                      then player gain
```

The current scalar Impact remains available as the fail-safe v1 path until an
event policy is independently validated. Collision gain and shaping are not
selected in R4.

## Gear-shift preservation requirement

The controlled v1 signature is left `.14`, right `0`, rise `.14`, and absolute
Impact approximately `.07735`, with zero recorded transition-row variance.
Project-owner physical assessment treats the current feel as good.

Force 2.0 must therefore provide a regression fixture that reproduces the
current shift presentation within documented numeric tolerance when the gear
policy is in compatibility mode. Collision or surface-event development must
not change that result accidentally. Any future shift change requires new
evidence; it is not a side effect of event separation.

## Directional-steering boundary

Directional Steering remains the owner of vehicle steering authority,
unloading/restoration, and validated grip/load interpretation. It may consume
read-only surface or event context only where a specific evidence-backed policy
requires it. Road and Event must not write M4/M5 state or bypass BITE authority.

The signal layer makes directional state available to other consumers, but
does not merge texture or transient impulses into the directional state model.
The current Reference+ directional intent is the compatibility reference.

## Force Character 2.0

Force Character belongs after interpretation:

```text
validated/interpreted intent → player presentation gain → composer
```

It must not be:

```text
ambiguous raw signal → large gain → hoped-for meaning
```

R3's v1 candidates remain:

| Channel | 0% | Recommended 1.00x marker | Candidate v1 100% |
|---|---:|---:|---:|
| Steering | 0x | 76.9% | 1.30x |
| Road | 0x | 50.0% | 2.00x |
| Impact | 0x | 66.7% | 1.50x |

Steering's candidate may remain relevant if the directional intent is
unchanged. Road and Impact candidates apply only to their current v1 scalar
intents. New 2.0 intents require independent normalization, combined replay,
and physical validation. Current 1.00x remains the compatibility/default
marker.

## Native per-corner validation gates

No E8/EC/EE field may influence Force until it passes its own gate.

| Field | Exact next question | Minimum independent evidence | Pass condition before Force use |
|---|---|---|---|
| E8 | Does it represent contact/context independently of raw surface identity and vehicle motion? | hold one raw class while inducing contact loss/re-entry, then change raw class while maintaining contact and low motion | repeatable E8 change tied to one condition, with counterexample against the other |
| EC | Does its signed paired response follow lateral direction/load rather than surface class or generic motion? | matched-speed opposed steady left/right corners on one stable surface, plus straight control | repeatable sign/scale relationship that reverses with direction and survives surface control |
| EE | Does it add drift/recovery information beyond EC and current response state? | clean-surface drift entry, sustained hold, and recovery with EC/EE/state synchronized | repeatable phase or magnitude feature not already explained by EC, steering, speed, or M4 phase |

Writer/reader lineage and executable stability remain required even if a
correlation passes. External semantic names are not adopted.

## LOAD, RELEASE, FREE, and BITE refinement hypotheses

| State | Current meaning | Controlled support | 2.0 hypothesis to test |
|---|---|---|---|
| LOAD | M4 normal/full directional authority | A01–A03 controls and A04–A07 returns | distinguish stable load from transient post-recovery normalization without adding surface semantics |
| RELEASE | M4 emerging transition toward unloading | brief A02/A03 and A04–A07 transitions | test whether validated native dynamics improve transition timing without broadening false releases |
| FREE | M4 established unloading state | sustained A04/A05/A06 intervals | test whether native dynamics distinguish sustained vehicle release from driver-only steering divergence |
| BITE | validated convergence-driven restoration event | active A04/A05; absent A07 despite maneuver intent | test whether a validated convergence candidate improves reacquisition timing without allowing driver-only or invalid-state advancement |

Classifier labels describe internal state, not the driver's maneuver name.
A “recovery” drive does not prove BITE, and a drift label does not guarantee
FREE. M4/M5 and existing safety invariants remain authoritative during research.

## HYP36R Signal State API assessment

A normalized internal layer is recommended because it improves:

- identity preservation: raw and interpreted members coexist;
- testability: policies accept deterministic snapshots instead of reading game
  memory directly;
- offline replay: the same snapshot/policy contract can run without hardware;
- future outputs: consumers subscribe to validated meaning, not final torque;
- AER separation: AER and Reference+ can use shared observations with different
  policies;
- versioning: additions can be append-only and policy versions explicit;
- fallback: invalid optional members can be rejected without damaging the
  compatibility path.

Conceptual contracts:

```text
SignalHeader
  schemaVersion, frame, monotonicTime, executableIdentity

Observed<T>
  value, valid, source, sourceRevision, age, finite

Interpreted<T>
  value, valid, policyVersion, confidence, contributingSources

SignalState
  vehicle
  surface[4]
  restoredEffects
  events
  gripLoad
  nativeCandidates

PresentationIntent
  directional
  roadActivity
  gearEvent
  collisionEvent
  supportedSurfaceEvent
```

Normalization belongs to the owning interpretation policy and must state the
observed range and saturation behavior. Unknown raw values remain raw. The API
must not expose mutable cross-consumer authority over the state model.

## External-output extension points

- Wheel FFB consumes bounded directional, Road, and event presentation intents.
- Active pedals may later consume validated gear and vehicle-state events, not
  wheel-force peaks.
- Seat/bass haptics may consume continuous surface activity and classified
  events with output-specific presentation.
- Motion may consume vehicle/load state and validated dynamics, not synthetic
  6 Hz Road or final DirectInput torque.
- Telemetry consumers may observe raw, interpreted, and presentation stages
  without controlling them.

No device protocol, routing, gain, or safety envelope is selected in R4.

## AER boundary

```text
validated Native Acquisition / HYP36R Signal State
          ├─> HYP36R Reference+ policies
          └─> future AER policies
```

Shared evidence does not imply shared equations. Reference+ remains HYP36R's
physics/state interpretation. AER must be based on official/arcade evidence and
must declare which signal members it uses. Neither policy may silently redefine
the other's state or presentation semantics.

## Compatibility and fail-closed behavior

| Failure | Required behavior |
|---|---|
| Optional native signal unavailable | mark only that member invalid; use validated compatibility policy |
| Unsupported executable/revision | do not read address-bound optional signals; retain supported v1 Reference+ path where safe |
| Validation probe fails | disable the dependent 2.0 policy, log one clear reason, and select compatibility behavior |
| Stale signal | reject by age/cadence contract; do not hold a force indefinitely |
| Nonfinite value | invalidate/reset the member and dependent intent before composition |
| Partial per-corner validity | preserve valid raw observations, but do not run policies requiring all four corners |
| Event classifier uncertain | keep `unknown_native_event` or fall back to v1 scalar Impact; never invent a class |
| Policy output invalid | zero that optional intent or choose v1 intent according to the declared fallback; keep bounded output conditioning |

Fallback is explicit and observable. It must not blend incompatible v1 and 2.0
meanings silently. Directional Reference+ must not become dependent on optional
Road/event/native-candidate availability.

## Replay and telemetry requirements

A future schema should remain append-only during development and record:

1. schema, executable, source, and policy versions;
2. raw four-corner surface and dynamic observations with per-member validity;
3. raw restored left/right effects and rises;
4. event observations, candidate features, selected class, confidence, and
   lifecycle;
5. raw and interpreted vehicle/grip/load state;
6. each pre-gain presentation intent separately;
7. player gain and post-gain value separately;
8. composition, conditioning, fallback reason, and final DirectInput request;
9. monotonic timestamps, frame IDs, stale-age information, and reset events;
10. passive shadow output beside active selection during validation.

Offline replay must be able to run interpretation and presentation policies
from recorded acquisition snapshots deterministically. Golden fixtures should
include the C01 gear signature, B04 continuous-surface case, B06 surface/re-entry
ordering, A04 limiting load, A05 BITE, and invalid/nonfinite/fallback cases.
The current 222-column schema is unchanged by R4.

## v1 to 2.0 migration

| Area | Migration rule | Reason |
|---|---|---|
| Directional steering | **PRESERVE**, then shadow any refinement | validated behavior and current product identity |
| Road | **REVALIDATE**; retain v1 scalar Road as compatibility path | activity is useful but spatial/source identity is lost |
| Impact | **REVALIDATE** and split only with evidence | current transient works but event classes collapse |
| Gear shift | **PRESERVE** with a numeric/physical compatibility fixture | known-good deterministic feel |
| Force Character | **PRESERVE 1.00x marker**; revalidate ceilings per new intent | gain belongs after meaningful interpretation |
| M4/M5 | **PRESERVE** authority and safety boundaries | validated foundation |
| LOAD/RELEASE/FREE/BITE | **REPLACE ONLY WITH EVIDENCE** and passive shadow first | controlled support exists; semantics remain bounded |
| Output conditioning | **PRESERVE** | established safety and device boundary |

Migration begins by observing richer state passively. No active consumer may
replace v1 until replay invariants and a targeted physical comparison pass.

## Staged implementation research

1. **R4.1 — Signal State contracts and passive adapter.** Add versioned data
   structures, provenance, validity, golden replay fixtures, and no active
   output change.
2. **R4.2 — Road interpretation shadow.** Preserve four-corner identity,
   occupancy, activity, and transitions; compare to v1 scalar Road passively.
3. **R4.3 — Event classifier shadow.** Separate gear compatibility, collision
   candidates, supported surface events, and unknown events without changing
   hardware output.
4. **R4.4 — Native-dynamics validation.** Run the exact E8/EC/EE lineage and
   independent experiments before any Force use.
5. **R4.5 — State-model refinement shadow.** Test only specific
   LOAD/RELEASE/FREE/BITE hypotheses against existing invariants.
6. **R4.6 — Controlled active prototypes.** Activate Road and Event candidates
   one at a time behind explicit modes, with v1 fallback and telemetry proof.
7. **R4.7 — Force Character 2.0 integration.** Normalize validated intents,
   replay combined ranges, then perform targeted wheel-class UAT.
8. **R4.8 — AER and external-output research.** Keep separate policies and
   output-specific safety contracts; do not decode final wheel torque.

This order validates identity before presentation and presentation before broad
device integration.

## Release boundary and empirical backlog

R4 and every stage above belong to 2.0. v1.5 remains controller/UX polish,
compatibility, diagnostics, upstream integration, and small fixes that do not
change vehicle, surface, event, or state interpretation.

No physical testing is required for R4. The architecture records four future
targeted questions, none of which blocks R4:

1. E8 contact/context isolation at constant raw surface identity.
2. EC/EE opposed steady-corner and clean drift-phase validation.
3. Broader event-class evidence separating vehicle collision, wall/edge strike,
   and physical surface transient without amplitude-only labels.
4. Raised-gain wheel-class UAT only after a candidate 2.0 intent and combined
   replay exist.

Generic additional driving is not requested.
