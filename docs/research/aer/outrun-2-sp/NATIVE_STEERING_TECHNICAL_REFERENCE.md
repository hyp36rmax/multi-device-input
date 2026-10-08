# OutRun 2 SP SDX Native Steering Technical Reference

## 1. Research purpose and scope

This document brings together my research into Sega's original steering-feedback system in *OutRun 2 SP SDX Rev A*. Its purpose is to explain the game-side architecture, the state that feeds it, the requests it constructs for the drive board, and the limits of what can be concluded without original board firmware or cabinet measurements.

The central finding is that the game contains a complete native steering-output pipeline. It maintains a continuous steering-magnitude request alongside discrete event-pattern requests, schedules those requests through the cabinet callback, and frames them for one or two drive boards. That is evidence about what the game asks the hardware to do. It is not, by itself, evidence of physical torque, waveform, polarity, or sensation. [AER-EV-EXE-002](EVIDENCE_REGISTER.md#aer-ev-exe-002--native-steering-output-pipeline) [AER-EV-HW-001](EVIDENCE_REGISTER.md#aer-ev-hw-001--unresolved-physical-output)

This reference uses the [Evidence Register](EVIDENCE_REGISTER.md) as its factual foundation and sends questions beyond the static-evidence boundary to the [Hardware Validation Register](HARDWARE_VALIDATION_REGISTER.md). It does not define a modern FFB implementation and does not treat HYP36rforce interpretation as recovered Sega behavior.

## 2. Original executable and asset identity

All executable addresses in this reference apply to:

```text
Game:       OutRun 2 SP SDX Rev A
Revision:   DVP-0015A
Executable: Jennifer
SHA-256:    f16fc04d836a2bd8e401d8f987d4fe694fa16e18a9f870da6623d7f884911075
```

The course investigation used:

```text
Archive:  data.zip
SHA-256: 6d0d6f98ad3ef03b74727a7aaa5e48917b570b0b82691a93ea5582eaf4502526
```

The hashes are part of the evidence boundary. Addresses, tables, and asset relationships must not be assumed to apply to another revision without separately verifying that revision. [AER-EV-EXE-001](EVIDENCE_REGISTER.md#aer-ev-exe-001--verified-executable-identity)

## 3. Native steering architecture

The verified original execution path is:

```text
CabinetCtrl_Main()   0x081048B2–0x08104AD3
    ↓
DrCtrlDataSet()      0x08104F02–0x081051F3
    ↓
SetOutFactor()       0x081052E4–0x081055F1
    ↓
DrCtrlMoveSend()     0x081051F4–0x081052E3
    ↓
steerReqSendA()      0x08105A48–0x08105AD1
    ↓
steerReqSendOut()    0x08105AD2–0x08105BFB
    ↓
hardcomSend()        0x0810735E
```

`DrCtrlDataSet()` derives the continuous magnitude from vehicle and contact state. `SetOutFactor()` selects discrete event patterns and their direction/countdown state. `DrCtrlMoveSend()` arbitrates between the two, giving a pending pattern request priority over the continuous request. The send functions turn the selected logical request into framed serial data. [AER-EV-EXE-002](EVIDENCE_REGISTER.md#aer-ev-exe-002--native-steering-output-pipeline) [AER-EV-PTN-005](EVIDENCE_REGISTER.md#aer-ev-ptn-005--pattern-priority-and-replacement)

The architecture therefore keeps distinct state for:

- continuous magnitude;
- discrete pattern selection;
- pattern direction and callback-count duration;
- dirty or pending output;
- framed transport, outstanding requests, and acknowledgments.

That separation matters. A game-side magnitude of `4–15`, or a pattern request sent with command `0x7B`, is a request to a separate controller. The physical motor output remains firmware- and cabinet-dependent. [AER-EV-MAG-005](EVIDENCE_REGISTER.md#aer-ev-mag-005--no-physical-torque-units) [AER-EV-PTN-007](EVIDENCE_REGISTER.md#aer-ev-ptn-007--physical-pattern-behavior)

## 4. Cabinet initialization and hardware readiness

`CabinetCtrl_InitDriver()` at `0x08103EAA` owns the recovered initialization state machine. It waits for readiness, delays 90 ticks, exchanges initialization and configuration requests, and searches for steering position using analog samples and bounded test levels. Active driver state is state 12. [AER-EV-PROTO-002](EVIDENCE_REGISTER.md#aer-ev-proto-002--initialization-and-calibration)

The executable also contains a motor-power configuration table at `0x081E23A0`:

```text
0x32  0x40  0x50  0x60
```

The game-side choice and transmission of these values are confirmed. Their electrical or mechanical meaning is not. The firmware handler or an original-hardware measurement is required before describing them as current, gain, or torque limits. [AER-HW-POWER-001](HARDWARE_VALIDATION_REGISTER.md)

Normal output depends on completion of readiness and calibration state. Earlier loader captures showed repeated reads without the regular gameplay writes expected from the recovered output path. This makes an initialization bypass or altered readiness transition a strong candidate, but not a proven root cause. [AER-EV-EXE-003](EVIDENCE_REGISTER.md#aer-ev-exe-003--loader-activation-hypothesis) [AER-HW-ACT-001](HARDWARE_VALIDATION_REGISTER.md)

## 5. Original vehicle-state sources

### Front-tire direction

`CalcTireDirection()` at `0x0806774E` converts the front-tire directions from radians to signed-16 angular units:

```text
signed angle = direction in radians × 32768 / π
```

It averages the front-left and front-right values with signed round-toward-zero correction and stores the result at `EVWORK_CAR+0x054`. This is a derived average front-tire direction. It is neither raw player steering nor a torque signal. [AER-EV-STATE-001](EVIDENCE_REGISTER.md#aer-ev-state-001--front-tire-direction-source)

### Tire load

`assDiagonalTireLoad()` at `0x080645DC` calculates diagonal imbalance, redistributes it through four tire-load fields using adjacent coefficients and `EVWORK_CAR+0x46C`, and prevents negative load. The routine does not send feedback itself; `DrCtrlDataSet()` later consumes the resulting front-load relationship. [AER-EV-STATE-002](EVIDENCE_REGISTER.md#aer-ev-state-002--tire-load-redistribution)

### Contact classifications

Per-tire contact fields retain classifications originating in authored collision polygons. The confirmed front-contact fields are `EVWORK_CAR+0x404` and `EVWORK_CAR+0x408`; the current aggregate used by transition logic is at `+0x3FC`. [AER-EV-STATE-003](EVIDENCE_REGISTER.md#aer-ev-state-003--per-tire-polygon-classification)

### Special state `0x1E`

The `SetOutFactor()` branch for state `0x1E` selects Pattern 0 or 2 from wall/collision-related state and direction information. The wall-rebound interpretation is strongly supported, while the physical board response remains unknown. [AER-EV-STATE-004](EVIDENCE_REGISTER.md#aer-ev-state-004--state-0x1e)

## 6. Continuous steering magnitude

The independently established lineage is:

```text
CalcTireDirection()
    ↓
average front-tire direction
    ↓
EVWORK_CAR+0x054
    ↓
DrCtrlDataSet()
    ↓
nonlinear shaping
    ↓
per-front-contact attenuation
    ↓
front tire/suspension-load adjustment
    ↓
integer quantization and clamp
    ↓
native request magnitude 4–15
```

### Nonlinear shaping

For the eligible near-center range, `DrCtrlDataSet()` computes:

```text
sin(15 × abs(EVWORK_CAR+0x054) × π / 32768)
```

Higher values follow branches that resolve to zero after clamping. `EVWORK_CAR+0x3AC` also controls a low-value substitution near `1092.27` angle units; the exact gameplay meaning of that field remains unresolved. [AER-EV-MAG-001](EVIDENCE_REGISTER.md#aer-ev-mag-001--nonlinear-magnitude-curve)

### Contact-dependent attenuation

The shaped result is multiplied by `0.8` for each front contact whose one-hot classification intersects mask `0xFF0FC07D`. The possible combined factors are therefore:

```text
no qualifying front contact: 1.00
one qualifying contact:       0.80
two qualifying contacts:      0.64
```

This is confirmed game-side attenuation, not a statement about the material's physical roughness. [AER-EV-MAG-002](EVIDENCE_REGISTER.md#aer-ev-mag-002--contact-attenuation)

### Load-ratio adjustment

The front-load relationship is:

```text
R = (front load A + front load B)
    / (front coefficient A + front coefficient B)

if R > 1.2:
    integer magnitude += 1
```

This establishes that tire/suspension load contributes to the request. [AER-EV-MAG-003](EVIDENCE_REGISTER.md#aer-ev-mag-003--front-load-adjustment)

### Encoding

The shaped result is multiplied by the fixed game-side scale `10`, truncated toward zero, adjusted for load, and clamped:

```text
result <= 3  → 4
result 4–15  → unchanged
result >= 16 → 15
```

The resulting game-side magnitude is always an integer from `4` through `15`. It must not be converted to torque units without firmware or cabinet evidence. [AER-EV-MAG-004](EVIDENCE_REGISTER.md#aer-ev-mag-004--quantized-native-magnitude) [AER-HW-TORQUE-001](HARDWARE_VALIDATION_REGISTER.md)

## 7. Discrete steering patterns

The game has 16 internal pattern indices. Each index has a board translation and a callback-tick duration, but the current evidence does not establish a normal-gameplay selector for every entry.

| Index | Translation | Duration ticks | Established selection/relationship | Direction source | Priority/replacement | Confidence |
|---:|---:|---:|---|---|---|---|
| 0 | `0x0B` | 8 | State `0x1E` wall-rebound branch | Wall/collision direction state | Reloaded on selection; sent before magnitude | STRONGLY SUPPORTED selector |
| 1 | `0x09` | 16 | No normal selector found | Not established | No normal replacement path found | CONFIRMED traced absence; broader reachability UNKNOWN |
| 2 | `0x00` | 4 | State `0x1E` wall-rebound branch | Wall/collision direction state | Reloaded on selection; sent before magnitude | STRONGLY SUPPORTED selector |
| 3 | `0x00` | 5 | No normal selector found | Not established | No normal replacement path found | CONFIRMED traced absence; broader reachability UNKNOWN |
| 4 | `0x0A` | 8 | No normal selector found | Not established | No normal replacement path found | CONFIRMED traced absence; broader reachability UNKNOWN |
| 5 | `0x08` | 8 | No normal selector found | Not established | No normal replacement path found | CONFIRMED traced absence; broader reachability UNKNOWN |
| 6 | `0x00` | 5 | Selector not preserved in current evidence register | Not established | Pattern rules apply if selected | UNKNOWN in current register |
| 7 | `0x00` | 6 | No normal selector found | Not established | No normal replacement path found | CONFIRMED traced absence; broader reachability UNKNOWN |
| 8 | `0x00` | 4 | Selector not preserved in current evidence register | Not established | Pattern rules apply if selected | UNKNOWN in current register |
| 9 | `0x00` | 5 | Selector not preserved in current evidence register | Not established | Pattern rules apply if selected | UNKNOWN in current register |
| 10 | `0x04` | 6 | Selected classification-family transition | Sign of steering-state difference | Reloads countdown; pattern first | CONFIRMED |
| 11 | `0x00` | 7 | No normal selector found | Not established | No normal replacement path found | CONFIRMED traced absence; broader reachability UNKNOWN |
| 12 | `0x00` | 5 | Front contact mask `0x00008014`; agreement/threshold branch | Branch-dependent; exact formula not retained here | Reloads countdown; pattern first | CONFIRMED family |
| 13 | `0x02` | 6 | Front contact mask `0x00008014`; agreement/threshold branch | Branch-dependent; exact formula not retained here | Reloads countdown; pattern first | CONFIRMED family |
| 14 | `0x00` | 7 | Front contact mask `0x02000008`; agreement/threshold branch | Branch-dependent; exact formula not retained here | Reloads countdown; pattern first | CONFIRMED family |
| 15 | `0x00` | 8 | Front contact mask `0x02000008`; agreement/threshold branch | Branch-dependent; exact formula not retained here | Reloads countdown; pattern first | CONFIRMED family |

The table deliberately avoids physical names. A zero translation is still a recovered table value; it is not proof that the board produces no response. Translation, direction bit, and firmware state must be considered together. [AER-EV-PTN-001](EVIDENCE_REGISTER.md#aer-ev-ptn-001--pattern-translation-table) [AER-EV-PTN-002](EVIDENCE_REGISTER.md#aer-ev-ptn-002--pattern-duration-table) [AER-EV-PTN-006](EVIDENCE_REGISTER.md#aer-ev-ptn-006--unselected-pattern-indices)

### Pattern 10

Pattern 10 is selected when:

```text
current != previous
current  & 0x00F03302 != 0
previous & 0x0200801C != 0
```

Its direction is derived from the sign of a steering-state difference. This proves a transition between selected collision-classification families, not a universal cobblestone effect. [AER-EV-PTN-003](EVIDENCE_REGISTER.md#aer-ev-ptn-003--pattern-10-transition)

### Patterns 12–15

Patterns 12/13 use mask `0x00008014`, corresponding to ordinals 2, 4, and 15. Patterns 14/15 use `0x02000008`, corresponding to ordinals 3 and 25. Selection distinguishes front-tire agreement and a threshold involving `EVWORK_CAR+0x3AC`. [AER-EV-PTN-004](EVIDENCE_REGISTER.md#aer-ev-ptn-004--patterns-1215)

### Priority

Selecting a pattern reloads its countdown and marks pattern output dirty. `DrCtrlMoveSend()` sends the pattern before the continuous request and returns, leaving the magnitude pending. While a pattern countdown remains active, ordinary replacement is suppressed. [AER-EV-PTN-005](EVIDENCE_REGISTER.md#aer-ev-ptn-005--pattern-priority-and-replacement)

## 8. Pattern translation and timing tables

### Translation table

Address `0x081E2380`, indexed 0–15:

```text
0B 09 00 00 0A 08 00 00 00 00 04 00 00 02 00 00
```

The translated value is carried by command `0x7B`; direction is encoded in bit `0x10`. [AER-EV-PTN-001](EVIDENCE_REGISTER.md#aer-ev-ptn-001--pattern-translation-table)

### Duration table

Address `0x081E2260`, indexed 0–15:

```text
8 16 4 5 8 8 5 6 4 5 6 7 5 6 7 8
```

These values count eligible `CabinetCtrl_Main()` callbacks. They are not milliseconds. Converting them to time requires an independently measured callback cadence, and determining whether the board adds its own timeout requires firmware or hardware evidence. [AER-EV-TIME-002](EVIDENCE_REGISTER.md#aer-ev-time-002--callback-tick-pattern-duration) [AER-HW-DURATION-001](HARDWARE_VALIDATION_REGISTER.md)

## 9. Road and collision classifications

The recovered `COLI0105` layout contains:

```text
+0x10  256×256 spatial lookup grid
+0x14  candidate polygon lists
+0x18  one-byte classification ordinals
+0x1C  polygon geometry, stride 0x50
+0x20  additional collision tables
```

Each collision polygon owns one unsigned-byte ordinal. Polygon geometry begins with four XYZ vertices, followed by a stored center and additional polygon data. [AER-EV-COLI-001](EVIDENCE_REGISTER.md#aer-ev-coli-001--coli0105-format) [AER-EV-COLI-002](EVIDENCE_REGISTER.md#aer-ev-coli-002--classification-ownership)

At runtime, the ordinal becomes a one-hot classification:

```text
roadClassification = 1u << classificationOrdinal
```

This explains why native masks select families of authored polygon classifications. The values are retained per tire/contact, allowing front-tire agreement and disagreement to affect pattern selection. [AER-EV-COLI-003](EVIDENCE_REGISTER.md#aer-ev-coli-003--one-hot-conversion) [AER-EV-STATE-003](EVIDENCE_REGISTER.md#aer-ev-state-003--per-tire-polygon-classification)

Collision classification and visible material are related only where asset lineage establishes a relationship. They are separate authored systems; a collision ordinal must not be named from appearance alone.

## 10. Tulip Garden and cross-course findings

Thirty main-course and sixteen branch collision resources were validated. All 46 identify as `SEGA-AM2 OUTRUN2`, have consistent decompressed sizes and bounded monotonic offsets, and match classification counts to polygon counts. [AER-EV-COLI-004](EVIDENCE_REGISTER.md#aer-ev-coli-004--cross-course-validation)

Ordinal 20 appears only in the Tulip, Lake, and Prin main-course assets. In each it forms one localized `1 → 20 → 1` run; it does not appear in the branch resources examined. Its geometry varies too much across those courses to support a universal roughness-magnitude meaning. [AER-EV-COLI-005](EVIDENCE_REGISTER.md#aer-ev-coli-005--cross-course-ordinal-20) [AER-EV-COLI-006](EVIDENCE_REGISTER.md#aer-ev-coli-006--ordinal-20-is-not-roughness-magnitude)

In Tulip Garden, ordinal 20 covers collision polygons 124–162: 39 polygons. Collision and visual XYZ triplets register byte-for-byte without scale, rotation, or translation. The corrected dominant visual owner is:

```text
re_CS_TULI_05_H_BLIDGE
```

The recovered lineage is:

```text
draw batches 22/23
    → material records 13/14
    → texture IDs 0x46/0x47
    → STG_5A_H_BLIDGE_LORD_03/_04
```

Batch 24/material 15 uses `STG_5A_H_BLIDGE_KAGE`; six marking matches use `O2S_XX_K_MICHI_HAKUSEN3` in the ROAD object. Following ordinal-1 roadway uses `O2S_XX_K_MICHI_ASFA5A` and the line texture. [AER-EV-VIS-001](EVIDENCE_REGISTER.md#aer-ev-vis-001--shared-mesh-coordinates) [AER-EV-VIS-002](EVIDENCE_REGISTER.md#aer-ev-vis-002--tulip-ordinal-20-visual-lineage)

The localized early bend, bridge-road imagery, exact mesh alignment, geometry variation, and independently identified physical location strongly support a relationship with Tulip Garden's known cobblestone section. No texture name literally says “cobblestone,” and ordinal 20 must not be generalized as cobblestone across all courses. [AER-EV-VIS-003](EVIDENCE_REGISTER.md#aer-ev-vis-003--cobblestone-interpretation)

## 11. Game-to-drive-board protocol

### Logical requests and framing

The game builds three-byte logical requests. `steerReqSendA()` and `steerReqSendOut()` frame these as:

- four-byte packets for one board;
- seven-byte packets for two boards.

The transport sets bit 7 on the first command byte. The final byte is an XOR of the unmarked command and payload bytes; there is no length field. [AER-EV-PROTO-001](EVIDENCE_REGISTER.md#aer-ev-proto-001--packet-framing)

### Recovered command families

| Command | Confirmed game-side use |
|---:|---|
| `0x7B` | Pattern request |
| `0x0B` | Continuous magnitude/power request |
| `0x06` | Directional/secondary request and initialization use |
| `0x04` | Calibration/test movement |
| `0x7D` | Idle/neutral slot or keepalive |
| zero request | Deactivation/shutdown |

These names describe construction and call sites, not firmware-side motor semantics. [AER-EV-PROTO-003](EVIDENCE_REGISTER.md#aer-ev-proto-003--runtime-command-families)

### Responses

`CabinetCtrlMIDI_Input()` at `0x08105BFC` consumes one response byte per configured board. The low three bits contain status, duplicated status bits are checked, invalid responses become `0xFF`, and `0xEE` remains special. Acknowledgment accounting is count-based rather than command-ID based. The physical or diagnostic meanings of status values remain unresolved. [AER-EV-PROTO-004](EVIDENCE_REGISTER.md#aer-ev-proto-004--response-validation) [AER-HW-RESP-001](HARDWARE_VALIDATION_REGISTER.md)

## 12. Command scheduling and buffering

The recovered game-loop order is:

```text
game_main()             0x0804D772
    → EventControl()    0x0806C4D2
    → CabinetCtrl_Main()
    → DrCtrlDataSet()
    → DrCtrlMoveSend()
    → later CabinetCtrlMIDI_Server()
    → WaitVSync / swap
```

Current-frame vehicle and contact state therefore produces requests before the later transport service. [AER-EV-TIME-001](EVIDENCE_REGISTER.md#aer-ev-time-001--callback-ownership-and-ordering)

`EventControl()` calls an active event only when pause bit `0x08` and suspend bit `0x10` are both clear. Skipping the callback establishes that no eligible update occurs; it does not establish whether the board holds, decays, or stops its last physical output. [AER-EV-TIME-003](EVIDENCE_REGISTER.md#aer-ev-time-003--pause-and-suspension) [AER-HW-PAUSE-001](HARDWARE_VALIDATION_REGISTER.md)

Up to four requests may be outstanding. Additional eight-byte slots enter a 512-entry circular queue, and the MIDI server drains at most four queued packets per service invocation. [AER-EV-PROTO-005](EVIDENCE_REGISTER.md#aer-ev-proto-005--queue-and-backpressure)

Pattern output has first-send priority. Continuous magnitude changes remain pending when a pattern is sent. Idle/neutral and explicit shutdown requests are separate parts of the protocol. No fixed 60 Hz force-update claim follows from this scheduling: one event pass occurs per main-loop iteration, but the executable's `WaitVSync()` only increments a counter and external display/swap behavior may own cadence. [AER-EV-PTN-005](EVIDENCE_REGISTER.md#aer-ev-ptn-005--pattern-priority-and-replacement) [AER-EV-TIME-004](EVIDENCE_REGISTER.md#aer-ev-time-004--update-frequency-boundary)

## 13. Cabinet configuration differences

The executable supports single- and dual-board packet topology. A single-board request is four bytes; a dual-board request is seven bytes and includes an inactive-slot neutral command where applicable. This confirms a configuration distinction in the original protocol, but not the physical role, wiring, or synchronization of the two boards. [AER-EV-PROTO-001](EVIDENCE_REGISTER.md#aer-ev-proto-001--packet-framing) [AER-HW-DUAL-001](HARDWARE_VALIDATION_REGISTER.md)

Similarly, the four recovered motor-power codes establish configuration choices but not their physical scale. Cabinet type, board count, readiness, calibration, and power configuration must be treated as activation inputs, not as interchangeable modern-wheel settings.

## 14. Existing runtime observations

DEV 1–3 captures established that the loader environment could poll the emulated drive-board path and shut down cleanly, but did not show the regular native gameplay writes expected from the static pipeline. The captured response bytes were sufficient to inspect polling and response handling, not to establish original board status meanings or physical behavior.

Static analysis subsequently established:

- the native output pipeline exists;
- initialization and steering-search states exist;
- normal output is gated by readiness/calibration state;
- loader patches alter or bypass parts of the original activation environment.

The resulting activation hypothesis is strong but remains unproven until the exact readiness/state transition through the first successful native write is observed. DEV 4 is available as a targeted research instrument for that question; it is not completed runtime validation. [AER-EV-EXE-003](EVIDENCE_REGISTER.md#aer-ev-exe-003--loader-activation-hypothesis) [AER-HW-ACT-001](HARDWARE_VALIDATION_REGISTER.md)

## 15. Confirmed original behaviors

The following conclusions are confirmed for the verified executable and assets:

- A complete game-side steering-output pipeline exists. [AER-EV-EXE-002](EVIDENCE_REGISTER.md#aer-ev-exe-002--native-steering-output-pipeline)
- The continuous source begins with derived average front-tire direction, not raw input. [AER-EV-STATE-001](EVIDENCE_REGISTER.md#aer-ev-state-001--front-tire-direction-source)
- The game applies nonlinear shaping, per-front-contact attenuation, load adjustment, integer quantization, and a `4–15` clamp. [AER-EV-MAG-001](EVIDENCE_REGISTER.md#aer-ev-mag-001--nonlinear-magnitude-curve) [AER-EV-MAG-002](EVIDENCE_REGISTER.md#aer-ev-mag-002--contact-attenuation) [AER-EV-MAG-003](EVIDENCE_REGISTER.md#aer-ev-mag-003--front-load-adjustment) [AER-EV-MAG-004](EVIDENCE_REGISTER.md#aer-ev-mag-004--quantized-native-magnitude)
- Collision classifications are authored per polygon, converted to one-hot values, and retained per tire/contact. [AER-EV-COLI-002](EVIDENCE_REGISTER.md#aer-ev-coli-002--classification-ownership) [AER-EV-COLI-003](EVIDENCE_REGISTER.md#aer-ev-coli-003--one-hot-conversion)
- Discrete patterns have recovered translation and callback-duration tables. [AER-EV-PTN-001](EVIDENCE_REGISTER.md#aer-ev-ptn-001--pattern-translation-table) [AER-EV-PTN-002](EVIDENCE_REGISTER.md#aer-ev-ptn-002--pattern-duration-table)
- Pattern output takes priority over pending continuous magnitude output. [AER-EV-PTN-005](EVIDENCE_REGISTER.md#aer-ev-ptn-005--pattern-priority-and-replacement)
- Single- and dual-board framing, XOR, response validation, and queue behavior are recovered. [AER-EV-PROTO-001](EVIDENCE_REGISTER.md#aer-ev-proto-001--packet-framing) [AER-EV-PROTO-004](EVIDENCE_REGISTER.md#aer-ev-proto-004--response-validation) [AER-EV-PROTO-005](EVIDENCE_REGISTER.md#aer-ev-proto-005--queue-and-backpressure)
- Pause and suspension suppress eligible cabinet callbacks. [AER-EV-TIME-003](EVIDENCE_REGISTER.md#aer-ev-time-003--pause-and-suspension)

## 16. Unresolved hardware interpretation

The executable ends at the board request boundary. The following questions remain open and are deliberately not filled with modern-wheel assumptions:

- physical torque for magnitude `4–15`: [AER-HW-TORQUE-001](HARDWARE_VALIDATION_REGISTER.md)
- motor left/right polarity: [AER-HW-DIR-001](HARDWARE_VALIDATION_REGISTER.md)
- firmware waveform selected by each translated pattern: [AER-HW-PATTERN-001](HARDWARE_VALIDATION_REGISTER.md)
- firmware-owned versus game-owned duration: [AER-HW-DURATION-001](HARDWARE_VALIDATION_REGISTER.md)
- meaning of motor-power codes: [AER-HW-POWER-001](HARDWARE_VALIDATION_REGISTER.md)
- command-to-actuation latency: [AER-HW-LATENCY-001](HARDWARE_VALIDATION_REGISTER.md)
- authoritative callback cadence: [AER-HW-CADENCE-001](HARDWARE_VALIDATION_REGISTER.md)
- original calibration motion and criteria: [AER-HW-CAL-001](HARDWARE_VALIDATION_REGISTER.md)
- response and alarm meanings: [AER-HW-RESP-001](HARDWARE_VALIDATION_REGISTER.md) and [AER-HW-ALARM-001](HARDWARE_VALIDATION_REGISTER.md)

The safest validation order is activation, callback cadence, firmware/service evidence, one isolated pattern, bounded direction, bounded torque/power, and synchronized latency. Broad hardware testing should not precede those narrower questions.

## 17. Evidence and reproduction references

The authoritative indexes are:

- [Arcade Experience Research — Evidence Register](EVIDENCE_REGISTER.md)
- [Arcade Experience Research — Hardware Validation Register](HARDWARE_VALIDATION_REGISTER.md)

Reproduction begins by verifying the exact executable and asset hashes in section 2. Static executable claims should be checked at their listed function or table addresses. Asset claims should be reproduced from the validated `COLI0105` resources and registered visual geometry, preserving original coordinates and ordinal ownership.

Major confidence terms retain the register definitions:

- **CONFIRMED** — directly established by original code, data, or reproducible observation.
- **STRONGLY SUPPORTED** — multiple sources agree but one link remains indirect.
- **POTENTIAL** — credible, incomplete hypothesis.
- **NOT ESTABLISHED** — available evidence does not support the proposed conclusion.
- **UNKNOWN** — behavior has not been recovered or observed.

## 18. Research corrections and history

This investigation became more useful as several early interpretations were corrected:

| Earlier interpretation | Evidence-backed correction | Evidence |
|---|---|---|
| `EVWORK_CAR+0x054` might be raw steering state | It is derived average front-tire direction | AER-EV-STATE-001 |
| State `0x1E` might represent drift/recovery | It belongs to a wall-rebound path | AER-EV-STATE-004 |
| Contact fields were generic vehicle flags | They are per-tire collision-polygon classifications | AER-EV-STATE-003, AER-EV-COLI-001–003 |
| Pattern 10 was a generic transition | It uses exact current/previous classification-family masks | AER-EV-PTN-003 |
| Tulip ordinal 20 was generic ROAD or rough-road data | It is a localized bridge-road region, dominated by `re_CS_TULI_05_H_BLIDGE` | AER-EV-VIS-002 |
| Ordinal 20 universally meant cobblestone or roughness | It is a localized contact category whose universal material meaning is not established | AER-EV-COLI-005–006, AER-EV-VIS-003 |
| Pattern durations were real-time values | They are eligible callback ticks | AER-EV-TIME-002 |
| Magnitude `4–15` might be a torque scale | It is a game-side request scale; torque is not established | AER-EV-MAG-004–005 |
| Missing loader writes might mean native FFB was absent | The native pipeline is complete; loader activation remains unresolved | AER-EV-EXE-002–003 |

These corrections are part of the result, not mistakes to hide. They show why the project keeps game state, game-generated requests, hardware response, research interpretation, and future HYP36rforce design as separate layers.
