# Arcade Experience Research History

## Why the investigation began

This research started with a practical mystery: *OutRun 2 SP SDX* appeared to poll a drive-board interface, but the first captures did not contain the regular outbound steering traffic we expected. At that point, several explanations were possible. The game might not have been reaching its native output path, the loader might have been routing traffic elsewhere, or the recorder might simply have been watching the wrong boundary.

The final architecture was not obvious from the beginning. Each stage narrowed the question and corrected part of the working model.

## AER-01C — first passive recorder

The first isolated recorder was introduced by commit `21aff02` (`Add passive AER drive-board transport recorder`). Its purpose was deliberately narrow: preserve raw reads and writes without interpreting them as force feedback.

DEV 1 produced a clean capture dominated by reads. It proved the game was interacting with the emulated endpoint, but it did not establish active gameplay participation or explain the absence of regular writes. The important lesson was that polling activity alone did not prove that steering output had activated.

Commit `de25b62` repaired recorder platform builds and artifact validation without changing the recorder schema or semantics. DEV 2 then expanded the observation boundary rather than assigning meanings to bytes.

## AER-01E through AER-01G — coverage before interpretation

The outbound-path audit traced the loader's serial interception, descriptor classification, and possible bypass paths. That work showed why recorder coverage had to include more than the first read/write wrapper.

Commit `953293a` (`Expand passive AER serial transport coverage`) extended the raw observation surface. The subsequent linkage repair at `c5e3ff2` allowed the C recorder API to be called safely from the Windows C++ bridge.

This stage corrected an early temptation: zero captured writes did not mean the original game lacked a steering system. It meant the observed runtime had not yet demonstrated one.

## AER-01H through AER-01I — activation and readiness

The next question was whether the original game's cabinet initialization and readiness gates were completing. Commit `22ad1c4` added passive activation diagnostics. Platform-only repairs followed at `ce4900c` and `8b22490`, preserving both research schemas and their behavior.

DEV 3 made readiness, patch state, lifecycle, and first-write observation more explicit. It still did not prove the exact activation failure. The evidence supported a narrower hypothesis: loader patches or altered readiness/calibration flow might prevent the native path from reaching regular output.

Commit `f20e323` added passive native steering activation diagnostics so a future targeted run can follow the original states through the first native write. DEV 4 remains an available instrument, not a completed validation result.

## AER-01J — returning to the original executable

The investigation then moved away from assuming the loader would explain the design. The original `Jennifer` executable was verified directly:

```text
OutRun 2 SP SDX Rev A — DVP-0015A
SHA-256: f16fc04d836a2bd8e401d8f987d4fe694fa16e18a9f870da6623d7f884911075
```

