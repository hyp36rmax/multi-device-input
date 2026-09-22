# Research preservation index

This is an inventory, not a new interpretation of the captures. Audit date:
2026-09-22. The raw CSVs below were found in
`/Users/felinnimarinas/Downloads/`, **outside Git**. That location is not a
durable project archive. No raw CSV was copied, moved, or edited in P1. The
repository preserves the analysis documents and one replay program, but a
future developer will need a separate, permissioned archive of the original
CSVs (with checksums and access notes) to reproduce the research elsewhere.

`RAW PRESERVED` means the named file was present on this Mac at audit time,
not that it is backed up. `DOCUMENTED RESULTS ONLY` means the finding is in a
document but no separate raw capture for that milestone was located. `UNKNOWN`
means the audit cannot establish whether such a capture was ever made. ZIPs
alongside extracted CSVs are alternate copies, not independent experiments.

## What built HYP36R Force

| Evidence | Status and current location | What it supports | Record |
| --- | --- | --- | --- |
| TP-01C early T02A captures (eight `telemetry_20260913_*.csv`) | RAW PRESERVED: Downloads; also `Archive.zip` (DUPLICATE) | Early steering-response observation, before native semantics were assigned | [Probe history](../docs/telemetry_probe.md), [development history](../docs/DEVELOPMENT_HISTORY.md) |
| TP-02C T10–T16 and T10A–T16A | RAW PRESERVED: Downloads folders `Outrun 2006 T10-T16 TEST/` and `Outrun 2006 RETEST 48 T10-T16/`; each also has a ZIP (DUPLICATE) | Controlled and retested steering-response relationships | [Test protocol](../docs/TELEMETRY_TEST_PROTOCOL.md), [native dynamics](../docs/NATIVE_DYNAMICS.md) |
| M3C-01–03 | RAW PRESERVED: Downloads `Outrun 2006 (M3C)/`; ZIP duplicate | Passive vehicle-state interpreter comparison | [Probe history](../docs/telemetry_probe.md), [native dynamics](../docs/NATIVE_DYNAMICS.md) |
| M3E physical test | RAW PRESERVED: Downloads `telemetry_20260919_004208 (M3E).csv`; two similarly named ZIPs (DUPLICATE, archive equivalence not verified) | Synchronized native/synthetic comparison | [Force foundation](../docs/HYP36R_FORCE_2_FOUNDATION.md) |
| M4B, M4C Legacy/Active, M4E, M4F, M4G, M4G-R1 | RAW PRESERVED: named CSVs in Downloads; most have ZIP duplicates, M4C pair is also in `Archive 2.zip` | Shadow composer, divergence, BITE restoration, routing proof | [Force foundation](../docs/HYP36R_FORCE_2_FOUNDATION.md), [telemetry semantics](../docs/TELEMETRY.md) |
| M5B C1–C3 | RAW PRESERVED: Downloads `Outrun 2006 (M5B)/`; ZIP duplicate | Four-corner candidate behavior under controlled contact | [Native dynamics](../docs/NATIVE_DYNAMICS.md), [probe schema](../docs/telemetry_probe.md) |
| M5C known-left surface | RAW PRESERVED: Downloads `telemetry_20260919_191133 (M5C).csv`; ZIP duplicate | Corner ordering and lateral surface context | [Native dynamics](../docs/NATIVE_DYNAMICS.md) |
| M5D context runtime | RAW PRESERVED: Downloads `telemetry_20260919_194555 (M5D).csv`; ZIP duplicate | Derived axle/side/corner context | [Force 2.1 status](../docs/HYP36R_FORCE_2_1_RESEARCH_STATUS.md) |
| M5G contextual intent | RAW PRESERVED: Downloads `telemetry_20260919_202300  (M5G).csv`; ZIP duplicate; header schema is `M5F` | Runtime check of the M5F context-informed intent | [Force 2.1 status](../docs/HYP36R_FORCE_2_1_RESEARCH_STATUS.md) |
| M5H | DOCUMENTED RESULTS ONLY: no M5H-named CSV located | Perceptual mapping was considered; cross-car evidence remained limited | [Force 2.1 status](../docs/HYP36R_FORCE_2_1_RESEARCH_STATUS.md) |
| M5I shadow | RAW PRESERVED: Downloads `telemetry_20260919_211815 (M5I).csv`; ZIP duplicate | Passive lateral-context communication | [M5I study](../docs/M5I_LATERAL_CONTEXT_SHADOW.md) |
| M5J initial M4/Active and corrected Active | RAW PRESERVED: Downloads `telemetry_20260920_183032 (M5j-M4).csv`, `183903 (M5J_LATERAL ACTIVE).csv`, and `192334 (M5J_LATERAL ACTIVE FIXED).csv`; first pair also `Archive 3.zip`, corrected CSV also ZIP | Invalid initial A/B, then runtime-active selection | [M5J study](../docs/M5J_ACTIVE_LATERAL_CONTEXT_EXPERIMENT.md), [history](../docs/DEVELOPMENT_HISTORY.md) |
| S2 M4/M5 | RAW PRESERVED: Downloads `telemetry_20260920_202027 (S2 M4).csv` and `202708 (S2 M5).csv`; also `Archive 4.zip` | Normalized output exposure and passive headroom | [S2](../docs/S2_PASSIVE_OUTPUT_EXPOSURE.md), [S3](../docs/S3_PASSIVE_INFORMATION_AMPLIFICATION_HEADROOM_STUDY.md) |
| S3–S8 | DERIVED DATA PRESERVED in milestone documents; no separate milestone-named raw CSV located | Replays and policy studies reuse S2, M5I, and M5J captures | [Documentation map](../docs/README.md) |
| S9 / S10 / Reference+ | RAW PRESERVED: Downloads `telemetry_20260921_075311 (S9).csv`; ZIP duplicate. S10 has DOCUMENTED RESULTS ONLY and reuses S9 | Reference+ physical capture, presentation/routing checks, consolidation | [S9](../docs/S9_ACTIVE_REFERENCE_PLUS.md), [S10](../docs/S10_PRESENTATION_FOUNDATION_CONSOLIDATION.md) |
| Reference+ 1.44 / F1.2 | DERIVED DATA PRESERVED in docs and replay program; no separate 1.44 raw CSV located in this audit | Counterfactual 1.44 channel study based on the older S9 Presence 1.20 capture, **not** a new physical capture | [F1.2](../docs/F1_2_FORCE_CHARACTER_CEILING_RESEARCH.md), [`f12_replay.py`](f12_replay.py) |

