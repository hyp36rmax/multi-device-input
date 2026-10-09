# AER — Current Cross-Repository Research Status

**Research authority:** [hyp36rmax/linuxloader — `research/aer-01c-driveboard-recorder`](https://github.com/hyp36rmax/linuxloader/tree/research/aer-01c-driveboard-recorder/docs/aer)

AER documents verified **original OutRun 2 SP SDX Rev A game-side behavior**. This is not a Sega drive-board firmware reconstruction or proof of cabinet torque, waveform, polarity, or physical kerb sensation. The purpose of mirroring these references is to keep future projects anchored to the same evidence without making their implementation branches dependent on LinuxLoader's research runtime.

## Current research conclusions

- **Native game-side steering activation is live validated.** The DVP-0015A `Jennifer` executable reached original driver state 12 and cabinet-check state 2 and ran its own steering callback during gameplay. The successful baseline is `f7b892b`. No original states or callbacks were forced.
- **SDX has two logical steering channels on one SERIAL0 transaction.** A complete SDX request frame is seven bytes. The left/right motor-driver assemblies and the separate motion-actuator path must not be mistaken for two serial ports.
- **Continuous command `0x0B` magnitude is packed in byte 1 (`value_a >> 3`).** AER-03's original constant-4 decode used the wrong byte and is superseded. Observed magnitudes were 4–11; this is not Nm or measured wheel torque.
- **V1 vehicle-road telemetry is unreliable.** It read `CAR_WORK` where `EVWORK_CAR` was required. AER-04 V2 corrects the pointer. The live V2 capture `20261008-200325-6ba226b8` contained meaningful one-hot per-front-tire contact masks and original steering commands. Earlier V1 samples must not be used for road attribution.
- **Road-contact correlation is established at the classification level.** The V2 session contained 46 Pattern-10 translations near front-tire classification changes and 34 Pattern-13 translations associated with both front tires sharing specific contact masks. Individual events are **not** established as physical kerbs, cobblestone vibration, bumps, or particular track materials.
- **The virtual research transport is distinct from authentic hardware.** Early live sessions exposed a `0x00 → 0x07` READY-state fault; the subsequent V2 session exposed a `0x03` configuration re-entry before `0x0B`. The merged research source has modeled fixes for these transitions. Full-session, fault-free acceptance after the last correction must not be assumed without a new live capture.
- **The [AER profile blueprint](AER_PROFILE_IMPLEMENTATION_BLUEPRINT.md) is implementation-ready as a *modern interpretation*.** It preserves game-side evidence, short event priority, direction separation, surface-classification transitions, and conservative force conditioning; it does not supply missing Sega firmware behavior.

## Repository-specific use

| Repository | Role | Boundary |
|---|---|---|
| [LinuxLoader](https://github.com/hyp36rmax/linuxloader) | Primary Sega game-side evidence, recordings, native runtime instrumentation and profile blueprint | Research mode remains isolated from physical motor, motion, and host FFB output |
| [OutRun 2006 Multi-Input](https://github.com/hyp36rmax/multi-device-input) | Independent HYP36rforce modern FFB research and **experimental**, optional Arcade Experience interpretation | No original SDX addresses or command IDs may be assumed valid PC game signals; Reference+ and v1.5 release behavior remain separate |
| [OutRun 2 SP Arcade Experience](https://github.com/hyp36rmax/OutRun-2-SP-Arcade-Experience) | Modular player experience framework; prospective consumer of native-game evidence | Research references do not imply current modern-wheel input, playable FFB, or authentic arcade hardware emulation |

## Synchronization rules

1. The seven original preservation documents at the consumer repositories retain their source SHA-256 values and historical provenance. Do not rewrite those archived references.
2. New findings are distributed as a **current research mirror** with a pinned authoritative LinuxLoader commit in its manifest, alongside a link to the living source branch.
3. The canonical source and mirrored files should match byte-for-byte for each recorded source revision. Project-specific navigation and implementation-status prose live **outside** that mirror.
4. Never publish firmware conjecture as original Sega behavior, nor describe an experimental HYP36rforce AER profile as shipped in v1.5 or the OutRun 2 SP Experience.
5. Before later syncs, compare source commits, mirror manifests, link targets, and project implementation status, then update only documentation on the appropriate active branches.

## Next evidence gates

- Verify full-session virtual-board acceptance after the late `0x03` correction.
- Correlate specific track positions and authored geometry before claiming a material-specific kerb/texture effect.
- Validate any modern AER DirectInput output in its *own* target game and hardware; this cannot establish original Sega cabinet behavior.

For stable findings, see [Evidence Register](EVIDENCE_REGISTER.md), [Native Steering Technical Reference](NATIVE_STEERING_TECHNICAL_REFERENCE.md), and [Research History](RESEARCH_HISTORY.md). For the profile design, see the [AER Profile Implementation Blueprint](AER_PROFILE_IMPLEMENTATION_BLUEPRINT.md).
