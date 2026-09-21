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
Tweaks unlock bypass. The early hypothesis was that a subsequent normal game
save would preserve it. Run #85 confirmed an immediate unlock but disproved
persistence across restart. Static analysis alone does not assign content
meaning to the changed regions or establish why the saved state did not
reproduce the unlock.

### E2-R5 persistence follow-up (Run #85)

The fresh disposable `MANAGED_TEST` run reached the native ENTIRETY action,
visibly unlocked content, exited normally, and restarted with the same managed
root. The content was locked again. This is a runtime observation, not proof
that the native cheat was designed to be session-only.

The instruction stream establishes these narrower facts:

- `0x4DE549` calls `0x447360` on the active licence at `0x7C23E0` before the
  later editor save opportunity at `0x4DE654` (`0x416420`). After the call,
  `0x4DE54E..0x4DE588` updates active-licence flags, two editor values, and
  the first 16 name bytes (`0x4DD590`). The cheat does not write a separate
  global unlock variable in this routine.
- Every transformation range in the table above lies within the 1,036-byte
  (`0x40C`) active-licence payload. These are in-memory persistent-*format*
  fields, not a proven durable save. Their individual content semantics and
  any derived caches remain unverified.
- `0x416420` first checks global `0x7457A9`; if zero it returns without
  writing. Otherwise it writes `common.dat`, then, if a selected licence is
  present, copies 1,036 bytes from `0x7C23E0` into that slot and calls the
  ordinary file writer (`0x406C50`) with the same active buffer and size.
  There is no filter of the transformed offsets in this observed write path.
- The editor reaches `0x416420` at `0x4DE654` only through its later gated
  `0x4DE63A..0x4DE654` branch. The event-1 handler containing ENTIRETY returns
  at `0x4DE59B`; it does **not** call the save writer immediately. Thus the
  ordering is transformation, then *possible* later save, not unconditional
  transformation followed by save. Whether Run #85 took that branch is not
  recorded by the available diagnostics.

No Run #85 `License*.dat` Before/After/Restarted snapshots or post-event save
trace were supplied with this investigation. File sizes, hashes, changed-byte
ranges, a normal-exit write, and the exact data loaded at restart therefore
cannot be reported. In particular, the evidence does not distinguish an
unsaved active licence from a saved licence subsequently reset/recomputed by
the loader or another progression path. The earlier statement that the editor
*later copies and saves* the transformation was too strong: it has a
conditional save path, not a proven Run #85 write.

For the future managed-profile design, retain an isolated native profile but
do not adopt either a persistent transformation or a per-launch native action
yet. A persistent managed transformation is justified only if the game itself
saves and reloads the exact transformed bytes and their unlock meaning is
verified. A per-launch native action is a fallback only if durable native
progress is conclusively excluded. Direct persistent progression editing is
not supported by the present evidence.

One narrowly scoped observation would resolve the immediate fork: provide
copies of the disposable managed `License1.dat` (or the selected slot) before
the event, after normal exit, and after restart, plus the corresponding Run
#85 log. Compare only that isolated root. If the transformed ranges never
reach disk, trace the `0x4DE654` gate; if they do, trace licence load and a
representative unlock reader. No further crash reproduction or test against
the legitimate save is warranted.

### E2 closure reported after the R5 static pass

The subsequent controlled workflow supplied the missing **player-action**
observation: after native ENTIRETY, selecting OutRun's **Save to Profile** and
then restarting preserved the unlocked content. The earlier restart without
that explicit save did not. This establishes persistence through the game's
normal profile save in that workflow; it does not imply that the event-1 action
automatically saves. The preceding R5 analysis records what could be concluded
*before* this observation and remains part of the investigation history.

E3 therefore need not synthesize save fields or automatically save after the
unlock. OutRun's Save to Profile remains the player's choice. The production
service must protect the existing SaveGame first and keep restoration separate
from a live game writer.

### E2-R1 trigger lineage

`0x447360` has one caller: the direct call at `0x4DE549` inside the licence-edit
update routine beginning at `0x4DE2B0`.

The update routine dispatches on its internal screen state at object offset
`0x38`. The comparison is reachable only in state `1`, the outer Edit Licence
screen. It first reads the outer screen's input event. Only event `1` enters the
comparison block; every other event is delegated back to the normal editor
state machine.

The name editor is a separate state (`2`). Accepting text there copies its
16-byte text buffer into the active licence name and returns to the outer edit
screen. That action alone does not compare the name. After returning to state
`1`, the user must perform the outer screen action that produces event `1`.
R1 initially described this as an outer completion/exit-accept path. R3 traces
the input precisely: it is the B/Back press while the outer editor is active,
not the ordinary A/Confirm press.

Both historical names share this trigger. Their common path is:

```text
Edit Licence outer screen, state 1
    -> activate Name, entering state 2
    -> type ENTIRETY
    -> accept the text editor, copying the name
    -> return to outer Edit Licence, state 1
    -> press B/Back (default keyboard Escape), event 1
    -> compare MILESANDMILES at 0x4DE505 (14 bytes including terminator)
       -> on match, update the native flag and miles value inline
       -> on mismatch, compare ENTIRETY at 0x4DE540
    -> exact nine-byte case-sensitive ENTIRETY match including terminator
    -> call 0x447360 at 0x4DE549 for ENTIRETY only
    -> return to normal licence commit/save flow
```

The earlier runtime attempts did not reach this outer B/Back event. Merely
entering text, using A/Confirm, or navigating selections cannot compare it.

`MILESANDMILES` and `ENTIRETY` differ only after recognition. The former updates
the native flag and floating-point miles value directly in the licence-edit
routine. The latter calls `0x447360` with the active licence name address.

R1 diagnostics are enabled only in `MANAGED_TEST`. They report:

- discrete outer licence-edit state/event values;
- entry to the shared licence-name evaluation;
- either known literal only when its bytes match, otherwise a redacted marker;
- separate `MILESANDMILES` and `ENTIRETY` comparison results;
- active zero-based licence index;
- entry to `0x447360`; and
- return from the transformation.

They observe existing control flow and do not call, patch, or modify the native
routine.

### E2-R2 diagnostic discrimination

The Run 82 label `E2 ENTIRETY licence-edit event` was too specific. Its hook is
at `0x4DE4DA`, before either string comparison, and therefore identifies the
shared outer licence-edit handler only. It cannot identify a candidate or a
matched action. Seeing that label during a `MILESANDMILES` attempt does not mean
that the game selected `ENTIRETY`.

The native evaluation order is fixed:

1. Outer Edit Licence state `1` receives event `1`.
2. `MILESANDMILES` is compared at `0x4DE505`; the result branch is `0x4DE507`.
3. A match enters its inline action at `0x4DE509`. It updates the native flag at
   `0x7C27D4` and adds the native constant at `0x5CDA1C` to `0x7C2404`.
4. Only a `MILESANDMILES` mismatch reaches the `ENTIRETY` comparison at
   `0x4DE540`; its result branch is `0x4DE542`.
5. An `ENTIRETY` match enters its action branch at `0x4DE544` and calls
   `0x447360` at `0x4DE549`.

R2 logs the shared event with a neutral label, records entry to candidate
evaluation, reports attempted/matched state for each comparison, and records
entry to each successful action branch. Ordinary licence names remain redacted.

Static selection-state transitions establish that event `4` moves Down and
event `2` moves Up through the editable selections.
They are navigation events, not the outer completion event. Event `1` is the
only state-`1` event that enters the native cheat evaluation and exit path.
R3 resolves its input mapping below.

The trace observes both native paths because failure of both names indicates a
shared trigger problem rather than an `ENTIRETY` transformation problem.

### E2-R3 outer-editor input trace

Run 83 passed. The controlled ENTIRETY attempt (Log 18) and separate
MILESANDMILES attempt (Log 19) both reached outer Edit Licence state `1`, event
`4`, but neither reached state `1`, event `1` or either string comparison.
These are preserved as failed trigger attempts, not evidence that either
native comparison or action is broken.

The editor's vtable at `0x5CD9F8` points to update `0x4DE2B0` and input
translator `0x48F5F0` (vtable offset `0x14`). The update calls that translator
at `0x4DE2CE/0x4DE2DA`, then reads its event result at `0x4DE4D6`. In the
translator, `0x4536F0` tests the game's menu switch press-edge bits in this
priority order:

| Press-edge mask | Menu action | Returned event | Outer-editor consequence |
| --- | --- | --- | --- |
| `0x1` or `0x4` | Start or A/Confirm | `0` | Activate selected edit item |
| `0x8` | B/Back | `1` | Exit outer editor; evaluate licence name first |
| `0x400` | Selection Up | `2` | Move selection Up |
| `0x800` | Selection Down | `4` | Move selection Down |
| `0x1000` / `0x2000` | Left / Right | `3` / `5` | Other selection navigation |

`0x4536F0` reads the press-edge word at `0x7D6778` for the active input slot
(or the fallback word at `0x7D6788`); `0x45369E..0x4536AD` computes these
edges from current and previous switch masks. With Tweaks' new input backend,
`SwitchOn` at `0x4536F0` is replaced by the bound `SwitchId` press-edge
implementation. The default keyboard B/Back binding is Escape, and the
default SDL gamepad binding is the B button. A wheel's user-assigned B/Back
binding works through the same switch bit. Return or gamepad A is A/Confirm,
which produces event `0`, not the cheat trigger. Escape also maps Start on
keyboard, but Tweaks suppresses the Start-only menu check; B/Back remains.

