# Arcade Experience Research — Evidence Register

This register is the factual foundation for the OutRun 2 SP SDX Arcade Experience Research (AER) documentation. It records what the original game and assets show, what we infer from that evidence, and where the original steering hardware is still needed.

It covers OutRun 2 SP SDX Rev A (`DVP-0015A`) and the verified original executable:

```text
Jennifer
SHA-256: f16fc04d836a2bd8e401d8f987d4fe694fa16e18a9f870da6623d7f884911075
```

The original asset collection used for course analysis is:

```text
data.zip
SHA-256: 6d0d6f98ad3ef03b74727a7aaa5e48917b570b0b82691a93ea5582eaf4502526
```

## How to read this register

Evidence IDs are permanent. New findings receive new IDs; existing IDs are not renumbered.

Confidence terms:

- **CONFIRMED** — directly established by original code, data, or a reproducible observation.
- **STRONGLY SUPPORTED** — multiple pieces of evidence support the interpretation, but one link remains indirect.
- **POTENTIAL** — credible hypothesis with incomplete supporting evidence.
- **NOT ESTABLISHED** — evidence does not currently support the proposed conclusion.
- **UNKNOWN** — the relevant behavior has not been observed or recovered.

The register distinguishes five layers:

1. **Original game state** — values calculated or stored by the game.
2. **Original game-generated request** — bytes or logical requests sent toward the drive board.
3. **Original hardware response** — physical behavior produced by board firmware and the motor.
4. **Research interpretation** — our explanation of the original evidence.
5. **Future HYP36rforce interpretation** — a separate implementation decision, not original-game evidence.

Game-generated requests are never treated as proof of a particular torque, waveform, or physical sensation. Hardware-dependent questions are tracked in the [Hardware Validation Register](HARDWARE_VALIDATION_REGISTER.md).

## Evidence index

