# AER-02D Offline Native FFB Activation Model

## Scope and evidence

This document reconstructs the activation path in the verified OutRun 2 SP SDX Rev A `Jennifer` executable (SHA-256 `f16fc04d836a2bd8e401d8f987d4fe694fa16e18a9f870da6623d7f884911075`). The executable is not modified. The accompanying test is an isolated model and is not linked into LinuxLoader; it cannot send serial data or activate a motor.

The reconstruction uses original executable instructions and symbols, plus the loader source. It does not use a third-party force interpretation. Board-side meanings not established by original-game evidence remain unknown.

## Original activation chain

```text
CabinetCtrl_InitDriver()                    0x08103EAA
  driver state 0..12                       0x0866D460
  validated replies via CabinetCtrlMIDI_Input()
  steering-position search via GetVolumeSrc()
          |
          | driver state == 12
          v
CabinetCtrl_Check()                         0x0810477E
  check state 0..2                         0x0866D48C
  emits 16 x 16 check-table entries
          |
          | check state 1 complete
          v
ChangeNowEventCtrlFunc(CabinetCtrl_Main)    0x0806D050
          |
          v
CabinetCtrl_Main()                          0x081048B2
  DrCtrlDataSet()                           0x08104F02
  DrCtrlMoveSend()                          0x081051F4
  steerReqSendOut()                         0x08105AD2
  hardcomSend()                             0x0810735E
```

`CabinetCtrl_IsCheckEnd()` returns true only when driver state is 12 and check state is 2. `CabinetCtrlMIDI_Server()` has the same two gates before it services runtime replies and queued output. The game event controller suppresses the installed callback while pause bit `0x08` or suspend bit `0x10` is set.

## `CabinetCtrl_InitDriver()` states

| State | Original behavior and exit condition |
|---:|---|
| 0 | Requires `amLibIsBasebdAvailable()`. An existing application error enters 11; otherwise initializes a 90-tick delay and enters 1. |
| 1 | Counts down 90 invocations, selects board 0, clears retry count, enters 2. |
| 2 | Allows two probe attempts. Clears receive data, sends the `0x7f` probe, starts a 16-tick response window, enters 3. A third attempt enters 11. |
| 3 | Uses `CabinetCtrlMIDI_Input()`. `0xff` (validation failure) and `0xee` retry through 2. Any other validated status sends the `0x01/0x30/0x7f` request, starts 32 ticks, enters 4. Timeout enters 11. |
| 4 | Waits for status 1. On status 1, sends `0x7c` with the evidenced initialization payload, starts 600 ticks, enters 5. Other replies repeat the preceding request. Timeout returns to 2. |
| 5 | Waits for status 0. On status 0, initializes the four-step configuration sequence and enters 6. Other replies send `0x7d`. Timeout returns to 2. |
| 6 | Sends one of four requests selected by substate: `0x7a`, `0x03` (configured motor-power table), `0x06`, then `0x08`; starts 10 ticks and enters 7. |
| 7 | On status 0 advances the configuration substate. After the fourth successful reply it samples the steering analog reference, initializes the bounded search and enters 8. Nonzero validated replies repeat state 6. Timeout enters 11. |
| 8 | Samples the configured steering analog channel and searches with signed test levels adjusted in steps of 10, bounded to the original interval `[-96, 96]`. A 900-tick limit is enforced. It constructs `0x00`/`0x04` calibration requests. Convergence enters 9; timeout calls `CabinetCtrl_Off()` and enters 11. |
| 9 | Drains replies, sends a zero request, restores the configured board-power request, and advances to the next board. Another board returns to 2; otherwise enters 10. |
| 10 | Drains pending replies, then writes active state 12. |
| 11 | Terminal driver-error/display state. |
| 12 | Terminal active state required by check, MIDI service, and ordinary output. |

The simulator deliberately models board input at the output of `CabinetCtrlMIDI_Input()`: missing, status 0, status 1, other validated status, invalid duplication (`0xff`), or special `0xee`. It does **not** invent raw board bytes.

## Response validation and calibration limits

`CabinetCtrlMIDI_Input()` requires one byte per configured board. It extracts the low three status bits, compares them with the duplicated high status bits unless bit 3 is set, maps inconsistent values to `0xff`, and preserves `0xee` as a special result. This establishes the game's validation rule but not the physical meaning of every status.

The static code establishes the calibration algorithm, analog dependency, timeouts, step size, and bounds. It does not establish the safe behavior of a real motor for each request. Consequently, no proposed loader change may replay these calibration requests against a consumer wheel without a separate safety design.

