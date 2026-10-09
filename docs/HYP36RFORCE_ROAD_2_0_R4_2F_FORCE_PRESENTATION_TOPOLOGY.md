# HYP36Rforce Road 2.0 R4.2F — force-presentation topology

**PART I — ENGINEERING RECORD**

## Scope and decision

R4.2F asks how an independently validated, unsigned surface-activity envelope
could be presented through a steering wheel without inventing steering
authority. It changes documentation only. Current Road, Reference+, Force
Character, Directional, Impact, M4/M5, Signal State, telemetry, DirectInput,
and controller behavior remain unchanged.

**Decision gate: A — PERIODIC TOPOLOGY WARRANTS AN INDEPENDENT PASSIVE
PROTOTYPE.**

This does not approve an active periodic effect. It approves a passive request
model that keeps amplitude evidence separate from unresolved waveform timing.
No wheel force, DirectInput effect, UAT artifact, or physical test follows from
this record.

The research lane is **ROAD 2.0 ONLY**. Alpha / Force Character, R5 / AER, and
Event 2.0 remain separate. Any relationship noted here is a
**CROSS-LANE CANDIDATE — NOT INTEGRATED**.

## Independent Road 2.0 baseline

HYP36Rforce independently established:

1. four native surface states and their occupancy topology;
2. surface transitions separate from continuous activity;
3. continuous native surface activity distinct from grip/load behavior;
4. `HYP36R_SIGNAL_STATE_V1` and `HYP36R_ROAD_POLICY_V1`;
5. Candidate A as a bounded continuous-activity envelope;
6. stable cobblestone activity with quiet normal-road controls;
7. R4.2W's negative signed-carrier result; and
8. R4.2N's mapped PC lineage from positive material coefficient through
   speed/context scaling to unsigned Xbox motor amplitude.

Candidate A answers **whether** supported continuous activity exists and
approximately **how much** exists. It does not provide ConstantForce polarity,
physical wheel handedness, bump spacing, waveform phase, or a selected player
gain. That distinction remains authoritative.

## Presentation problem

The Road channel owns continuous supported surface presentation. Directional
owns steering/load/grip authority and LOAD/RELEASE/FREE/BITE recovery. Event
owns discrete effects. A valid Road topology must therefore:

- have no persistent left/right steering bias;
- remain quiet when Road policy reports no meaningful activity;
- avoid deriving polarity from steering, Directional, total force, or neutral
  surface groups;
- generalize without a material-strength lookup table;
- preserve event exclusions and state resets;
- expose every invented presentation choice as presentation policy, not native
  telemetry; and
- remain bounded independently of master FFB strength and device behavior.

## Candidate topology matrix

| Topology | Signedness requirement | Zero-mean behavior | Steering-authority risk | Required source information | Independently possessed | Still missing | Normal-road risk | Generalization | Reference+ compatibility | Testability | Confidence / decision |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Current Road baseline | Synthetic sine already supplies sign | Yes over complete cycles | Low when bounded, but shares composed ConstantForce | Existing scalar motor envelope and fixed presentation policy | Runtime implementation and regression evidence | Physical meaning of carrier timing | Existing controls are established | Works as current fallback, but loses spatial identity | Established | High | Confirmed baseline; unchanged |
| ConstantForce-derived Road | Requires a signed carrier every update | Only if a valid bipolar source exists | High: arbitrary sign can bias or contaminate steering | Road-specific bipolar source, direction and reset semantics | Unsigned envelope only | All meaningful polarity evidence | Derivatives can create boundary impulses | Envelope changes generalize, semantics do not | Poor until carrier is proven | Offline derivations already tested | Rejected for continuous Road on current evidence |
| Periodic Road | Effect class supplies symmetric oscillation; amplitude remains unsigned | Intrinsic when centered and correctly stopped | Lower: separate effect class can remain tactile rather than directional | Activity amplitude, rate/period, waveform class, phase/reset, attack/release, device capability | Activity authority, speed, timing, policy validity/exclusions | Evidence-backed spatial rate, selected waveform, device behavior | Low if activity gate and reset are authoritative | Plausible across supported surfaces without material gains | Strong if separately budgeted and unable to alter Directional | Passive request/replay first, then later hardware gate | Promising topology; passive prototype warranted |
| Spring/damper-modulated Road | No oscillator sign required; force follows displacement or velocity | Condition effect is opposing/centering rather than oscillatory | High: changes steering weight, centering, or resistance | Surface authority plus defensible condition modulation | Surface authority only | Physical link between surface and steering condition | Can create persistent feel from noisy state | Could generalize, but as disguised load modulation | Conflicts with Directional ownership | Easy to prototype, hard to interpret | Rejected for Road 2.0 |
| No additional active Road | None | Yes | None | None | Complete | None | None | Universal | Perfect | Trivial | Safe fallback if passive evidence fails |

