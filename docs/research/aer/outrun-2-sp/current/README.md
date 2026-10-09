# Arcade Experience Research

Arcade Experience Research (AER) is my investigation into how Sega's original *OutRun 2 SP SDX* game generated steering requests and communicated with its dedicated drive-board hardware.

I started with a simple question: what was the arcade game actually asking the steering system to do? Following that question led from serial traffic and loader behavior into the original executable, vehicle state, collision classifications, course assets, command scheduling, and the boundary between game-side requests and physical cabinet behavior.

## What we have established

The verified original game contains two complementary steering-output paths:

- a continuous request derived from front-tire direction, contact, and load state;
- short numbered patterns selected by collision and road-contact conditions.

It initializes and calibrates dedicated steering hardware, frames requests for one or two boards, tracks acknowledgments, and queues traffic. Course research also established how authored collision ordinals reach per-tire state and participate in pattern selection. Tulip Garden provided an especially useful collision-to-visual match around its recognizable bridge-road section.

What the original motor torque, direction, and pattern waveforms felt like remains a hardware and firmware question. AER documents that boundary rather than filling it with assumptions.

## Live recovery milestone

**The original Sega game-side steering path has now been observed running during real OutRun 2 SP SDX gameplay.** DEV 5 reached driver state 12, cabinet-check state 2, and the native callbacks without forcing those states; the game emitted steering requests through our physically isolated virtual SERIAL0 board. The board's response bytes and calibration remain synthetic, and this does not establish authentic arcade motor force.

The current work is evidence-based interpretation and experimental profile development, not another attempt to recover basic initialization. The V2 live recording corrected the earlier V1 road-pointer mistake and established road-contact/command correlations; a later virtual-transport READY/configuration correction still awaits fault-free full-session confirmation.

[**Current cross-repository research status →**](CROSS_REPOSITORY_STATUS.md) · [DEV 5 historical live-recovery record](LIVE_NATIVE_FFB_RECOVERY_STATUS.md)

## Choose a starting point

### Explanation

[Discovering OutRun 2 SP's Original Arcade Steering System](ARCADE_STEERING_DISCOVERY.md) tells the story of the investigation and the main discoveries without requiring disassembly knowledge.

[Letting OutRun Complete Its Own Steering Startup](VIRTUAL_DRIVEBOARD_OVERVIEW.md) explains the confirmed loader activation failure and the safety boundary for a possible virtual drive board.

[SDX Steering Hardware Topology](SDX_HARDWARE_TOPOLOGY.md) reconciles Sega's two L/R motor-driver assemblies with Jennifer's two-slot SERIAL0 packet contract and the first DEV 5 runtime failure.

[AER-03 Native FFB Capture Analysis](AER03_NATIVE_FFB_CAPTURE_ANALYSIS.md) preserves the first live command-distribution and timing findings, with an explicit correction to its superseded magnitude decoder. Its reusable decoder is under `research/aer/tools/ffb_analyzer/`.

[AER-04 Native FFB and Vehicle Telemetry](AER04_VEHICLE_FFB_TELEMETRY.md) defines the synchronized passive vehicle/command schema, verified signal lineage, integrity corrections, and explicit unknown fields.

[Designing the Virtual Drive-Board Boundary](VIRTUAL_DRIVEBOARD_INTEGRATION_OVERVIEW.md) explains how that model could fit behind LinuxLoader's existing serial bridge while the game retains ownership of initialization.

### Technical research reference

[Native Steering Technical Reference](NATIVE_STEERING_TECHNICAL_REFERENCE.md) contains the recovered execution path, equations, masks, pattern tables, protocol, scheduling, and confidence limits.

[AER-02F Virtual Drive-Board Technical Design](VIRTUAL_DRIVEBOARD_MODEL.md) documents the original activation states, loader divergence, synthetic protocol model, safety invariants, and remaining firmware assumptions.

[AER-02G Virtual Drive-Board Runtime Integration Specification](VIRTUAL_DRIVEBOARD_INTEGRATION_SPEC.md) defines revision locking, configuration conflicts, virtual descriptor behavior, calibration isolation, failure handling, and the pre-implementation validation gate.

[AER Profile Implementation Blueprint](AER_PROFILE_IMPLEMENTATION_BLUEPRINT.md) consolidates the recovered steering character, command matrix, road/contact evidence, collision limits, modern interpretation boundary, integration architecture, and validation gate for a future independently selectable HYP36rforce Arcade Experience profile.

[DEV 5 Virtual Drive-Board Assumptions](DEV5_VIRTUAL_BOARD_ASSUMPTIONS.md) records the exact synthetic response and centered-calibration policy used by the isolated Windows research build. These assumptions are not Sega firmware claims.

## Supporting references

- [Evidence Register](EVIDENCE_REGISTER.md) — stable evidence IDs and confidence classifications.
- [Hardware Validation Register](HARDWARE_VALIDATION_REGISTER.md) — questions that require firmware, original documentation, targeted runtime observation, or cabinet measurement.
- [Research History](RESEARCH_HISTORY.md) — how the investigation evolved and where earlier interpretations changed.
- [Course Classification Matrix](COURSE_CLASSIFICATION_MATRIX.md) — polygon counts and ordinal distributions for all 46 validated collision resources.

## Current status

The original game-side steering pipeline has been **live-validated**, and the AER-04 V2 recording established meaningful front-tire road-mask and command correlation. The [implementation-ready AER profile blueprint](AER_PROFILE_IMPLEMENTATION_BLUEPRINT.md) is an *independent modern interpretation*, not recovered Sega firmware. The latest virtual-board configuration-transition fix still requires sustained live acceptance validation. Exact physical torque, waveform, timing, and kerb/material-specific sensations remain unverified.

## Relationship to future projects

AER records original arcade evidence. HYP36rforce is my independent modern FFB system. A future selectable Arcade profile may use validated AER findings as design input, but it would remain a modern interpretation—not recovered Sega firmware behavior. Reference+ remains a separate existing HYP36rforce experience.

OutRun 2 SP Arcade Experience is another potential consumer of this research. AER-MOTION and SimHub remain separate future research directions.