The shortest traceable sequence is: outer Edit Licence state `1` → select
Name with A/Confirm (state `2`) → enter the exact uppercase name → close the
name editor through its accepting path (the editor's event `1` copies the
16-byte text buffer into the active licence at `0x4DDBBF..0x4DDC02`) → return
to outer state `1` → press B/Back (Escape by default). The outer handler
branches on event `1` at `0x4DE4DA` to the MILESANDMILES comparison at
`0x4DE505`, then on mismatch to ENTIRETY at `0x4DE540`; respective successful
actions begin at `0x4DE509` and `0x4DE544` (the latter calls `0x447360`).
The runtime test must confirm that the name editor's accepting path was taken;
closing it by its alternate event `2` discards the entered text.

No independent mouse-click-to-event-`1` branch is present in this translator.
Mouse input would have to generate the same B/Back switch elsewhere; that
mapping is not established. The supported replacement executable retains the
translator, event-`1` branch, both comparisons, and native actions. This is
evidence that its trigger is connected, but without a binary comparison to the
original EXE it does not establish whether any older build mapped inputs
differently. The remaining proof is a controlled runtime B/Back press yielding
`state=1,event=1` and the comparison/action logs. R3 changes no executable code.

### E2-R4 first evaluation crash and hook correction

The first real event-`1` ENTIRETY attempt in Run 83 reached the shared editor
gate and logged `candidate=ENTIRETY` at 13:37:48.723, then crashed with
`0xC0000005`. This is **not** evidence of a native ENTIRETY failure. The
comparison-result and action diagnostics never ran. Earlier Logs 18 and 19
remain separate failed trigger attempts; this is the first attempted native
evaluation.

The minidump module list maps `OR2006C2C.exe` at `0x00400000`, size
`0x0058E000`, ending before `0x0098E000`. Faulting EIP and attempted read
`0x4019CE1A` are far outside that image and absent from the dump's captured
memory regions. They are not an executable address in the supported game image
or an identified valid trampoline. The dump does not retain the patched code
pages or allocated trampoline bytes, so an exact byte-level trampoline
reconstruction is unavailable. The crash logger's guessed stack is not a
native licence call stack: `0x00442D0F`, `0x00445C8D`, and `0x0043FB29`
lie in generic UI/input update and callback-dispatch code, not the licence
transformation or save writer.

The instruction stream reveals the instrumentation defect. A mid-hook needs
room for a jump patch, but Run 83 placed separate hooks at `0x4DE505`
(`rep cmpsb`, two bytes), `0x4DE507` (conditional branch, two bytes), and
`0x4DE509` (action entry). Their patch regions overlap. The ENTIRETY probes
at `0x4DE540` (`rep cmpsb`) and `0x4DE542` (conditional branch) likewise
overlap. The later hooks at `0x4DE549` and `0x4DE54E` were also in a sensitive
call/return region. This prevents the hooks from preserving independent
original instructions and control flow. At the crash, EDI=`0x5CDA21`,
ESI=`0x7C23E1`, ECX=`13`: each comparison pointer advanced once and the
14-byte MILESANDMILES comparison count fell by one. That directly corroborates
one native `cmpsb` iteration before control left the valid execution path.
EFLAGS showed a mismatch. The highest-confidence cause is overlapping
comparison/action mid-hook patches producing a bad control-flow target, not
candidate inspection or a completed native comparison. Logging happened
successfully before the sensitive instruction; there is no indication of a
stack or register failure in that callback.

R4 removes **all** comparison-result and action-region hooks, and also the
adjacent candidate-entry hook. It retains only the already-observed outer
event hook at `0x4DE4DA`, which records state/event and, for state `1`, event
`1`, a redacted-or-known-literal candidate before the comparison region. The
native instructions from `0x4DE4E3` through the transformation/commit path
are no longer instrumented by E2. No comparison outcome or transformation
invocation is claimed by the reduced diagnostic; those require runtime
behavior and later save evidence. Ordinary licence names are never logged.

No ENTIRETY transformation (`0x447360`) or subsequent licence commit/save is
confirmed in the crash capture. The dump proves entry to the first string
comparison, not completion of that comparison. The managed root may already
contain the earlier ordinary test licence and saves, but this crashed attempt
provides no evidence of cheat-induced persistence. Preserve the failed root
and logs as evidence; use a **new or expendable managed test root/clone** for
the next controlled run, without touching the legitimate original root.

The same Run 83 log also enumerated two Fanatec DD2 DirectInput interfaces
(8 axes/108 buttons/1 POV and 12 axes/63 buttons/4 POVs), both advertising
FFB, with WheelFFB selecting the first. This is retained for the future
Universal FFB Device Resolver and is outside E2-R4. No FFB code changed.

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
8. Capture the Before state, open Edit Licence, enter `ENTIRETY` in Name,
   accept the text editor so it copies the name, then press B/Back (default
   keyboard Escape) on the outer Edit Licence screen. Check for
   `state=1,event=1` and comparison logs before testing any save effect.
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
