# HYP36Rforce Road 2.0 R4.2N — native pre-motor surface lineage

**PART I — ENGINEERING RECORD**

R4.2N traces the surface-conditioned path upstream of the restored Xbox motor
outputs. It is static source/lineage research based on the restored routine,
the supported PC structure layout and the accepted R2/R4 captures. It changes
no Force, Road, DirectInput, telemetry or controller behavior.

## Result

**C. PC SURFACE EFFECT IS PROCEDURALLY GENERATED AS UNSIGNED TACTILE OUTPUT.**

More precisely, the surface effect available to the current PC implementation
is the restored Xbox-compatible controller path. The source notes that the
original PC executable omits `CalcVibrationValues()`; Multi Input restores the
Xbox routine because the relevant structure layout and engine calls match.
The result is therefore native game-authored Xbox controller logic operating
on PC game state, not an original PC steering-force routine.

Within the path that is currently mapped, the game does not supply
`CalcVibrationValues()` with a signed surface waveform. It supplies four
categorical surface states, a speed-like scalar, vehicle/event state and
flags. `sub_1149C0()` converts each surface state to a positive material
coefficient. `CalcVibrationValues()` selects the strongest coefficient, scales
and mixes it with speed and other conditions, clamps the two results, and
publishes Xbox left/right motor amplitudes.

This fully explains the dominant stable-cobblestone observation without a
hidden signed carrier: constant material coefficient plus changing speed gives
a changing unsigned envelope. It also explains why R4.2W could not recover
physical direction from the final motor values.

The conclusion is scoped to the mapped PC/Xbox controller-effect lineage. It
does not prove that no other physics quantity exists elsewhere in the game.

## Call graph

```text
WheelForceFeedback::update(EVWORK_CAR*)             PC hook, once per update
  -> CalcVibrationValues(car)                       restored Xbox 0x114C60
       -> sub_1149C0(surface0, context, flags)      material coefficient
       -> sub_1149C0(surface1, context, flags)
       -> sub_1149C0(surface2, context, flags)
       -> sub_1149C0(surface3, context, flags)
       -> Game::GetNowStageNum / GetStageUniqueNum  selected table cases
       -> Fabsf                                     state/event magnitudes
       -> optional event-work lookup                event_data + 0x1C4
       -> VibrationLeftMotor / VibrationRightMotor  unsigned floats
       -> SetVibration() on the internal stop path
  -> HYP36R v1 / passive Signal State observation
  -> SetVibration()                                 Xbox motor presentation
  -> GamePlCar_Ctrl.call(car)
```

The PC game does not contain this function; the project restores the closely
matching Xbox assembly against the shared `EVWORK_CAR` layout. The hook calls
it before the original player-car control call in this update path.

## Data-flow graph

```text
EVWORK_CAR::water_flag_24C[0..3]
  -> four sub_1149C0 lookups
  -> maximum positive material coefficient
                                  EVWORK_CAR + 0x1C4 speed-like scalar
                                                  |
                         coefficient * scalar * 0.1
                                      |
                  surface-conditioned unsigned working level
                                      |
 flags + vehicle dynamics + gear/event state + timers
                                      |
             two non-negative accumulators, holds and clamps
                                      |
        global VibrationStrength multiplier (0.0 through 1.0)
                                      |
       VibrationLeftMotor / VibrationRightMotor (0.0 through 0.75)
                                      |
             Xbox WORD left/right motor speeds (0 through 65535)
```

Four-corner surface identity is lost when the four lookup results are reduced
to their maximum. Physical direction is not created later; L/R motor identity
is presentation identity, not signed steering direction.

## Input and dependency inventory

The restored assembly directly reads the following state. Neutral names are
retained where semantics are not proven.

