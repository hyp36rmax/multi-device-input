# E3 save protection and native unlock service

## Boundary established by E2

The initial licence-name attempts did not reach the native comparison. B/Back
from the outer Edit Licence screen was eventually identified as event 1. The
first event-1 run crashed because E2 diagnostic jump patches overlapped; after
removing the invasive probes, Run #85 executed native ENTIRETY and visibly
unlocked content. A restart without Save to Profile lost the effect. The later
controlled Save to Profile workflow retained it across restart. This correction
is why E3 can leave ordinary saving entirely to OutRun.

The E2 managed root remains a disposable research environment, not an Unlock
All feature or a second player profile. E3 must not route ordinary players to
that root.

## Service contract and ownership

`SaveProtection` owns `CreateRestorePoint(reason)`, `ListRestorePoints()`,
`ValidateRestorePoint(id)` and a restore request. `NativeUnlock` owns only
supported-EXE validation, active-licence validation and the call to the
confirmed native transformation. The future Gameplay UI calls a transaction
coordinator: verify executable and active licence, create and validate a
`PRE_UNLOCK_ALL` restore point, invoke ENTIRETY, and report verification status.
If backup fails, the native call must not occur. No automatic Save to Profile.

This is an interface boundary, not a claim that these methods exist yet.

## Restore-point representation

Use `<game>/MultiInput/SaveRecovery/<id>/SaveGame/` for the full recursive
snapshot, outside active `<game>/SaveGame/`. The sibling manifest records schema
version, ID, UTC time, reason, canonical source root, recognized executable
identity where available, active licence index when reliable, and a relative
path/size/SHA-256 inventory. Do not put ordinary licence names or account data
in metadata. The snapshot itself may contain private save content; never upload
it by default. Preserve all pre-unlock snapshots until the player explicitly
chooses a retention action.

Generate a non-reused ID. Copy into a unique temporary sibling directory,
enumerate only regular files and directories without following directory
symlinks/reparse points outside the root, hash source and copy, write and flush
the manifest, validate it, then rename the temporary directory to the final
ID in the same parent. An interrupted temporary directory is not listed as a
restore point. Reject path traversal, a missing/extra file, digest mismatch,
duplicate manifest path, schema mismatch, or a source root different from the
one the player intends to replace.

## Restore lifecycle

Do not replace SaveGame from the loaded DLL while OutRun can write it. The
current DLL has no demonstrated game-closed execution context. A restore
request must therefore be deferred to a dedicated, separately validated
game-closed operation; merely writing a next-launch marker inside the game is
not enough to prove the save subsystem has not initialized. The operation
revalidates the chosen point, snapshots the current SaveGame as
`PRE_RESTORE`, stages a validated replacement as a sibling, then renames the
active root aside and the staged root into place. Retain the displaced root
until the restored root has been checked and the operation recorded. On failure
before publication, leave the active root unchanged; on a failed second
rename, attempt to rename the displaced root back and surface a recovery path.
Same-volume directory rename is required, and a power loss between the two
renames still needs a startup journal/recovery rule. Never promise a single
atomic swap of two directories on Windows.

No live restore is implemented or authorized by this design. Until a safe
game-closed execution mechanism and its crash tests exist, the restore API
must return an explicit unavailable/deferred state rather than touching the
active root.

## Native invocation boundary

The supported executable SHA-256 is
`68ceb386829066f8455b9d027320af962584321f3e2e8a79c72841495a6134c3`.
Verify the on-disk EXE before using addresses, and ensure it is the loaded
module. `0x447360` takes the active licence pointer in ECX (thiscall-style),
modifies offsets within its 1,036-byte payload and returns; E2's event-1 caller
supplies `0x7C23E0`. This proves the argument and transformation, **not** that
an arbitrary menu state is safe. Validate a selected licence (index 0..3),
initialized save state, and a menu/lifecycle point where no native save writer
is active. Do not call from a worker thread or DLL initialization. A concrete
safe trigger and runtime verification are prerequisites to enabling the
invocation service. Never simulate the name-editor UI, patch unlock readers,
or infer a generic unlocked marker from the name alone.

The unlock transaction should distinguish native call completed from content
verified and profile saved. No persistent “Unlocked” badge is justified solely
by a successful call. The status can remain “action available” until a native
progression-state check is established.

## Gate to implementation

The restore-point creation/validation logic can be tested first on a disposable
E2 managed root. Inject copy failures, corrupt a backup, and confirm no
published point appears for incomplete work. Then exercise deferred restore
with the game closed on the same disposable root, including interruption at
both rename boundaries. Only after that should a developer action invoke
native ENTIRETY, always preceded by a verified restore point. No test against
the legitimate save is required for this stage.

The current work has **not** implemented or validated this service. In
particular, it must not expose an Unlock All control before the backup,
invocation and restore paths pass controlled Windows tests. HYP36R Force and
controller behavior are outside this milestone.
