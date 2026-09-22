# E3B native licence-slot separation

## Why the direction changed

E2's parallel managed SaveGame was the right way to research without risking a
player's profile. After native ENTIRETY and Save to Profile were shown to work,
the v1 experience shifted toward OutRun's own four licence slots: retain the
original slot, create an Experience clone in a free slot, and retain full-root
Save Recovery as a second safeguard. This does not discard the earlier root
work; it remains the disposable verification fixture. E3B does not invoke
ENTIRETY, restore files or add a Gameplay control.

## Supported executable trace

Addresses refer only to EXE SHA-256
`68ceb386829066f8455b9d027320af962584321f3e2e8a79c72841495a6134c3`.

- `0x4162D0` allocates four `0x40C`-byte in-memory slots and initializes each
  with `0x4471A0`. `0x416380` reads `common.dat`, then loops across the four
  `LicenseN.dat` names at pointer table `0x7457AC`, reading each payload of
  exactly `0x40C` bytes into its corresponding in-memory slot. The shared
  reader requires a four-byte length header. It does not perform a slot-index
  rewrite of loaded payloads.
- `0x4E1C00` scans the four slots using bit 0 at payload offset `0x3F4` to
  build the occupied-slot list. It exposes the first unoccupied slot as a
  New Licence choice. `0x4471A0` initializes a slot to a blank state, and
  `0x4DD590` sets the occupied bit when the native name/edit path commits.
  Mere file absence is therefore insufficient evidence of an occupied *valid*
  slot, but a missing destination file is required to avoid overwrite.
- Selected slot index is the signed global at `0x7B17F8`, with `-1` meaning
  none. `0x448520` changes selection and copies the chosen slot's whole
  `0x40C` payload into active licence `0x7C23E0`. `0x416420` writes
  `common.dat`, copies the active payload into the selected in-memory slot,
  and writes the same `0x40C` bytes to the selected `LicenseN.dat` name. The
  filename is chosen from the slot index; no slot-specific field is patched
  by that load/save path.
- `common.dat` is read as a `0x10BE4`-byte payload directly into `0x7B17F8`;
  its first payload dword is therefore the selected index. Its full layout is
  not decoded. E3B reads and checks that index but never edits it. A new clone
  becomes selectable on a later native load; the current active slot remains
  the source.
- The researched native functions provide create-blank, select and save, not
  a confirmed duplicate-existing-licence command. Because the native file
  loader/writer treats each `0x40C` payload identically and uses the filename
  table for slot identity, a byte-identical file clone is the conservative
  offline mechanism. No per-slot payload index was observed in this path;
  physical game acceptance and independent evolution still require UAT.

## Implemented E3B boundary

`ExperienceLicence::Service::CloneActiveLicenceToFreeSlot(activeSlot)` is an
offline, game-closed transaction. There is **no production caller yet**.
Its caller-supplied gate must remain true while it works. The service rejects
an unsupported executable by full SHA-256, a requested index that disagrees
with the selected index in `common.dat`, invalid source file/header,
and all-slots-occupied state. It treats *any existing destination filename* as
occupied, including a damaged one, rather than risking replacement. It selects
the first absent filename in slot order.

The order is: read/hash source → find free destination → create and validate
an E3A `PRE_UNLOCK_ALL` full-root restore point → recheck closed phase, source
hash and destination absence → create destination with Windows `CREATE_NEW` →
verify its `0x40C` header, native occupied bit and exact hash/bytes → publish
Multi Input metadata. Failure before backup never creates a destination.
The source is opened read-only and re-hashed after clone. The active index and
`common.dat` are not changed. The clone's visible name initially matches the
source; E4 may choose a clearer player-facing name through normal game means.

Metadata lives at `<parent of SaveGame>/MultiInput/ExperienceLicences/metadata.json`,
outside OutRun files. Schema 1 records source/destination slot, source and
initial destination SHA-256, restore-point ID and build version. A matching
managed destination is reused. If metadata is malformed or its destination
hash has changed, the service returns `STALE_MANAGED_SLOT`, never silently
claims ownership of the replacement or creates a second clone. This also
conservatively marks a legitimately progressed clone stale; without a native
immutable licence ID, a changed payload cannot be distinguished from a
replacement. E3C/E4 must resolve that UX, not weaken ownership detection by
guessing from the visible name.