| Evidence ID | Milestone | Layer | Finding | Confidence | Public use |
|---|---|---|---|---|---|
| AER-EV-EXE-001 | J.1 | Game state | Verified executable identity | CONFIRMED | Both references |
| AER-EV-EXE-002 | J.1/K.9 | Game request | Native steering-output pipeline exists | CONFIRMED | Both references |
| AER-EV-EXE-003 | J.1 | Research interpretation | Loader activation bypass may prevent normal output | STRONGLY SUPPORTED | Technical, qualified summary |
| AER-EV-STATE-001 | K.1/K.2/K.9 | Game state | `EVWORK_CAR+0x054` is average front-tire direction | CONFIRMED | Both references |
| AER-EV-STATE-002 | K.9 | Game state | `assDiagonalTireLoad()` redistributes tire loads | CONFIRMED | Both references |
| AER-EV-STATE-003 | K.4/K.9 | Game state | Tire contact fields contain polygon classifications | CONFIRMED | Both references |
| AER-EV-STATE-004 | K.3/L.1 | Game state | State `0x1E` belongs to wall rebound handling | STRONGLY SUPPORTED | Both references, qualified |
| AER-EV-MAG-001 | K.9 | Game request | Continuous request uses a bounded sine curve | CONFIRMED | Technical; simplified publicly |
| AER-EV-MAG-002 | K.9 | Game request | Front contact classes apply per-wheel attenuation | CONFIRMED | Both references |
| AER-EV-MAG-003 | K.9 | Game request | Tire-load ratio can increment magnitude | CONFIRMED | Both references |
| AER-EV-MAG-004 | K.9 | Game request | Native magnitude is quantized to `4–15` | CONFIRMED | Both references |
| AER-EV-MAG-005 | K.9 | Hardware response | Magnitude `4–15` is not a torque measurement | NOT ESTABLISHED | Both references as limitation |
| AER-EV-PTN-001 | K.3/K.9 | Game request | Pattern translation table recovered | CONFIRMED | Technical; summarized publicly |
| AER-EV-PTN-002 | K.3/K.9 | Game request | Pattern-duration table recovered | CONFIRMED | Technical; summarized publicly |
| AER-EV-PTN-003 | K.4/K.9 | Game request | Pattern 10 is contact-classification-transition driven | CONFIRMED | Both references |
| AER-EV-PTN-004 | K.4/K.9 | Game request | Patterns 12–15 depend on front-tire classifications | CONFIRMED | Both references |
| AER-EV-PTN-005 | K.9 | Game request | Pattern output has priority over magnitude output | CONFIRMED | Both references |
| AER-EV-PTN-006 | K.9 | Game request | Patterns 1, 3, 4, 5, 7, 11 lack a normal selector | CONFIRMED | Technical |
| AER-EV-PTN-007 | K.9 | Hardware response | Pattern physical meanings remain unknown | UNKNOWN | Both references as limitation |
| AER-EV-COLI-001 | K.5/K.7 | Game asset | `COLI0105` collision layout recovered | CONFIRMED | Technical; simplified publicly |
| AER-EV-COLI-002 | K.5/K.7 | Game asset | Classification is a one-byte ordinal per polygon | CONFIRMED | Both references |
| AER-EV-COLI-003 | K.4/K.7 | Game state | Ordinal becomes one-hot classification bit | CONFIRMED | Both references |
| AER-EV-COLI-004 | K.9 | Game asset | Forty-six collision resources validated | CONFIRMED | Both references |
| AER-EV-COLI-005 | K.9 | Research interpretation | Ordinal 20 is a localized special-contact segment | CONFIRMED | Both references |
| AER-EV-COLI-006 | K.9 | Research interpretation | Ordinal 20 is not a universal roughness magnitude | CONFIRMED | Both references |
| AER-EV-VIS-001 | K.8/K.9 | Game asset | Collision and visual meshes share coordinates | CONFIRMED | Both references |
| AER-EV-VIS-002 | K.9 | Game asset | Tulip ordinal 20 belongs primarily to `BLIDGE` roadway | CONFIRMED | Both references |
| AER-EV-VIS-003 | K.9 | Research interpretation | Tulip ordinal 20 matches the known cobblestone location | STRONGLY SUPPORTED | Both references, qualified |
| AER-EV-PROTO-001 | K.3/K.9 | Game request | Single/dual packet framing and XOR recovered | CONFIRMED | Both references |
| AER-EV-PROTO-002 | J.1/K.9 | Game request | Initialization and steering calibration recovered | CONFIRMED | Both references |
| AER-EV-PROTO-003 | K.9 | Game request | Runtime command families recovered | CONFIRMED | Technical; summarized publicly |
| AER-EV-PROTO-004 | K.9 | Game request | Response validation and count-based acknowledgments | CONFIRMED | Technical |
| AER-EV-PROTO-005 | K.9 | Game request | Four-outstanding/512-entry queue behavior | CONFIRMED | Technical |
| AER-EV-TIME-001 | K.9 | Game state | Cabinet output runs through the event callback | CONFIRMED | Both references |
| AER-EV-TIME-002 | K.9 | Game request | Pattern duration is measured in callback ticks | CONFIRMED | Both references |
| AER-EV-TIME-003 | K.9 | Game state | Pause/suspend suppress eligible callbacks | CONFIRMED | Technical |
| AER-EV-TIME-004 | K.9 | Research interpretation | Authoritative update frequency is not established | NOT ESTABLISHED | Both references as limitation |
| AER-EV-HW-001 | K.9/L.1 | Hardware response | Torque, waveform and motor direction remain unresolved | UNKNOWN | Both references as limitation |

## Executable and control-flow evidence

### AER-EV-EXE-001 — Verified executable identity

- **Milestone:** J.1
- **Source:** Original `Jennifer` executable
- **Identity:** SHA-256 `f16fc04d836a2bd8e401d8f987d4fe694fa16e18a9f870da6623d7f884911075`
- **Observation:** Static addresses and data tables in this register resolve in this exact executable.
- **Conclusion:** Results apply to OutRun 2 SP SDX Rev A (`DVP-0015A`) with this identity.
- **Confidence:** **CONFIRMED**
- **Uncertainty:** Other revisions may differ.
- **Reproduction:** Hash the executable before using the listed addresses.