## ConstantForce assessment

R4.2W and R4.2N remain controlling evidence. The mapped PC tactile path has no
validated signed Road carrier. First difference and local-mean subtraction can
make an envelope bipolar mathematically, but neither was separable from speed
and envelope change. Using steering sign, Directional sign, total-force sign,
surface-group sign, or an arbitrary alternating sign would create semantics
outside the evidence.

ConstantForce-derived continuous Road therefore remains rejected. Current Road
is retained as the product baseline; this decision does not remove or retune
it. A later independently observed bipolar Road source could reopen the class,
but R4.2F finds none.

## Periodic-effect assessment

A periodic effect solves one structural problem: it can turn unsigned activity
authority into a symmetric, zero-mean tactile presentation without asking the
Road source which steering direction is correct. Keeping it as a separate
effect class also makes Road ownership clearer than injecting guessed polarity
into the structural ConstantForce request.

It does not solve timing automatically. A defensible periodic request needs:

| Input | Current evidence | Status for passive prototype |
| --- | --- | --- |
| Amplitude authority | Candidate A plus Road-policy validity and exclusions | Available as evidence; normalization not selected |
| Frequency / period | Speed and elapsed/delta time exist; no physical texture spacing is known | Research variable; not product-ready |
| Phase | No absolute physical phase was observed | May be internally accumulated and reset, but must be labeled synthetic presentation state |
| Direction/orientation | Physical handedness is unresolved | Use a centered one-axis zero-mean class only; no spatial sign |
| Attack/release | Policy validity, transitions, exclusions, and activity cadence exist | State transitions are available; envelopes and time constants are unselected |
| Bounds | Candidate A is bounded and existing safety layers are known | A separate conservative passive authority budget can be specified later; no value selected here |

The topology is therefore suitable for passive modeling, not hardware output.

## Amplitude evidence

Candidate A and its upstream left/right activity provide meaningful evidence
for **when** activity exists and its relative magnitude inside accepted
captures. Cobblestone, rough/sand, partial runoff, broad occupancy, and re-entry
all exercise that authority, while normal-road controls remain zero.

Candidate A's present normalization must not be copied unchanged into a force
magnitude. The passive prototype should retain raw policy inputs, Candidate A,
requested presentation amplitude, and any conditioning as separate fields.
Material IDs may gate independently validated applicability later; they must
not become an arbitrary strength table.

## Frequency, period, and distance versus time

No accepted capture establishes a physical cobblestone bump rate or spatial
wavelength. Native game timing can advance an oscillator reliably, but fixed
time frequency would make the same surface pass at the same temporal rate at
all speeds. That is easy to implement and weakly grounded physically.

A distance-domain model is more defensible as a class:

```text
distance increment = validated vehicle-speed magnitude * valid delta time
phase increment    = distance increment / unresolved spatial wavelength
```

It naturally converts spatial texture encounter into a faster temporal rate as
vehicle speed rises. HYP36Rforce possesses speed, valid frame timing, surface
authority, and reset/exclusion state. It does **not** possess independently
validated texture spacing or proof that every supported surface should share a
spatial rate.

Accordingly, R4.2F selects neither a fixed Hz value nor a wavelength. The
passive milestone should compare time-domain and distance-domain request state
against accepted captures, explicitly treating rate parameters as research
variables. It must not optimize them by feel or name a material-specific rate.

## Phase and orientation