## `CabinetCtrl_Check()` states

| State | Original behavior |
|---:|---|
| 0 | Runs only when driver state is 12 and configured board count is nonzero. Drains replies, emits two entries per invocation from the native 16-by-16 request table using commands `0x1d` and `0x1e`, then advances table row/column. |
| 1 | Drains remaining replies, calls `ChangeNowEventCtrlFunc(CabinetCtrl_Main)`, then enters 2. |
| 2 | Complete. Together with driver state 12, this is the native check-complete condition. |

## Exact loader divergence

For DVP-0015A, `SKIP_OUTRUN_CABINET_CHECK` replaces `CabinetCtrl_InitDriver()` at `0x08103EAA` with a function that returns 1. It also replaces `hardacuIsInitEnd()` at `0x08105D88` with return 1. Returning 1 does not write driver state `0x0866D460`; it remains at its initialized value rather than reaching 12.

This is decisive because `CabinetCtrl_Check()` begins with:

```text
if (driver_state != 12) return;
```

Therefore the check table never completes, `ChangeNowEventCtrlFunc(CabinetCtrl_Main)` is never reached through native flow, and normal `DrCtrlDataSet()` / `DrCtrlMoveSend()` output ownership is never installed.

`EMULATE_DRIVEBOARD` separately:

- changes the state-11 assignment byte at `0x0810401B` to 12;
- patches the output-related constant at `0x081E2180`;
- also replaces `hardacuIsInitEnd()` with return 1;
- reports serial readability unconditionally from `sharedSelect()`;
- may return a one-byte read even when no response is pending; and
- implements response values according to loader assumptions rather than the original validation sequence.

The first state patch can turn a native error path into state 12, but it does not reproduce the successful states 0–10, calibration, or response validation. With `SKIP_OUTRUN_CABINET_CHECK` also enabled, the replaced initializer never executes the patched error assignment anyway. These mechanisms are therefore not equivalent to native activation.

## Offline simulation results

The research-only harness validates:

- normal single-board progression through driver state 12, check state 2, callback installation, and callback execution;
- dual-board progression, including per-board repetition;
- no progress when the baseboard is unavailable;
- invalid response retries and terminal failure after the original retry allowance;
- missing-response timeout failure;
- the exact loader-skip dead path: successful replacement return, driver state unchanged, check state unchanged, no callback;
- pause and suspend suppression of the callback;
- callback resumption when neither bit is set; and
- shutdown removing callback ownership.

The harness preserves current loader behavior by default because it is compiled only by its dedicated test script.

## Correction options

### A. Preserve native initialization with an evidence-accurate virtual board

Highest fidelity, but not yet justified. It requires raw reply bytes and lifecycle behavior for every initialization and calibration request. The original game gives validation conditions but not enough evidence to guarantee safe physical-motor behavior. This option must terminate calibration at the virtual boundary and never forward calibration motor requests unintentionally.

### B. Reconstruct the proven native state transitions without physical calibration

Smallest potentially safe architecture. A research-only activation provider could own a virtual-board state machine, satisfy only proven validation/check gates, keep all motor output disabled during initialization, and transfer ownership to the original `CabinetCtrl_Main()` only after an explicit, internally verified ready state. It must fail closed on invalid input, timeout, revision mismatch, pause, disconnect, or shutdown. Writing state 12 directly is not sufficient and is not recommended.

### C. Restore callback ownership directly

Mechanically smallest but least faithful. Installing `CabinetCtrl_Main()` without the original initialization/check invariants creates hidden dependencies on globals, board count, analog center, queue state, and shutdown ownership. This option is rejected as a standalone correction.

## Recommendation and implementation gate

Use B as the architecture direction, but do not implement runtime activation yet. First specify a virtual initialization boundary that:

1. never forwards original calibration/test requests to physical motors;
2. supplies only response statuses established by the original state machine;
3. preserves native check-table and callback installation flow where safe;
4. records an explicit ready/failed/timed-out lifecycle;
5. supports one or two virtual boards without changing the default loader path; and
6. guarantees deactivation on pause, suspend, disconnect, and shutdown.

The remaining uncertainty is not whether the current bypass blocks activation—that is confirmed. It is the exact raw drive-board response encoding and the safe physical meaning of the initialization/calibration command payloads. Those unknowns prevent an evidence claim that option A is accurate and prevent direct motor-facing implementation. They do not prevent completion of the offline activation model.
