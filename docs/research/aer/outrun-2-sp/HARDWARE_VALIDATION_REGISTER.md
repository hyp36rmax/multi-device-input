# Arcade Experience Research — Hardware Validation Register

This register tracks questions that cannot be answered safely from the original game executable and course assets alone. It complements the [AER Evidence Register](EVIDENCE_REGISTER.md).

The original game-side pipeline is substantially understood. This document deliberately begins where that evidence ends: drive-board firmware, electrical behavior, cabinet mechanics, and real physical steering response.

## Validation principles

- Prioritize original documentation and firmware analysis before physical testing.
- Prefer one bounded question per test.
- Do not use a broad gameplay session when one request or state transition will answer the question.
- Preserve original request bytes, timestamps, responses and test conditions.
- Do not describe a command value as torque or a pattern value as a waveform until measured evidence establishes that relationship.
- Keep future HYP36rforce interpretation separate from original cabinet validation.

Status values:

- **OPEN — STATIC FIRST** — more documentation or firmware analysis may answer part of the question.
- **OPEN — HARDWARE REQUIRED** — physical behavior is the missing evidence.
- **OPEN — RUNTIME REQUIRED** — original runtime scheduling or activation must be observed.
- **BLOCKED — SOURCE NEEDED** — required firmware, service information, or hardware is unavailable.
- **VALIDATED** — the question has repeatable original-hardware evidence.

## Priority register

| ID | Question | Existing evidence | Missing evidence | Static path | Firmware needed | Hardware needed | Smallest justified validation | Status |
|---|---|---|---|---|---|---|---|---|
| AER-HW-TORQUE-001 | What physical torque corresponds to magnitude `4–15`? | Game curve, clamp, packet fields and fixed scale are confirmed | Motor current, torque and cabinet mechanical response | Inspect firmware lookup/output stages if obtained | Yes, useful | Yes, authoritative | Send two bounded fixed magnitudes with centered wheel and measure current/torque | OPEN — HARDWARE REQUIRED |
| AER-HW-DIR-001 | Which game direction value produces physical left/right torque? | Direction byte construction and pattern direction bit are confirmed | Motor polarity and cabinet orientation | Trace firmware H-bridge/polarity logic | Yes, useful | Yes | Send one low bounded request in each direction and record motion/torque sign | OPEN — HARDWARE REQUIRED |
| AER-HW-PATTERN-001 | What physical waveform does each translated pattern produce? | Internal-to-board translation table and `0x7B` packet are confirmed | Firmware pattern tables and motor response | Search firmware for translated-value dispatch/tables | Yes, preferred | Yes for final proof | Trigger one translated pattern at a time with the wheel safely constrained and record current/position | OPEN — STATIC FIRST |
| AER-HW-DURATION-001 | Does the game, firmware, or both own pattern duration? | Game suppresses replacement for callback-table ticks | Whether board continues, repeats or times out independently | Trace firmware pattern state machine | Yes, preferred | Yes if firmware unavailable | Send one pattern request without refresh and measure response end time | OPEN — STATIC FIRST |
| AER-HW-POWER-001 | What do motor-power codes `0x32/0x40/0x50/0x60` control physically? | Initialization writes and table at `0x081E23A0` are confirmed | Electrical limit, gain or mode meaning | Trace firmware handler for command `0x03` | Yes, preferred | Yes for physical scale | Compare two codes using one identical low request and measure current/torque | OPEN — STATIC FIRST |
| AER-HW-LATENCY-001 | What is command-to-actuation latency? | Request ordering, queue and write ownership are confirmed | Serial edge to motor-current/position timing | Firmware interrupt/loop analysis may bound it | Useful | Yes | Timestamp one serial packet and first measurable motor response on shared clock | OPEN — HARDWARE REQUIRED |
| AER-HW-CADENCE-001 | What is the authoritative cabinet callback frequency? | One callback per eligible event pass; static `WaitVSync()` does not establish Hz | Runtime event timestamps under original execution conditions | Further loader/display synchronization audit may help | No | Not necessarily | Timestamp entry to `CabinetCtrl_Main()` for a short stationary interval | OPEN — RUNTIME REQUIRED |
| AER-HW-CAL-001 | How does the cabinet physically respond during steering calibration? | Search/ramp sequence, analog sampling, bounds and timeout are confirmed | Endpoint motion, centering behavior and success criteria at hardware | Trace firmware calibration commands and service documentation | Yes, useful | Yes | Observe one standard startup calibration with serial, steering ADC and motor response captured | OPEN — STATIC FIRST |
| AER-HW-RESP-001 | What do valid drive-board response values mean? | Per-board byte validation and statuses are confirmed | Firmware state represented by each status | Firmware response construction or service manual | Yes, preferred | Possibly | Map responses during one normal initialization before inducing any fault | OPEN — STATIC FIRST |
| AER-HW-ALARM-001 | What physical faults correspond to runtime status/alarm classes 1–3? | Game maps statuses to three alarm categories | Board-side fault sources and service meanings | Firmware and original service documentation | Yes | Possibly | Do not induce faults until documentation/firmware identifies safe conditions | BLOCKED — SOURCE NEEDED |
| AER-HW-PAUSE-001 | What happens physically when the cabinet event is paused or suspended? | Pause/suspend skips the callback; `CabinetCtrl_Off()` sends an explicit zero request elsewhere | Whether another owner stops output or board retains its previous state | Complete pause caller audit | Useful | Yes for final behavior | After caller audit, pause following one low stable request and observe whether output decays, stops or persists | OPEN — STATIC FIRST |
| AER-HW-ACT-001 | Why does the loader environment fail to reach regular outbound gameplay commands? | Complete native pipeline and readiness gates are confirmed; prior capture showed reads but no regular writes | Exact gate altered or bypassed by loader patches | Compare patch sites with original initialization transitions | No | No initially | Instrument only readiness/state transitions through first successful native write | OPEN — STATIC FIRST |
| AER-HW-DUAL-001 | How are two steering boards used in supported cabinet modes? | Dual seven-byte framing and inactive-slot neutral command are confirmed | Physical board roles and synchronization | Firmware/configuration and cabinet wiring documentation | Yes, useful | Yes for final proof | Identify cabinet wiring before sending any request; then observe normal dual-board initialization | BLOCKED — SOURCE NEEDED |

