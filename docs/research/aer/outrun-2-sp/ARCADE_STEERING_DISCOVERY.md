# Discovering OutRun 2 SP's Original Arcade Steering System

## Why I Started Looking Into Arcade FFB

I've always been interested in the part of an arcade game that exists beyond the screen: the controls, the cabinet, and the way dedicated hardware helped shape the experience. With *OutRun 2 SP*, that curiosity kept returning to the steering wheel. What information did the game actually send to the cabinet? Was it simply resistance tied to steering input, or was there more happening underneath?

The goal was not to declare that we had recreated Sega's original force feedback. It was to understand and preserve what the original game itself can tell us.

That led the investigation through the original *OutRun 2 SP SDX Rev A* executable, its cabinet-control code, vehicle state, collision resources, visual course geometry, and communication with the steering drive board. The result is a much clearer picture of Sega's game-side design—and a clear boundary around what still belongs to the original hardware.

For readers who want the addresses, equations, masks, tables, and packet details, the companion [Native Steering Technical Reference](NATIVE_STEERING_TECHNICAL_REFERENCE.md) documents the full lineage. The [Evidence Register](EVIDENCE_REGISTER.md) records the supporting findings and their confidence levels.

## Finding Sega's Original Steering System

The first major discovery was that the game contains a complete steering-output system. It does not just expose a stray steering value. The original code gathers vehicle and contact state, calculates a continuous request, selects shorter event-driven patterns, schedules those requests, and packages them for dedicated steering hardware. [AER-EV-EXE-002](EVIDENCE_REGISTER.md#aer-ev-exe-002--native-steering-output-pipeline)

At a high level, the system has two complementary paths:

- A continuous steering request derived from the direction of the front tires, then adjusted using contact and load information.
- Discrete event patterns selected for particular collision, rebound, and road-contact conditions.

Those paths remain separate until the game decides what to send. When a discrete pattern is pending, it is sent before the continuous magnitude; the continuous request remains waiting rather than disappearing. [AER-EV-PTN-005](EVIDENCE_REGISTER.md#aer-ev-ptn-005--pattern-priority-and-replacement)

That was the first sign that the arcade steering design was more structured than a single “force” number.

## More Than Just Steering Resistance

The continuous path produces a compact integer magnitude from `4` to `15`. The game shapes its source nonlinearly, adjusts it for selected tire contacts, considers the front tire-load relationship, and finally quantizes it into that bounded range. [AER-EV-MAG-001](EVIDENCE_REGISTER.md#aer-ev-mag-001--nonlinear-magnitude-curve) [AER-EV-MAG-002](EVIDENCE_REGISTER.md#aer-ev-mag-002--contact-attenuation) [AER-EV-MAG-003](EVIDENCE_REGISTER.md#aer-ev-mag-003--front-load-adjustment) [AER-EV-MAG-004](EVIDENCE_REGISTER.md#aer-ev-mag-004--quantized-native-magnitude)

It is tempting to read those values as levels of torque. The evidence does not support that. They are game-side command values sent toward a separate drive board. Without the board firmware, motor-current behavior, cabinet mechanics, or measurements from original hardware, `4–15` cannot be translated into physical strength. [AER-EV-MAG-005](EVIDENCE_REGISTER.md#aer-ev-mag-005--no-physical-torque-units)

The event path adds another layer. The game has 16 numbered internal patterns, with a translation table for board requests and a duration table counted in eligible cabinet callbacks. Some are selected by known wall, collision, and contact-classification logic. Others have table entries but no normal selector found in the path traced so far. [AER-EV-PTN-001](EVIDENCE_REGISTER.md#aer-ev-ptn-001--pattern-translation-table) [AER-EV-PTN-002](EVIDENCE_REGISTER.md#aer-ev-ptn-002--pattern-duration-table) [AER-EV-PTN-006](EVIDENCE_REGISTER.md#aer-ev-ptn-006--unselected-pattern-indices)

The important distinction is that the game selects a pattern number, direction, and duration state. The executable does not reveal what each pattern physically felt like at the wheel. Calling one a kick, vibration, pulse, or jolt would go beyond the evidence.

## How Tire Direction and Load Influence Feedback

One of the most interesting corrections came from tracing the continuous source back to its writer. The value is not simply the player's steering-wheel input. The game converts the directions of the two front tires into its signed angle representation, averages them, and stores that derived state for the cabinet-control system. [AER-EV-STATE-001](EVIDENCE_REGISTER.md#aer-ev-state-001--front-tire-direction-source)

That matters because front-tire direction is part of the simulated vehicle response. It can reflect more than the position of the player's hands.

The game then shapes the absolute value through a bounded curve. Selected classifications under either front tire attenuate the result independently, and the combined front-load relationship can add one step to the final integer magnitude. Tire and suspension load therefore participate in the original request rather than sitting outside the feedback path. [AER-EV-STATE-002](EVIDENCE_REGISTER.md#aer-ev-state-002--tire-load-redistribution)

The precise equations are preserved in the [continuous steering magnitude section](NATIVE_STEERING_TECHNICAL_REFERENCE.md#6-continuous-steering-magnitude) of the technical reference. The approachable summary is simple: Sega's game-side request was informed by what the front of the car was doing, where the tires were in contact, and how the front tires were loaded—not only by raw steering input.

## Collisions, Rebounds, and Road Contact

The discrete-pattern investigation considered wall rebound, wall friction or contact, car-to-car contact, changes between road-contact classifications, and differences between the two front tires. The preserved evidence register establishes the wall-rebound and tire/contact-classification relationships directly; it does not retain enough selector detail to assign separate physical meanings to wall-friction or car-contact patterns. Those distinctions should therefore remain research questions rather than labels attached to pattern numbers.

One special state, `0x1E`, belongs to a wall-rebound path and selects between two pattern indices using collision-related state and direction information. Earlier in the investigation it looked like a possible drift or recovery state; following the surrounding control flow changed that interpretation. [AER-EV-STATE-004](EVIDENCE_REGISTER.md#aer-ev-state-004--state-0x1e)

Pattern 10 is selected by a transition between two specific families of contact classifications. Its direction comes from the sign of a steering-state difference. Patterns 12 through 15 also respond to front-tire classifications, including whether both front tires agree and a separate vehicle-state threshold. [AER-EV-PTN-003](EVIDENCE_REGISTER.md#aer-ev-ptn-003--pattern-10-transition) [AER-EV-PTN-004](EVIDENCE_REGISTER.md#aer-ev-ptn-004--patterns-1215)

This shows that the original system reacts to more than a generic “collision happened” flag. It can distinguish authored road-contact families and what each front tire is touching. What it does not tell us is the waveform or physical character the drive-board firmware produced for those requests. [AER-EV-PTN-007](EVIDENCE_REGISTER.md#aer-ev-ptn-007--physical-pattern-behavior)

## What Tulip Garden Revealed

Tulip Garden gave the course-data investigation a particularly useful trail to follow.

The original collision format contains a spatial lookup, polygon lists, polygon geometry, and a one-byte classification ordinal for every collision polygon. At runtime, the game converts each ordinal into a one-hot classification bit and retains classifications per tire/contact. [AER-EV-COLI-001](EVIDENCE_REGISTER.md#aer-ev-coli-001--coli0105-format) [AER-EV-COLI-002](EVIDENCE_REGISTER.md#aer-ev-coli-002--classification-ownership) [AER-EV-COLI-003](EVIDENCE_REGISTER.md#aer-ev-coli-003--one-hot-conversion)

Across the course set, we validated 46 collision resources. In Tulip Garden, a localized region of only 39 polygons—polygons 124 through 162—uses classification ordinal 20. The collision coordinates align exactly with the original visual mesh, allowing the region to be followed into the visible course assets without guessing about scale or placement. [AER-EV-COLI-004](EVIDENCE_REGISTER.md#aer-ev-coli-004--cross-course-validation) [AER-EV-VIS-001](EVIDENCE_REGISTER.md#aer-ev-vis-001--shared-mesh-coordinates)

The dominant visual owner is the bridge-road object `re_CS_TULI_05_H_BLIDGE`, with distinct bridge-road materials. Its location and geometry strongly associate it with the recognizable cobblestone section, and ordinal 20 is eligible for the classification transition involved in Pattern 10. [AER-EV-VIS-002](EVIDENCE_REGISTER.md#aer-ev-vis-002--tulip-ordinal-20-visual-lineage) [AER-EV-VIS-003](EVIDENCE_REGISTER.md#aer-ev-vis-003--cobblestone-interpretation)

There are two important limits to that conclusion. The original material names do not literally identify cobblestone, so the association is strongly supported rather than proven by a label. Ordinal 20 also appears in localized sections of Lake and Prin, where its geometry does not support treating it as a universal roughness scale or universal cobblestone identifier. [AER-EV-COLI-005](EVIDENCE_REGISTER.md#aer-ev-coli-005--cross-course-ordinal-20) [AER-EV-COLI-006](EVIDENCE_REGISTER.md#aer-ev-coli-006--ordinal-20-is-not-roughness-magnitude)

Most importantly, this evidence supports a contact-classification transition. It does not prove that the original wheel produced a continuous cobblestone vibration.

## How the Game Communicates With the Steering Board

The original game sends compact logical requests to dedicated steering hardware. During startup it waits for readiness, exchanges initialization and configuration commands, and performs a steering-position search using analog samples and bounded test levels. Once active, it can send continuous magnitude requests, event-pattern requests, idle or neutral requests, and explicit deactivation. [AER-EV-PROTO-002](EVIDENCE_REGISTER.md#aer-ev-proto-002--initialization-and-calibration) [AER-EV-PROTO-003](EVIDENCE_REGISTER.md#aer-ev-proto-003--runtime-command-families)

The transport supports one or two boards. It frames requests, adds an XOR check byte, limits the number of outstanding requests, and queues additional packets. Incoming bytes are checked for valid status encoding, while acknowledgment ownership is based on counts rather than matching a command identifier. [AER-EV-PROTO-001](EVIDENCE_REGISTER.md#aer-ev-proto-001--packet-framing) [AER-EV-PROTO-004](EVIDENCE_REGISTER.md#aer-ev-proto-004--response-validation) [AER-EV-PROTO-005](EVIDENCE_REGISTER.md#aer-ev-proto-005--queue-and-backpressure)

This tells us a great deal about what the game asks for and when. It still stops at the hardware boundary. The firmware decides how a request becomes motor current and movement, how long a board-owned effect might persist, and how cabinet configuration changes the physical result.

## What We Still Don't Know

Understanding the request path is not the same as reproducing the original arcade experience. Several of the most interesting questions remain open:

- How much physical torque did magnitudes `4–15` produce?
- Which direction values produced left and right motor torque in the cabinet?
- What waveform or movement did each translated pattern create?
- Did the firmware add its own duration, gain, or timeout behavior?
- How did the four motor-power configuration values affect the hardware?
- What was the true cabinet callback frequency under original runtime conditions?
- What did each valid response and alarm status mean?
- How did the steering wheel move during original calibration?

The [Hardware Validation Register](HARDWARE_VALIDATION_REGISTER.md) turns those unknowns into narrow, testable questions. It deliberately prioritizes firmware, service information, and bounded measurements over broad gameplay impressions.

There is also an unresolved loader-side question. Static analysis proves the original output path exists, but earlier research captures showed polling without the regular gameplay writes expected from that path. An altered readiness or initialization transition is a strong explanation, not yet a demonstrated root cause. [AER-EV-EXE-003](EVIDENCE_REGISTER.md#aer-ev-exe-003--loader-activation-hypothesis)

## What This Could Mean for Modern Wheels

The recovered architecture offers useful design ideas: derive steering response from vehicle state, preserve tire-contact distinctions, let short events temporarily take priority, and keep game interpretation separate from hardware transport.

Those ideas may eventually inform a selectable HYP36rforce Arcade profile. If that happens, it will be our own modern interpretation guided by original evidence—not a claim that a consumer wheel has become Sega's original drive board, and not a claim that unknown firmware behavior has been recovered.

That possible Arcade profile also remains separate from the existing HYP36rforce Reference+ experience. Reference+ has its own goals and development history. AER-MOTION and SimHub are separate future research opportunities rather than extensions of this steering document.

No Arcade profile is implemented by this research.

## Why I'm Sharing the Research

Arcade hardware is easy to remember as a feeling and difficult to preserve as an engineering system. The original game still contains part of that system: the vehicle evidence Sega chose, the conditions it classified, the requests it scheduled, and the protocol it used to reach dedicated hardware.

Sharing the work makes those findings easier to inspect, challenge, reproduce, and extend. An enthusiast can start with the story here, then follow the links into the technical reference and evidence ledger instead of relying on a collection of disconnected observations.

It also leaves room for different independent approaches. Someone interested in original hardware can focus on firmware and cabinet measurements. Someone building a modern force-feedback interpretation can use the verified game-side architecture as design input while clearly labeling the parts that are new.

The most valuable result is not a claim that every question has been answered. It is a durable record of what the original game establishes, what the evidence strongly suggests, and exactly where the unanswered hardware questions begin.