### AER-EV-EXE-002 — Native steering-output pipeline

- **Milestones:** J.1, K.9
- **Source:** Executable call graph
- **Functions:**

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

- **Observation:** The original game calculates steering state, selects continuous and event requests, frames packets, and reaches the serial-write owner.
- **Conclusion:** A complete original game-side steering-output system exists.
- **Confidence:** **CONFIRMED**
- **Uncertainty:** Successful activation in the loader environment remains unresolved.

### AER-EV-EXE-003 — Loader activation hypothesis

- **Milestone:** J.1
- **Sources:** Original initialization flow, loader patch sites, AER captures
- **Observation:** The game requires readiness and calibration states before normal output. Earlier loader captures showed reads without the expected gameplay writes.
- **Previous interpretation:** Missing writes might mean the game did not implement native output.
- **Current interpretation:** The native output path is complete; loader-side initialization bypass or altered readiness is a strong candidate for preventing activation.
- **Confidence:** **STRONGLY SUPPORTED**
- **Uncertainty:** The cause has not been demonstrated by a successful runtime activation comparison.

## Native state and continuous magnitude

### AER-EV-STATE-001 — Front-tire direction source

- **Milestones:** K.1, K.2, K.9
- **Function:** `CalcTireDirection()` at `0x0806774E`
- **State:** `EVWORK_CAR+0x054`
- **Observation:** Front tire directions are converted from radians to signed-16 angle units using `angle × 32768 / π`; the two front values are averaged with signed round-toward-zero correction.
- **Transformation:**

  ```text
  EVWORK_CAR+0x054 = mean(front-left tire direction,
                           front-right tire direction)
  ```

- **Previous interpretation:** Potential raw steering-related value.
- **Current interpretation:** Derived average of the front-tire directions.
- **Confidence:** **CONFIRMED**
- **Uncertainty:** It is not a torque value and is not raw player input.

### AER-EV-STATE-002 — Tire-load redistribution

- **Milestone:** K.9
- **Function:** `assDiagonalTireLoad()` at `0x080645DC`
- **Observation:** The function calculates a diagonal imbalance and redistributes it through four tire-load fields using adjacent coefficients and `EVWORK_CAR+0x46C`, then prevents negative tire load.
- **Relationship:** It does not send FFB directly; `DrCtrlDataSet()` later consumes the affected front-load ratio.
- **Confidence:** **CONFIRMED**

### AER-EV-STATE-003 — Per-tire polygon classification

- **Milestones:** K.4, K.9
- **State:** Front contact classifications at `EVWORK_CAR+0x404` and `+0x408`; current aggregate at `+0x3FC`
- **Observation:** Values originate in authored collision-polygon ordinal data and are retained per tire/contact.
- **Previous interpretation:** Generic vehicle-control flags.
- **Current interpretation:** Per-tire collision-polygon classifications.
- **Confidence:** **CONFIRMED**

### AER-EV-STATE-004 — State `0x1E`

- **Milestones:** K.3, L.1
- **Function:** `SetOutFactor()` special-state branch
- **Observation:** The branch selects Pattern 0 or 2 using wall/collision-related state and direction information.
- **Previous interpretation:** Possible drift or recovery state.
- **Current interpretation:** Wall-collision rebound state.
- **Confidence:** **STRONGLY SUPPORTED**
- **Uncertainty:** The exact physical rebound produced by the board is unknown.

### AER-EV-MAG-001 — Nonlinear magnitude curve

- **Milestone:** K.9
- **Function:** `DrCtrlDataSet()` at `0x08104F02`
- **Source:** Absolute `EVWORK_CAR+0x054`
- **Transformation:** For the eligible near-center range:

  ```text
  sin(15 × abs(frontTireAverage) × π / 32768)
  ```

  Higher values pass through branches that resolve to zero after clamping. State `EVWORK_CAR+0x3AC` also controls a low-value substitution near `1092.27` angle units.
