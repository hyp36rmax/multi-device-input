# E1 managed game profile architecture

Status: Historical feasibility research. This work is not part of the current
Multi Input product roadmap. Future development is undecided. The designs below
record what was considered at the time, not current player instructions.

E1 defines how Experience features can use progression state without treating a
player's legitimate save as scratch space. It is research and design only. No
save was opened, modified, redirected, or replaced during this work.

The central rule is simple: the legitimate save remains the authority and is
never a transformation target. Experience state is created from a clone and
all progress made while that clone is active stays with the clone.

## What the game stores today

The current evidence supports this file layout:

```text
<game directory>/SaveGame/
    Common.dat       global/common options and account-related state
    License*.dat     player licences: identity, settings, and progression
    rankings.dat     high-score records
    GHOST**.dat      Time Attack ghost data

<game directory>/OnlineLoginData.dat
    encrypted online login data when ProtectLoginData is enabled
```

The repository itself confirms that the save root is `<game>/SaveGame` and
creates that directory when it is missing. It also hooks the game's
`Common.dat` read and two write paths. With login protection enabled, online
credentials are encrypted to `OnlineLoginData.dat` beside the executable and
temporarily zeroed in memory while the game writes `Common.dat`.

The PC manual confirms native licence selection, automatic loading of the last
selected licence, and support for up to four licences. Each licence stores its
settings and progress. Public save-location documentation identifies
`Common.dat` as options, `License*.dat` as player profiles, `rankings.dat` as
high scores, and `GHOST**.dat` as Time Attack ghosts.

Some boundaries still need direct verification. The exact `License*.dat`
numbering convention, the last-selected-licence field, which settings are
licence-local versus common, and the precise ownership of aggregate statistics
are not mapped. The repository warning that the first `0xB0` bytes of a damaged
licence can restore name/model confirms an identity region, not the remainder
of the file format.

### Confidence summary

| Finding | Confidence |
| --- | --- |
| Save root is `<game>/SaveGame` | Confirmed by code |
| Up to four native licences; last selected is loaded | Confirmed by PC manual |
| `License*.dat` contains player profile/progression | Confirmed at file-role level |
| `Common.dat` contains common/options state and can contain login data | Confirmed |
| `rankings.dat` and `GHOST**.dat` are separate record files | Strongly supported |
| Exact licence filename/index mapping | Unknown |
| Exact progression offsets and types | Unknown |
| Checksum, CRC, signature, size marker, or version field | Unknown |
| Atomic/temp-file save behavior | Unknown |
| Exact save-write timing for licences, rankings, and ghosts | Unknown |

## Progression and unlock evidence

The repository does not currently define the binary layout of licence
progression. There is not enough evidence to classify it as one flag, a group
of flags, counters, completed-event records, derived logic, or some combination
of those.

There is, however, an important native path: the PC game recognizes the
case-sensitive licence-edit code `ENTIRETY`. The documented flow is to enter it
as the proposed licence name and decline the name change. Reports describe the
result as 100 percent completion or everything unlocked and purchased. Because
this is game-owned behavior, it is a much better future transformation target
than fabricating completion bytes or bypassing unknown integrity logic.

The precise scope still needs a controlled before/after capture. Current
inventory is therefore evidence-bounded:

| Content/state | Current evidence |
| --- | --- |
| Cars and purchases | Strongly supported as part of `ENTIRETY` completion |
| Routes/courses and starting variants | Strongly supported |
| Coast 2 Coast missions, ranks, and completion | Strongly supported |
| OutRun Miles/currency | Progression-related and separately affected by the native `MILESANDMILES` code; exact `ENTIRETY` value needs verification |
| Music | Reported as unlockable content; exact PC coverage needs verification |
| Holly MIX 2 course | Repository code confirms a separate mission/rank gate and a Tweaks policy setting |
| Game modes, cosmetics, bonuses, characters, and platform-exclusive content | Unknown by category |
| Records, rankings, statistics, and ghosts | Separate persistent state; not assumed to be part of `ENTIRETY` |

