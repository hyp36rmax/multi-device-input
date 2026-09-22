# FFB Experience: Reference+ intensity and known-issue polish

This pass starts at `c5bb31ed1e1b1306eb9df6d31598eb77db909d59` on `multi-device-input`. It keeps the restored race/gameplay lineage and the three functional Force Character controls. No Controller, Unlock, save, M4, M5, road-generation, impact-generation, or DirectInput force equation was changed.

## The +15% Reference+ change

Reference+ Presence moves from `1.20` to `1.38` (`1.20 × 1.15`) in `HYP36RPresentation::initialize()`. The change multiplies only the already policy-limited directional value. Reference still has Presence `1.00`. Contrast `4`, the five-percent M5 secondary budget, the Legacy secondary boundary, eligibility gates, road, impact, output ramp, master Strength, inversion, and DirectInput clipping are unchanged. At Steering Load 100%, Force Character remains an identity operation.

I replayed the S9 physical capture `telemetry_20260921_075311 (S9).csv` (19,931 frames, approximately 332 seconds). For each frame the candidate composer input was the recorded `s2_composer_input + 0.18 × s9_directional_pre_presence`. I applied the existing `tanh` and the recorded output ramp inferred from `ffb_raw / s2_post_tanh` where that ratio was numerically stable. The recorded output was used as the baseline. Quantized CSV values make this an offline approximation, not a hardware torque measurement.

| Normalized software output | Current Reference+ | +15% candidate |
| --- | ---: | ---: |
| Instantaneous P95 / P99 / maximum | 0.378874 / 0.468083 / 0.588180 | 0.423924 / 0.518235 / 0.630034 |
| One-second RMS P95 / maximum | 0.327202 / 0.446214 | 0.367179 / 0.497060 |
| Three-second RMS P95 / maximum | 0.279331 / 0.356084 | 0.314841 / 0.398608 |
| Three-second occupancy frames at or above .75 / .90 / .98 | 0 / 0 / 0 | 0 / 0 / 0 |
| Instantaneous frames at or above .75 / .90 / .98 | 0 / 0 / 0 | 0 / 0 / 0 |
| DirectInput clamp candidates at Strength 100% | 0 | 0 |

No candidate output was nonfinite. The candidate's directional contribution had zero primary-sign violations and created no force from zero primary. The 1,347 frames with permitted M5 information retained the same M5 decisions; no secondary-budget violation appeared. Road and impact samples were not rescaled. Final composed output crossed sign on 48 frames where opposing directional and road/impact contributions were already nearly cancelling: the largest old/new magnitudes in those frames were 0.01667/0.01993. This is not a reversal of directional steering authority. Output slew P99 moved from 4.589 to 4.818 normalized units per second; the maximum moved from 19.139 to 19.539. No existing software invariant failed in this capture. None of these normalized values establishes safe torque, comfort, or fatigue on a particular wheel.

## Wheel endpoint and direction tests

Previously the resolver trusted a driver's FFB capability flag and successful actuator command. The Run #101 DD2 log shows why that was insufficient: the 8-axis/108-button interface reached `ready`, but the two-axis test creation returned `0x80070057` and the one-axis fallback returned `0x80040205`. The exact meaning of the latter vendor/runtime result is not established by that log; it does not prove the 12-axis sibling succeeds on that session.

Each candidate now has to create and start a one-unit constant-force probe (0.01% nominal, stopped immediately) before it is marked ready. Failure moves to a sibling interface with the same product name, or to the next attached candidate when the saved interface is absent. This is a generic capability test, not a Fanatec rule. A winning interface ID is saved to the user settings for the next launch. The normal menu shows one resolved wheel name; there is no duplicate-device picker. The 350 ms Left/Right tests still request at most 20% nominal output and use the same single Invert Wheel setting as live drive. They do not use Reference+ Presence or Force Character gains. Whether this resolves the reported `0x80040205` on the affected hardware remains a physical UAT question, not a claimed root-cause fix.

## Settings and known issues

Strength and the three Force Character sliders default to 100% and clamp to 0–100% in both the player UI and settings parser. Invert Wheel defaults Off, writes through the same user-settings path, and is applied once in live drive and once in the bounded tests. FFB Reset to Defaults now restores `Force2Mode=Active`, `PresentationMode=REFERENCE_PLUS_EXPERIMENTAL`, Strength 100%, the three channel sliders 100%, and Invert Wheel Off. Profile changes require restart. Reset does not touch controller bindings, telemetry, or other Developer settings.

| Issue | Status at code/CI stage | Evidence or remaining check |
| --- | --- | --- |
| Duplicate FFB interfaces and endpoint fallback | STILL OPEN physically | Generic effect validation and same-name fallback implemented; needs a DD2 test. |
| Left/Right `0x80040205` | STILL OPEN | Exact provider cause unproven; new resolver avoids calling an endpoint that fails the same effect-initialization check. |
| Re-detect intermittent freeze | NOT REPRODUCED | Run #101 log did not show the click callback; phase diagnostics from `c5bb31e` remain. No speculative refresh change. |
| Wheel endpoint binding persistence | FIXED in code, physical check pending | Validated winning GUID is saved; missing GUID triggers rediscovery. |
| Controller binding persistence | FIXED in existing lineage | Device-aware bindings and saved identity paths untouched by this pass; verify once on restart/reconnect. |
| Invert Wheel and Force Character persistence | FIXED in code, physical check pending | Each slider/checkbox writes to the user INI through `setting_changed`/`flush_settings`. |
| FFB Reset to Defaults | FIXED in code, physical check pending | Now includes the two profile selectors; unrelated settings stay untouched. |
| Clean fresh-install race startup | STILL OPEN for this build | The restored baseline was physically validated; the new build needs the combined pass. |
| Windows `0xc0000142` on an affected PC | DEFERRED to Windows dependency repair | It can occur before our logger starts; README points to current x86/x64 VC++ redistributables and Event Viewer. |
| Duplicate `[Developer]` entries in a hand-edited INI | DEFERRED | The shipped INI has one section; the settings reader still rejects malformed/duplicate entries. Normal setup needs no INI editing. |

The prior Re-detect investigation remains in `F1_1_R1_REDETECT_INVESTIGATION.md`; it should not be rewritten as a confirmed hang fix. The player artifact still requires the hash-validated replacement `OR2006C2C.exe` and only the six approved files.

## One physical pass before release

Fresh install from the new six-file artifact; launch without INI edits; confirm Reference+ and the connected wheel; start a race and drive a familiar route at Strength 100% with a conservative hardware torque limit; adjust Steering Load, Road Detail, and Impact independently; test Invert Wheel; run Test Left and Test Right; click Re-detect Wheel once; continue driving and check for stable FFB, no protection behavior, and normal exit. Capture a log only if an anomaly appears.
