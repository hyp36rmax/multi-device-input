# R2 cleanup regression audit: controlled pre-cleanup test

## Evidence and scope

S10 at `7036d3d8b6cfd4af6454636ce3e0c1c1bed2b35b` is the last physically confirmed driving baseline. Later race starts repeatedly fault at `OR2006C2C.exe` `0x00405F9D`. This record audits the public branch through `a330c550e80b3afb4bcdef0b3336e1991340eb18`; the separate isolation build's source commit was not available here. This test does not alter historical runtime code.

## Cleanup candidates, chronological

| Commit | Actual change | Race-start relationship |
| --- | --- | --- |
| `a247f0e` | Documentation consolidation only. | No runtime change. |
| `497935e` | Removes six E2 licence-comparison/action midhooks and their diagnostic flags/callbacks. Retains the save-root inline hook and one licence-edit event midhook. | First meaningful cleanup boundary. Removed hooks were installed only in `MANAGED_TEST`, so direct relevance to default `ORIGINAL` driving is weak. |
| `b176c24` | Removes E3B/E3C developer UI and helper-status reads; adds Gameplay Unlock UI and changes packaging. The retained E2 save-root hook remains. | Not behavior-neutral overall, but the removed calls were debug/UI paths, not a demonstrated race-start dependency. Later removal of Native Unlock runtime did not resolve the physical crash. |

No candidate above changes DirectInput, wheel initialization, FFB equations, race/session constructors, or the Force output path in its cleanup diff. The E2 cleanup removes hook registrations and static diagnostic state; it does not change global initialization order or pointer ownership directly. The E4 cleanup removes conditional helper file reads and UI callbacks, and adds a new overlay tab; it does not change the native scene object's known creation path. These conclusions are from the diffs, not a claim that the cleanup is safe.

## Crash address

The existing dump reports `0xC0000005` reading `0x0975A000` at `0x00405F9D`. In the hash-validated supported EXE, `0x00405F9D` is `cmp [eax+ebx*4+0xC], ebp`. Dump registers give `eax=0x09759FF0`, `ebx=1`, and `ebp=0`, exactly producing the fault address. The containing native function begins at `0x00405F00` and is called from the neighboring routine at `0x00405D0E`. Nearby calls are consistent with scene/graphics processing, but the precise record type, allocator, owner, and expected lifetime are not established by this dump. Do not label the bad pointer as a specific wheel, save, or licence object.

The first earlier post-S10 runtime addition, E2 `bfad932`, installs an inline hook at EXE-relative `0x5B70` even in `ORIGINAL` mode. Disassembly of the supported EXE places absolute `0x00405B70` within the relative operand of a call beginning at `0x00405B6E`. This is a separate, potentially stronger hook-site concern, not proof of the current crash. Do not change it in this R2 test.

## Controlled boundary and decision

The selected pre-cleanup source is `a7f66b6a30a6d77cace33625c0fdd89820b8fb0b`, the immediate parent of post-cleanup `497935e1c21515e78d480e8dab50c61bbca6c310`. The test build changes only CI staging: it packages the historical DLL with the supported, SHA-256-checked EXE and four normal companion files. Its staged INI selects the established S9/S10 `Active` + `REFERENCE_PLUS_EXPERIMENTAL` driving profile and leaves `E2SaveRootMode=ORIGINAL`; no Force source or equations change.

If the pre-cleanup build drives, compare it directly against `497935e` to isolate the behavior-changing hook removal. If it still crashes, `497935e` is insufficient; inspect the earlier E2 runtime-addition boundary next. Do not revert or fix either boundary on suspicion alone.
