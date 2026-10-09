# AER Profile Implementation Blueprint

## Status and boundary

This closes the independent Sega game-side steering study sufficiently to begin a modern **Arcade Experience (AER)** profile. It does not reproduce Sega drive-board firmware, assign torque units to command bytes, or change Reference+. The proposed profile is an independent, evidence-informed DirectInput interpretation.

Evidence sources are the verified DVP-0015A `Jennifer` executable, original course assets, AER-03 capture `20261008-172718-8c375f9c`, and complete AER-04 V2 capture `20261008-200325-6ba226b8`.

## A. Arcade FFB behavior reference

Sega's game-side system communicates more than a centering spring:

1. It derives a continuous request from average front-tire direction.
2. It applies nonlinear shaping, per-front-contact attenuation, front-load adjustment, integer quantization, and a 4–15 clamp.
3. Direction uses a separate request path; magnitude bytes are not signed torque.
4. Short patterns are selected from collision-classification transitions, front-contact families, and wall-rebound state.
5. Pattern output has priority. A pending continuous update is deferred rather than mixed into that request.
6. Pause/suspend suppresses eligible cabinet callbacks. Neutral, idle, and shutdown are explicit protocol states.

The physical board firmware remains the boundary: motor current, waveform, decay, and cabinet torque are not recoverable from game requests alone.

## B. Original signal and command matrix

| Source / request | Original role | Conditions | Direction | Persistence / priority | Evidence |
|---|---|---|---|---|---|
| `EVWORK_CAR+0x054` | Average front-tire direction | Valid vehicle state | Signed-16 angle, π/32768 | Recomputed by original vehicle logic | CONFIRMED |
| `EVWORK_CAR+0x3AC` | Threshold/selection input | Magnitude substitution and pattern selection | Not a direction value | Current native state | CONFIRMED source; meaning UNKNOWN |
| `EVWORK_CAR+0x3FC` | Aggregate road/contact transition input | Current contact state | N/A | Current native state | CONFIRMED |
| `EVWORK_CAR+0x404/+0x408` | Per-front-tire one-hot classification | Valid front contacts | Left/right asymmetry retained | Current native state | CONFIRMED |
| `0x0B` | Quantized continuous magnitude | Active gameplay after shaping | Separate direction path | Sent when dirty and no pattern has priority | CONFIRMED |
| `0x06` | Directional/secondary request; also initialization | Direction change or initialization path | Payload carries request state | Separately scheduled | CONFIRMED family; physical meaning UNKNOWN |
| `0x7B` | Discrete pattern | Pattern selected and dirty | Bit `0x10` | Pattern first; countdown suppresses replacement | CONFIRMED |
| `0x7D` | Idle/neutral slot | No active request | N/A | Repeated protocol state | CONFIRMED |
| `0x00` | Deactivation/shutdown family | Lifecycle transitions | N/A | Explicit stop/deactivation | CONFIRMED |

### Continuous magnitude evidence

The corrected decoder uses `value_a >> 3`. Complete AER-04 contains 1,692 observations:

| Magnitude | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Count | 134 | 137 | 174 | 189 | 230 | 339 | 400 | 89 |

There were 1,687 magnitude transitions. This disproves the earlier apparent fixed-magnitude result caused by decoding the wrong byte. Values 12–15 are valid by static lineage but were not observed here.

### Discrete pattern evidence

The capture contains 190 pattern requests. Translation and direction remain distinct from internal index.

| Translation | Direction | Candidate indices | Count |
|---:|---:|---|---:|
| `0x00` | 0 | 2,3,6,7,8,9,11,12,14,15 | 85 |
| `0x04` | 0 | 10 | 41 |
| `0x02` | 0 | 13 | 34 |
| `0x00` | 1 | ambiguous zero-translation family | 15 |
| `0x0B` | 1 | 0 | 6 |
| `0x04` | 1 | 10 | 5 |
| `0x0B` | 0 | 0 | 4 |

## C. Road surface character matrix

| Relationship | Established | Not established | Confidence |
|---|---|---|---|
| Pattern 10 | Transition between `current & 0x00F03302` and `previous & 0x0200801C`; direction follows steering-state difference | Universal kerb/cobblestone/material effect | CONFIRMED selection |
| Patterns 12/13 | Contact family `0x00008014` (ordinals 2,4,15), agreement and threshold branch | Firmware waveform; unique material per ordinal | CONFIRMED family |
| Patterns 14/15 | Contact family `0x02000008` (ordinals 3,25), agreement and threshold branch | Firmware waveform; unique material per ordinal | CONFIRMED family |
| Contact attenuation | Each qualifying front contact multiplies shaped continuous request by 0.8 (1.0/0.8/0.64) | Every qualifying ordinal is physically rough | CONFIRMED |
| AER-04 V2 | 37,268 road-valid command rows; patterns under symmetric and asymmetric masks | Causation from one uncontrolled run | CONFIRMED observations; relationship STRONGLY SUPPORTED |
| Tulip ordinal 20 | Localized bridge-road geometry and exact visual registration; Pattern-10-current eligible | Universal ordinal-20 meaning | STRONGLY SUPPORTED cobblestone relationship only at Tulip |

