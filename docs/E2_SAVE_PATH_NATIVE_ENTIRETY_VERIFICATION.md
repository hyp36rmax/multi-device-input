# E2 save path and native ENTIRETY verification

E2 is split into a static executable trace and a controlled Windows run. The
static trace is complete. Runtime before/after verification remains required;
no player save was opened or modified during the static work.

## Supported executable

The executable is the same file downloaded by the production workflow:

```text
File: OR2006C2C.exe
Format: PE32 Intel 80386, Windows GUI
Size: 3,674,112 bytes
PE timestamp: 2006-05-26 10:04:57
SHA-256: 68ceb386829066f8455b9d027320af962584321f3e2e8a79c72841495a6134c3
```

All addresses below are preferred-image virtual addresses for this exact
executable. They are not portable to another build without validation.

## Save root construction

The common root resolver is at `0x406B70`.

It calls `GetCurrentDirectoryA`, appends `\SaveGame\`, and returns a shared
buffer. If current-directory discovery fails, it returns the literal
`.\SaveGame\`.

The four observed file helpers call this resolver before appending an
individual filename:

| Helper | Address | Behavior |
| --- | ---: | --- |
| Delete | `0x406BD0` | Builds the path and calls `DeleteFileA` |
| Write | `0x406C50` | Opens, writes a size header and payload, flushes, closes |
| Exists | `0x406DB0` | Attempts a read open and closes it |
| Read | `0x406E50` | Opens, validates the size header, reads the payload, closes |

Static callers cover common data, all four licences, rankings, and the
dynamically named ghost store. No second persistent root constructor was found
for these systems. The resolver is therefore the earliest common redirection
boundary supported by current evidence.

This is stronger than intercepting individual `CreateFileA` calls: it changes
the root before any game-owned filename is appended while leaving unrelated
game files alone.

## Persistent inventory from the executable

| File | Static role and behavior | Payload size | Expected file size |
| --- | --- | ---: | ---: |
| `common.dat` | Common/global state; read and written separately | 68,580 | 68,584 |
| `License1.dat` | Licence slot 1 | 1,036 | 1,040 |
| `License2.dat` | Licence slot 2 | 1,036 | 1,040 |
| `License3.dat` | Licence slot 3 | 1,036 | 1,040 |
| `License4.dat` | Licence slot 4 | 1,036 | 1,040 |
| `rankings.dat` | Rankings/record state | 11,408 | 11,412 |
| `GHOST%02d.DAT` | Per-index ghost container | Dynamic | Dynamic plus four bytes |

The licence pointer table is ordered `License1.dat` through `License4.dat`.
The current selected slot is used to choose which 1,036-byte licence payload is
copied to and from the active licence state at `0x7C23E0`.

The exact ownership of every `common.dat`, rankings, and ghost field remains a
runtime research question. The table records file-level behavior only.

## File format and write safety findings

The shared writer places a four-byte payload-size value at the start of every
observed save file, followed by the payload. The shared reader requires that
header to equal the caller's expected payload size before reading the payload.

For the observed common, licence, and ranking paths:

- the first four bytes are a size marker;
- fixed payload sizes are enforced by each caller;
- no checksum, CRC, signature, or cryptographic integrity calculation appears
  in the shared helper;
- the writer uses `OPEN_ALWAYS`, writes from the beginning, calls
  `FlushFileBuffers`, and closes the handle;
- the executable imports no `MoveFile` or `CopyFile` operation for this path;
- the observed writer does not publish through a temporary file and rename;
- licence deletion directly calls `DeleteFileA`.

This establishes direct-overwrite behavior for the traced helpers. A runtime
trace still needs to confirm that no higher-level backup convention surrounds
them and that no other persistent subsystem escapes this path.

## Native ENTIRETY path

The supported PC executable contains one direct reference to `ENTIRETY` in the
licence-edit flow at `0x4DE52F`.

The comparison is case-sensitive and checks nine bytes against the active
licence name at `0x7C23E0`. On a match, it calls `0x447360` with that active
licence structure.

`0x447360` changes the in-memory licence payload as follows:

| Licence-relative region | Operation |
| --- | --- |
| `0x028..0x0BD` | Fill 150 bytes with `0xFF` |
| `0x125..0x176` | Fill 82 bytes with `0x06` |
| `0x177..0x1C8` | Fill 82 bytes with `0x06` |
| `0x1C9..0x1D2` | Clear 10 bytes |
| `0x1D3..0x1DC` | Clear 10 bytes |
| `0x1DD..0x2A4` | Fill 200 bytes with `0x06` |
| `0x2A5..0x36C` | Fill 200 bytes with `0x06` |
| `0x36E..0x3F1` | Fill 66 words with `0x6666` |

The handler itself performs no file operation. Control returns to the normal
licence-edit flow, which later copies the active 1,036-byte structure to the
selected slot and uses the ordinary save writer.

This proves that `ENTIRETY` is a native PC licence-state transformation, not a
Tweaks unlock bypass. It also proves that persistence depends on a subsequent
normal game save. Static analysis alone does not assign content meaning to the
changed regions.

`MILESANDMILES` is handled by the adjacent licence-edit logic but follows a
different transformation: it changes a flag and a floating-point value. It is
not part of the `ENTIRETY` routine and is outside E2's transformation target.

## Developer redirection proof

E2 adds one hidden restart-only setting:

```ini
[Developer]
E2SaveRootMode = ORIGINAL
```

Accepted values are:

- `ORIGINAL`: calls the unmodified game resolver;
- `MANAGED_TEST`: returns
  `<game>\_E2ManagedTest\SaveGame\` from the common resolver.

Missing or invalid values fail closed to `ORIGINAL`. Failure to create the
disposable directory also falls back to `ORIGINAL`. The selected mode and every
resolved root are written to `OutRun2006Tweaks.log`.

This is research instrumentation only. It does not clone data, expose a
Gameplay option, invoke `ENTIRETY`, merge progress, or define the eventual
production location.

## Controlled Windows verification

The runtime phase must use a new empty `MANAGED_TEST` root or an expendable
clone. Legitimate `SaveGame` content must remain outside the test root.

The minimum run is:

1. Hash and timestamp the legitimate root without changing it.
2. Start once with `E2SaveRootMode=ORIGINAL`; confirm the log resolves the
   ordinary root, then exit without performing the unlock test.
3. Set `MANAGED_TEST`, remove only an earlier disposable E2 root if a clean run
   is required, and start the game.
4. Create a disposable licence and complete one ordinary save.
5. Record the complete test-root inventory, sizes, hashes, and timestamps.
6. Perform a settings change and an ordinary progression save; record the
   changed files after each action.
7. Produce a ranking and ghost only when easily reproducible; record their
   paths and timing.
8. Capture the Before state, invoke `ENTIRETY` through Edit Licence, decline the
   name change as required by the authentic path, and let the game save.
9. Capture the After state and visible content inventory.
10. Restart in `MANAGED_TEST` and verify persistence.
11. Return to `ORIGINAL`, restart, and verify the original licence and its
    hashes are unchanged.

The capture should include the log plus zipped Before, After, and Restarted
copies of `_E2ManagedTest\SaveGame`. Do not include `OnlineLoginData.dat` or
real account credentials.

## Evidence still required

Runtime evidence must answer:

- exact file creation and update timing;
- whether all actual paths remain in the disposable root;
- the exact visible PC content unlocked by `ENTIRETY`;
- persistence after restart;
- deterministic changed regions across repeated clean tests;
- whether runtime behavior reveals any validation beyond the static size
  marker;
- whether a ghost or another edge path bypasses the common resolver;
- whether ordinary and managed roots remain completely independent.

Until that capture exists, E2 has a verified static boundary and authentic
native transformation lineage, but not a completed isolation or content proof.

## Minimum E3 gate

E3 may begin only after the controlled run confirms full isolation. A managed
clone must then validate:

- recognized executable identity;
- expected file set and four-byte size headers;
- exact fixed sizes for common, licences, and rankings when present;
- parseable selected licence after restart;
- no path observed outside the managed root;
- source and clone hashes stored in external Experience metadata;
- transformation state stored outside game-owned files; and
- no partial creation state presented as ready.

If any persistent write escapes the common resolver, E3 must stop and address
that single path before adding player-facing behavior.