- **Conclusion:** The continuous request is a bounded nonlinear near-center response.
- **Confidence:** **CONFIRMED**
- **Uncertainty:** Exact gameplay semantics of `+0x3AC` remain unresolved.

### AER-EV-MAG-002 — Contact attenuation

- **Milestone:** K.9
- **Function:** `DrCtrlDataSet()`
- **Observation:** The result is multiplied by `0.8` once for each front contact intersecting mask `0xFF0FC07D`.
- **Possible factors:** `1.0`, `0.8`, `0.64`
- **Conclusion:** Selected tire-contact classes attenuate the continuous request independently per front tire.
- **Confidence:** **CONFIRMED**

### AER-EV-MAG-003 — Front-load adjustment

- **Milestone:** K.9
- **Function:** `DrCtrlDataSet()`
- **Transformation:**

  ```text
  R = (front load A + front load B)
      / (front coefficient A + front coefficient B)

  if R > 1.2: integer magnitude += 1
  ```

- **Conclusion:** Tire/suspension load affects native steering magnitude.
- **Confidence:** **CONFIRMED**

### AER-EV-MAG-004 — Quantized native magnitude

- **Milestone:** K.9
- **Observation:** The shaped value is multiplied by a fixed game-side scale of 10, truncated toward zero, load-adjusted, and clamped.
- **Transformation:**

  ```text
  result <= 3  → 4
  result 4–15  → unchanged
  result >= 16 → 15
  ```

- **Conclusion:** Native game-side magnitude is an integer in the range `4–15`.
- **Confidence:** **CONFIRMED**

### AER-EV-MAG-005 — No physical torque units

- **Milestones:** K.9, L.1
- **Observation:** Static code establishes the request values but not board firmware, current, motor torque, or cabinet mechanics.
- **Conclusion:** Mapping `4–15` to physical torque is **NOT ESTABLISHED**.
- **Previous interpretation corrected:** A native request magnitude must not be described as measured torque.
- **Reproduction:** Requires firmware or original-hardware measurement; see `AER-HW-TORQUE-001`.

## Discrete pattern evidence

### AER-EV-PTN-001 — Pattern translation table

- **Milestones:** K.3, K.9
- **Address:** `0x081E2380`
- **Values by internal pattern index 0–15:**

  ```text
  0B 09 00 00 0A 08 00 00 00 00 04 00 00 02 00 00
  ```

- **Relationship:** Command `0x7B` receives the translated value, with direction encoded as bit `0x10`.
- **Confidence:** **CONFIRMED**
- **Uncertainty:** Translated values do not identify physical waveforms.

### AER-EV-PTN-002 — Pattern duration table

- **Milestones:** K.3, K.9
- **Address:** `0x081E2260`
- **Values by index 0–15:**

  ```text
  8 16 4 5 8 8 5 6 4 5 6 7 5 6 7 8
  ```

- **Relationship:** Values count eligible `CabinetCtrl_Main()` callbacks.
- **Confidence:** **CONFIRMED**
- **Previous interpretation:** Potential real-time duration.
- **Current interpretation:** Callback ticks; milliseconds are not established.

### AER-EV-PTN-003 — Pattern 10 transition

- **Milestones:** K.4, K.9
- **Function:** `SetOutFactor()`
- **Condition:**

  ```text
  current != previous
  current  & 0x00F03302 != 0
  previous & 0x0200801C != 0
  ```

- **Direction:** Derived from the sign of a steering-state difference.
- **Previous interpretation:** Unidentified vehicle-state transition.
- **Current interpretation:** Transition between selected contact-classification families.
- **Confidence:** **CONFIRMED**

### AER-EV-PTN-004 — Patterns 12–15

- **Milestones:** K.4, K.9
- **Masks:**

  ```text
  Patterns 12/13: 0x00008014 → ordinals 2, 4, 15
  Patterns 14/15: 0x02000008 → ordinals 3, 25
  ```

- **Observation:** Selection distinguishes front-tire agreement and a threshold involving `EVWORK_CAR+0x3AC`.
- **Conclusion:** Original pattern selection responds to per-front-tire contact and whether the two front tires agree.
- **Confidence:** **CONFIRMED**

