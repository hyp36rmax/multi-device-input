# Documentation map

The canonical documents describe the project as it exists now. The milestone
notes remain beside them because they contain the experiments, replay tables,
mistakes, and corrections that support the current conclusions.

`multi-device-input` is the one active development branch. F1, F1.1, F2, E3,
E4, and the S-series name engineering milestones, not ongoing Git branches.
FFB Experience is part of Multi Input, alongside Controller Experience,
HYP36Rforce FFB, and Developer/Telemetry. The E-series save/unlock documents are
historical feasibility research, not current architecture or roadmap work.
Their experimental runtime is not in the current stable product. Whether any
of that research is revisited is undecided; the earlier commits remain in Git
history.

## Where to start

### Development journey

Start with [Development history](DEVELOPMENT_HISTORY.md) for the accessible
story of what was investigated, what failed, what changed, and which questions
remain open. Then read [HYP36Rforce FFB](HYP36R_FORCE.md) for the current model
in plain technical language.

### Technical and engineering

Start with [HYP36Rforce FFB](HYP36R_FORCE.md), [Native dynamics](NATIVE_DYNAMICS.md),
and [Telemetry](TELEMETRY.md). Use the milestone records below when exact
equations, captures, confidence limits, commits, or rejected paths matter.

## Canonical

- [HYP36Rforce FFB](HYP36R_FORCE.md): current force architecture and limitations.
- [Native dynamics](NATIVE_DYNAMICS.md): native state, four-corner structure,
  evidence confidence, and unknowns.
- [Telemetry](TELEMETRY.md): capture use and today's authoritative routing
  fields.
- [Research preservation index](../research/README.md): located raw captures,
  duplicate archives, replay code, missing evidence, and provenance limits.
- [Development history](DEVELOPMENT_HISTORY.md): the engineering journey and
  the failures that changed it.
- [Presentation and safety](PRESENTATION_AND_SAFETY.md): Presence, Contrast,
  budgets, software headroom, and the hardware-safety boundary.
- [Roadmap](ROADMAP.md): current status and deferred work.

## Historical research records

These retain experiment detail and should not be read as the current overview:

Human-readable prose uses the current **HYP36Rforce FFB** identity. Historical
filenames, configuration keys, telemetry/schema identifiers, scenario IDs,
commit references, and quoted source language remain unchanged so old evidence
and links stay auditable. Research lanes remain separate until an explicit
integration milestone says otherwise:

- **ALPHA / FORCE CHARACTER**
- **ROAD 2.0**
- **R5 / AER**
- **EVENT 2.0**
- **2.0 INTEGRATION**

A cross-reference between lanes is a **CROSS-LANE CANDIDATE — NOT INTEGRATED**,
not evidence that implementation was merged.

Version language follows the same boundary. The current public release is
`v1.0.0`; current development builds identify as
`1.0.0-dev+<shortSHA>`; **HYP36Rforce FFB 2.0** describes a future milestone,
not the identity of today's development artifacts. Experimental findings in
the records below are not released features unless the public documentation
and release history explicitly say so.

- [HYP36Rforce Signal State R4.1](HYP36R_FORCE_2_0_R4_1_SIGNAL_STATE.md)
  implements the versioned, identity-preserving observation contract and its
  strict no-output boundary.
- [HYP36Rforce Road 2.0 R4.2 passive policy](HYP36R_FORCE_2_0_R4_2_ROAD_POLICY.md)
  interprets spatial occupancy, continuous native activity and surface
  transitions without producing wheel force.
- [HYP36Rforce Road 2.0 R4.2P passive presentation](HYP36R_FORCE_2_0_R4_2P_ROAD_PRESENTATION.md)
  compares direct and minimally conditioned presentation models, selects the
  direct evidence baseline, and records the cobblestone evidence gate.
- [HYP36Rforce Road 2.0 R4.2C cobblestone evidence UAT](HYP36R_FORCE_2_0_R4_2C_COBBLESTONE_UAT.md)
  provides the three-capture physical gate for continuous cobblestone evidence
  while Road 2.0 remains passive.
- [HYP36Rforce Road 2.0 R4.2C cobblestone evidence analysis](HYP36R_FORCE_2_0_R4_2C_COBBLESTONE_ANALYSIS.md)
  confirms continuous bilateral evidence in the stable controlled section and
  establishes the unsigned continuous-activity envelope with explicit caveats.
- [HYP36Rforce Road 2.0 R4.2W waveform research](HYP36R_FORCE_2_0_R4_2W_ROAD_WAVEFORM.md)
  traces the restored Xbox motor signals, tests native and AC-coupled signed
  carriers, and keeps active Road paused because no wheel waveform is yet
  evidence-backed.
- [HYP36Rforce Road 2.0 R4.2N native surface lineage](HYP36R_FORCE_2_0_R4_2N_NATIVE_SURFACE_LINEAGE.md)
  traces four native surface states through the material-coefficient lookup,
  speed scaling and composite Xbox motor presentation, establishing the mapped
  PC surface effect as procedurally generated unsigned tactile output.