| Source | Representation and role | Classification |
| --- | --- | --- |
| `VibrationStrength` | integer clamped to 0–10, converted to `0.0–1.0` master multiplier | user/controller presentation |
| `EVWORK_CAR +0x4`, `+0x8` | bitfields selecting vehicle/event branches and modifying a state bit | derived game/event state |
| `+0x38` | integer gate in a low-speed/event branch | game state; meaning unresolved |
| `OnRoadPlace_5C` passed as integer context | second `sub_1149C0` argument; changes selected values for IDs `2` and `0x400000` | stage/road context |
| `+0x178` | float added at half scale on selected flag branches | game state; effect contribution |
| `+0x1C4` | non-negative speed-like float; primary multiplier, thresholds and event comparison | continuous vehicle state |
| `+0x1D0` | signed float; raw bits retained and absolute magnitude calculated | vehicle/event comparison state |
| `+0x1D4` | signed float used in zero/range comparisons | vehicle-dynamic state |
| `+0x1D8`, `+0x208` | previous/current gear-like integer comparison | gear event state |
| `+0x1DC`, `+0x1E0`, `+0x1E4`, `+0x2DC` | float comparisons selecting a strong event branch | vehicle/event state |
| `water_flag_24C[0..3]` | four unsigned categorical surface flags | raw surface context |
| `+0x264`, `+0x268` | signed floats transformed, capped and speed-scaled in selected branches | vehicle-dynamic modifiers, not surface waveform |
| signed word `+0xD34` | absolute value minus 2000, then small positive motor additions | native steering/handling cluster input |
| `+0xDD4` and event-work `event_data +0x1C4` | indexed comparison with the player-car speed-like scalar, converted through absolute value | event interaction state |
| `CalcVibrationValue_FrameCnt0/1` | persistence/decay counters | procedural controller-effect state |

The routine is composite. The table above does not make every branch a Road
source. Gear, collision/interaction and other vehicle states join the same two
motor accumulators after or alongside the surface-conditioned base.

## Surface path and coefficient table

`water_flag_24C[0..3]` are read directly at offsets `0x24C`, `0x250`, `0x254`
and `0x258`. Each is passed to `sub_1149C0(surface, context, flagsOut)`. The
largest returned coefficient becomes the surface response input.

| Raw surface value | Returned coefficient | Conditional behavior |
| ---: | ---: | --- |
| `1` | `0.00` | none |
| `2` | `0.25` normally | selected stages return about `.73/.79/.76` and set a flag |
| `4` | `0.70` | none |
| `8` | `0.85` | none |
| `0x10` | `0.90` | none |
| `0x80` | `0.85` | none |
| `0x100` | `0.45` | none |
| `0x200` | `0.35` | none |
| `0x400` | `0.30` | none |
| `0x800`, `0x1000`, `0x2000` | `0.35` | none |
| `0x8000` | `0.40` | none |
| `0x100000` | `0.71` | stable R4.2C cobblestone context |
| `0x200000` | `0.80` | none |
| `0x400000` | `.25` or `.90` | context/stage dependent |
| `0x800000` | `0.50` | none |
| other values | `0.31` | default |

These are amplitude/response coefficients. There is no frequency, phase,
waveform shape, signed direction or event probability in the table. Taking the
maximum makes broad and partial occupancy non-additive: another active corner
does not automatically increase the coefficient.

The output flag written by the value-`2` special stages alters later branches;
it does not carry temporal polarity.

## Continuous temporal source and speed scaling

After the four lookups, the maximum coefficient is multiplied by
`EVWORK_CAR +0x1C4`. The initial surface working level is then scaled by `0.1`.
Selected branches add other speed-scaled terms or rescale the working values.
The routine also uses `+0x1C4` for low-speed stop/persistence thresholds at
`.001` and `.01`, and compares it with another event object's `+0x1C4`.

The earliest supported continuous variation in the stable-surface path is
therefore the speed-like scalar, not an oscillating material phase. With a
constant `0x100000` surface, the coefficient remains `0.71`; changes in
`+0x1C4` change the motor envelope every frame. This is consistent with the
R4.2C stable interval's envelope-to-observed-speed correlation of effectively
`1.0` and fixed approximately `4:1` motor ratio.

The accepted stable interval supplies unusually strong numeric corroboration.
Least-squares replay against recorded vehicle speed gives:

```text
left motor  = 0.14910032 * speed - 0.00000012   r ~= 0.9999999999
right motor = 0.03727507 * speed - 0.00000003   r ~= 0.9999999999
```

The slopes are `0.71 * 0.21` and `0.71 * 0.0525`, preserving the observed
exact 4:1 relationship. The `.21/.0525` effective branch scales need not exist
as single literals; they are the net result of the active assembly path. The
near-zero intercepts and sub-`1.4e-6` maximum regression error leave no
measurable independent cobblestone waveform in that interval.

No time accumulator, travel-distance phase, random generator, periodic lookup
or signed surface sample appears in the mapped routine. The two frame counters
provide hold/decay behavior for the controller effect; they do not generate a
bipolar waveform.

## Per-wheel and per-corner state

The only per-contact surface inputs consumed directly are
`water_flag_24C[0..3]`. They update at the car-physics cadence and are reduced
through the coefficient maximum. `CalcVibrationValues()` does not dereference
the separate four-corner `PhysicsContext` blocks at executable offset
`0x42E7F0`.

The observed per-corner fields `+0x28`, `+0xAC`, `+0xB0`, `+0xE8`, `+0xEC` and
`+0xEE` therefore do not feed this controller surface path. They remain useful
native dynamics observations, but no writer-to-motor lineage exists in the
current evidence.

## E8, EC and EE findings

| Field | Storage | R2 behavior | Reachability from vibration path | Classification |
| --- | --- | --- | --- | --- |
| corner `+0xE8` | signed IEEE-754 float | usually `1.0`; changes to `.7` or mixed values in rough/re-entry context and may lag raw transitions | not read by `CalcVibrationValues()` or `sub_1149C0()` | surface/contact-context candidate; writer and physical meaning unresolved |
| corner `+0xEC` | signed 16-bit | signed paired values vary even on stable road and expand with load/drift | not read by the motor path | vehicle-dynamic/orientation correlation candidate, not Road direction |
| corner `+0xEE` | signed 16-bit | related to EC but with larger drift/recovery excursions and persistence after surface stability | not read by the motor path | vehicle-dynamic delta/correlation candidate, not Road direction |

The project reads these fields passively from the native four-corner context;
it does not write them. Their native writers are not present in the repository
and are not resolved by the available restored assembly. Storage signedness of
EC/EE does not establish physical force direction.

## Other candidate assessment

R4.2N adds no new memory offset. It sharpens the role of existing fields:

- `+0x1C4` is the high-value causal temporal input for stable surface motor
  amplitude. Its use supports speed-like semantics, not a Road waveform.
- the output of `sub_1149C0()` is the pre-motor material-response state. It is
  positive, categorical-to-amplitude and stateless except for stage/context
  selection.
- `+0x264/+0x268` can be signed in storage, but the routine thresholds,
  offsets, caps and speed-scales them into motor magnitude. Their R2 behavior
  correlates with vehicle dynamics rather than validated stable-surface
  texture.
- `+0xD34` participates in small positive motor additions after absolute-value
  processing. It belongs to the steering/handling cluster and cannot provide
  Road polarity.

None qualifies as a native physical Road carrier.

## Controlled-capture reconstruction

### Cobblestone

The stable tuple `(0x100000,0x100000,0x100000,0x100000)` produces four equal
`.71` lookup results. Their maximum stays `.71`. The speed-like multiplier and
fixed branch/scaling relationship create the continuously varying unsigned
motor levels. Constant surface identity, changing speed and a fixed motor ratio
match the accepted R4.2C evidence without requiring a hidden bump frequency.

Exact numeric replay is not claimed because `+0x1C4` and every branch flag were
not recorded explicitly, but the causal structure and observed proportionality
agree.

### Rough and sand

B04/B05 contain values such as `4`, `8` and `0x2000`, which select `.70`, `.85`
and `.35` coefficients. Mixed/partial occupancy selects the largest current
coefficient rather than summing corners. This explains persistent activity,
non-monotonic partial-versus-broad behavior and generalization beyond
cobblestone. Vehicle/event branches can still alter the final motor levels.

### Normal road