### AER-EV-PTN-005 — Pattern priority and replacement

- **Milestone:** K.9
- **Functions:** `SetOutFactor()`, `DrCtrlMoveSend()`
- **Observation:** A selected pattern reloads its countdown and marks pattern output dirty. Pattern output is sent before continuous magnitude and returns immediately, leaving magnitude pending.
- **Conclusion:** Event patterns temporarily take priority over continuous steering requests.
- **Confidence:** **CONFIRMED**

### AER-EV-PTN-006 — Unselected pattern indices

- **Milestone:** K.9
- **Observation:** Patterns 1, 3, 4, 5, 7, and 11 have translation/duration entries, but no normal `SetOutFactor()` selector was found.
- **Conclusion:** Normal gameplay reachability and semantics remain unresolved.
- **Confidence:** **CONFIRMED** for the traced selector; broader reachability is **UNKNOWN**.

### AER-EV-PTN-007 — Physical pattern behavior

- **Milestone:** K.9
- **Observation:** The executable constructs indices, translated values, direction and callback durations, but does not expose board firmware motor behavior.
- **Conclusion:** Physical pattern waveforms and torque are **UNKNOWN**.
- **Reproduction:** See `AER-HW-PATTERN-001` and `AER-HW-DURATION-001`.

## Collision and visual-asset evidence

### AER-EV-COLI-001 — `COLI0105` format

- **Milestones:** K.5, K.7
- **Assets:** Original `coli_cs_*` and `coli_bk_*` resources
- **Header relationships:**

  ```text
  +0x10 → 256×256 spatial lookup grid
  +0x14 → candidate polygon lists
  +0x18 → one-byte classification ordinals
  +0x1C → polygon geometry, stride 0x50
  +0x20 onward → additional collision tables
  ```

- **Geometry:** Four XYZ vertices followed by a stored center and additional polygon data.
- **Confidence:** **CONFIRMED**

### AER-EV-COLI-002 — Classification ownership

- **Milestones:** K.5, K.7
- **Observation:** Each collision polygon has one unsigned byte classification ordinal.
- **Conclusion:** Classifications are authored per polygon, not assigned once per stage.
- **Confidence:** **CONFIRMED**

### AER-EV-COLI-003 — One-hot conversion

- **Milestones:** K.4, K.7
- **Transformation:**

  ```text
  roadClassification = 1u << classificationOrdinal
  ```

- **Conclusion:** Native masks select families of polygon classifications.
- **Confidence:** **CONFIRMED**

### AER-EV-COLI-004 — Cross-course validation

- **Milestone:** K.9
- **Source:** 30 main-course and 16 branch collision resources
- **Observation:** All 46 identify as `SEGA-AM2 OUTRUN2`, match declared decompressed sizes, have bounded monotonic offsets, and match classification to polygon counts.
- **Conclusion:** The recovered format and ordinal extraction apply across the available original course set.
- **Confidence:** **CONFIRMED**

### AER-EV-COLI-005 — Cross-course ordinal 20

- **Milestone:** K.9
- **Observation:** Ordinal 20 occurs only in Tulip, Lake and Prin main-course assets, always as one localized `1 → 20 → 1` run; no branch resource contains it.
- **Conclusion:** Ordinal 20 is a localized authored contact segment eligible for Pattern 10.
- **Confidence:** **CONFIRMED**

### AER-EV-COLI-006 — Ordinal 20 is not roughness magnitude

- **Milestone:** K.9
- **Observation:** Ordinal-20 height variation differs substantially among Tulip, Lake and Prin.
- **Previous interpretation:** Possible universal rough-road/cobblestone value.
- **Current interpretation:** Structural special-contact category; universal material and roughness meanings are not established.
- **Confidence:** **CONFIRMED**

### AER-EV-VIS-001 — Shared mesh coordinates