"All Content" should eventually mean the exact set observed in a controlled
native `ENTIRETY` transformation on the supported PC executable. It should not
be expanded by assumption.

## Managed profile model

The managed unit should be a complete Experience save root rather than only a
patched licence file:

```text
Legitimate SaveGame root (read-only to Experience)
        |
        +-- defensive snapshot
        |
        +-- clone into a new temporary directory
                    |
                    +-- validate structure
                    +-- apply named transformation
                    +-- let the game write normally
                    |
                    v
          Managed Experience SaveGame root
```

Cloning the complete root isolates licence progress, common settings, rankings,
and ghosts. A native spare licence slot is less invasive, but it leaves global
files shared and consumes one of only four player slots. It can become an
optimization only if E2 proves that every state affected by the first feature
is licence-local and that shared records are an intentional product decision.

The preferred target is therefore a separate managed save root selected at the
game's save-path boundary. The legitimate root should not be renamed out of the
way or overwritten. If a safe early path-redirection point cannot be proven,
the project should stop rather than silently fall back to destructive swapping.

## Identity and metadata

Metadata belongs outside the game's files. A small manifest can establish the
relationship without storing a player name, Windows account, or other personal
data:

```text
manifest version
managed-profile identifier
source relative file set
source SHA-256 hashes, sizes, and timestamps
creation time
supported game executable timestamp/version
Experience profile version
ordered transformation IDs and versions
state: creating / ready / active / needs-rebuild
last verified clean close, if observable
```

Hashes identify the exact source revision and detect later legitimate progress.
If the source changes, the managed clone is stale. The user can continue using
the isolated clone or rebuild it explicitly; it is never silently merged or
silently rebased.

## ON and OFF lifecycle

Mode changes should require a game restart. Save roots must not be switched
while the game may retain profile state in memory.

### Off

The game resolves its ordinary `<game>/SaveGame` path. Experience code does not
intercept writes, restore a backup, or copy managed data into the legitimate
root. Turning the option off simply returns to the untouched native path.

### On

Before the game can load profile state:

1. Identify and validate the legitimate source files.
2. Create a defensive, read-only snapshot if the source revision has not been
   recorded before.
3. Build or select the managed root that matches the source hash and requested
   transformation set.
4. Complete creation in a temporary sibling directory.
5. Validate the clone and atomically publish it as ready.
6. Redirect only the game's save-root resolution to that managed root.
7. Confirm the effective path in the log before any load or write occurs.

Ordinary save writes, records, settings changes, and statistics then remain in
the managed root. They do not update the legitimate source. Returning to Off
reveals exactly the progression the player had before Experience mode.

## Progress while Experience mode is active

Managed progress is real only inside that managed profile. Completed events,
records, statistics, changed settings, rankings, and ghosts remain there. E1
defines no merge path.

Some settings may prove safe and desirable to share, but sharing should be an
explicit later design. The conservative first implementation keeps the entire
save root isolated. Tweaks configuration and controller bindings already live
outside the game save root and should remain independent. HYP36R Force,
presentation, safety, and device calibration are not profile transformations.

## Crash and write safety

Creation should use a journaled state machine and same-volume temporary
directories so the final publish can be atomic. A partially created clone is
never marked ready. On the next start it is discarded or rebuilt.

Once active, the game writes only to the managed root. A game crash can damage
the managed copy but cannot replace the legitimate source. A disk error during
creation fails closed to Off before save loading. A disk error during managed
play is reported as a managed-profile failure; recovery offers a rebuild from
the last legitimate source or an older managed snapshot.

Backups should be automatic, versioned by source hash and time, and never the
only copy of legitimate progress. Backup creation should use copy-to-temp,
flush, validation, and atomic rename. Retention policy and disk-space handling
belong to implementation design, but deletion must never target the active
legitimate save.

