# HYP36R Force Research II — R0 research map

## Boundary and baseline

R0 starts from released Multi Input v1.0.0, commit
`8d67c16792a386d5c3a03d42f2316115bb516abc`, on `multi-device-input`.
The release is immutable. This milestone changes no runtime code, telemetry,
Force equation, setting, device behavior, or output. It only maps what is
already established and defines the next observation boundary.

External material is kept in a separate evidence lane:

```text
external claim -> independent HYP36R investigation ->
confirmed / partially corroborated / not corroborated / unresolved
```

Nothing in this document imports third-party code, offsets, equations,
thresholds, force models, or signal-combination strategies.

## External evidence register

| Evidence | Claim or observation | Status in R0 | Use |
| --- | --- | --- | --- |
| @GATS field report, v1.0.0, Simucube 3 Pro | Valid Reference+ setup felt weak or absent around edges, sand, rough/off-track transitions, especially with only part of the car crossing a boundary; gear shifts could feel strong | Field observation, not a defect finding | Defines surface and shift scenarios; does not justify tuning |
| @GATS log and screenshots | Correct build and defaults; selected Simucube endpoint completed CREATE, PARAMETERS and START and persisted | Reported runtime evidence | Rules out basic device selection as the first explanation for the reported feel |
| thp32 technical note | Four spatially independent surface/material states and multiple per-wheel physics quantities exist; `EVWORK_CAR + 0xDBC` belongs to catch-up/handicap behavior | External research hypotheses | Compared only with our existing structures, XREF history and captures |
| Lambada / S0L lead | Japanese PS2 OutRun2SP has official Logitech wheel FFB; OutRun Online Arcade added wheel FFB and consulted arcade code | Historical lead with mixed provenance | Defines provenance work; does not establish shared force semantics |

Useful external follow-up material would be address-independent XREF traces,
annotated runtime logs, exact executable identities, and primary provenance
records. A read-only helper is useful only if its output format and source
identity can be audited. Source code, offset tables and ready-made force logic
are not required for R1.

## Track 1 — surface signals

### Field-feedback interpretation

The Simucube report is compatible with the current design, but it does not by
itself distinguish among three possibilities:

1. OutRun did not raise the restored vibration signal for that transition.
2. A signal exists, but the current composite vibration path loses spatial or
   material contrast before HYP36R receives it.
3. Reference+ intentionally lightened directional load during partial support
   loss, while the separate Road cue remained subtle on that hardware setup.

The endpoint log makes a dead FFB interface unlikely for that session. The
wheel-side 5 Nm limit, Feel/Detail/Aggressiveness settings and nonzero damping
remain presentation context, not proof of a software defect. Road Texture
Boost being zero may affect perception, but R0 does not prescribe a driver
setting.

### Existing independent knowledge

The supported PC runtime already supplies four untouched values at
`EVWORK_CAR::water_flag_24C[0..3]`. They are recorded as `surface_0..surface_3`
on every telemetry row. Controlled M5 work established the ordering used by
the current interpreter as FL, FR, RL and RR. The interpreter can therefore
detect front, rear, left, right and whole-car equality/asymmetry without
collapsing the four values first.

This establishes **spatially independent per-corner classification state**.
It does not yet establish the material represented by any numeric value.
“Water” is an inherited member name, not a complete semantic definition.

The separately validated four-corner native block also contains, per corner:

- `surfaceFlags_14` at block `+0x14`;
- `displacementCandidate_28` at `+0x28`;
- `directionalCandidate_AC` at `+0xAC`;
- `directionalCandidate_B0` at `+0xB0`;
- `surfaceResponseCandidate_E8` at `+0xE8`;
- two signed orientation candidates at `+0xEC/+0xEE`.

Only `+0x28`, `+0xAC` and `+0xB0` pass the current pointer-topology and finite
checks and enter telemetry. `+0x14`, `+0xE8`, `+0xEC` and `+0xEE` are available
in the known block layout but are not captured or semantically validated.

### Surface hypothesis decision