Static analysis established cabinet initialization, readiness, steering calibration, serial ownership, and the native output call chain. This changed the central conclusion: the original game does contain a complete steering-output architecture. Missing loader writes became an activation question, not evidence of absence. [AER-EV-EXE-002](EVIDENCE_REGISTER.md#aer-ev-exe-002--native-steering-output-pipeline) [AER-EV-EXE-003](EVIDENCE_REGISTER.md#aer-ev-exe-003--loader-activation-hypothesis)

## AER-01K.1 through K.3 — finding the native signal

Tracing writers and readers established that `EVWORK_CAR+0x054` is the average direction of the two front tires. It is derived vehicle state, not raw wheel input and not physical torque. [AER-EV-STATE-001](EVIDENCE_REGISTER.md#aer-ev-state-001--front-tire-direction-source)

From there, the continuous path could be reconstructed: nonlinear shaping, per-front-contact attenuation, front-load adjustment, quantization, and a final game-side range of `4–15`. Those values are commands to the board, not torque measurements. [AER-EV-MAG-001](EVIDENCE_REGISTER.md#aer-ev-mag-001--nonlinear-magnitude-curve) [AER-EV-MAG-004](EVIDENCE_REGISTER.md#aer-ev-mag-004--quantized-native-magnitude) [AER-EV-MAG-005](EVIDENCE_REGISTER.md#aer-ev-mag-005--no-physical-torque-units)

Parallel pattern research recovered the complete 16-entry translation and duration tables. Duration turned out to be measured in eligible cabinet callbacks rather than fixed milliseconds. Pattern output also has transmission priority over a pending magnitude update. [AER-EV-PTN-001](EVIDENCE_REGISTER.md#aer-ev-ptn-001--pattern-translation-table) [AER-EV-TIME-002](EVIDENCE_REGISTER.md#aer-ev-time-002--callback-tick-pattern-duration)

## AER-01K.4 through K.7 — contact classifications

Pattern selection led into per-tire contact state and then into the original course collision resources. The `COLI0105` format revealed a spatial lookup, candidate lists, polygon geometry, and one unsigned classification ordinal for every polygon. At runtime, the ordinal becomes a one-hot bit used by native masks. [AER-EV-COLI-001](EVIDENCE_REGISTER.md#aer-ev-coli-001--coli0105-format) [AER-EV-COLI-003](EVIDENCE_REGISTER.md#aer-ev-coli-003--one-hot-conversion)

This corrected another early interpretation. The contact fields were not generic vehicle flags; they carried authored polygon classifications per tire. Pattern 10 responds to a transition between selected classification families, while Patterns 12–15 distinguish front-tire classifications and agreement. [AER-EV-PTN-003](EVIDENCE_REGISTER.md#aer-ev-ptn-003--pattern-10-transition) [AER-EV-PTN-004](EVIDENCE_REGISTER.md#aer-ev-ptn-004--patterns-1215)

## AER-01K.8 — Tulip Garden visual lineage

Collision coordinates from Tulip Garden were matched byte-for-byte to the visual mesh. A localized ordinal-20 region of 39 polygons aligned with distinct bridge-road geometry owned primarily by:

```text
re_CS_TULI_05_H_BLIDGE
```

The material lineage reached `STG_5A_H_BLIDGE_LORD_03/_04`, with adjacent and supporting textures preserved in the technical reference. The location strongly supports a relationship with the recognizable cobblestone section, but no original material name literally identifies cobblestone. [AER-EV-VIS-002](EVIDENCE_REGISTER.md#aer-ev-vis-002--tulip-ordinal-20-visual-lineage) [AER-EV-VIS-003](EVIDENCE_REGISTER.md#aer-ev-vis-003--cobblestone-interpretation)

## AER-01K.9 — six-track reconciliation

Six parallel lines of investigation were reconciled: executable control flow, magnitude, patterns, protocol, timing, and course/visual assets. Cross-course analysis validated 30 main-course and 16 branch collision resources.

That comparison prevented Tulip's finding from becoming an overgeneralization. Ordinal 20 also occurs in localized Lake and Prin segments, and their geometry differs. The evidence supports a special authored contact category; it does not support a universal cobblestone or roughness magnitude. [AER-EV-COLI-005](EVIDENCE_REGISTER.md#aer-ev-coli-005--cross-course-ordinal-20) [AER-EV-COLI-006](EVIDENCE_REGISTER.md#aer-ev-coli-006--ordinal-20-is-not-roughness-magnitude)

## AER-01L.1 — evidence reconciliation

L.1 separated stable findings from remaining interpretations. Important corrections were preserved rather than rewritten out of the story:

- front-tire direction replaced the raw-steering assumption;
- state `0x1E` moved from a drift/recovery candidate to a wall-rebound path;
- generic contact flags became per-polygon classifications;
- Pattern 10 became a specific classification-family transition;
- Tulip ordinal 20 moved from generic ROAD ownership to bridge-road ownership;
- callback ticks replaced assumed real-time duration;
- request magnitude was separated from physical torque;
- missing writes became an activation question rather than proof that native output was absent.

## AER-01L.2 through L.4 — publication

Commit `eeb50f2` published the [Evidence Register](EVIDENCE_REGISTER.md) and [Hardware Validation Register](HARDWARE_VALIDATION_REGISTER.md). Together they provide stable evidence IDs and place torque, direction, waveform, cadence, firmware, and cabinet behavior on the correct side of the hardware boundary.

Commit `2acb3c0` published the [Native Steering Technical Reference](NATIVE_STEERING_TECHNICAL_REFERENCE.md), preserving the execution path, equations, tables, protocol, timing, and corrections in one developer-oriented document.

Commit `f4ae0e7` published [Discovering OutRun 2 SP's Original Arcade Steering System](ARCADE_STEERING_DISCOVERY.md), a human-friendly account of why the research began, what surprised us, what the game establishes, and what remains unknown.

## Where the research stands

The original game-side steering architecture is substantially understood. The loader activation path and original hardware response remain separate validation tracks. A future HYP36rforce Arcade profile may be informed by this evidence, but it would be an independent modern interpretation. Nothing in this history claims that the original board firmware or physical cabinet response has already been recreated.
