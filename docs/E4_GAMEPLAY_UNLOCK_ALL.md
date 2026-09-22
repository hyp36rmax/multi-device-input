# E4: Gameplay Unlock All and clean product artifact

Baseline: E3C commit `84a146882479888be45a18562e05e54ea01f6a61`, Windows Run #90 passed. E2 tested the native unlock and ordinary game-save persistence. E3A built restore-point research; E3B tested licence cloning with a separate helper. E3C changed Phase 1 direction to an in-process native service. E4 makes that service a player action and retires the temporary UI routes. This history is not a claim that the earlier managed-profile design shipped.

## Run #90 artifact audit

Run #90 uploaded the whole `build/bin/` tree, not a selected package. This inventory is derived from its workflow and CMake output paths; the historical ZIP was not available locally for a byte-level audit. The E4 workflow now prints and checks the exact uploaded file list.

| Run #90 file or output | Classification | E4 disposition |
| --- | --- | --- |
| `dinput8.dll` | Production required | Ship at package root |
| `OutRun2006Tweaks.ini`, `OutRun2006Tweaks.lods.ini` | Optional user configuration | Ship defaults |
| `README.md`, `LICENSE.md` | Production documentation/licensing | Ship |
| `OR2006C2C.exe` | Validated upstream replacement executable | E4-R1 ships only the hash-verified copy from the established upstream release source |
| `save_recovery_test.exe` | Test only | Build and run in CI; do not ship |
| `experience_licence_test.exe` | Test only | Build and run in CI; do not ship |
| `native_unlock_test.exe` | Test only | Build and run in CI; do not ship |
| `dinput8.pdb`, `dinput8.map` where emitted under `build/bin/` | Research/debug only | Do not ship |
| `experience_clone_helper.exe` | E3B research only; built under `build/research/`, not the Run #90 `build/bin/` upload | Preserve source, do not ship or invoke from the DLL |

E4-R1 corrects the five-file policy: the controlled-test artifact requires six files, `OR2006C2C.exe`, `dinput8.dll`, the two INIs, `README.md`, and `LICENSE.md`. CI obtains the replacement EXE from the same [upstream v0.1 release URL](https://github.com/emoose/OutRun2006Tweaks/releases/download/v0.1/OR2006C2C.EXE) used by upstream's [distribution workflow](https://github.com/emoose/OutRun2006Tweaks/blob/master/.github/workflows/build.yml). Before any test or packaging, CI compares its SHA-256 against the single `SupportedExecutable::Sha256` constant in `src/supported_executable.hpp`; the in-process native unlock and E3B research verifier use that same constant. CI logs the verified hash, rechecks the staged copy, and fails closed on a mismatch.

CI still builds and executes the three test programs before staging. The package assertion fails on any missing or extra file, or any EXE other than the verified `OR2006C2C.exe`. This excludes temporary telemetry and research utilities. The game executable is an intended replacement distribution file, not a Multi Input helper process; no Multi Input gameplay feature requires a companion process. Redistribution continues to follow the upstream project's existing release and licensing terms.

## Player flow

The old overlay had a diagnostic Debug section labelled Gameplay, not a player-facing Gameplay tab. E4 adds the tab to F11. Its single action is **Unlock All Content**, with an Unlock button and a hover-only information icon. The tooltip explains current-profile scope and how to preserve existing progress. Unlock opens one modal; Cancel is first and receives initial keyboard focus. Confirmation directly calls `NativeUnlock::UnlockAllContent()` on the render thread. Success and each service failure map to plain-language messages; the log retains service code, selected slot, executable gate, invocation and verification outcomes. There is no toggle, helper, profile switch, backup, name entry, automatic save or additional confirmation.

Unlock immediately changes the currently active in-memory profile. It may not persist across restart unless the player uses OutRun's own **Save to Profile**. E2 physical testing established that a normal save can persist the unlocked state. E4 does not silently save. The native service's supported-executable, selected-profile, initialized-save, safe-menu, transformation-verification and idempotence gates remain unchanged.

The E3C hidden setting and Debug UAT button are removed. The E3B helper-launch button and startup helper probe are removed from the DLL; its source and E3A/E3B engineering records remain for provenance. The E2 disposable managed-root research mechanism remains hidden and is not part of Unlock All.

## Validation boundary

Offline Win32 CI confirms the service safety tests, licence-clone and restore-point tests, build, executable hash, and package allowlist. It cannot prove the in-game UI or visible content unlock. The E4-R1 physical test must use the complete six-file artifact, including its verified `OR2006C2C.exe`: select a disposable/new OutRun profile; open F11 Gameplay; hover the information icon; press Unlock; inspect the confirmation; confirm; verify content becomes available. Optional persistence retesting is needed only if observed behavior differs from E2.

Release invariant: the player launches OutRun normally with `dinput8.dll`; no Multi Input helper EXE, launcher, service or manually run test tool is required.