| External hypothesis | Independent result | R0 classification |
| --- | --- | --- |
| Four wheel surface states remain independent | Four raw classifications are captured separately and known-left testing established corner ordering | **Confirmed independently** for spatial classification |
| Each value is a material identity | Values distinguish equality and transition, but no value-to-material table or writer lineage is established | **Partially corroborated** |
| Partial surface contact can be preserved | Current raw telemetry preserves each corner; the restored vibration routine then reduces inputs into two motor levels and HYP36R Road uses only the right level | **Confirmed independently** that early collapse can lose context |
| Striped runoff, curb, sand and rough terrain share one signal | No such equivalence has been demonstrated | **Unresolved** |

The existing `fc_surface_asymmetry` is a derived boolean. It is useful as a
gate but is not a surface effect and cannot recover which corner or material
changed. M5 deliberately treats asymmetry as contamination and does not create
surface force from it.

## Track 2 — native vehicle and tyre physics

### What is established

The validated `PhysicsContext` has four inline `0xF4` blocks and four pointers
that must point exactly to them. `NativeFourCorner::observe()` fails closed if
that topology is wrong or if `+0x28/+0xAC/+0xB0` is non-finite. Existing
captures establish:

- four independent corner channels;
- front pair 0/1 and rear pair 2/3;
- left/right ordering through the known-left controlled study;
- `+0x28` as displacement/chassis-response-like context;
- `+0xAC` and `+0xB0` as two signed directional response channels;
- repeatable front/rear and left/right relationships useful for context.

The project names remain candidates. Physical units and physical identities
are not proven. The M5 normalization scales are research conditioning, not
vehicle constants. `combinedResponse = hypot(+0xAC, +0xB0)` is created by us;
it is not a native combined-slip value.

### External tyre-physics hypothesis status

| Claimed quantity | Our nearest independent evidence | Status |
| --- | --- | --- |
| Suspension/contact compression | Per-corner `+0x28` responds as displacement context, but writer semantics and units remain open | Partially corroborated |
| Compression rate | Can be derived offline from successive `+0x28` samples; no native rate field is established | Unresolved as a native signal |
| Normal/vertical load | No independently identified field | Unresolved |
| Reference load | No independently identified field | Unresolved |
| Tyre slip angle | No independently identified per-wheel field | Unresolved |
| Lateral tyre force | `+0xAC` has lateral-response-like correlations, not proven force or units | Partially corroborated |
| Longitudinal/local tyre force | `+0xB0` has longitudinal-response-like correlations, not proven force or units | Partially corroborated |
| Grip/force capacity | No independently identified field | Unresolved |
| Native combined-slip state | No independently identified field; our `hypot` must not be cited as native | Not corroborated by current evidence |

### Corroborating HYP36R states

Future synchronized observations can challenge, rather than replace, the
existing interpretation:

| HYP36R state | Independent comparison target |
| --- | --- |
| LOAD | Stable steering-response error/authority plus persistent front/rear corner-response magnitude on clean, uniform surface |
| RELEASE | Divergence/emerging phase versus changes in per-corner `+0xAC/+0xB0`, without assuming either is grip |
| FREE | Current derived slip/grip-loss state versus low or reorganizing native corner activity |
| BITE | Vehicle-led reconvergence and restored load versus renewed corner response and displacement settling |
| M5 permitted activity | Existing front/rear lateral balance during clean-surface RELEASE only |
| Grip loss/recovery | Current synthetic ratio versus native steering and corner timing; agreement is corroboration, disagreement is a research result |

No native SAT, tyre-force composer, or replacement model follows from these
comparisons.

### `EVWORK_CAR + 0xDBC` audit

The current structure declares `float actionforce_DBC`. That label was
inherited in upstream commit `021dc0c` alongside nearby fields already named
for communication-race rank and handicap state. Across all current HYP36R
source, telemetry, documentation and reachable Git history, `actionforce_DBC`
is never read outside its declaration. It is not captured and never influences
M4, M5, Reference+, Road, Impact, output conditioning or DirectInput.

The external claim that it is catch-up/handicap state is plausible given its
neighborhood, but R0 has no independent writer/XREF proof. The independent
classification is therefore **unknown / unresolved semantics, confirmed zero
production impact**. Our prior codebase did not interpret it as X-Force or as
a HYP36R input, so no production correction is required.

## Track 3 — Force Character ceiling evidence

### Current coverage