Absolute phase has no independently observed physical meaning in the current
PC evidence. A passive oscillator may maintain internal phase continuity only
as synthetic presentation state. Phase must reset or become invalid on session
start, state loss, Road exclusion, discontinuous time, or unsupported context
so stale activity cannot reappear as a pulse.

Orientation is simpler: a one-axis centered periodic class can be symmetric
around zero without selecting left or right steering authority. Group 0/2 and
1/3, steering sign, Directional sign, and total-force sign remain prohibited as
Road polarity. No spatial imbalance is converted to wheel direction.

## Attack, release, and event isolation

Immediate start/stop preserves evidence most literally but can produce an
implementation edge unrelated to the surface itself. A bounded attack/release
envelope may be justified as presentation hygiene, but no time constant is
selected here. The passive prototype must compare direct gating with a minimal
stateful envelope and record both requested and admitted activity.

Gear and classified collision frames remain Event-owned and excluded from
continuous Road. A stateful Road request must clear or hold safely across those
exclusions without ringing, phase jumps, or a release tail being mislabeled as
surface evidence.

## Spring and damper assessment

Spring follows wheel displacement and damper follows wheel velocity. Changing
either with surface activity would communicate resistance, centering, or
steering weight—not merely continuous texture. Even if bounded, it would make
Road alter the same perceptual territory owned by Directional and Reference+.

Spring/damper modulation is therefore rejected for Road 2.0 on ownership
grounds. This does not prohibit wheel/device damping or low-speed stabilization
elsewhere; it says surface activity must not control them in this lane.

## Scenario assessment

### Cobblestone

- Current Road: remains the unchanged fallback; elevated gain did not establish
  a unique cobblestone structure.
- ConstantForce derivation: Candidate A stays active, but no polarity or bump
  timing exists; derivatives mostly describe envelope/speed changes.
- Periodic: activity can authorize a centered waveform, while rate remains the
  unresolved question. Distance-domain timing is plausible but unvalidated.
- Spring/damper: would make a stable surface alter steering resistance.
- No additional Road: preserves safety but leaves the known perception question
  unresolved.

The controlled interval supports amplitude authority and false-positive
separation, not a selected waveform.

### Rough, sand, striped runoff, and re-entry

R2-B shows sustained activity, mixed and broad occupancy, and transitions
without needing material-specific strength. A periodic topology can use the
same activity contract across them. It must keep transitions discrete and must
not turn partial occupancy into signed steering. Generalization is plausible
at the topology level; rate and perceptual equivalence remain unproven.

### Normal road

Normal controls are the primary false-positive gate. When Road policy reports
no meaningful continuous candidate, requested amplitude must be exactly zero,
state must not free-run into audible output, and re-entry must not create a
synthetic impulse. Any passive model failing this gate is rejected before UAT.

## Ownership and safety boundary

```text
Directional = steering/load/grip authority
Road        = continuous supported surface presentation
Event       = discrete events
```

The periodic class is acceptable only because it can remain separate from
Directional and Event. It does not reduce structural force to make texture
audible, modify spring/damper, consume steering polarity, or reinterpret event
onsets. Future device fallback behavior is a later hardware architecture
question and cannot silently inject Road into ConstantForce.

## External reference — not integrated

