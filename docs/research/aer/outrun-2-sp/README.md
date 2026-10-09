# Arcade Experience Research

Arcade Experience Research (AER) is my investigation into how Sega's original *OutRun 2 SP SDX* game generated steering requests and communicated with its dedicated drive-board hardware.

I started with a simple question: what was the arcade game actually asking the steering system to do? Following that question led from serial traffic and loader behavior into the original executable, vehicle state, collision classifications, course assets, command scheduling, and the boundary between game-side requests and physical cabinet behavior.

## What we have established

The verified original game contains two complementary steering-output paths:

- a continuous request derived from front-tire direction, contact, and load state;
- short numbered patterns selected by collision and road-contact conditions.

It initializes and calibrates dedicated steering hardware, frames requests for one or two boards, tracks acknowledgments, and queues traffic. Course research also established how authored collision ordinals reach per-tire state and participate in pattern selection. Tulip Garden provided an especially useful collision-to-visual match around its recognizable bridge-road section.

What the original motor torque, direction, and pattern waveforms felt like remains a hardware and firmware question. AER documents that boundary rather than filling it with assumptions.

## Choose a starting point

### Human-friendly explanation

[Discovering OutRun 2 SP's Original Arcade Steering System](ARCADE_STEERING_DISCOVERY.md) tells the story of the investigation and the main discoveries without requiring disassembly knowledge.

### Technical research reference

[Native Steering Technical Reference](NATIVE_STEERING_TECHNICAL_REFERENCE.md) contains the recovered execution path, equations, masks, pattern tables, protocol, scheduling, and confidence limits.

## Supporting references

- [Evidence Register](EVIDENCE_REGISTER.md) — stable evidence IDs and confidence classifications.
- [Hardware Validation Register](HARDWARE_VALIDATION_REGISTER.md) — questions that require firmware, original documentation, targeted runtime observation, or cabinet measurement.
- [Research History](RESEARCH_HISTORY.md) — how the investigation evolved and where earlier interpretations changed.
- [Course Classification Matrix](COURSE_CLASSIFICATION_MATRIX.md) — polygon counts and ordinal distributions for all 46 validated collision resources.

## Current status

The game-side steering architecture is substantially reconstructed. The most important open runtime question is why the loader environment has not yet completed the native activation path through regular gameplay writes. The most important physical questions begin at the drive-board boundary: torque, polarity, waveform, timing, calibration motion, and firmware behavior.

## Relationship to future projects

AER records original arcade evidence. HYP36rforce is my independent modern FFB system. A future selectable Arcade profile may use validated AER findings as design input, but it would remain a modern interpretation—not recovered Sega firmware behavior. Reference+ remains a separate existing HYP36rforce experience.

OutRun 2 SP Arcade Experience is another potential consumer of this research. AER-MOTION and SimHub remain separate future research directions.