- **Milestones:** K.8, K.9
- **Assets:** `coli_cs_tuli_bin.gz`, `cs_cs_tuli_xmt.gz`
- **Observation:** Collision XYZ triplets occur byte-for-byte in the visual XMT mesh without scale, rotation or translation.
- **Conclusion:** Visual and collision geometry share the same coordinate system.
- **Confidence:** **CONFIRMED**

### AER-EV-VIS-002 — Tulip ordinal-20 visual lineage

- **Milestone:** K.9
- **Collision:** Ordinal 20, polygons 124–162, 39 polygons
- **Visual owner:** `re_CS_TULI_05_H_BLIDGE`
- **Lineage:**

  ```text
  draw batches 22/23
      → material records 13/14
      → texture IDs 0x46/0x47
      → STG_5A_H_BLIDGE_LORD_03/_04
  ```

- **Additional overlap:** Batch 24/material 15 uses `STG_5A_H_BLIDGE_KAGE`; six marking matches use `O2S_XX_K_MICHI_HAKUSEN3` in the ROAD object.
- **Adjacent road:** Following ordinal-1 geometry uses `O2S_XX_K_MICHI_ASFA5A` and the line texture.
- **Correction:** Earlier generic ROAD ownership was incomplete; dominant ownership is the bridge roadway.
- **Confidence:** **CONFIRMED**

### AER-EV-VIS-003 — Cobblestone interpretation

- **Milestones:** K.7, K.8, K.9
- **Evidence:** Localized early bend, distinct bridge-road imagery, exact mesh registration, increased Tulip geometry variation, and independently identified cobblestone location.
- **Conclusion:** Relationship to Tulip Garden's known cobblestone section is **STRONGLY SUPPORTED**.
- **Not established:** No original texture name literally identifies cobblestone; ordinal 20 is not universally proven to mean cobblestone.

## Protocol evidence

### AER-EV-PROTO-001 — Packet framing

- **Milestones:** K.3, K.9
- **Functions:** `steerReqSendA()` `0x08105A48–0x08105AD1`; `steerReqSendOut()` `0x08105AD2–0x08105BFB`
- **Observation:** Single-board packets are four bytes; dual-board packets are seven bytes. The first command byte has bit 7 set for transport. The final byte is XOR of unmarked command and payload bytes. There is no length field.
- **Confidence:** **CONFIRMED**

### AER-EV-PROTO-002 — Initialization and calibration

- **Milestones:** J.1, K.9
- **Function:** `CabinetCtrl_InitDriver()` at `0x08103EAA`
- **Observation:** The state machine waits for readiness, delays 90 ticks, exchanges initialization/configuration requests, then searches steering position using analog samples and bounded test levels before reaching active driver state 12.
- **Motor-power table:** `0x32, 0x40, 0x50, 0x60` at `0x081E23A0`.
- **Confidence:** **CONFIRMED** for game-side requests; physical calibration behavior is unresolved.

### AER-EV-PROTO-003 — Runtime command families

- **Milestone:** K.9
- **Game-side relationships:**

  | Command | Game-side use |
  |---:|---|
  | `0x7B` | Pattern request |
  | `0x0B` | Continuous magnitude/power request |
  | `0x06` | Directional/secondary request and initialization use |
  | `0x04` | Calibration/test movement |
  | `0x7D` | Idle/neutral slot or keepalive |
  | zero request | Deactivation/shutdown |

- **Confidence:** **CONFIRMED** for construction and call sites.
- **Uncertainty:** Board-side motor meanings remain unknown.

### AER-EV-PROTO-004 — Response validation

- **Milestone:** K.9
- **Function:** `CabinetCtrlMIDI_Input()` at `0x08105BFC`
- **Observation:** One response byte is consumed per configured board. Low three bits are status; duplicated status bits are checked; invalid responses become `0xFF`; `0xEE` remains special. Acknowledgment accounting is count-based, not command-ID based.
- **Confidence:** **CONFIRMED**
- **Uncertainty:** Physical alarm causes for runtime statuses 1–3 are unknown.

### AER-EV-PROTO-005 — Queue and backpressure