The S9 capture supplies reconstructed directional, Road, Impact, composer,
post-`tanh`, output exposure and final request data, but it was physically
recorded at Presence 1.20. F1.2 counterfactually replayed directional Presence
1.44. It found the first observed steering headroom warning near 1.40x in that
single context, no standalone Road boundary through 2.00, and increasing
Impact slew/dominance. Those are replay observations, not validated ceilings.

Earlier runs add known-left runoff, BITE, M5 and combined-corner activity but
predate the complete S9 presentation schema. Most evidence is Dino/Sunny Beach.
No player-facing ceiling is established.

### Native Reference+ 1.44 schema audit

| Required value | Current status |
| --- | --- |
| Raw directional contribution and M4/M5/BITE lineage | Already captured |
| Reference+ Presence/Contrast and selected directional request | Already captured |
| Raw Road and Impact inputs | Available in composer; presentation request records them at the current point, but explicit pre-character names are preferable |
| Post-channel directional, Road and Impact | Directional is explicit; Road/Impact are only reconstructable while gains and schema remain known | **Must be explicit** |
| Output ramp | Used by composer but not an explicit telemetry column | Missing |
| Pre-composition channel values | Partly reconstructable | Must be explicit for future ceiling work |
| Pre-`tanh` sum and post-`tanh` output | Already captured as `s2_composer_input` and `s2_post_tanh` |
| Post-ramp value passed to `drive()` | Captured as `ffb_raw` when the output observation aligns; should be explicitly named in the schema |
| Strength and inversion | Strength captured; inversion configuration not captured per row | Inversion missing |
| Unclamped DirectInput request, clamp state and final request | Clamp flag and final normalized request captured; unclamped request missing |
| Left/right restored vibration levels and rise | Available at runtime but not captured | Missing |
| Per-corner surface and native corner values | Already captured for current fields |
| Uncaptured known corner fields `+0x14/+0xE8/+0xEC/+0xEE` | Available but not captured; semantics require validation before use |
| Gear current/previous and shift transition | Available but not captured | Missing |

### Proposed unified telemetry schema

Keep the schema append-only and preserve every existing authoritative field.
Add neutral observations only:

```text
capture_build_commit
capture_product_version
capture_exe_sha256

vibration_left_raw
vibration_right_raw
vibration_combined_raw
vibration_rise
gear_current
gear_previous_native
gear_transition

road_pre_character
impact_pre_character
directional_post_character
road_post_character
impact_post_character
output_ramp
composer_pre_tanh
composer_post_tanh
force_pre_drive
ffb_invert_enabled
directinput_unclamped_request
directinput_clamp_active
directinput_final_request

corner0..3_surface_flags_14
corner0..3_surface_response_e8
corner0..3_orientation_ec
corner0..3_orientation_delta_ee
corner0..3_displacement_delta_derived
```

The new corner fields remain anonymous until writer/reader lineage and
controlled captures establish meaning. The displacement delta must be labeled
derived. Add scenario event markers for asphalt, stripe/runoff, curb,
shoulder/sand, re-entry, impact and shift so offline comparisons do not infer
events from output alone.

## Track 4 — PC effects and AER lineage

### Current Road and Impact pipeline

```text
PC vehicle state
    -> restored C2C Xbox CalcVibrationValues()
    -> VibrationLeftMotor / VibrationRightMotor

Road:
    right motor -> clamp 0..1 -> WheelFFBRoadStrength -> 0.25
    -> synthetic 6 Hz sine carrier
    -> suppressed when vibrationRise > 0.12
    -> Road Detail attenuation

Impact:
    max(left,right) -> positive rise detector (> 0.12)
    -> direction from steering, alternating at center
    -> bounded kick (max 0.55) -> 0.90 per-update decay
    -> Impact attenuation

directional + Road + Impact -> tanh -> output ramp -> drive()
    -> inversion -> Strength -> DirectInput nominal clamp -> request
```

The original PC executable has no established native DirectInput wheel-force
writer in this project. `CalcVibrationValues()` is upstream Tweaks' restoration
of the closely matching Xbox C2C routine, not proof of an original PC wheel
FFB path. It reads native PC car state and creates two gamepad-motor levels.
HYP36R then interprets those composite levels for Road and Impact. Vehicle
simulation, restored effect generation, and wheel presentation are separate.

### Gear-shift path

