# AER-02F Virtual Drive-Board Technical Design

## Evidence boundary

Target executable:

```text
OutRun 2 SP SDX Rev A — DVP-0015A
Jennifer
SHA-256 f16fc04d836a2bd8e401d8f987d4fe694fa16e18a9f870da6623d7f884911075
```

This design combines the original-executable findings in [AER-EV-PROTO-001 through AER-EV-PROTO-005](EVIDENCE_REGISTER.md#protocol-evidence) with the completed DEV 4 activation capture. It models the game-side contract only. It does not claim to reproduce Sega drive-board firmware, motor behavior, torque, polarity, or calibration motion. Those remain under [AER-HW-CAL-001](HARDWARE_VALIDATION_REGISTER.md), [AER-HW-RESP-001](HARDWARE_VALIDATION_REGISTER.md), and the other hardware evidence gates.

## Original initialization transitions

`CabinetCtrl_InitDriver()` is at `0x08103EAA`; its state is stored at `0x0866D460`.

| State | Entry condition | Outbound request | Accepted game-side evidence | Mutation / next state | Timeout / error |
|---:|---|---|---|---|---|
| 0 | Initial state | None | `amLibIsBasebdAvailable()` and no application error | Timer 90; state 1 | Existing application error → 11; unavailable baseboard holds 0 |
| 1 | Startup delay | None | Timer expiration | Board 0, retries 0; state 2 | None |
| 2 | Probe attempt, maximum two | Logical `0x7f`, arguments `0,0` | A reply later accepted by state 3 | Timer 16; state 3 | Third attempt → error/state 11 |
| 3 | Probe pending | None until reply | Any parser-valid status except special `0xee`; invalid becomes `0xff` | Send logical `0x01`, arguments `0x30,0x7f`; timer 32; state 4 | `0xff`/`0xee` retry state 2; timeout → 11 |
| 4 | Initialization reply pending | Repeats `0x01/0x30/0x7f` for other replies | Parsed status 1 | Send `0x7c/0/0x20`; timer 600; state 5 | Timeout retries state 2 |
| 5 | Initialization completion pending | `0x7d/0/0` for nonzero replies | Parsed status 0 | Configuration substate 0; state 6 | Timeout retries state 2 |
| 6 | Configuration substate 0 | `0x7a/0/0x1f` | Reply evaluated in state 7 | Timer 10; state 7 | — |
| 6 | Configuration substate 1 | `0x03/<power>/0x04` | Power comes from `0x081E23A0`: `0x32,0x40,0x50,0x60` | Timer 10; state 7 | Physical meaning unresolved |
| 6 | Configuration substate 2 | `0x06/0x01/0x02` | Reply evaluated in state 7 | Timer 10; state 7 | — |
| 6 | Configuration substate 3 | `0x08/0/0x04` | Reply evaluated in state 7 | Timer 10; state 7 | — |
| 7 | Configuration reply pending | Repeats current substate on nonzero reply | Parsed status 0 | Advance substate; after fourth, sample analog reference and enter 8 | Expired timer → 11 |
| 8 | Steering-position search | Logical `0x00` and `0x04` calibration/test requests | Analog feedback from configured channel; no response value alone establishes convergence | Adjust signed test level by 10 within `[-96,96]`; convergence → 9 | 900-tick expiration calls `CabinetCtrl_Off()` and enters 11 |
| 9 | Board calibration complete | Zero request, then logical `0x01` with configured power | Reply buffer is drained | Next board → state 2; otherwise state 10 | — |
| 10 | All boards processed | None new | Pending replies drained | State 12 | — |
| 11 | Error | Error display / prior shutdown request | Terminal | Remains 11 | — |
| 12 | Active | Runtime pipeline becomes eligible | Required by check and MIDI server | Remains 12 | — |

`CabinetCtrl_Check()` at `0x0810477E` refuses to progress unless driver state is 12 and board count is nonzero. Check state 0 sends two entries per invocation from a 16-by-16 table using logical commands `0x1d` and `0x1e`. After all 256 entries, state 1 drains replies and calls `ChangeNowEventCtrlFunc(CabinetCtrl_Main)` at `0x0806D050`; check state then becomes 2. `CabinetCtrl_IsCheckEnd()` requires both driver 12 and check 2.

## Framing and response validation

The original request framing is confirmed by [AER-EV-PROTO-001](EVIDENCE_REGISTER.md#aer-ev-proto-001--packet-framing):

```text
single board: [command|0x80, arg1, arg2, xor(command,arg1,arg2)]
dual board:   [command0|0x80, a0, b0, command1, a1, b1, xor(all logical bytes)]
```

`CabinetCtrlMIDI_Input()` at `0x08105BFC` consumes one byte per configured board. For a normal byte, status is the low three bits. Bits 4–6 must duplicate those bits and bit 3 must be clear. A mismatch is exposed to the caller as `0xff`; `0xee` is preserved as a special result.

Confirmed parser encodings include:

| Encoded byte | Parser result | Confidence |
|---:|---:|---|
| `0x00` | status 0 | Confirmed by validation algorithm |
| `0x11` | status 1 | Confirmed by validation algorithm |
| `0x22` … `0x77` | statuses 2 … 7 | Confirmed structurally; physical meanings unresolved |
| bit-3 set or duplicated fields unequal | `0xff` | Confirmed |
| `0xee` | special `0xee` | Confirmed |

What is **not** confirmed is which raw status the original board returns for every request or what each status means physically. The offline harness therefore injects response statuses explicitly. A test's injected sequence is a declared assumption, not a recovered firmware table.

## Loader divergence matrix

| Area | Original expectation | Current loader behavior | Classification |
|---|---|---|---|
| Initializer ownership | Execute states 0–10 and reach 12 only through success | `SKIP_OUTRUN_CABINET_CHECK` replaces `0x08103EAA` with return 1 | Confirmed defect for native activation |
| Driver state | State 12 gates all later ownership | DEV 4 initial/final state 0; states 11/12 absent | Runtime confirmed |
| Cabinet check | Runs only after state 12 | Invoked 7,303 times but remained state 0 | Runtime confirmed |
| Callback | Installed by check state 1 | `CabinetCtrl_Main()` count 0 | Runtime confirmed |
| Readiness | Readable when an actual board response is pending | `sharedSelect()` reports readable whenever emulation is enabled | Confirmed inconsistency |
| Read result | Missing response must not become a valid byte | `driveboardRead()` returns 1 even when `enableRead` is false and leaves caller data unchanged | Confirmed defect |
| Response generation | Must satisfy original per-state validation and timing | `processDrivePacket()` uses a broad command switch and one global response | Confirmed structural mismatch; exact compatibility remains untested |
| Configuration | Four native requests require acknowledged progression | Existing emulator recognizes several framed forms; global prior response may be reused | Partial support, not equivalent lifecycle |
| Calibration | Analog position must change consistently with bounded virtual movement | Existing emulator changes input analog values for command `0x84` before `wheelInitialized` | Partial synthetic behavior; convergence fidelity unproven |
| Error-to-active patch | Normal path reaches 12 after state 10 | Byte at `0x0810401B` changes one state-11 assignment to 12 | Confirmed patch; not a reconstruction |
| Check/callback ownership | Original control flow owns both | Initializer bypass prevents reaching them | Confirmed |

## Calibration findings

The game samples the steering analog channel through `GetVolumeSrc()` using the per-board channel table at `0x0866C3E8` and subtracts the configured center at `0x0866C3F0`. State 8 compares successive signed offsets, adjusts a test level in increments of 10, clamps it to `[-96,96]`, emits logical `0x00` and `0x04` requests, and applies a 900-tick bound.

This proves that analog steering feedback participates in convergence. No separate encoder read was found in the game-side state-8 loop. It does not prove that the original board lacks its own encoder or safety logic; the game may only see the cabinet analog channel while firmware closes its own loop.

An offline virtual sensor can model a deterministic position response to test-level changes and allow the original comparison logic to converge. That model must be labeled synthetic and isolated from physical input/output. Exact motion direction, rate, inertia, end-stop behavior, and firmware completion semantics remain unresolved under [AER-HW-CAL-001](HARDWARE_VALIDATION_REGISTER.md).

## Virtual-board architecture

```text
original Jennifer state machine
        |
        | four-/seven-byte request frame
        v
revision-locked virtual transport (research-only, default off)
        |
        +-- strict frame/checksum decoder
        +-- per-board lifecycle
        +-- bounded per-board response queues
        +-- explicit assumed-response policy
        +-- synthetic steering-position provider
        +-- timeout/disconnect/fault controller
        +-- passive trace
        |
        X  no serial passthrough
        X  no SDL FFB
        X  no motor API
```

Required lifecycle states are disconnected, idle, initializing, configuring, calibrating, ready, shutdown, and fault. Invalid configuration, malformed framing, unexpected commands, queue overflow, disconnect, timeout, or illegal lifecycle transition must clear pending replies and enter fault. Pause/suspend suppresses the original callback; shutdown clears all virtual output and response state. Single- and dual-board state must remain independent even when one combined frame carries both requests.

The current AER-02F harness is deliberately standalone. Its synthetic policy is supplied by tests, it has no physical transport symbols, and it is not linked into LinuxLoader.

## Architecture comparison

| Option | Fidelity | Safety | Compatibility | Complexity | Unsupported assumptions | Reversibility/testability |
|---|---|---|---|---|---|---|
| A. Preserve native initialization; emulate protocol | Highest: original states/check/callback retained | Good only with a hard virtual boundary around calibration | Revision- and cabinet-sensitive | Highest | Exact original status timing and calibration behavior | Strong if default-off and transport-injected |
| B. Reconstruct transitions without physical calibration | High when implemented as A plus synthetic sensor/blocked motor path | Best practical safety | Requires explicit virtual calibration contract | Moderate-high | Synthetic analog response | Strong |
| C. Restore callback through native lifecycle | Low if treated as direct installation; acceptable only as the consequence of A/B | Direct injection is unsafe | Hidden globals and queue state | Superficially low, actually risky | Many lifecycle prerequisites | Poor as standalone |

Recommended architecture: **A with B's synthetic-calibration safety boundary**. C must occur only as the original result of check completion, never as a loader action.

## Offline validation

The activation-state harness covers normal and failure progression through driver 12, check 2, and callback eligibility. The virtual-board harness independently validates:

- single- and dual-board framing and XOR checksums;
- response encoding/validation, including invalid and special values;
- explicit injected responses and delayed availability;
- bounded per-board queues;
- valid synthetic initialization/configuration/calibration command families;
- malformed frame and unexpected-command rejection;
- disconnect during initialization or active operation as fail-closed;
- invalid board counts;
- queue overflow;
- repeated initialization and shutdown; and
- the invariant that no physical transport is accessed.

Missing replies are intentionally left absent for the game-side activation model to time out. Pause/suspend and callback shutdown are validated in that model because those are game-event rather than board-transport concerns.

## Remaining evidence gaps and gate

The offline architecture is complete, but runtime implementation must not claim original-firmware accuracy. Remaining evidence gaps are:

1. authoritative request-to-status mapping and response timing (`AER-HW-RESP-001`);
2. physical and firmware semantics of calibration requests (`AER-HW-CAL-001`);
3. physical meaning of power codes (`AER-HW-POWER-001`);
4. dual-board cabinet roles (`AER-HW-DUAL-001`); and
5. authoritative callback cadence after activation (`AER-HW-CADENCE-001`).

The smallest justified next milestone is a **design-only runtime integration specification**: revision lock, configuration gate, synthetic transport interface, virtual sensor equation, fail-closed lifecycle, and proof that all physical output functions are unreachable during initialization. It should precede any loader integration or Windows artifact.