## Detailed validation notes

### AER-HW-TORQUE-001 — Native magnitude versus physical torque

What is known:

```text
front-tire direction
    → nonlinear curve
    → contact attenuation
    → load adjustment
    → integer 4–15
    → command 0x0B fields
```

Why this is insufficient: the executable stops at a board request. It contains no authoritative motor-current calibration, steering gear ratio, cabinet friction, or torque measurement.

Minimum safe evidence should include request value, configured motor-power code, steering position, supply voltage, current or force measurement, and board response. One low and one higher bounded value are enough for the first comparison; a full sweep is not justified initially.

### AER-HW-PATTERN-001 — Pattern behavior

What is known:

- internal patterns 0–15;
- translation table;
- direction bit;
- game-owned callback countdown;
- command `0x7B` construction.

What remains unknown:

- whether translated values select firmware waveforms, scripts, or modes;
- whether amplitude is fixed or affected by another board state;
- whether direction changes waveform phase, polarity, or motor target;
- whether the board owns an additional timeout.

The preferred next source is original drive-board firmware. If it is unavailable, test one translated value rather than all sixteen.

### AER-HW-CADENCE-001 — Callback frequency

Earlier serial-read polling near 60 Hz is not accepted as callback-frequency evidence. The minimum validation is a short timestamp series at `CabinetCtrl_Main()` under the original runtime path. It does not require driving and should not alter request construction.

Report:

- timestamp source and resolution;
- number of eligible callbacks;
- pause/loading intervals;
- median and percentile interval;
- whether display synchronization changes cadence.

Only after this result may callback-duration ticks be expressed as approximate time.

### AER-HW-ACT-001 — Loader activation

The working hypothesis is not that native FFB is absent. Static evidence proves a full game-side output path. The unresolved question is which readiness, initialization, calibration, or event-ownership transition fails to complete in the loader environment.

The smallest diagnostic sequence is:

```text
CabinetCtrl_InitDriver state
hardacu readiness
response validation
calibration state
CabinetCtrl_Check transition
first CabinetCtrl_Main callback
first native serial write
```

No force interpretation is required for this validation.

## Validation priority

Recommended order:

1. `AER-HW-ACT-001` — establish the unmodified native activation path.
2. `AER-HW-CADENCE-001` — establish callback cadence.
3. Obtain and inspect original firmware or service information.
4. `AER-HW-PATTERN-001` and `AER-HW-DURATION-001` — one isolated pattern.
5. `AER-HW-DIR-001` — bounded direction convention.
6. `AER-HW-TORQUE-001` and `AER-HW-POWER-001` — bounded physical scaling.
7. `AER-HW-LATENCY-001` — synchronized protocol/motor timing.

Alarm and dual-board fault testing should wait for documentation or firmware evidence.

## Relationship to future work

AER records original arcade evidence. HYP36rforce is an independent modern FFB implementation. A future optional Arcade profile may use validated AER findings as design input, but it must document its own equations and validation. It must not present an interpretation as recovered Sega behavior.

Reference+ remains a separate HYP36rforce experience. AER-MOTION and SimHub are separate research topics and are outside this register.
