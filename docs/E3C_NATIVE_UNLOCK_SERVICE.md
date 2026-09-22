# E3C native Unlock All service

Status: Historical feasibility research. This work is not part of the current
Multi Input product roadmap. Future development is undecided. The service and
developer instructions below refer to an experimental build, not the current
runtime.

Historical note: E4 replaced the temporary setting and Debug UAT button described below with the player-facing F11 Gameplay action. The E3C test workflow is retained here as research history, not current user instructions. See [E4 Gameplay Unlock All](E4_GAMEPLAY_UNLOCK_ALL.md).

E3C starts from `31be836` (Run #89, Win32 Release passed). This is a service
and temporary developer test, not the eventual Gameplay UI.

## Product boundary

Phase 1 intentionally leaves profile choice and persistence with the player.
Full managed-root isolation and native licence cloning were investigated in
E2/E3A/E3B; the clone passed offline tests. They remain useful research but
were not used by the E3C experimental service.
The player selects a profile, Multi Input transforms its current in-memory
licence through OutRun's own routine, and OutRun's **Save to Profile** remains
an explicit, separate player action. Without it, the unlock may not survive
a restart. No backup, clone, profile switch or save occurs in E3C.

## Native call evidence and gates

Only the supported `OR2006C2C.exe` with SHA-256
`68ceb386829066f8455b9d027320af962584321f3e2e8a79c72841495a6134c3`
may be called. The loaded process module must be the module Tweaks initialized
against, and the original instruction bytes at both the native caller and
callee must match. A modified or unknown executable fails closed.

At `0x4DE544`, the original caller moves the fixed active-licence address
`0x7C23E0` into ECX, then calls `0x447360`. The callee copies ECX into EDX,
saves EDI, writes fixed ranges within the 0x40C-byte licence payload, restores
EDI and returns with a plain `ret`; it takes no stack arguments and makes no
other calls. This supports the `void(__thiscall*)(void*)` adapter with the
active buffer as the sole argument. The adapter runs synchronously from the
game's F11 overlay render callback, not a worker or DLL initialization. It
does not execute the licence-name comparison or simulate input.

The native selected slot is read from `0x7B17F8` and must be 0..3. The active
payload at `0x7C23E0` must have its native occupancy bit at `+0x3F4` set.
The native save state at `0x7457A9` must be initialized and the game must be
in `STATE_MENU`. These checks avoid invoking during load, gameplay or without
a selected profile. They are not a promise of safety for other executables,
menu states or modified code. The service never changes the selected slot.

`UnlockAllContent()` reports one of `SUCCESS`, `UNSUPPORTED_EXECUTABLE`,
`NO_ACTIVE_LICENCE`, `INVALID_NATIVE_CONTEXT`, `INVOCATION_FAILED`, or
`VERIFICATION_FAILED`, along with the slot and separate `invoked` and
`verified` flags. Verification checks only the deterministic native write
ranges (FF, 06, 00 and 6666 fills) in the active payload. It does not claim
that every content cache or on-disk profile has been verified. Physical UAT
must confirm visible content and, separately, optional save persistence.

The native routine overwrites fixed ranges with constants and does not read
their previous values. Repeating it on the same loaded licence is therefore
idempotent at the payload level. No additional guard is needed; the developer
button may be pressed again, but it never saves or duplicates anything.

## Developer-only UAT

`[Developer] E3CNativeUnlockUAT=true` in the user INI exposes **Invoke Native
Unlock All** in the existing F11 main-overlay **Debug** tab. The option is
hidden from normal settings and defaults to false. The action logs the native
slot index, supported-EXE result, invocation result and verification result,
never the ordinary licence name. It does not require E2 MANAGED_TEST or any
helper EXE.

1. On the supported EXE, enable the developer option, restart, and select a
   disposable/test OutRun profile. Make sure the game is at a menu.
2. Press F11 (or the configured main-overlay toggle), open **Debug**, and
   press **Invoke Native Unlock All** once.
3. Return to OutRun and check that content is available. If testing
   persistence, choose OutRun's normal **Save to Profile**, restart and check
   again; otherwise do not save. Share the E3C log result if it fails.

No ENTIRETY text entry is required. Do not run the E3B helper manually.

## E3B helper disposition and test limits

`experience_clone_helper.exe` remains buildable in `build/research` for E3B
provenance, but it is no longer under `build/bin` and is excluded from future
normal controlled-test artifacts. It is not an E3C or v1 runtime dependency.
The Win32 offline test exercises menu/selection gates, the fixed-range
verifier and unsupported-process rejection. It does **not** pretend to invoke
OutRun's native code. The first real invocation and player-visible behavior
remain for physical UAT.
