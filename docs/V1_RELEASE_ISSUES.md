# V1 release issue matrix

Baseline: `33bd3cf90aac53099856fb611429d5209ca9e0b6` (Run #106) plus the documentation-only F1.2 commit `5264ad7`. HYP36R Force is frozen for V1: Reference+ Presence 1.44, Strength and the three independent attenuation controls at 0–100%, all defaulting to 100%. Vibration is not a player wheel control. This record supersedes the status tables in earlier experiment notes; those notes remain historical evidence. A code or offline result below is not a claim of physical validation.

| Item | Status | Evidence and RC check |
| --- | --- | --- |
| Reference+ 1.44 | UNVALIDATED PHYSICALLY | Offline replay passed and initial driving feedback was positive. Combined hardware UAT at this exact candidate remains open. No further intensity change for V1. |
| Force Character separation and bounds | VALIDATED OFFLINE | Existing channel-isolation test passes. Each 0–100 setting attenuates its own resolved contribution; 100% is identity. F1.2 found no validated above-Reference ceiling. |
| Duplicate FFB interfaces | UNVALIDATED PHYSICALLY | Run #101 showed a DD2 interface advertise FFB and enable actuators but fail effect creation. The generic one-unit create/start probe and same-name sibling order exist. Physical fallback on the affected setup still needs confirmation. |
| Endpoint preference after restart/reconnect/order change | UNVALIDATED PHYSICALLY | Instance GUID, not enumeration index, is the saved key. This pass adds offline tests for candidate order and preserves an absent saved preference while using a temporary endpoint. Confirm with a reconnect/restart on the actual wheel. |
| Left/Right Strength independence | FIXED | The test previously multiplied its request by global Strength. It now sends a fixed 20% nominal request for 350 ms; inversion remains single-path. Offline policy tests pass. |
| Left/Right `0x80040205` | NOT REPRODUCED | Run #101 established effect creation failure on one interface. Its provider-level root cause is unproven; no post-resolver physical reproduction has been supplied. Do not describe the error itself as fixed. |
| Re-detect freeze | NOT REPRODUCED | One report, later attempts did not reproduce it, and the Run #101 log did not show the click callback. Phase diagnostics remain. Static audit found no recursive enumeration, internal retry loop, persistent UI vector reference across refresh, or Controller-assignment reset. DirectInput calls remain synchronous and could block. |
| FFB settings persistence | UNVALIDATED PHYSICALLY | UI changes write the user INI. Default ranges and endpoint preference are checked in code; restart/reconnect needs a physical pass. |
| Invert Wheel | UNVALIDATED PHYSICALLY | Live drive and tests each use the existing single inversion setting. Check both test labels and live steering on hardware. |
| FFB Reset to Defaults | VALIDATED OFFLINE | Code restores Active/Reference+, Strength and three channels to 100%, Invert Off, and clears an old disabled-FFB override. It does not touch bindings, telemetry, M5 mode or device ownership. Physical UI check remains. |
| P1/P2 ownership | OPEN — REPRODUCTION REQUIRED | The transient primary gamepad is not a proven P1/P2 defect. OutRun currently receives one aggregated player state. No seat-management or Controller-menu redesign. |
| Sign In mapping | OPEN — REPRODUCTION REQUIRED | Only a second-hand “could not map” report exists. Native menu action is contextual; screen, binding and expected/actual action remain unknown. No guessed hook. |
| Change Car Class mapping | OPEN — REPRODUCTION REQUIRED | Same missing reproduction. The existing `0x2000` car-list input candidate does not prove the full contextual action. No speculative binding. |
| Fresh-install INI | VALIDATED OFFLINE | Strict parser accepts the shipped INI: 11 sections, no duplicate sections/keys. Developer defaults are `Active`, `M4_ONLY`, `REFERENCE_PLUS_EXPERIMENTAL`; no E2–E4 setting is shipped. Missing numeric controls keep defaults and ranged values clamp. Invalid presentation mode deliberately falls back to Reference, not an amplified mode. Fresh launch still needs UAT. |
| Race start on current candidate | UNVALIDATED PHYSICALLY | The restored #78 foundation launched and drove. This changed DLL must pass launch, race start and normal driving before RC. No E1–E4 Unlock/save runtime work was brought forward. |
| Windows `0xc0000142` dependency | VALIDATED PHYSICALLY | An affected PC launched after repairing the x86 and x64 Microsoft VC++ redistributables. A loader failure can precede the patch logger. README explains the repair and Event Viewer evidence; no helper installer is bundled. |
| Controlled artifact | VALIDATED OFFLINE | CI packaging requires the six approved files, including the upstream-sourced `OR2006C2C.exe` with SHA-256 `68ceb386829066f8455b9d027320af962584321f3e2e8a79c72841495a6134c3`. Helper/test EXEs are excluded. Confirm the next CI artifact. |

## Current code audit

The endpoint resolver first tries the saved interface, then same-name siblings, then remaining candidates. It validates actual effect creation and start rather than trusting a capability flag. If the saved GUID is absent, a usable temporary endpoint can drive without replacing the preferred GUID in the user INI; the menu displays the active endpoint. If a present saved endpoint fails and a sibling succeeds, the winner is persisted. The test effect remains independent of Reference+ Presence, Force Character and global Strength. This pass changes no force-generation equation or output conditioning.

Re-detect stops effects, releases only the FFB device, re-enumerates candidates, validates the endpoint and returns to the FFB menu. The status/log phases retained from `c5bb31e` identify the last completed call if a later hang occurs. It does not reset SDL Controller bindings. Because no matching freeze has been reproduced, there is no freeze-specific code fix in this pass.

## One combined physical UAT before RC1

Install the new six-file CI artifact into a fresh game directory. Launch without editing an INI, confirm Reference+ and the wheel name, start a race and drive. In Advanced FFB, reduce and restore Steering Load, Road Detail and Impact one at a time. Check Invert Wheel during a brief Left and Right test, then turn it back off. Set Strength below 100% and confirm the direction tests remain at the same gentle level; live driving Strength should still change. Press Re-detect once, return to the race and verify FFB continues. Exit normally, restart, and verify profile, settings and the working wheel endpoint persisted. Capture a log only if something behaves unexpectedly. Do not raise hardware torque for this test.

RC1 gate: green Win32 CI and a clean combined physical pass. Keep P1/P2, Sign In and Change Car Class on the reproduction-required list. If direction tests still fail with `0x80040205`, collect the log and endpoint identity; do not advance RC1 by relabeling that error fixed.