## Capture context and limits

The inspected TP-02C, M3, M4, M5 and S2/S9 CSV headers record Ferrari Dino
246 GTS, Sunny Beach, Fanatec DD2, hardware FFB 50%, and game FFB 100%.
They include `test_scenario`, `telemetry_probe_version`, `tweaks_version`
(`0.6.1.0`), and a local start time. The M5G filename denotes the test, while
its header retains the M5F schema: do not silently relabel the schema. The
M5C header similarly retains M5B. The header does **not** record an exact Git
commit, firmware version, or wheel-driver version; those are UNKNOWN unless a
linked engineering record establishes them. A milestone name is not proof of
the binary's exact commit. These are predominantly one-car/one-route/one-wheel
observations, not universal hardware calibration.

The M4G telemetry review initially made restored BITE look absent from the
hardware path. A code trace showed that M4F restoration had reached hardware;
M4G-R1 corrected stale field semantics and the following capture proved the
route. The first M5J A/B was invalid because mode selection occurred before
settings initialization; the corrected capture is the active comparison.
Neither failure should be discarded or counted as a valid negative result.

| Correction or rejected path | What changed the direction | Surviving record |
| --- | --- | --- |
| Early telemetry and Windows build failures | Capture controls initially missed the effective settings path; new units exposed Windows-header and build-list assumptions | [History](../docs/DEVELOPMENT_HISTORY.md), [probe history](../docs/telemetry_probe.md) |
| M4 routing alarm | Stale labels implied M4C-only output; code trace and M4G-R1 hardware-selection fields proved restored M4F was routed | [Telemetry](../docs/TELEMETRY.md), [foundation](../docs/HYP36R_FORCE_2_FOUNDATION.md) |
| M5 surface and lateral hypotheses | Candidate corner quantities gained bounded context, not proven physical units; invalid M5J A/B was repeated after initialization-order repair | [Native dynamics](../docs/NATIVE_DYNAMICS.md), [M5J](../docs/M5J_ACTIVE_LATERAL_CONTEXT_EXPERIMENT.md) |
| DD2 high-output shutdown | One wheel/setup entered protection or power shutdown; no universal safe torque or software threshold followed | [Presentation and safety](../docs/PRESENTATION_AND_SAFETY.md) |
| E2 instrumentation crash and later race-start regression | Overlapping diagnostic hooks caused the first event-1 crash; later unlock integration was removed from the stable product lineage after race-start failure | [E2](../docs/E2_SAVE_PATH_NATIVE_ENTIRETY_VERIFICATION.md), [history](../docs/DEVELOPMENT_HISTORY.md) |
| Helper-app approach | A controlled licence-clone helper was tried for feasibility, then rejected as a player-package dependency; source/history remain, runtime package excludes it | [E3B](../docs/E3B_NATIVE_LICENCE_SLOT.md), [docs map](../docs/README.md) |
| Extra branch paths | The project consolidated on the single `multi-device-input` development branch; milestone names are not branches | [Docs map](../docs/README.md) |
| F1.2 ceiling study | One-car/one-route replay at counterfactual Presence 1.44 did not justify independent player amplification ceilings | [F1.2](../docs/F1_2_FORCE_CHARACTER_CEILING_RESEARCH.md) |

## Reproduction and archival boundary

[`f12_replay.py`](f12_replay.py) is the **only replay/analysis script found in
this repository**. S3–S8 and other milestone results survive in Markdown;
their original one-off replay scripts were not located in the audited project
workspace (UNKNOWN, not asserted deleted). No raw CSV is tracked in Git.
`Archive 5.zip` and `Archive 6.zip` contained no telemetry CSV paths in the
file listing and are not counted as telemetry backups.

Before deleting or moving any local Downloads copy, create an intentional
research archive of the source CSVs, record SHA-256 per file, verify a restore,
and update this index to its stable location. Do not rely on the ZIP/extracted
pairs alone as independent preservation. No such archive was created in P1.
