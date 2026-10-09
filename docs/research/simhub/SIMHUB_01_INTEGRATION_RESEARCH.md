# SIMHUB-01 — External Simulation integration research

## Scope and research baseline

This document is a post-1.5 feasibility and architecture record. It does not
implement SimHub support. No UDP sender, registration file, plugin, memory
hook, setting, dependency, background service or hardware output belongs to
this milestone.

The original repository baseline reviewed was `9a60b76`
(`multi-device-input`). The research was preserved on
`feature/aer-arcade-profile` without changing runtime code. SimHub research was
performed on 2026-09-29 and rechecked on 2026-10-08 against the live official
[External Simulation manual](https://manual.simhubdash.com/external-sim-integration).
The manual says the facility is available from SimHub 9.11.5 and is currently
beta. It does not declare a maximum supported version. The future adapter must
therefore treat the generated contract for the target installed SimHub version
as authoritative and version its own definition.

No external OutRun FFB or telemetry implementation was used for addresses,
constants or design. The findings below come from this repository, its existing
research, current runtime ownership and the documented SimHub interface.

## Official External Simulation contract

External Simulation is a simple binary UDP integration described by a
`.simdef` definition. The definition declares a unique game identity, display
metadata and icon, process detection, default UDP port and the packet fields.
SimHub can generate matching C++ or C# structures and a minimal sending loop,
including the required packet header. The generated field comments are the
authority for units, directions and encodings; they must be retained when an
implementation is designed.

Registration can be installed directly under SimHub's local ExternalSims
definitions directory or indirectly through a one-line link file pointing to
the definition. The current manual names the registration concept `.simlink`,
while its production path uses
`%localappdata%\SimHub\ExternalSims\Registrations\{UniqueId}.shlink`. The link
contains the absolute path of the `.simdef`, its filename must match the
definition UUID, and registered definitions take priority over dropped-in
definitions with the same UUID. This terminology/path distinction must be
verified against the target installed SimHub build during receiver validation.
Process detection expects the executable name without `.exe`. An optional
extractor is supported by the format but is not needed for an in-process OutRun
sender.

Only fields that the game actually provides should be declared. Standard fields
drive SimHub's built-in feature availability. Custom fields are exposed as
properties but are not interpreted internally by SimHub. The exact generated
standard-field catalogue, types and comments for the target SimHub version must
be captured during SIMHUB-01 design review; the public manual page does not
publish that complete table.

The official recommendation is at least 60 updates per second for good fidelity.
Higher rates are accepted, although SimHub may ignore excess packets. The
Telemetry Receiver Tester is the intended first validation tool. The feature's
beta status means a future definition and packet adapter need explicit contract
versioning and compatibility testing.

Dashboard/DDU and LED availability should follow from correctly declared
standard properties such as speed, gear and RPM. Custom OutRun context can be
exposed for dashboards. Wind should remain a SimHub presentation of validated
vehicle speed. ShakeIt can use standard game properties, while whether a target
SimHub version can conveniently bind arbitrary custom External Simulation
properties in every ShakeIt editor requires product validation. The same is true
for motion: External Simulation is an appropriate boundary, but its generated
motion-field contract and OutRun's source semantics both need validation. No
fixed fan, LED, shaker or actuator presentation should live in OutRun.

## Intended architecture and ownership

```text
OutRun 2006
    |
Native game signals
    |
HYP36rforce telemetry foundation
    |
    +-- Reference+ FFB
    +-- Arcade Experience (AER)
    +-- Surface 2.0 research
    +-- Optional SimHub adapter
             |
           SimHub
             |
             +-- Bass shakers / ShakeIt
             +-- Wind simulation
             +-- Dashboards and external displays
             +-- LEDs
             +-- Other supported devices
```

HYP36rforce remains authoritative for the native game evidence it observes.
Reference+, Arcade Experience, Surface 2.0 and the future SimHub adapter are
independent consumers or interpreters of that evidence. SimHub must not consume
their generated wheel-force output as vehicle physics, and none of those force
modules may depend on SimHub. Normal gameplay, Multi-Input and steering-wheel
FFB remain fully functional when SimHub is absent or disabled.

## Classification rule

- **VERIFIED**: a stable current runtime owner exists and its stated semantics
  are established. A verified raw property may still require a separate mapping
  before it can satisfy a standardized SimHub unit.
- **NEEDS VALIDATION**: a likely source exists but its scale, encoding, event
  identity or public meaning is not sufficiently proven.
- **MISSING**: no authoritative current source has been established.

These statuses describe the proposed public property, not merely whether a C++
member can be read.

## Signal matrix

| Signal | Current source | Authority | Units / range | Existing consumer | SimHub mapping candidate | Status | Missing work |
|---|---|---|---|---|---|---|---|
| Steering command | `InputManager_SteeringValue()`; ultimately `InputManager::GetVolume(ADChannel::Steering) / 127` | Calibrated Multi-Input command | normalized `[-1, 1]` | HYP36R Force and research telemetry | standard steering/input position | **VERIFIED** | Confirm the generated SimHub sign convention; adapt only at the boundary. |
| Throttle command | `InputManager::GetVolume(ADChannel::Acceleration) / 255` | Calibrated Multi-Input command | normalized `[0, 1]` after existing binding/calibration | native game input | standard throttle | **VERIFIED** | Add a non-invasive live-model reader; validate the generated SimHub type and combined-pedal configurations. |
| Brake command | `InputManager::GetVolume(ADChannel::Brake) / 255` | Calibrated Multi-Input command | normalized `[0, 1]` after existing binding/calibration | native game input | standard brake | **VERIFIED** | Same boundary/type and combined-pedal validation as throttle. |
| Vehicle motion magnitude | magnitude of `EVWORK_CAR::spd_mb_20` in `GamePlCar_Ctrl_Hook` | Observed native three-vector, magnitude derived by HYP36R | native units, non-negative; physical unit not established | Force speed authority, research telemetry | source for standard speed after conversion | **NEEDS VALIDATION** | Establish metres/second or km/h conversion against timed distance/speedometer evidence; validate reverse handling. |
| Normalized speed | `clamp(length(spd_mb_20) / 1.5, 0, 1)` | HYP36R-derived control quantity | `[0, 1]` | Force/Surface frequency policy | custom diagnostic only, not vehicle speed | **VERIFIED** as an internal derivation | Must not be exported as standardized physical speed. |
| Current gear raw | `EVWORK_CAR::cur_gear_208` | Native current drivetrain field | `uint32`; encoding not yet established | signal-state observer, telemetry, shift detection | standard gear after mapping | **NEEDS VALIDATION** | Prove reverse, neutral, 1-N, automatic/manual and displayed-vs-drivetrain behavior. |
| Previous gear raw | `EVWORK_CAR::dword1D8` | Native prior/companion gear state | `uint32`; encoding not yet established | transition test against `cur_gear_208` | custom shift-state diagnostic if useful | **NEEDS VALIDATION** | Prove exact update order and meaning across shifts. |
| Gear transition | `cur_gear_208 != dword1D8` | Derived from two native fields | boolean | HYP36R event classification and telemetry | custom event, not primary gear | **VERIFIED** for the current shift detector | Validate duration/cadence before promising an external pulse. |
| Engine RPM | none established | none | unknown | none | standard engine RPM | **MISSING** | Trace tachometer/engine/audio lineage and correlate against vehicle/gear/speed. |
| Maximum RPM / redline | none established | none | unknown, potentially per car | none | standard max RPM/redline | **MISSING** | Find native per-car limit or validate a table from original game data; do not infer it from LEDs/audio. |
| Vehicle running/driving | `Game::is_in_game()` over `current_mode`, plus non-null player car | Existing authoritative project gameplay predicate | boolean | Force, overlays, graphics and telemetry | part of `GameRunning` policy | **VERIFIED** for current in-game ownership | Split active driving from pause, goal/time-up/try-again and start transition for SimHub semantics. |
| Detailed lifecycle state | `Game::current_mode` (`GameState`) | Native state machine | enum `0x00..0x24` currently declared | numerous runtime gates/debug | session phase / paused / finished | **VERIFIED** raw state | Validate policy table for loading, attract, pause, goal, result and completion. |
| Game mode identity | `Game::game_mode` | Native global with only partial local interpretation | integer; repository only establishes multiplayer checks for values 3/4 | notifications/debug | game mode name/type | **NEEDS VALIDATION** | Map OutRun, Time Attack, Heart Attack, multiplayer and variants with controlled observations. |
| Car ID | `Game::pl_car()->car_kind_11` | Native player-car selection | verified IDs `0..14` | overlay, telemetry metadata | custom `car_id` / vehicle identity | **VERIFIED** | Preserve numeric ID and safe unknown handling. |
| Car name | centralized `CarIdentity::friendly_name/display_name` | Physically validated mapping of `car_kind_11` | string | player telemetry overlay/reporting | vehicle name / custom `car_name` | **VERIFIED** | Reuse resolver; never duplicate the table. |
| Stage ID | `*Game::stg_stage_num` | Native current-stage global | `GameStage`; known table indices `0..0x41` | graphics, course/editor debug, telemetry metadata | track/stage identifier | **VERIFIED** | Add safe unknown behavior; current friendly resolver falls back to Palm Beach outside the known range. |
| Stage name | `Game::GetStageFriendlyName(GameStage)` / `StageNames` | Repository mapping over native stage ID | string | overlay and telemetry metadata | track name / custom `stage_name` | **VERIFIED** for known IDs | Verify naming stability for every special/reverse/night stage and avoid false Palm Beach fallback. |
| Route context | current stage plus `EVWORK_CAR::OnRoadPlace_5C.curStageIdx_C`, road section and branch state candidates | Native candidates, no public semantic contract | unknown | course/editor and research context | custom route/checkpoint context | **NEEDS VALIDATION** | Establish branch/route identity and transition semantics before PB keys use it. |
| Capture elapsed time | `TelemetryProbe` steady-clock elapsed time | Recorder-owned wall time | seconds | research CSV/session report | none as game timing | **VERIFIED** as capture time | Must not be represented as lap/stage/race time. |
| Time Attack current/last/best/lap/sector | no established live sources | none | unknown | none | standard lap/timing fields | **MISSING** | Trace native timing display/state and validate reset, pause and completion behavior. |
| OutRun stage/run timing | `stage_info_timer` exists but only counts stage-info display calls; it is not a race timer | unsuitable candidate | display-call ticks, not elapsed stage time | UI behavior | future custom stage time/delta | **MISSING** | Find authoritative run/stage clock and transition/completion events. |
| Raw surface identity | `EVWORK_CAR::water_flag_24C[4]`, mirrored/validated against four-corner `field14` | Native per-corner material/state identity | four raw `uint32` values; material meanings only partially researched | HYP36R signal state and research CSV | optional custom per-corner context | **NEEDS VALIDATION** | Finish semantic map and stability rules; do not expose guessed material names. |
| Road activity | `HYP36RSignalState::roadIntent.continuousActivityEvidence`, native right motor, and Road2 policy/presentation | Derived from restored native effect plus native surface context | current internal normalized-like magnitude; public normalization not frozen | Road 2.0 and research telemetry | custom `road_activity` | **NEEDS VALIDATION** | Choose pre-presentation physical evidence, define `[0,1]`, prove cross-car/surface comparability. |
| Surface activity | shared `HYP36RRoad2Active::resolve_surface_source(generatedSurface, roadDetailScale)` | HYP36R-derived physical-intent source before Texture waveform/ceiling | finite scalar, not yet a frozen public normalized contract | Texture and Bump share it; telemetry `surface_source` | custom `surface_activity` | **NEEDS VALIDATION** | Define calibration-independent normalized semantics; separate player intent from physical activity. |
| Texture output | `surface_requested_magnitude`, `surface_bounded_magnitude`, frequency/waveform | Rendered DirectInput request | normalized DirectInput request with player scaling/ceiling | periodic wheel effect | diagnostic only | **VERIFIED** as wheel rendering | Do not use as primary external surface telemetry. |
| Surface Bump | `HYP36RSurfaceRenderer::BumpDetector`: delta of shared Surface source, threshold, bounded magnitude and trigger | HYP36R-derived transient event | event boolean plus internal magnitude | DirectInput bump pulse and research telemetry | custom `surface_bump_event`, `surface_bump_intensity` | **NEEDS VALIDATION** | Validate event identity and normalize an input-side magnitude independent of wheel strength/pulse cap. Export event and magnitude, not only one. |
| Native effect channels | restored `VibrationLeftMotor` / `VibrationRightMotor` from `CalcVibrationValues(car)` | Game-derived composite Xbox motor requests | observed floats, normally treated as `[0,1]` | HYP36R Road/event evidence and telemetry | upstream evidence, possibly custom diagnostics | **VERIFIED** as native composite effect requests | They collapse several causes and are not direct physical measurements. |
| Impact candidate | bilateral native-effect rise, no gear transition, current `EventClass::CollisionCandidate` | Derived classifier from game-native composite effect | event class plus rise/evidence magnitudes | signal-state observer | custom collision event/intensity | **NEEDS VALIDATION** | Broader collision UAT; reject gear/surface transitions; define stable magnitude and pulse lifetime. |
| Rendered wheel Impact | signed decaying `impact` / post-gain Impact contribution | Synthetic DirectInput presentation | signed normalized force contribution | HYP36R composer | diagnostic only | **VERIFIED** as wheel rendering | Never make this the primary physical Impact property. |
| Synthetic lateral speed | dot of `spd_mb_20` with `matrix_70` first basis row | HYP36R-derived kinematic proxy | native speed units | Force research and telemetry | precursor only | **NEEDS VALIDATION** | Validate basis orientation, sign and physical scale. |
| Synthetic slip ratio | `abs(lateralSpeed) / speed`, clamped `[0,1]` | HYP36R-derived proxy | `[0,1]` | HYP36R Force/research | possible custom `slip` | **NEEDS VALIDATION** | Prove relationship to tire/vehicle slip across low speed, drift, collision and airborne cases. |
| Grip state/loss | thresholded slip proxy, BITE and contextual interpretations | HYP36R semantic model, not native tire grip | internal enum/scalars | Force 2.0 research and telemetry | possible custom grip/recovery properties | **NEEDS VALIDATION** | Define public physics meaning; avoid exporting tuning coefficients as grip. |
| Yaw-like response | `HYP36RVehicleState::responseAngleRad` and rate from D44/D46 lineage | Native steering/handling-response state, semantic scope limited | radians and radians/s | HYP36R vehicle-state research | research candidate, not yet standard yaw | **NEEDS VALIDATION** | Establish relation to world/body yaw and sign before motion use. |
| World position/orientation | `position_14`, `matrix_70` exist | Native transform candidates | game units / matrix, coordinate system unvalidated | graphics and kinematic research | position/orientation inputs | **NEEDS VALIDATION** | Establish axes, units, handedness, interpolation and teleport/reset behavior. |
| Surge / sway / heave | no validated accelerations | none | unknown | none | standard motion acceleration channels | **MISSING** | Derive only after position/velocity axes and timebase are validated; filter/reset rules required. |
| Pitch / roll / yaw for motion | matrices and anonymous four-corner candidates exist, but no validated public motion semantics | candidates only | unknown | research only | standard motion attitude channels | **MISSING** | Validate Euler extraction, axes, units and discontinuities independently of FFB. |

## Core findings

### Speed

The current source is the magnitude of native `EVWORK_CAR::spd_mb_20` sampled
inside the existing player-car control path. It is authoritative enough to say
the car is moving and to drive existing relative policies, but its physical
unit is not established. `normalizedSpeed = clamp(speed / 1.5, 0, 1)` is a
HYP36R Force policy, not a km/h conversion. SIMHUB-01 cannot honestly publish a
standard speed until a controlled distance/time or known speedometer comparison
establishes scale, direction and reverse behavior.

Once converted to a validated physical unit, speed is sufficient for SimHub to
own wind/fan presentation. OutRun should not calculate fan curves.

### Inputs

Steering is already stable and normalized through Multi-Input. Acceleration and
brake use the same calibrated input owner: non-steering analogue channels are
returned on `0..255`, so dividing by 255 yields player command values in
`[0,1]`. These represent requested controls, not engine torque, brake pressure
or wheel telemetry. That is appropriate for standard telemetry input channels.
No new game-memory hook is required.

### Gear

`cur_gear_208` is a high-confidence native current-gear source and
`dword1D8` is already compared with it to detect transitions. The raw encoding
has not been documented for reverse, neutral or forward gears, nor compared
against the displayed gear in both automatic and manual modes. The field is
therefore not yet a valid public SimHub gear contract.

Future validation should log the two raw fields while deliberately selecting
reverse/neutral where possible, launching in first, shifting every forward gear
in manual mode, repeating in automatic mode and recording the on-screen gear.
It should establish whether the transition comparison spans one or several
ticks and whether the drivetrain value leads or trails the display.

### RPM and maximum RPM

No authoritative engine RPM, tachometer, engine-speed or rev-limit source is
present in current runtime ownership or the accepted research model. No per-car
maximum RPM/redline source is established. Audio pitch or a display animation
must not be promoted to engine RPM without lineage and correlation. RPM and
MaxRPM are **MISSING**, so shift LEDs and RPM-driven ShakeIt cannot be part of
the smallest honest first implementation.

The future research path is: trace the native tachometer's read source; compare
it with any engine-audio parameter; observe idle, held gears, shifts, limiter
and coast-down; establish units; then find whether the limit is native per-car
data or a stable game-wide value. No constants should be guessed from Ferrari
specifications.

### Car and stage context

Car identity is ready. The numeric authority is `car_kind_11`; all friendly
presentation must use the single `CarIdentity` resolver and the physically
verified mapping in [`docs/VEHICLE_IDENTIFICATION.md`](../../VEHICLE_IDENTIFICATION.md). Both ID and
name belong in the future model.

Stage identity is also available from `stg_stage_num` and the existing central
`StageNames` table. Branching changes the native stage ID as the game enters the
selected stage, so it is useful context, but a route/PB key requires validation
of transition timing and all reverse/night/special values. The current friendly
resolver returns Palm Beach for an unknown ID; a future stable model must retain
the raw ID and report an explicit unknown name instead.

### Game and session state

`current_mode` is the authoritative native lifecycle state. `Game::is_in_game()`
is the project's established broad gameplay predicate and currently includes
normal play, goal, time-up, try-again, OutRun Miles, pause, and one late start
state. That broad predicate is appropriate for safe runtime ownership but too
coarse to copy directly into one SimHub boolean.

The future state mapper should read one snapshot of `current_mode`,
`game_start_progress_code`, player-car availability and the validated game-mode
identity, then derive at least `GameDetected`, `SessionActive`, `Driving`,
`Paused`, `Finished` and `InMenu`. `GameRunning` should mean the executable and
integration are alive; a separate driving/session property should prevent
dashboards or hardware effects from interpreting pause/goal as active driving.
Attract/demo and loading need controlled validation. `game_mode` exists, but
its single-player values have not been mapped; only the repository's values 3/4
multiplayer test is currently documented.

## Timing and future stage PBs

The repository currently owns capture wall time, not authoritative game race
time. `TelemetryProbe::elapsedTime` is a steady-clock duration of a research
capture. `stage_info_timer` counts calls associated with the next-stage sign and
is explicitly not a stage stopwatch. No live current/last/best lap, lap number,
sector, Time Attack clock, stage clock or total OutRun run clock is established.

Time Attack timing and OutRun timing must remain separate semantic models.
OutRun stages should not be invented as conventional laps. A future stage PB
service requires:

1. authoritative session clock with pause/time-up behavior;
2. exact stage-entry and stage-completion events;
3. stable stage/route identity through branches;
4. run invalidation/restart/continue rules;
5. optional stable car ID in the key;
6. persisted schema/version, atomic writes and migration policy;
7. current, previous, best and delta calculations based on the same clock.

None of those persistence or UI pieces belongs in the SimHub adapter. Timing
research should first expose trustworthy values in the live model; an optional
PB service can then consume them and publish results back to that model.

## HYP36R physical telemetry boundary

The preferred external property is the car/game evidence before wheel-specific
presentation. Current wheel outputs are useful diagnostics, not primary physics.

| Concept | Preferred future source | Do not use as the primary property |
|---|---|---|
| Road activity | validated pre-presentation Road intent/native activity | calibrated/slew-limited directional Road force |
| Surface activity | a validated, calibration-independent form of the shared Surface source | Texture waveform, frequency, ceiling or periodic magnitude |
| Bump | input-side transient event plus normalized intensity | DirectInput pulse magnitude/duration |
| Impact | validated native-effect event classification and source magnitude | signed/decaying wheel Impact contribution |
| Slip | validated kinematic/native slip state | grip-loss tuning coefficient |
| Grip/recovery | separately validated public semantic state | BITE/Force internal thresholds or unloading amount |

The shared Surface source is structurally the right ownership point because
Texture and Bump consume it before their distinct DirectInput presentations.
It is not yet a frozen `[0,1]` external property: it includes HYP36R generation,
Road Detail scale and shipping calibration. SIMHUB-03 must define a stable
physical/intensity source that does not change merely because a player changes
wheel strength or waveform.

Bump should be represented as both an event and a magnitude. An event supports
one-shot dashboard/tactile behavior; magnitude preserves severity. Both must be
created from the validated source transient, not from whether a wheel pulse was
successfully played.

The Impact classifier already separates known gear transitions and calls a
bilateral rise a collision candidate. Controlled research supports that as a
promising source, not a universal collision truth. It needs broader validation
before tactile publication. Native left/right motor values are genuine restored
game effect requests, but they are composite outputs that already collapse
surface, collision, gear and other state.

The current `slipRatio` is deliberately synthetic: lateral projection divided
by motion magnitude. It is useful Force research but is not validated tire slip.
Grip/BITE values are HYP36R interpretations. Both remain **NEEDS VALIDATION**.

## ShakeIt, wind, LEDs and motion feasibility

- **DDU/dashboard:** feasible through standard core fields and custom car,
  stage, mode and event properties. Standard/custom names and types must be
  locked in a versioned definition.
- **ShakeIt/bass shakers:** feasible in principle. Standard speed/RPM inputs and
  validated custom physical events are the right feed. SimHub must own effect
  frequencies, mixing, gain and hardware. Custom-property usability in the
  target ShakeIt editors must be verified with the Receiver Tester and a small
  dashboard/property inspection before promising first-class presets.
- **Wind:** feasible after physical speed conversion is validated. SimHub owns
  fan curves, minimum duty, smoothing and controller output.
- **LEDs/shift lights:** blocked on authoritative RPM and MaxRPM. Gear alone is
  not an honest substitute.
- **Motion:** not ready. Native position/matrices and steering-response/four-
  corner candidates exist, but validated surge, sway, heave, yaw, pitch and roll
  do not. Motion research needs axes, handedness, units, timebase, reset/teleport
  handling and filtering. No motion value should be derived from FFB output.

## Initial tactile hardware research

The proposed first hardware arrangement is deliberately external to the wheel
FFB path:

```text
SimHub ShakeIt
    |
Dedicated USB stereo audio adapter
    |
Fosi Audio TB10D candidate amplifier
    |
    +-- Left channel: rear-left Buttkicker Mini LFE
    +-- Right channel: rear-right Buttkicker Mini LFE
```

The available transducers are two Buttkicker Mini LFE units, positioned rear
left and rear right. The Fosi Audio TB10D and a dedicated USB stereo adapter are
candidates, not validated project requirements. Amplifier power and impedance
compatibility, channel wiring, cooling, gain structure, mounting, mechanical
limits and safe physical operation must be established from the manufacturers'
specifications before powered testing.

The smallest proposed effects are shared left/right Road Activity, Impact and
Gear Change outputs. SimHub should own their waveform, frequency, mixing and
gain. Directional road contact, asymmetric surface activity and directional
collision response remain future work and must not be fabricated from visual
placement or from non-directional event evidence.

## Surface research priorities

Visual context identifies repeatable places to investigate; it does not prove
a native material identity or a particular tactile/FFB output.

| Priority | Location | Current observation / question |
|---|---|---|
| High | Imperial Avenue | Sustained paver-style roadway; test whether validated activity is sustained. |
| High | Tulip Garden | Localized bridge/suspected cobblestone section with preserved collision-classification evidence. |
| High | National Park | Distinctive bridge roadway; compare approach, crossing and exit. |
| High | Sunny Beach | Striped runoff and sand/road-edge transitions. |
| High | Snow Mountain | Snow- and ice-looking sections; appearance is not a material proof. |
| High | Desert stage, exact identity pending | Dirt/loose-surface-looking roadway; establish stage and native evidence first. |
| Secondary | Alpine | Mountain-road classification variation. |

An icy or slippery-feeling sensation has repeatedly retriggered during
continuous-stage play but was not observed in Time Attack. This is an
unverified mode-dependent observation. A controlled comparison must separate
game state, stage transitions, native classification, interpreter persistence,
renderer activation and actual output before assigning a cause.

## Proposed stable live model

The suggested abstraction is appropriate if it distinguishes raw authority,
validity and derived presentation. A compact future model should be an immutable
snapshot produced once per game tick:

```text
OutRunLiveTelemetrySnapshot
  sequence, monotonicTime, validity

  vehicle
    speedMps, speedValid
    gear, gearStateValid
    rpm, maxRpm, rpmValid
    throttle01, brake01, steeringSigned01

  context
    carId, carName
    stageId, stageName
    gameMode
    lifecycleState
    gameDetected, sessionActive, driving, paused, finished

  timing
    discipline                 # none / time_attack / outrun_stage
    currentTime, lastTime, bestTime
    stageTime, stageBest, stageDelta
    validity

  physical
    roadActivity01
    surfaceActivity01
    surfaceBump { eventId, active, intensity01 }
    impact { eventId, class, active, intensity01 }
    slip01, gripState
    per-field validity/confidence
```

Unknown fields remain invalid; they are not zero-filled as if observed. Discrete
events need monotonically increasing IDs so a UDP consumer can distinguish a
new event from repeated samples. Strings should come from central resolvers.
Standard physical units should be used internally where known, leaving final
SimHub encoding to the adapter.

This model must not be an alias for the 254-column research CSV. The research
recorder can continue adding experimental columns. Both it and SimHub should
consume authoritative state independently:

```text
authoritative game/input/HYP36R evidence
                 |
       live telemetry snapshot
          /                \
evolving research CSV   versioned SimHub adapter
```

## Adapter and dependency boundary

The future `SimHubExternalSimulationAdapter` should depend on the stable live
snapshot and own only:

- generated SimHub packet/schema translation;
- `.simdef` identity, UUID, icon, process and default-port metadata;
- registration/install guidance;
- UDP socket lifecycle and packet sequence/header;
- standard and custom property naming;
- target-SimHub compatibility/version checks and diagnostics.

The dependency direction is one way:

```text
Game + Multi-Input + HYP36R observers
                 -> OutRunLiveTelemetry
                 -> SimHub adapter -> UDP
```

HYP36R Force, Multi-Input and game-state instrumentation must not include,
configure or call SimHub. The adapter must not read game memory directly or
consume the research CSV. This keeps SimHub optional and prevents a transport
failure from affecting Force or input.

## Performance and lifecycle design

- Produce one coherent snapshot at the existing nominal game tick. Target a
  60 Hz UDP stream, matching the official minimum and current observed control
  cadence. Do not send multiple inconsistent field reads per tick.
- A compact fixed-layout packet of core scalars plus bounded custom properties
  should be far smaller than the research row and inexpensive. Packet size must
  be checked against the generated definition and kept below fragmentation-
  prone UDP payloads.
- Snapshot construction can remain on the owning game/control path if it only
  copies already-owned values. Socket creation, error logging and transmission
  should be isolated and non-blocking. If measurement shows any game-thread
  jitter, hand immutable latest-snapshot copies to a bounded sender; never queue
  an unbounded backlog.
- Start the adapter only when explicitly enabled in a future milestone. Create
  and close its socket independently of DirectInput/FFB. Reset validity and
  event IDs on session transition. Send an explicit non-driving/session state
  if the generated contract supports it, then stop cleanly on shutdown.
- Rate-limit diagnostics. No receiver acknowledgement or retry loop is needed
  for ordinary UDP telemetry; a stale packet is less harmful than blocking the
  game.

## Failure and optionality model

The default future state should be disabled unless the product decision changes
after validation. When enabled:

- SimHub absent or closed: UDP sends may be discarded; gameplay is unchanged.
- Port unavailable/socket creation failure: disable only the adapter for that
  session, log once with actionable context, and never retry in a tight loop.
- Consumer disappears: continue best-effort sends or suspend through a bounded
  policy; no effect on input, Force or game state.
- Integration disabled: create no socket, registration side effect or worker.
- Invalid field/source: mark it unavailable or omit it according to the generated
  contract; do not manufacture zero physics.
- Game/menu/loading transitions: publish validated lifecycle state and suppress
  stale driving/events.
- Game exits unexpectedly: OS resource cleanup is sufficient; no save/gameplay
  data is owned by the adapter.

SimHub availability must never gate or mutate OutRun, Multi-Input, HYP36R Force,
DirectInput, research recording or save behavior.

## Development roadmap

### Prerequisite research gates

Before implementation, close the physical speed conversion, raw gear encoding,
single-player game-mode map and lifecycle policy. RPM can remain a later gate if
the first implementation honestly omits it.

### SIMHUB-01A — telemetry inventory and integration research

- preserve the evidence/confidence matrix in this document;
- close physical speed conversion, raw gear encoding, single-player game-mode
  mapping and lifecycle policy;
- keep RPM, timing, motion and physical events unavailable until proven.

**Status:** research foundation complete; validation gates remain open.

### SIMHUB-01B — versioned telemetry contract

- create the stable snapshot and validity rules;
- validate speed units and gear encoding;
- include speed, gear, throttle, brake and steering;
- include car ID/name, stage ID/name and validated lifecycle state;
- generate and freeze the target-version `.simdef` contract.

### SIMHUB-01C — synthetic UDP transmitter

- implement a standalone/synthetic producer against the generated contract;
- prove packet identity, ordering, rate handling and clean lifecycle without
  reading game memory or changing OutRun.

### SIMHUB-01D — SimHub receiver validation

- validate the synthetic feed with Telemetry Receiver Tester;
- verify standard/custom property exposure, `.simdef` registration and the
  `.simlink`/`.shlink` terminology on the target SimHub version.

### SIMHUB-01E — live OutRun telemetry integration

- add the disabled-by-default, nonblocking adapter over the stable live snapshot;
- preserve complete separation from DirectInput, Reference+, AER and Surface;
- validate gameplay/menu transitions and shutdown without hardware output.

### SIMHUB-01F — dual-channel bass-shaker validation

- verify amplifier/transducer electrical and mechanical safety;
- validate shared Road Activity, Impact and Gear Change presentation through the
  rear-left/rear-right arrangement before considering directional effects.

### SIMHUB-02 — directional surface and impact research

- freeze calibration-independent Road/Surface semantics;
- validate directional/asymmetric evidence before exposing left/right effects;
- validate Bump and Impact event IDs, magnitude contracts and lifetimes.

### SIMHUB-03 — wind, dashboards and LEDs

- let SimHub present validated physical speed as wind;
- provide dashboard/external-display examples over verified fields;
- add LEDs only after authoritative RPM and MaxRPM are established.

This split improves the provisional plan by not blocking useful speed/input/
context telemetry on missing RPM and by separating contract, transport,
receiver, live integration and physical hardware validation.

## Smallest useful future SIMHUB-01

The smallest honest implementation after 1.5 is **not** the full provisional
list. It is:

- validated physical vehicle speed;
- validated current gear mapping;
- normalized throttle, brake and steering;
- car ID/name;
- stage ID/name;
- executable/session/driving/paused lifecycle state;
- optional, isolated 60 Hz External Simulation UDP transport.

RPM, MaxRPM, shift LEDs, game timing, PBs, HYP36R events and motion should be
omitted until their independent gates close. This core is already useful for a
basic DDU and wind while preserving a truthful contract.

## Open validation work

1. Convert `spd_mb_20` to physical units and establish reverse semantics.
2. Map every raw gear state in automatic and manual play.
3. Trace authoritative RPM and redline sources.
4. Map all single-player `game_mode` values and lifecycle transitions.
5. Find native Time Attack and OutRun clocks plus completion events.
6. Validate route identity and special-stage transitions for PB keys.
7. Freeze calibration-independent normalized Road/Surface properties.
8. Validate Bump and Impact event identity/intensity across cars and contexts.
9. Validate lateral axes/slip before exposing it publicly.
10. Validate world axes, units and attitude/acceleration sources before motion.
11. Generate the target SimHub version's actual field catalogue and test custom
    property visibility in dashboards and ShakeIt.
12. Resolve the official documentation's `.simlink`/`.shlink` registration-name
    discrepancy on a clean target installation.

## Decision

SimHub External Simulation is a strong architectural fit: it provides a small,
documented UDP contract without coupling OutRun to a plugin API. The repository
already owns enough trustworthy context and normalized player inputs to justify
a future core integration, but speed units and gear encoding must be closed
first. RPM, timing, physical events and motion must not be claimed ready.

Version 1.5 remains unchanged. No SimHub integration or dormant implementation
was added by this research milestone.