The controlled `(2,2,2,2)` state normally selects the lower `.25`
coefficient. The routine does not universally hard-zero value `2`; selected
stages can assign larger values. In CST01/CST03, the lower base plus the active
branch conditions did not produce sustained bilateral activity. Isolated
left-channel events remained possible, which is why Road policy additionally
requires non-reference surface context. Normal-road silence is therefore
contextual and empirical, not a universal material-ID-zero rule.

## Gear and collision join points

Gear-like state joins after the surface lookup through the `+0x208` versus
`+0x1D8` comparison and adds a bounded motor contribution. Collision/event
state joins through flags, the `+0x1DC/+0x1E0/+0x1E4/+0x2DC` branch, and the
indexed event object's absolute `+0x1C4` difference. Other flag branches add
`.2`, `.3`, `.5` or `.75`-class values and set hold counters.

These paths share final accumulators and clamps with surface response, which is
why final L/R motor values are composite. They do not change the finding that
the surface base itself is a positive coefficient times speed-like state.

## Signedness and physical direction

Some inputs are signed floats or integers. The surface IDs and coefficient
table are not. More importantly, every signed candidate relevant to this
routine is compared, absolute-valued, thresholded or converted into positive
motor additions. The output clamps are upper magnitude clamps (`.5` and `.75`)
followed by a non-negative strength multiplier.

There is no supported mapping from negative/positive storage to left/right
physical surface force. The routine is designed to drive unidirectional Xbox
rumble motors, not to command a bidirectional steering actuator.

## AER, haptics and motion

For future AER work, this establishes a precise PC comparison question: does
the arcade drive-board path receive a signed force/phase signal that bypasses
the Xbox material-coefficient-to-motor generator, or does the board synthesize
its own Road response from material and vehicle state? The PC path supplies no
evidence for either arcade design.

The result strengthens the restored motor envelope as a possible future bass
shaker source. It is already a game-authored unsigned tactile intent with
material and speed sensitivity. A dedicated haptics policy would still need
event separation, device safety and presentation design. Nothing is
implemented here.

These controller-effect signals must not be promoted to motion output. Motion
should use validated vehicle, contact, surface and event state through its own
policy.

## Probe decision and remaining unknowns

No telemetry change or physical UAT is justified for the primary R4.2N
question. Static lineage plus accepted captures explain the stable-surface
effect as procedural unsigned tactile amplitude. Recording the same final
motor levels again would not discover signed direction.

The unresolved writers and semantics of E8/EC/EE are separate native-dynamics
research. If a future static trace finds an earlier candidate outside this
controller-effect function, the minimum new probe should record that exact raw
field, its immediate material/contact gate, `+0x1C4`, the four surface IDs and
the existing motor outputs in a new explicitly versioned research schema. The
physical test would then be one constant-speed normal/surface/normal sequence.
No such probe is implemented or scheduled by R4.2N.

## Active Road gate

**ACTIVE ROAD REMAINS PAUSED.** R4.2N closes the hypothesis that the mapped
controller-effect path hides a signed surface waveform. Candidate A remains a
valid activity envelope and a strong tactile candidate, but steering-wheel
ConstantForce still lacks evidence-backed temporal polarity.

The next Road investigation should not apply more signal processing to the
unsigned motor envelope. It should either trace a separate force-oriented
native/arcade lineage or explicitly design and validate a new Road presentation
as HYP36R behavior, clearly distinguished from recovered native physics.

## Part II — Development Journey Recap

We traced the PC surface path to find out whether a signed Road force was being
lost before the restored Xbox motor output. Instead, the code showed positive
material coefficients, speed scaling, and other vehicle/event branches joining
an unsigned tactile presentation.

That answered an important question by ruling something out: the mapped path
does not hide a ready-to-use signed steering-wheel waveform. Candidate A still
describes activity and may be useful to a separately designed tactile policy,
but it does not authorize active Road torque.

THP32 material may help identify alternate places to investigate, but remains
**EXTERNAL REFERENCE — NOT INTEGRATED**. Any future result must be reproduced
against this project's supported binaries and evidence standards.