- [HYP36Rforce Road 2.0 R4.2F force-presentation topology](HYP36RFORCE_ROAD_2_0_R4_2F_FORCE_PRESENTATION_TOPOLOGY.md)
  compares ConstantForce, periodic, spring/damper and no-new-effect topologies;
  it selects only a passive periodic-request study and preserves current Road.
- [HYP36Rforce Road 2.0 R4.2F-P passive periodic request](HYP36RFORCE_ROAD_2_0_R4_2FP_PASSIVE_PERIODIC_REQUEST.md)
  implements and replays time- and distance-domain research requests without
  entering Force composition or DirectInput; both remain research-only.
- [HYP36Rforce FFB Research II — R0 research map](HYP36R_FORCE_RESEARCH_II_R0.md)
  defines the post-v1.0.0 evidence boundary, unresolved native signals, future
  passive telemetry schema, and controlled capture campaign. It proposes no
  runtime or Force change.
- [HYP36Rforce FFB Research II — R1 passive schema](HYP36R_FORCE_RESEARCH_II_R1.md)
  records the append-only synchronized surface, native-corner, Force Character,
  gear/effect, composition, and final-request observation contract.
- [Research II R1 Windows schema UAT](HYP36R_FORCE_RESEARCH_II_R1_UAT.md)
  defines the reusable UAT package layout, safe automatic evidence directory,
  session provenance record, and short pre-R2 runtime validation.
- [Research II R2-A scenario-runner UAT](HYP36R_FORCE_RESEARCH_II_R2_UAT.md)
  is the operator guide for the 15 separately captured vehicle, surface, and
  transient scenarios used to build the controlled Reference+ dataset.
- [Research II R2-A vehicle and Force baseline analysis](HYP36R_FORCE_RESEARCH_II_R2A_ANALYSIS.md)
- [Research II R2-B surface and Road UAT](HYP36R_FORCE_RESEARCH_II_R2B_UAT.md)
- [Research II R2-B surface and Road analysis](HYP36R_FORCE_RESEARCH_II_R2B_ANALYSIS.md)
- [Research II R2-C transient and Impact UAT](HYP36R_FORCE_RESEARCH_II_R2C_UAT.md)
- [Research II R2-C transient and Impact analysis](HYP36R_FORCE_RESEARCH_II_R2C_ANALYSIS.md)
- [Research II integrated R2-A/B/C analysis](HYP36R_FORCE_RESEARCH_II_INTEGRATED_R2_ANALYSIS.md)
- [HYP36Rforce FFB 2.0 signal-preservation architecture](HYP36R_FORCE_2_0_SIGNAL_ARCHITECTURE.md)
- `S2_PASSIVE_OUTPUT_EXPOSURE.md` through
  `S10_PRESENTATION_FOUNDATION_CONSOLIDATION.md` preserve the presentation and
  software-headroom studies.
- `M5I_LATERAL_CONTEXT_SHADOW.md` and
  `M5J_ACTIVE_LATERAL_CONTEXT_EXPERIMENT.md` preserve the final M5 research
  equations, passivity boundary, and active test.
- `telemetry_probe.md` preserves the append-only schema's development from
  TP-01 through S9.
- `TELEMETRY_TEST_PROTOCOL.md` preserves the controlled capture procedures used
  by the research milestones.
- `F1_1_R1_REDETECT_INVESTIGATION.md` records the corrected freeze report, the
  duplicate-interface evidence, and the diagnostic boundary before any FFB
  device-lifecycle correction is accepted.
- `E1_*` through `E4_*` preserve the managed-root, native ENTIRETY, Save
  Recovery, licence-clone and race-start integration experiments. They are not
  player instructions or a product plan.

## Superseded but useful

- `HYP36R_FORCE_2_FOUNDATION.md` is the detailed frozen M4 foundation.
- `HYP36R_FORCE_2_1_RESEARCH_STATUS.md` is the detailed M5K research freeze.
- `HYP36R_FORCE_SIGNAL_LINEAGE.md` records the original legacy-force lineage
  before native M4/M5 integration.
- `HYP36R_DYNAMICS_REFERENCE_MODEL.md` records the methodology that constrained
  interpretation of the grip envelope.
- `HYP36R_INFORMATION_AMPLIFICATION_SAFETY_ARCHITECTURE.md` and
  `S4_PRESENTATION_CALIBRATION_ARCHITECTURE.md` retain the full design work that
  preceded the smaller canonical presentation document.
- `S5_PASSIVE_PRESENTATION_REPLAY.md` through
  `S9_ACTIVE_REFERENCE_PLUS.md` retain the equations and measurements behind
  Reference+.

## Redundant candidates

`multi_device_input.md` is now largely duplicated by the project README,
`HYP36R_FORCE.md`, and `ROADMAP.md`. It is retained for provenance in D1, but a
future housekeeping change may move it to a historical directory or remove it
after verifying that no external links depend on it.

The milestone documents should not be deleted merely because a canonical
summary exists. If they are moved later, preserve filenames or provide redirects
so old commit and discussion links remain understandable.