## Controlled proof and remaining gate

The Win32 CI test uses a disposable E2-shaped root and the downloaded,
hash-validated supported EXE. It checks gate rejection, backup before clone,
byte-identical source/destination, source non-mutation, initial reuse, changed
destination detection, all-four-occupied rejection without backup, and
unsupported-EXE rejection. The destination is edited in the fixture and the
original source remains byte-identical. This is a file-level independence
proof, **not** a physical OutRun selection/progression proof.

Before enabling a production caller, E3B still needs a controlled Windows game
test on the disposable root: restart after clone, confirm both slots appear,
select each using OutRun's native selector, change a harmless setting or
progress value in one, Save to Profile, restart, and verify the other remains
unchanged. Also establish a truly game-closed execution phase for the file
transaction. Do not test this on the legitimate save or invoke ENTIRETY.

E3C should address safe in-game/closed-game handoff and native ENTIRETY on the
validated Experience slot, while preserving the source and E3A recovery point.

## E3B Run #88 developer runtime entry

Run #87 passed the Win32 disposable-root tests, but that build had no runtime
button. Run #88 adds one action in the main F11 overlay's Debug tab (not the
F1/controller settings overlay), visible only when
`E2SaveRootMode=MANAGED_TEST` actually resolved to the
disposable root: **Clone Active Licence to Free Slot (after exit)**. It is not
in the normal Gameplay UI. The button reads the native selected index and
queues a single helper process. It never copies files inside the running game.

The startup log reports `E3B developer clone test: available` only when the
managed root is active and `experience_clone_helper.exe` is beside `dinput8.dll`.
It also reports the mode and whether the helper was found. The Debug tab is
still visible in Release builds; a missing helper prevents the clone, not the
tab from appearing.

`experience_clone_helper.exe` is packaged beside `dinput8.dll`. It checks the
supported EXE and exact non-symlinked `_E2ManagedTest/SaveGame` root, holds a
handle to the current game process, and waits for it to exit. Only then does
it call the E3B service with a continuously checked game-closed gate. The
service independently checks the selected index against `common.dat`, chooses
an absent slot, creates and validates the E3A restore point, creates the
destination without overwrite, and verifies source/destination bytes. No
ENTIRETY action or active-slot switch occurs.

The helper writes a concise result to
`_E2ManagedTest/MultiInput/ExperienceLicences/clone-run88-status.txt`. On the
next launch the E2 hook logs that result to `OutRun2006Tweaks.log`; it includes
source slot, destination slot, restore-point ID, result and independent
source-hash comparison. The Debug tab also shows the result. A failed root,
process wait or backup check means no clone is attempted.

### Exact controlled user procedure

1. Use only the disposable E2 managed test setup. Put the new `dinput8.dll`
   **and** `experience_clone_helper.exe` beside the game EXE, with
   `[Developer] E2SaveRootMode=MANAGED_TEST` already selected. Leave the
   legitimate `SaveGame` untouched.
2. Launch OutRun and confirm the log says `E2 save root mode: MANAGED_TEST`.
   Select the disposable source licence and use OutRun's **Save to Profile**.
3. Close the F1/controller settings overlay if it is open. Press **F11** (or
   the configured main-overlay toggle), then open its **Debug** tab. Under
   **E3B Developer Test**, note the displayed source licence number and press
   **Clone Active Licence to Free
   Slot (after exit)** once. It should say the action is queued.
4. Exit OutRun normally. Wait for the helper to finish, then relaunch with
   `MANAGED_TEST` still selected. The log and Debug tab should show the Run
   #88 result, destination number, restore-point ID and `sourceIntegrity=unchanged`.
5. In OutRun's native licence selector, check that both the original and new
   slot appear. Select each and compare name, progression and settings. Make
   one harmless change on the clone, Save to Profile, restart and verify the
   original is unchanged. Do not use ENTIRETY in this test.

If the helper reports anything other than `CREATED` (or `REUSED` for an
already-created matching test clone), stop and share the managed-root status
file and `OutRun2006Tweaks.log`. Do not manually copy `License*.dat`.