## Alternatives considered

| Approach | Advantage | Problem | E1 decision |
| --- | --- | --- | --- |
| Native spare licence slot | Uses existing selection UI and game behavior | Four-slot limit; common/rankings/ghost state remains shared | Investigate first, but insufficient for guaranteed full isolation today |
| Separate native save path, if one exists | Clean isolation with little interception | No existing selector or command-line path is currently known | Preferred if E2 discovers it |
| Separate managed root with narrow runtime path redirection | Full isolation; no slot limit; original never replaced | Requires an early, proven path boundary and careful compatibility work | Preferred target if no native path selector exists |
| Pre-launch/post-exit directory swapping | Can work without internal path hooks | Crash recovery and external launch order are harder; a bad swap can obscure the original | Reject as primary design |
| Edit legitimate `License*.dat` and restore a backup later | Simple transformation target | Violates the core invariant and makes recovery depend on backup correctness | Rejected |
| Patch guessed unlock offsets | Small binary change | Unknown schema/integrity; fabricates semantics and risks corruption | Rejected |

## E1 proposed transformation

The transformation pipeline should be:

```text
validated source root
    -> managed clone
    -> invoke or reproduce the game's validated ENTIRETY path on the clone
    -> allow the game's normal serializer to write it
    -> validate file set, size, integrity, and observed unlock inventory
    -> publish managed root
```

E2 should trace the native `ENTIRETY` handler, determine exactly which in-memory
states it changes, and observe which files the game writes afterward. If the
game-owned path covers the required inventory and produces structurally valid
saves, it should be preferred over direct binary editing. Direct clone editing
is acceptable only after every changed field and integrity rule is mapped and
validated independently.

## Player experience

The normal UI can remain small:

```text
Gameplay

Unlock All Content                         Off / On
Uses a separate Experience profile. Your normal progress is preserved.
```

Changing the option should explain that a restart is required. Advanced
recovery can offer **Rebuild Experience Profile**, **Reset Experience
Profile**, and **Open Backup Location**. "Restore Original Profile" is not a
normal action because Off already uses the untouched original.

The same managed-root framework can support later validated Gameplay
transformations. E1 does not invent those transformations. Its secondary
development benefit is immediate: an isolated unlocked roster would allow
controlled HYP36R cross-car validation without progression grinding or risking
the developer's real profile.

## E2 evidence needed

E2 should be a read-only save-path and native-transformation trace. It needs to
resolve:

- exact licence filenames, indices, and active-selection state;
- every file opened, read, created, renamed, or written from boot through a
  controlled save;
- whether licence, rankings, ghost, or common writes use temporary files or
  direct overwrite;
- binary sizes, version markers, and integrity/checksum behavior;
- which settings and statistics live in each file;
- the native `ENTIRETY` handler, its in-memory changes, and its exact PC unlock
  inventory;
- whether the game exposes a native alternate save-root mechanism;
- the earliest safe save-root redirection boundary if it does not; and
- behavior when all four native licence slots are occupied.

No write implementation should begin until those points are answered.

## Research sources

- Repository code: `src/dllmain.cpp`, `src/hooks_misc.cpp`,
  `src/hooks_bugfixes.cpp`, and `OutRun2006Tweaks.ini`.
- [OutRun 2006 PC manual](https://oldgamesdownload.com/wp-content/uploads/OutRun_2006_Coast_2_Coast_Manual_Win_EN.pdf)
  for native licence selection and the four-licence limit.
- [PCGamingWiki save data summary](https://www.pcgamingwiki.com/wiki/OutRun_2006%3A_Coast_2_Coast)
  for file roles outside the repository's current hooks.
- [GameFAQs PC cheat documentation](https://gamefaqs.gamespot.com/ps2/930972-outrun-2006-coast-2-coast/cheats)
  for the native `ENTIRETY` and `MILESANDMILES` flows. These claims still
  require controlled verification against the supported PC executable.