In the restored Xbox routine, `EVWORK_CAR::cur_gear_208` is compared with
`dword1D8`. A mismatch adds `0.2` to the left-motor accumulator before the
routine's final strength and caps and sets an event duration/priority value.
HYP36R computes `vibration = max(left,right)`. A sufficiently large rise enters
the Impact detector; Road uses the right motor and is suppressed on an impact
leading edge. Therefore the reported shift transient currently reaches the
wheel principally through **Impact**, sourced from a restored native shift
event rather than a dedicated HYP36R gear channel.

The exact final shift magnitude depends on the composite routine state,
VibrationStrength, rise threshold, Impact strength, channel attenuation,
composition and master Strength. It must be measured beside controlled mild
and strong collisions before any later tuning decision.

### AER evidence branches

```text
Original OutRun 2 / OutRun 2 SP arcade  [CONFIRMED hardware branch]
          |\
          | \ reported/unknown code lineage
          |  \
          |   JP PS2 OutRun2SP          [SUPPORTED wheel-FFB lead]
          |
          +--- PC C2C                    [CONFIRMED target; no proven native wheel FFB]
          |
          +--- OutRun Online Arcade      [SUPPORTED official wheel branch;
                                          REPORTED arcade-code consultation]
```

The arcade service documentation confirms a steering motor, drive-board
tests, centering control and selectable motor power; it does not document the
game's force equations or event semantics. Contemporary reporting says the
Japanese PS2 release supports GT Force/GT Force Pro, but R0 has not located
primary controller documentation or inspected an implementation. A published
Sumo interview states that OutRun Online Arcade added Xbox 360 Wheel force
feedback and that the team checked original arcade code while starting from
Coast 2 Coast. This supports a comparative branch; it does not prove that its
wheel model equals arcade behavior.

Reference records:

- [OutRun 2 arcade service-manual mirror](https://manualzz.com/doc/4143014/sega-outrun-2-arcade-game-installation-and-service-manual)
- [Japanese PS2 release wheel-support record](https://www.mobygames.com/game/23324/outrun-2006-coast-2-coast/trivia/)
- [Sumo OutRun Online Arcade interview mirror](https://www.mondoxbox.com/news/16314/outrun-online-arcade--intervista-a-sumo-digital.html)

### Original arcade evidence still required

- exact cabinet and drive-board variants;
- service/operator settings used during comparison;
- motor/servo controller commands synchronized with gameplay;
- steering input geometry, stops, center and rotation;
- repeatable recordings for straight, corner, drift, surface, impact and shift;
- code or debug-symbol evidence that identifies effect producers and consumers;
- provenance linking any home implementation to arcade source rather than
  resemblance alone.

The key AER question remains whether official paths consume detailed physics
state or generate a smaller set of event/effect commands. Physics data in an
executable does not prove that its FFB path used it.

## Future active-pedal and motion candidates

Preserve source signals before wheel composition. Candidate categories are
per-corner surface transitions, corner displacement/response, speed, native
gear transition, collision/event state, engine/throttle candidates, native
steering response, derived LOAD/RELEASE/FREE/BITE and output-independent
grip/recovery timing. No pedal or motion protocol belongs in R0. Future outputs
must not reverse-engineer their inputs from final wheel torque.

## Unified future capture campaign

### Smallest useful car/course matrix

Use three matched sessions, not a full combinatorial campaign:

1. **Dino / Sunny Beach** — continuity with existing evidence and the known
   left-side runoff location.
2. **A higher-load reference car / Sunny Beach** — isolates car-dependent load
   and shift behavior on the same route; select and record one exact car before
   capture.
3. **Dino / one route selected during scouting for repeatable curb plus sand or
   rough shoulder** — isolates environment while retaining the baseline car.

Do not claim the second car or third route before they are selected and
documented. This three-session matrix changes one major variable at a time.

### Exact scenario set

Each scenario is a separate manually started capture with the same controller,
wheel-side settings, game Strength and Reference+ 1.44:

```text
RII_01_stationary_center
RII_02_straight_asphalt_speed_sweep
RII_03_progressive_left_clean
RII_04_progressive_right_clean
RII_05_sustained_high_load_clean
RII_06_drift_initiation_hold_release_bite
RII_07_striped_runoff_front_then_side
RII_08_striped_runoff_full_transition
RII_09_curb_single_side_then_reentry
RII_10_sand_or_rough_partial_then_full
RII_11_surface_reentry_settle
RII_12_mild_impact
RII_13_controlled_stronger_impact
RII_14_upshift
RII_15_downshift
```

R0 does not ask the owner to drive these. R1 must first produce and validate
the observation schema.

## Instrumentation and safety assessment

R1 needs one extension of the existing player-car telemetry path and one
extension at the already-existing DirectInput observation boundary. It does
not need a new output hook. Known per-corner fields should be read only after
the current exact-pointer-topology validation. Native event and vibration
values should be sampled in the same update that records current force state.

Required controls:

- validate the exact supported EXE and record the hash in each capture;
- use existing hook ownership; add no overlapping patch;
- keep telemetry developer-only and disabled by default;
- fail closed on pointer mismatch, unavailable state or non-finite values;
- leave every Force/output value read-only;
- distinguish unavailable cells from numeric zero;
- preserve old columns and append new fields;
- add schema/version tests and replay reconstruction checks;
- prove research-disabled runtime remains equivalent to v1.0.0;
- keep high-gain testing offline.

Risk is **low for a passive extension at existing boundaries**, provided no
new native patch site is introduced. Reading the currently uncaptured corner
members is **medium research risk** until their writer lineage and validity
conditions are independently checked. Any proposal requiring a new hook is a
separate milestone with patch-ownership proof before implementation.

## R1 recommendation

Proceed with **R1 — Passive Unified Observation Schema**, not a force change.
R1 should:

1. statically validate readers/writers and validity conditions for the known
   uncaptured corner fields;
2. append the neutral surface, vibration, gear, post-channel, ramp and
   unclamped-output fields above at existing safe observation boundaries;
3. add build/executable/schema identity to each capture;
4. prove Reference+ 1.44 reconstruction from per-channel values through final
   DirectInput request;
5. ship research disabled by default and stop before physical capture.

Only after R1 passes CI and passive-safety review should the unified capture
campaign begin. R0 does not establish a new surface effect, native tyre model,
Force Character ceiling, AER profile, gear-shift change, active-pedal output,
or motion output.

## R0 deliverable index

| Requested output | R0 location / decision |
| --- | --- |
| 1. GATS evidence interpretation | Track 1, field-feedback interpretation |
| 2. Surface hypothesis classification | Track 1, surface hypothesis decision |
| 3. Native per-corner surface state | Track 1, existing independent knowledge |
| 4. Partial-contact preservation | Track 1; preserved raw, later collapsed by restored vibration path |
| 5. Native four-corner topology | Track 2, what is established |
| 6. Suspension/contact candidate | `+0x28`, partially corroborated without units |
| 7. Compression-rate candidate | Derived delta only; no native rate identified |
| 8. Vertical/load candidate | Unresolved |
| 9. Reference-load candidate | Unresolved |
| 10. Slip-angle candidate | Unresolved |
| 11. Lateral-force candidate | `+0xAC`, partially corroborated response only |
| 12. Longitudinal-force candidate | `+0xB0`, partially corroborated response only |
| 13. Grip/capacity candidate | Unresolved |
| 14. Combined-slip candidate | Not corroborated; current `hypot` is HYP36R-derived |
| 15. HYP36R-state comparisons | Track 2, corroborating HYP36R states |
| 16. `EVWORK_CAR + 0xDBC` | Declared but unused; semantics unresolved, production impact zero |
| 17. Existing ceiling evidence | Track 3, current coverage |
| 18. Missing ceiling evidence | Track 3, native Reference+ 1.44 schema audit |
| 19. Unified telemetry schema | Track 3, proposed unified telemetry schema |
| 20. Current PC Road path | Track 4, current Road and Impact pipeline |
| 21. Current PC Impact path | Track 4, current Road and Impact pipeline |
| 22. Gear-shift path | Track 4, gear-shift path |
| 23. AER/official lineage | Track 4, evidence branches and required evidence |
| 24. Active-pedal/motion candidates | Future active-pedal and motion candidates |
| 25. Minimum car/course matrix | Unified future capture campaign |
| 26. Exact scenarios | `RII_01` through `RII_15` |
| 27. Next milestone | R1 passive unified observation schema only |
