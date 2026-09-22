# Documentation map

The canonical documents describe the project as it exists now. The milestone
notes remain beside them because they contain the experiments, replay tables,
mistakes, and corrections that support the current conclusions.

`multi-device-input` is the one active development branch. F1, F1.1, F2, E3,
E4, and the S-series name engineering milestones, not ongoing Git branches.
FFB Experience is part of Multi Input, alongside Controller Experience,
HYP36R Force, and Developer/Telemetry. The E-series save/unlock documents are
historical research records; their experimental runtime is not in the current
stable product. The earlier commits remain in Git history.

## Canonical

- [HYP36R Force](HYP36R_FORCE.md): current force architecture and limitations.
- [Native dynamics](NATIVE_DYNAMICS.md): native state, four-corner structure,
  evidence confidence, and unknowns.
- [Telemetry](TELEMETRY.md): capture use and today's authoritative routing
  fields.
- [Development history](DEVELOPMENT_HISTORY.md): the engineering journey and
  the failures that changed it.
- [Presentation and safety](PRESENTATION_AND_SAFETY.md): Presence, Contrast,
  budgets, software headroom, and the hardware-safety boundary.
- [Roadmap](ROADMAP.md): current status and deferred work.

## Historical research records

These retain experiment detail and should not be read as the current overview:

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
