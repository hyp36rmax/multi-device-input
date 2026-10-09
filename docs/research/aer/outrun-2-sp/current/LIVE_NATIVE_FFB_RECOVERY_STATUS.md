# Native Steering Recovery — Live Evidence and Current Status

## Where the project stands

I started AER to answer a simple question: what did *OutRun 2 SP SDX* actually tell its steering drive board to do? By DEV 5, the original game had completed its own steering initialization and was generating requests during an actual race, without a physical steering motor connected. That matters. The commands are Sega's original game-side output, not a force model invented by LinuxLoader.

**Native FFB command recovery is live-validated. Authentic physical arcade force feedback is not.** The virtual board supplies explicitly synthetic responses and synthetic steering-position feedback. Nothing here establishes physical torque, waveform, electrical polarity, mechanical feel, firmware effect behavior, or safety values for a modern wheel.

## Proven runtime path

Verified DVP-0015A `Jennifer` (SHA-256 `f16fc04d836a2bd8e401d8f987d4fe694fa16e18a9f870da6623d7f884911075`) runs its own:

```text
CabinetCtrl_InitDriver → native driver state 12
CabinetCtrl_Check → check state 2 → CabinetCtrl_Main
    → DrCtrlDataSet → DrCtrlMoveSend
    → steerReqSendOut → hardcomSend
    → isolated virtual SERIAL0 → passive raw capture
```

Neither the game states nor the callbacks are forced by the research adapter. The successful DEV 5 session `20261008-172718-8c375f9c` recorded 7,323 calls each to `DrCtrlDataSet` and `DrCtrlMoveSend`, and 10,025 calls each to `steerReqSendOut` and `hardcomSend`. The later AER-04 session recorded another 12,657 vehicle-data snapshots and 16,557 raw events with **zero dropped records**. These are distinct observation layers: callback calls, serial write attempts, accepted virtual frames, and decoded logical commands must not be added together or treated as equivalent.

The first live-validation baseline is commit `f7b892b`. Later commits change research instrumentation, not the historical significance of that result.

## Why earlier runs failed

DEV 4 established that enabling `SKIP_OUTRUN_CABINET_CHECK` replaced the original `CabinetCtrl_InitDriver()` with a loader return-one hook. Original driver state remained 0, cabinet check remained 0, and the native output callback was never installed. Restoring native ownership required the isolated virtual board.

The first DEV 5 initialization failed because the virtual board expected **one four-byte channel** while SDX sends **two logical steering-channel slots in one seven-byte SERIAL0 packet**. Its authentic first probe `FF 00 00 7D 00 00 02` was rejected before a response could be delivered. The game eventually entered native error state 11. The topology correction is recorded in [SDX Steering Hardware Topology](SDX_HARDWARE_TOPOLOGY.md).

Once that handshake progressed, the separate SDX **motion-actuator readiness check** prevented gameplay. The existing LinuxLoader actuator-readiness hook was enabled **only for eligible research mode**, leaving native steering initialization untouched. Steering and motion are different subsystems; this is not a reproduction of SDX motion hardware.

## What the steering recordings establish

The original game emits continuous `0x0B` magnitude requests, companion/directional `0x06` requests, discrete `0x7B` patterns, and initialization/idle commands. SDX channel 0 supplied the observed active gameplay requests; channel 1 was predominantly idle in the recorded single-player sessions.

**Correction to the initial analyzer:** continuous `0x0B` magnitude is encoded in logical request byte 1 (`value_a >> 3`), **not byte 2**. Reanalysis of the existing captures found game-side magnitudes **4–11** instead of a constant 4. The original permitted range remains **4–15**. The earlier AER-03 result of 283 constant-four observations reflects a superseded decoder and must not be cited as a real force-character finding. The command values are not physical torque. Bit `0x10` in this packed magnitude byte is not an independently established direction flag; pattern `0x7B` direction handling is separate.

Translated pattern observations include the unique `0x04` encoding for internal Pattern 10 and `0x02` for Pattern 13, plus ambiguous zero-translated values shared by several original indices. Pattern 10 is tied to a **classification-family transition**, not proven kerb or cobblestone vibration. No live command has yet been independently matched to a particular kerb, physical bump, wall strike, or specific course location. The Tulip Garden bridge-road ordinal-20 lineage remains a separate static course-evidence finding.

## AER-04 telemetry version boundary

The first synchronized AER-04 recording contained 58,126 CSV observations. It is valuable for commands and timestamps, but the observer mistakenly read `EVWORK_CAR` offsets from the **`CAR_WORK` pointer**. Consequently **AER_VEHICLE_FFB_V1 front-tire direction and road-field values are not reliable for surface attribution** and cannot be repaired retrospectively from those columns.

**AER_VEHICLE_FFB_V2** changes the observer to sample `EVWORK_CAR` at the original documented offsets (`0x054`, `0x3AC`, `0x3FC`, `0x404`, `0x408`). The recorder and analyzer preserve V1 files unchanged and quarantine their invalid road fields. V2 code and regression tests are committed; a live V2 capture remains necessary to establish whether road masks and command timing now correlate correctly. [AER-04 Vehicle Telemetry](AER04_VEHICLE_FFB_TELEMETRY.md) is the authoritative schema reference.

## Transport stability: important qualification

The two earlier successful sessions captured thousands of game-side callback executions but only roughly 500 successful serial writes before a virtual-board fault. Both showed the consecutive frames `80 00 00 00 00 00 00` and `87 00 7F 07 00 7F 00`. The transport treated a runtime zero command as a new calibration request and left READY; the subsequent `0x07` request failed under the pre-READY policy.

Commit `fab8780` preserves READY on the runtime zero request and tests that exact two-channel sequence. It also keeps an unexpected `0x07` during initialization fail-closed. **This correction is supported by live packet evidence and regression tests, but a new live recording must confirm sustained accepted writes and a fault-free final lifecycle.** Prior native callback counts are not a guarantee that transport communication remained healthy.

Recorder loss (28 records in the earlier DEV 5 capture) was independently traced to try-lock contention and corrected before the first AER-04 capture, which had zero dropped raw events.

## Current engineering boundary

- Verified: original game-side steering lifecycle and command generation, two logical SDX slots over one serial endpoint, independent motion readiness, valid packet framing and live original request captures.
- Modeled rather than verified as Sega hardware: virtual replies, analog calibration behavior, firmware interpretations and electrical effects.
- Awaiting live checks: corrected V2 road classification values and the `fab8780` READY-state correction during gameplay.
- Still unknown: motor torque and polarity, firmware waveforms, authentic drive-board response timing, specific kerb/bump sensations, callback-to-motor timing.

No modern wheel input or physical FFB output is enabled by AER. The OutRun 2 SP Arcade Experience player-facing input/FFB modules and independent HYP36rforce work are separate projects. See [Evidence Register](EVIDENCE_REGISTER.md), [Hardware Validation Register](HARDWARE_VALIDATION_REGISTER.md), and [Research History](RESEARCH_HISTORY.md) for the evidence lineage.