- **Milestone:** K.9
- **Observation:** Up to four requests may be outstanding. Additional eight-byte slots enter a 512-entry circular queue. The MIDI server drains at most four queued packets per service invocation.
- **Confidence:** **CONFIRMED**

## Timing evidence

### AER-EV-TIME-001 — Callback ownership and ordering

- **Milestone:** K.9
- **Functions:** `game_main()` `0x0804D772`; `EventControl()` `0x0806C4D2`; `CabinetCtrl_Main()` `0x081048B2`
- **Order:**

  ```text
  EventControl
      → CabinetCtrl_Main
      → DrCtrlDataSet
      → DrCtrlMoveSend
      → later CabinetCtrlMIDI_Server
      → WaitVSync / swap
  ```

- **Conclusion:** Current-frame vehicle/contact state generates requests before the later transport-service step.
- **Confidence:** **CONFIRMED**

### AER-EV-TIME-002 — Callback-tick pattern duration

- **Milestone:** K.9
- **Observation:** `SetOutFactor()` decrements the active pattern countdown once per eligible callback and suppresses ordinary replacement while it remains active.
- **Previous interpretation:** Fixed real-time effect duration.
- **Current interpretation:** Eligible callback invocations.
- **Confidence:** **CONFIRMED**

### AER-EV-TIME-003 — Pause and suspension

- **Milestone:** K.9
- **Observation:** `EventControl()` calls an active event only when pause bit `0x08` and suspend bit `0x10` are both clear.
- **Conclusion:** Pause/suspend suppress the cabinet callback.
- **Confidence:** **CONFIRMED**
- **Uncertainty:** Whether the hardware retains its last physical state until another request is sent remains unknown.

### AER-EV-TIME-004 — Update frequency boundary

- **Milestone:** K.9
- **Observation:** There is one event pass per main-loop iteration, but this executable's `WaitVSync()` only increments a counter. External display/swap behavior may determine cadence.
- **Correction:** Earlier approximately 60 Hz serial-read polling is not proof of FFB update frequency.
- **Conclusion:** Authoritative callback frequency is **NOT ESTABLISHED** statically.

## Hardware boundary

### AER-EV-HW-001 — Unresolved physical output

- **Milestones:** K.9, L.1
- **Known:** Original state sources, request construction, pattern tables, framing, scheduling and initialization are substantially reconstructed.
- **Unknown:** Torque, motor direction, pattern waveform, firmware-owned duration, latency and electrical behavior.
- **Conclusion:** These physical meanings require firmware or original-hardware evidence and must not be inferred from command names or numeric values.
- **Confidence:** **UNKNOWN** for physical behavior.
- **Next reference:** [Hardware Validation Register](HARDWARE_VALIDATION_REGISTER.md)

## Corrected interpretations at a glance

| Topic | Earlier interpretation | Current interpretation | Evidence that changed it |
|---|---|---|---|
| Steering source | Raw steering-related value | Derived average front-tire direction | `CalcTireDirection()` writer lineage |
| State `0x1E` | Drift/recovery candidate | Wall-rebound path | `SetOutFactor()` collision branch |
| Contact fields | Generic flags | Per-tire polygon classifications | `COLI0105` lineage and mask consumers |
| Pattern 10 | Generic state transition | Selected classification-family transition | Exact current/previous masks |
| Tulip ordinal 20 | Generic rough-road candidate | Distinct localized bridge-road region | Collision/XMT/material registration |
| Ordinal 20 globally | Universal cobblestone/roughness | Localized contact segment; material varies or remains unresolved | Cross-course comparison |
| Pattern duration | Real-time duration | Callback ticks | Countdown owner and event scheduling |
| Magnitude `4–15` | Possible torque scale | Game-side request scale only | Protocol boundary; no firmware evidence |
| Missing writes | Native FFB may be absent | Native pipeline exists; activation remains unresolved | Complete call graph and initialization state machine |

## Future documentation rule

The eventual human-friendly and technical references must derive factual claims from this register. Future HYP36rforce or Arcade-profile design decisions should cite applicable AER evidence but must be documented as independent interpretations, not as recovered Sega behavior.