The profile should respond to **classification transitions and contact asymmetry**, not a material-name table. Continuous texture synthesis, if used, is explicitly our modern interpretation.

## D. Collision and impact character

Static lineage strongly associates internal Patterns 0 and 2 with the state-`0x1E` wall-rebound branch. Pattern 0 translates to `0x0B`, lasts eight eligible callbacks, and was observed ten times in AER-04. Direction comes from wall/collision state, and pattern output precedes continuous magnitude.

- A short directional impact keyed by verified Pattern 0 is supported.
- Naming every Pattern-0 request as a specific wall, vehicle, or barrier impact is unsupported.
- Sustained wall friction needs a verified native source before becoming a distinct component.
- Board-owned waveform and cabinet impulse strength remain unknown.

## E. Modern FFB interpretation specification

Everything below is **our synthesis**, not firmware reconstruction.

### Continuous steering

- Normalize decoded magnitude 4–15 to 0–1 only inside the selected AER profile.
- Use verified native direction state; never infer polarity from magnitude bits.
- Apply bounded monotonic shaping and time-domain smoothing.
- Hold valid evidence only for a short watchdog period, then decay to zero.
- Never convert request units directly to Nm.

### Contact/surface events

- Treat Pattern 10 and Patterns 12–15 as bounded short event families.
- Preserve direction when unambiguous.
- Preserve callback-duration ordering, but derive modern time from measured cadence.
- Classification transitions may select event character; material names may not.
- Contact asymmetry may bias direction after replay validation.

### Collision events

- Pattern 0 may produce a bounded directional impulse with highest event priority.
- Do not synthesize sustained friction without evidence.
- Bound repeats with cooldown and a total event budget.

### Safety

- Compose continuous and event layers in normalized space.
- Use a final soft limiter, slew protection, finite checks, focus/gameplay gates, disconnect stop, stale-evidence decay, and explicit shutdown.
- AER and Reference+ use separate settings/state. Selecting AER must not mutate Reference+.

## F. AER profile architecture

```text
verified game evidence
    ├── tire direction + contact/load context
    ├── magnitude and direction requests
    └── discrete pattern request
            ↓
    AER evidence adapter
            ↓
    continuous interpreter | event interpreter | lifecycle gate
            ↓
    bounded AER composer
            ↓
    existing HYP36rforce conditioning / DirectInput device path
```

Future Multi-Input modules:

1. `AerEvidenceFrame`: timestamp, validity, direction, magnitude, road masks, pattern, lifecycle.
2. `AerContinuousInterpreter`: normalization and direction ownership.
3. `AerEventInterpreter`: arbitration, duration, retrigger and cooldown.
4. `AerComposer`: continuous/event budgets and soft limiting.
5. `AerSafetyGate`: focus, gameplay, stale input, disconnect, invalid data and shutdown.
6. `AerProfileSettings`: player Strength and Road Detail; engineering controls remain Debug/UAT.
7. `AerTelemetry`: evidence, interpretation, pre-limit and final request with a versioned schema.

Player experience:

```text
Force Profile: Arcade Experience
Strength:       0–100%
Road Detail:    0–100%
```

Reference+ remains default/fallback until AER completes physical validation.

## G. Validation plan

### Deterministic replay

- Replay AER-03/AER-04 ordering.
- Verify magnitude 4–11 remains monotonic after normalization.
- Verify pattern priority defers rather than destroys pending continuous state.
- Verify Pattern-10/13 examples remain bounded under symmetric/asymmetric masks.
- Verify Pattern 0 produces one bounded event and honors cooldown.
- Verify missing, invalid, or stale evidence decays to zero.
- Verify no NaN/Inf, unbounded reversal, or output above normalized limits.
- Verify inactive AER leaves Reference+ state and output unchanged.

### Cross-wheel UAT

After implementation and replay pass, test one belt-driven, one mid-torque DD, and one high-torque DD base. Validate continuous clarity, event direction, road separation, collision comfort, and disconnect/shutdown. UAT validates our presentation, not cabinet equivalence.

## Evidence limits and exact implementation requirements

Implementation can begin without Sega firmware. Remaining uncertainties are firmware waveforms/decay/current, cabinet torque, direct wall-friction and vehicle-contact state, `+0x3AC` semantics, callback-tick conversion, and runtime coverage of magnitudes 12–15.

Before release: implement a clean-room AER evidence reader in Multi-Input, passive shadow mode, deterministic replay, bounded DirectInput composition, versioned telemetry, focused hardware UAT, and an opt-in selector. None belongs in LinuxLoader's research transport.