THP32's public fork was reviewed at commit
[`8415ab82ad8ad32ea1b08252af8731d68e90b1ec`](https://github.com/thp32tt/OutRun2006Tweaks/commit/8415ab82ad8ad32ea1b08252af8731d68e90b1ec)
only to compare broad topology. Its documentation and implementation show a
wheel-specific DirectInput path that can use a periodic effect class for Road
and a ConstantForce fallback on hardware where the periodic result is not
useful. It also separates structural steering, condition effects, tactile
effects, and events. See its
[`WHEEL_FFB.md`](https://github.com/thp32tt/OutRun2006Tweaks/blob/8415ab82ad8ad32ea1b08252af8731d68e90b1ec/WHEEL_FFB.md)
and production wheel path for the external record.

That comparison demonstrates that multiple wheel-presentation topologies are
practical software architectures. It does not validate one for HYP36Rforce,
prove its signals equivalent to ours, or provide parameters we may adopt. No
external equation, constant, period, phase, direction, threshold, lookup,
offset, or code is copied into Road 2.0.

GATS's feedback helped surface perception questions around one-sided edges,
rough/sand, and gear shifts. It remains other-user experiential feedback, not
universal consensus or technical proof. The controlled HYP36Rforce captures,
not that feedback, establish the independent findings used in this decision.

## Independent reproduction targets

The highest-value external observations to reproduce independently are:

1. whether a retail force-feedback OutRun branch has a wheel-specific surface
   path separate from controller rumble;
2. whether that path submits a periodic effect class for continuous Road;
3. which game-state classes authorize amplitude and rate;
4. whether rate is tied to time, speed, traveled distance, or material context;
5. how state loss, events, ordinary road, and mixed occupancy silence/reset it;
6. whether hardware compatibility requires a different presentation class.

These are questions only. Tracing retail/PS2/arcade submission paths belongs to
R5/AER and is not performed or integrated in R4.2F.

## Minimum next passive milestone

**R4.2F-P — Passive periodic request topology** should remain offline or shadow
only and produce no DirectInput effect. It should consume
`HYP36R_ROAD_POLICY_V1` through the existing passive presentation boundary and
record:

- source validity, exclusions, surface context, and Candidate A;
- direct and conditioned amplitude requests separately;
- valid delta time, speed, and accumulated distance;
- a time-domain rate request and distance-domain phase-rate request as clearly
  labeled research variables;
- synthetic phase state, reset reason, and active/quiet transitions;
- attack/release request state without selecting a product constant;
- requested zero-mean waveform value for replay only;
- proposed authority before and after passive bounds; and
- proof that the hardware-selected Road and final DirectInput request remain
  bit-identical to the current baseline.

The milestone must test accepted cobblestone, rough/sand, runoff, re-entry,
normal-road, gear, collision, steering, RELEASE/FREE/BITE, invalid-delta, and
state-loss records. It should compare topology sensitivity, not search for a
preferred feel. No material table, external parameters, or player setting is
allowed.

## Unresolved questions

- What independently defensible spatial rate, if any, maps traveled distance
  to periodic phase?
- Should all supported surfaces share a rate class?
- Does source amplitude need conditioning before presentation authority?
- What is the minimum edge-safe attack/release policy?
- How do DirectInput periodic capabilities and device differences affect a
  later active design without forcing a ConstantForce semantic fallback?
- Can an official retail/arcade force path independently establish topology or
  timing without contaminating this PC evidence lane?
- Is an additional wheel Road effect perceptually useful once it passes all
  evidence and ownership gates, or should current Road remain?

## Next recommendation

Proceed only to R4.2F-P passive periodic request topology. Use existing accepted
captures first; do not create UAT. If passive replay cannot distinguish a
defensible rate model without guessed spatial parameters, stop at decision D:
current Road remains until better evidence exists.

## Part II — Development Journey Recap

### What were we trying to understand?

We had good evidence for how much continuous surface activity existed, but no
evidence telling ConstantForce which way to push the wheel. R4.2F asked whether
a different presentation class could respect that limitation.

### What did we compare?

We compared the current Road baseline, a newly derived ConstantForce signal, a
separate periodic effect, spring/damper modulation, and doing nothing new. We
also reviewed THP32's work only as an external example of alternate topology,
without importing its implementation or tuning.

### What did we find?

ConstantForce still needs polarity we do not possess. Spring and damper would
turn Road into steering-load behavior. A periodic class can stay centered and
zero-mean while Candidate A controls only activity authority. The missing piece
is its rate: speed and distance are available, but physical texture spacing is
not.

### Why does it matter?

This separates two questions that had been mixed together: the game can tell us
that a surface is active without also supplying the wheel waveform. Treating
the waveform as presentation policy keeps the evidence honest and protects
Reference+'s steering ownership.

### What did we deliberately not conclude?

We did not select a frequency, wavelength, phase, waveform, gain, material
table, hardware fallback, or player setting. We did not prove that periodic
Road will feel better, and we did not activate Road 2.0.

### What happens next?

The next step is one passive replay/shadow model that measures time-based and
distance-based request behavior while proving current hardware output remains
unchanged. Physical testing happens only after a specific, independently
supported active hypothesis is approved.
