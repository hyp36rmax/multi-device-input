# HYP36Rforce AER R5.0 — force-oriented OutRun lineage

**PART I — ENGINEERING RECORD**

## Scope and decision

R5.0 asks how force-feedback-capable OutRun 2 branches presented steering and
Road information. It is lineage research only. No external implementation,
offset, algorithm or tuning is imported into HYP36Rforce FFB.

The current decision is:

> **PARTIAL — FORCE EFFECT EXISTS BUT ROAD SEMANTICS INCOMPLETE.**

The arcade cabinet unquestionably has a bidirectional steering motor and drive
board. The Japanese PS2 release and OutRun Online Arcade also have credible
native wheel-FFB evidence. None of the sources examined establishes the
gameplay command representation or the path from surface/material state to a
signed steering-wheel force. Road2 therefore remains paused as an active
feature. The result is enough to narrow the next reverse-engineering target,
but not enough to define an AER signal policy.

## Evidence method

Evidence is ranked as follows:

| Tier | Evidence | Treatment in R5.0 |
| --- | --- | --- |
| 1 | Executable/code/disassembly, drive-board protocol or firmware, official service documentation, official SDK implementation, original-hardware capture | May establish architecture or behavior within its exact scope |
| 2 | Developer statement, verified technical documentation, platform source lineage, well-supported reverse engineering | May establish intent or a strong implementation lead; does not replace a trace |
| 3 | Community observation, forum recollection, secondary quotation or summary | Lead or corroboration only |

Absence from a service manual is not proof that an effect did not exist. A
manual can prove installed hardware and diagnostic behavior without revealing
gameplay algorithms. Likewise, advertised wheel compatibility does not by
itself prove the effect API, command form or Road semantics.

## Track D control — PC OutRun 2006 C2C

The established R4.2N result remains the control:

```text
four categorical surface states
    -> positive material-coefficient lookup
    -> maximum coefficient
    -> speed/context scaling
    -> non-negative tactile envelope
    -> Xbox left/right motor amplitudes
```

The original PC executable omits `CalcVibrationValues()`. Multi Input restores
the closely matching Xbox controller routine against shared PC game state.
Within that mapped path:

- material coefficients are non-negative;
- four corners collapse to the largest coefficient;
- speed-like state provides the dominant continuous variation;
- gear, collision and other event branches join two non-negative accumulators;
- the final values drive unidirectional Xbox rumble motors;
- no signed Road waveform or physical steering direction is created.

HYP36R's directional model is an independent vehicle-state interpretation. It
is not recovered arcade force logic. The current HYP36R Road presentation uses
the understood tactile carrier and remains the valid fallback. R5.0 does not
change it or its existing research ceiling.

**Research boundary:** stop looking for steering-wheel polarity inside the
restored Xbox motor envelope. A signed Road carrier, if one exists in the
lineage, must be found in a separate force-oriented implementation or designed
later as explicitly new HYP36R behavior.

Primary internal evidence:

- [R4.2N native pre-motor surface lineage](HYP36R_FORCE_2_0_R4_2N_NATIVE_SURFACE_LINEAGE.md)
- [Research II R0 map](HYP36R_FORCE_RESEARCH_II_R0.md)

## Track A — original arcade OutRun 2 / OutRun 2 SP

### Hardware architecture

The original OutRun 2 Twin service manual is Tier 1 hardware evidence. It
identifies:

- a 500 W servo motor in the handle mechanism;
- `838-14174 Servo Motor Drive BD Midi`;
- a JVS I/O control board;
- a Chihiro game system;
- steering-position input and separate steering-motor diagnostics.

The game test exposes `STOP MOTOR`, `ROLL RIGHT`, `ROLL LEFT`, a configurable
center-of-control position and motor power levels from 60% through 100%.
Fault handling identifies drive-board connection, origin, centering, encoder,
over-current and over-temperature conditions. This proves a closed-loop,
bidirectional powered steering mechanism with position/centering awareness. It
does not expose the gameplay command protocol.

OutRun 2 SP documentation must not be treated as one uniform cabinet. Sega's
archive provides separate Standard and Deluxe service/wiring documentation.
The standard lineage is Chihiro-based and retains a steering assembly. The
large Special Tours SDX cabinet is a later Lindbergh motion configuration with
per-seat steering and motion hardware. Findings from the SDX cannot be
silently projected back onto the original Chihiro Twin/Standard cabinet.

Tier 1 sources:

- [Sega Amusements OutRun 2 SP manual archive](https://backup.segakore.fr/sauservice/Manuals/OutRun2SP/ORTMain.html)
- [OutRun 2 Twin owner/service manual](https://arcarc.xmission.com/PDF_Arcade_Manuals_and_Schematics/Outrun%202%20Twin.pdf)
- [OutRun 2 SP Standard wiring](https://backup.segakore.fr/sauservice/Manuals/OutRun2SP/OutRun2SPStdWiring.pdf)

### Force-command findings

The manuals prove that the system can command full rotation in either
direction, establish a control center and vary resistance. They do **not**
establish whether gameplay sends:

- signed torque values;
- target-position or resistance values;
- high-level effect/event commands;
- a mixture of those forms;
- or data from which the drive board synthesizes effects locally.

No verified drive-board protocol or gameplay command capture was located.
MAME's current public Lindbergh driver is useful platform context, but does not
emulate the OutRun 2 SP SDX force path and cannot answer the command question.
The third-party FFB Arcade Plugin lists OutRun 2 Special Tours Deluxe as a
supported target. That is a reverse-engineering lead, not evidence that its
translation matches the original cabinet algorithm, and no code or offsets
are imported here.

Reference leads:

- [MAME Lindbergh driver](https://github.com/mamedev/mame/blob/master/src/mame/sega/lindbergh.cpp)
- [FFB Arcade Plugin supported-game list](https://github.com/Boomslangnz/FFBArcadePlugin)

### Arcade Road and effect classes

| Effect class | Evidence | Finding | Confidence |
| --- | --- | --- | --- |
| Centering | Center-of-control setting, origin/centering diagnostics | Closed-loop centering capability is confirmed; gameplay centering equation is unknown | Confirmed hardware, unknown gameplay semantics |
| Steering load/resistance | Motor-power setting and powered servo | Adjustable steering resistance exists | Confirmed hardware, unknown dynamic source |
| Signed direction | Explicit roll-right and roll-left tests | Motor can be commanded bidirectionally | Confirmed diagnostic capability |
| Road texture | No gameplay trace or protocol found | UNKNOWN | Unknown |
| Curb/surface | No gameplay trace or protocol found | UNKNOWN | Unknown |
| Collision | No gameplay trace or protocol found | UNKNOWN | Unknown |
| Gear shift | No gameplay trace or protocol found | UNKNOWN | Unknown |
| Drift/slip | No gameplay trace or protocol found | UNKNOWN | Unknown |
| Grip loss/recovery | No gameplay trace or protocol found | UNKNOWN | Unknown |
| Drive-board-local synthesis | Board boundary exists, firmware/protocol not resolved | Possible, not established | Unknown |

The arcade evidence is force-oriented but not Road-oriented. It proves a
signed actuator topology, not how surface identity, contact, speed or vehicle
state became force.

## Track B — Japanese PS2 OutRun2 SP

### Exact release and support

Sega's Japanese catalog confirms `OutRun2 SP（アウトラン2 スペシャルツアーズ）`
for PlayStation 2, released 8 February 2007. Contemporary product records
identify serial `SLPM-66628` and advertise GT Force / GT Force Pro support.
This is the Japanese PS2 OutRun2 SP branch, not the PAL or North American
OutRun 2006 C2C release.

Sources:

- [Sega Japanese PS2 catalog](https://www.sega.jp/game/ps2/?page=2) — Tier 1 title, platform and date
- [Neowing SLPM-66628 product record](https://www.neowing.co.jp/product/SLPM-66628) — Tier 2 serial and contemporary wheel-support description
- [Japanese PS2 hardware report](https://www.ukvac.com/forum/threads/jap-ps2-outrun-sp-force-feedback.82884/) — Tier 3 original-user corroboration

The combination of contemporary support material, the later developer-related
lineage statement below, and original-user reports strongly supports actual
native wheel FFB in this Japanese release. It does not establish which effect
types were used.

### PS2 force implementation and Road

No executable disassembly, official wheel API call trace or USB effect capture
was located in this milestone. The supported wheel names suggest a Logitech
USB force-feedback path, but R5.0 does not infer the API or effects from device
compatibility alone.

The following remain **UNKNOWN**:

- constant-force representation and sign;
- spring, damper or periodic-effect use;
- per-frame versus persistent effect submission;
- surface/material inputs;
- Road waveform, magnitude and timing;
- collision, gear, drift, grip-loss and recovery mappings;
- whether the game or wheel firmware synthesized any component.

PS2 OutRun2 SP is therefore the strongest accessible executable candidate for
answering whether a force-oriented home branch contains behavior absent from
PC C2C, but it does not yet answer that question.

## Track C — OutRun Online Arcade

### Developer statement and source lineage

A launch-period Sumo Digital interview published 24 April 2009 names Paul
Tankard, Steve Lycett and Pat Phelan as interview participants. It states:

> “We started with the Coast 2 Coast code, but also checked back to the original arcade code.”

The same answer explicitly says the team added force-feedback support for the
Xbox 360 Wheel. This is Tier 2 developer evidence for the Xbox 360 product. It
establishes both C2C and arcade consultation, but does **not** say that the
arcade force algorithm was copied, nor identify a Road model.

Source: [Sumo Digital OutRun Online Arcade interview](https://www.mondoxbox.com/news/16314/outrun-online-arcade--intervista-a-sumo-digital.html)

### S0L attribution

A later forum preserves a quotation attributed to Sumo developer S0L. Its
platform-specific claims are that the PS3 version supports certain Logitech
wheels, OOA supports the Xbox 360 Wheel, and native Logitech-wheel FFB was
specific to the Japanese PS2 OutRun2 SP release rather than the PAL/US PS2
versions.

The shortest directly relevant wording is:

> “We've also got support in OOA for the 360 wheel too.”

The original post URL and full original context were not recovered. The
preserved quotation is therefore Tier 3/provenance-incomplete, even though its
Xbox 360 claim agrees with the named-developer interview.

Source: [preserved S0L quotation and discussion](https://www.yaronet.com/topics/171773-pc-outrun-2006-coast2coast-avec-le-retour-de-force)

### Wheel and FFB findings

| Platform | Wheel input | Native FFB | What is actually established |
| --- | --- | --- | --- |
| Xbox 360 | Xbox 360 Wheel support confirmed | Confirmed by the Sumo interview | Support was newly added to the C2C-derived OOA branch; API calls, effect classes and Road semantics remain unknown |
| PlayStation 3 | Logitech-wheel support is credibly reported | Likely, but not established by a recovered primary technical source | S0L-attributed statement and user reports support it; exact devices, API and force behavior remain unresolved |

Some launch-era community reports describe weak, rumble-like or absent
wheel force. These are useful warnings about shipped behavior, configuration
and subjective quality, but they do not overturn the developer statement or
reveal the implementation. The exact Xbox 360/PS3 force APIs, effect topology,
surface inputs, collision mapping and steering behavior remain unknown.

## Cross-platform lineage matrix

`Confirmed` always means confirmed only at the stated level. `Likely` is not
promoted to implementation fact.

| Capability | Arcade OR2 / OR2 SP | PS2 JP OutRun2 SP | OOA Xbox 360 | OOA PS3 | PC C2C control |
| --- | --- | --- | --- | --- | --- |
| Wheel input | Confirmed | Confirmed | Confirmed | Likely | Original single-device input; Multi Input extends it |
| Native FFB | Confirmed motor system | Strongly supported | Confirmed | Likely | No native PC wheel FFB found; HYP36R adds it |
| Signed force | Bidirectional actuator confirmed; gameplay representation UNKNOWN | UNKNOWN | UNKNOWN | UNKNOWN | No signed native Road carrier in mapped path |
| Spring/centering | Hardware centering confirmed; gameplay equation UNKNOWN | UNKNOWN | UNKNOWN | UNKNOWN | Synthetic HYP36R centering/load behavior |
| Road effect | UNKNOWN | UNKNOWN | UNKNOWN | UNKNOWN | Positive material/speed tactile envelope |
| Surface/material input | UNKNOWN in force path | UNKNOWN | UNKNOWN | UNKNOWN | Four categories -> positive coefficient -> max |
| Collision effect | UNKNOWN | UNKNOWN | UNKNOWN | UNKNOWN | Native positive motor event joins tactile output; HYP36R Impact is separate presentation |
| Gear effect | UNKNOWN | UNKNOWN | UNKNOWN | UNKNOWN | Native positive motor event joins tactile output |
| Drift/slip effect | UNKNOWN | UNKNOWN | UNKNOWN | UNKNOWN | HYP36R-derived, not a recovered native PC wheel effect |
| Force API/protocol | Chihiro/JVS + dedicated servo board boundary; command protocol UNKNOWN | Logitech USB wheel path suggested; API UNKNOWN | Platform wheel support; API/effects UNKNOWN | Logitech/platform wheel support reported; API/effects UNKNOWN | DirectInput output is Multi Input/HYP36R; Xbox vibration routine is restored game logic |
| Local hardware synthesis | Possible; unproven | UNKNOWN | UNKNOWN | UNKNOWN | Xbox motor hardware consumes amplitudes; mapped game routine creates envelope |
| Confidence | High for hardware, low for gameplay semantics | Moderate/high for FFB existence, low for semantics | High for support, low for semantics | Moderate for support, low for semantics | High for mapped PC path |
| Best source | Tier 1 service manual | Tier 1 catalog + Tier 2 product record | Tier 2 developer interview | Tier 3 preserved developer attribution | Tier 1 project source/disassembly + validated captures |

## Road2 implications

### Strongest force-oriented Road evidence

The strongest relevant evidence is a **combination**, not a recovered Road
equation:

1. the arcade cabinet has a closed-loop bidirectional steering motor, center
   control and variable resistance;
2. Japanese PS2 OutRun2 SP and OOA shipped or credibly supported native wheel
   FFB;
3. the OOA team explicitly consulted both C2C and original arcade code;
4. the mapped PC C2C/Xbox tactile path contains material and speed intent but
   destroys physical direction before output.

This makes a separate force-oriented lineage plausible and worth tracing. It
does not show whether Road used surface identity, a signed vehicle/contact
state, a predefined effect, or drive-board-local synthesis.

### Unlock status

**PARTIAL — FORCE EFFECT EXISTS BUT ROAD SEMANTICS INCOMPLETE.**

What is unlocked:

- a defensible search boundary outside the PC Xbox motor envelope;
- confirmed force-capable targets;
- a specific executable target for the next trace;
- a clear distinction between actuator direction and gameplay Road direction.

What is not unlocked:

- a signed Road carrier;
- effect polarity or waveform;
- Road timing, magnitude or frequency;
- an arcade-authentic effect topology;
- permission to implement Road2.

## AER implications

AER is **not ready for signal-policy definition**. Evidence is sufficient to
define an AER research taxonomy—centering, steering load, Road, curb, impact,
gear, drift/grip and recovery—but not to assign signals or behavior.

Reference+ remains HYP36R's independently validated player-facing policy. AER
must remain a separate evidence-backed policy rather than an “arcade” tuning
preset. Neither the service-test resistance setting nor anecdotal wheel feel
is enough to reproduce authentic gameplay behavior.

## Gaps and exact next target

The remaining high-value gaps are:

- arcade game-to-drive-board command format and cadence;
- division of synthesis between game and drive board;
- PS2/OOA force API calls and effect lifetimes;
- the source and transformation of Road/surface state;
- effect separation for Road, curb, collision, gear and drift/grip;
- signed magnitude, timing and saturation behavior;
- regional differences among Japanese PS2 SP and PAL/US C2C;
- original provenance for the S0L quotation.

The **one exact next reverse-engineering target** is the Japanese PS2
`SLPM-66628` executable's Logitech wheel-effect submission routine. Start from
its USB/wheel initialization or device-descriptor references, identify the
function that submits force effects, then trace that function's Road-relevant
arguments backward to material/contact and vehicle-state readers. The required
deliverable is the effect type, signed representation, update cadence and the
first proven surface/material join point. Do not begin with a broad memory
scan and do not import external force logic.

This target is preferable to another PC capture because it directly addresses
a native force-feedback branch while remaining more reproducible than an
unresolved arcade drive-board protocol.

## Runtime and hardware-test decision

No physical test or new telemetry is required to close R5.0. Existing HYP36R
runtime testing cannot reveal semantics absent from the PC path.

A later targeted hardware test becomes justified only after a specific arcade
command channel or PS2 effect-submission boundary is identified. The minimum
test would synchronize captured force commands with matched-speed passages on
normal pavement, one repeatable rough/curb surface, re-entry, one mild impact
and one controlled drift/recovery. Until a command boundary exists, subjective
wheel feel alone cannot distinguish game-generated force from device-local
synthesis.

## Next recommendation

Close R5.0 as a partial lineage result. Keep current HYP36R Road active only in
its existing form, keep Road2 and AER implementation paused, and begin a
separate read-only trace of the `SLPM-66628` wheel-effect submission path. No
Alpha 1 setting, force equation, telemetry schema, workflow or artifact should
change as a consequence of this document.

## Credits & Reference Context

THP32 provided additional PC/PS2 FFB research and technical reference material
that helped identify alternate branches and comparison questions. It is
**EXTERNAL REFERENCE — NOT INTEGRATED**. This record does not adopt THP32 code,
equations, constants, or implementation choices, and it does not treat an
external claim as independently reproduced unless the engineering record says
so.

## Part II — Development Journey Recap

R5 asked whether another OutRun platform exposed a more direct force-oriented
lineage than the PC's unsigned controller-motor path. Historical arcade, PS2,
and Online Arcade evidence made that plausible, but did not reveal enough of
the command format, direction, cadence, or surface join point to build from.

The result was partial rather than negative: there are credible native FFB
branches worth tracing, but no validated AER implementation yet. Current Road
therefore stayed unchanged. The next bounded step is the Japanese PS2
`SLPM-66628` effect-submission routine—not a broad scan and not imported force
logic.
