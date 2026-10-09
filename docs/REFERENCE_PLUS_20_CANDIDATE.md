# Reference+ 1.44 controlled candidate

The candidate changes only `HYP36RPresentation::initialize()` Presence from `1.38` to `1.44`, or 20% above the historical `1.20` S9 value. Reference, Force-generation equations, M4/M5 policy, road, impact, Strength ceiling, Force Character, inversion, and DirectInput conditioning are unchanged. This is an offline-validated **physical candidate**, not a frozen tune.

The 19,931-frame S9 capture `telemetry_20260921_075311 (S9).csv` was replayed with candidate composer input equal to the recorded composer input plus `(Presence - 1.20) × s9_directional_pre_presence`, followed by the existing `tanh` and inferred recorded output ramp. CSV quantization makes these software-output approximations. They do not establish wheel torque or comfort.

| Normalized output metric | 1.20 | 1.38 | 1.44 |
| --- | ---: | ---: | ---: |
| Instantaneous P95 | .378875 | .423924 | .439410 |
| Instantaneous P99 | .468083 | .518235 | .534713 |
| Instantaneous maximum | .588180 | .630034 | .643234 |
| One-second RMS P95 / maximum | .327202 / .446214 | .367179 / .497060 | .380328 / .513316 |
| Three-second RMS P95 / maximum | .279331 / .356084 | .314841 / .398608 | .326565 / .412310 |
| Frames at or above .75 / .90 / .98 | 0 / 0 / 0 | 0 / 0 / 0 | 0 / 0 / 0 |
| Three-second occupancy at those thresholds | 0 / 0 / 0 | 0 / 0 / 0 | 0 / 0 / 0 |
| Slew P99 / maximum (normalized units/s) | 4.588520 / 19.138764 | 4.818305 / 19.538802 | 4.961191 / 19.671219 |

No replay output was nonfinite or clamped. Directional contribution had no primary-sign reversal and created no force from zero primary. The 1,347 permitted M5 frames retained their secondary-budget decisions. Final composite output crossed sign near cancellation with unchanged road/impact contributions on 48 frames at 1.38 and 61 at 1.44; the largest compared magnitudes at 1.44 were .024202/.026797. This is not evidence of steering-direction reversal. No established software invariant failed in this capture.

## Scope and UAT status

- Run #105's wheel endpoint, test direction, Re-detect, Force Character, inversion, and persistence changes are retained, but their combined physical UAT remains incomplete.
- P1/P2 ownership is a possible multi-gamepad limitation, not a reproduced bug; controller behavior is unchanged.
- Sign In and Change Car Class mapping reports lack a reproducible screen/button sequence. Static native-menu candidates are recorded in `HOTFIX_R1_CONTROLLER_MENU_AUDIT.md`, but neither action is claimed fixed.
- A fresh-install driving pass is required before 1.44 can be approved. Keep a conservative hardware torque limit and report any oscillation, saturation, or wheel protection behavior.
