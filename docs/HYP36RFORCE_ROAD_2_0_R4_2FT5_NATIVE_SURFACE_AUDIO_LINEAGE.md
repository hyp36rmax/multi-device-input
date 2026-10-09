# HYP36Rforce Road 2.0 R4.2F-T5 — native surface-audio construction lineage

## Part I — Engineering record

### Decision

The T5 decision is **D — no defensible surface-audio lineage identified**. The
presentation classification is **6 — insufficient evidence**.

The supported PC executable contains a native packed sound-request path and
several functions that read or copy the four surface fields. Static call and
data-flow analysis does not connect those two groups. No selector, sound ID,
loop, one-shot, pitch, volume, timer, random state, wheel rotation, distance
accumulator, or contact phase can presently be attributed to `0x100000`.

This result does not dispute the synchronized runtime observation that Tulip
Garden cobblestone is audibly distinguishable. It means the executable map is
not sufficient to establish how that audible presentation is constructed.
Surface semantics improve only through the existing physics and restored Xbox
vibration lineage; audio timing remains unresolved.

### Evidence and method

The analysis used the validated replacement executable:

```text
OR2006C2C.exe
SHA-256 68ceb386829066f8455b9d027320af962584321f3e2e8a79c72841495a6134c3
image base 0x00400000
```

The complete executable map contained 7,238 mapped functions, 443,734
instructions, 33,008 calls, and 170,525 cross-references. The map archive hash
was verified before use. The trace covered the four known surface offsets,
their credible current-car readers and copies, direct and bounded transitive
calls, native sound-request callers, sound-bank strings, and the known scripted
Pegasus clop control.

Evidence labels in this record mean:

- **STATICALLY PROVEN** — visible instruction/call/data relationship in the
  validated executable or project source.
- **RUNTIME CORROBORATED** — supported by accepted synchronized telemetry/audio.
- **INFERRED** — plausible relationship without a closed lineage.
- **UNKNOWN** — the available evidence does not establish the relationship.

### Known `0x100000` starting point

The controlled starting point remains:

```text
EVWORK_CAR::water_flag_24C[0..3]
  offsets +0x24C, +0x250, +0x254, +0x258
  stable tuple 0x100000 / 0x100000 / 0x100000 / 0x100000
```

The only closed material lineage remains the independently restored Xbox
controller-effect path:

```text
surface state
  -> sub_1149C0 material coefficient (.71 for 0x100000)
  -> strongest of four coefficients
  -> speed/context scaling
  -> unsigned left/right motor envelope
```

That path is not an audio path and supplies no signed or periodic Road carrier.

### Direct and derived surface readers

Offset-shaped references occur in many unrelated structures, so an offset hit
alone was not treated as an `EVWORK_CAR` surface reader. The credible readers
were constrained by current-car provenance, surrounding known car fields, or
the established four-field copy pattern.

| Function | Relationship | Finding | Confidence |
| --- | --- | --- | --- |
| `0x0041BD50` | reads all four current-car fields and copies them to `0x00934488`, `0x009344E4`, `0x00934540`, `0x0093459C`; later OR/tests the copied flags | derived presentation/vehicle-state consumer; no sound request in its call graph | STATICALLY PROVEN |
| `0x004256F0` | copies `+24C/+250/+254/+258` into a compact state block at `+54/+6C/+84/+9C`, alongside speed and vehicle state | snapshot/derived-state construction; no sound request | STATICALLY PROVEN |
| `0x0044EDE0` | reads all four current-car fields and combines/tests them with broader vehicle state | vehicle/gameplay consumer; no closed sound lineage | STATICALLY PROVEN |
| `0x0045E2F0` | reads the four current-car fields | surface-aware consumer; purpose unresolved, no sound boundary reached | STATICALLY PROVEN reader; purpose UNKNOWN |
| `0x004628D0` | reads the four current-car fields | surface-aware consumer; purpose unresolved, no sound boundary reached | STATICALLY PROVEN reader; purpose UNKNOWN |
| `0x00475720`, `0x004A1140`, `0x004A69F0` | write/copy four values at the same offsets in their own object contexts | possible producers/copies, but object identity is not sufficient to call every instance `EVWORK_CAR` | PROVISIONAL |

The wider offset scan contained false positives such as stack-frame sizes,
unrelated object fields, and immediate sound/resource IDs. In particular,
`0x0049C9C0` calls the sound dispatcher but its `PUSH 0x258` is an immediate
resource/action value, not a read of `EVWORK_CAR +0x258`. It is not a surface-
audio bridge.

No credible direct or derived surface reader reaches `PrjSndRequest` within the
bounded call graph. No sound-request caller reads all four surface fields.

### Native sound-request ownership

The established native boundaries are:

```text
SetSndQueue      0x00424940
PrjSndRequest    0x004249F0
request worker   0x0042F0D0
play/route       0x0042F1A0
```

`PrjSndRequest` forwards one packed integer command. The worker masks the lower
11 bits for the sound identifier, recognizes stop and pan/paired routing flags,
and forwards play requests toward `0x0042F1A0`. `SetSndQueue` maintains a
deduplicated 32-entry command queue under game-state gating.

This establishes a sound ID and command boundary. It does **not** expose a
surface-material selector or a general pitch, playback-rate, volume, filter,
or distance parameter at the request call. Those controls could exist below
the request boundary or in a separate subsystem, but no such surface-owned
lineage was established.

Functions `0x00486730` and `0x004B2F00` both access the current-car global and
eventually issue dynamic sound requests. Neither reads `+0x24C..+0x258`, and no
derived surface value was demonstrated to reach its request. They remain
gameplay/event audio owners, not surface-audio candidates.

### Presentation mechanism findings

| Question | Result |
| --- | --- |
| Surface-dependent sound ID | UNKNOWN; no selector linked to surface state |
| Loop start/stop | Packed loop/stop machinery exists generally; no `0x100000` owner found |
| One-shot request | Many native one-shot callers exist; none is linked to the known surface lineage |
| Volume | No surface-derived volume input identified |
| Pitch/playback rate | No surface-derived pitch or rate input identified |
| Filtering/crossfade | No surface-derived filter or crossfade input identified |
| Speed | Speed-like `EVWORK_CAR +0x1C4` appears in native gameplay/audio-adjacent functions, but no closed surface-audio parameter path exists |
| Distance/travel | No travel accumulator or distance state linked to a surface sound request |
| Wheel rotation | No wheel angular state linked to a surface sound request |
| Timer/counter | Generic counters and sound lifecycle machinery exist; none is owned by `0x100000` |
| Random/procedural state | No random selector or modulator linked to the known surface state |
| Entry/exit versus continuous | Native distinction remains UNKNOWN; no surface-owned start/stop or transition branch was found |

The known sound `0x8D` path at `0x0044BB30` remains a negative control. It uses
a scripted counter and loop/stop requests for Pegasus clopping. Repository
lineage independently identifies that ownership. It is not Road and was not
used to fill any surface-audio evidence gap.

### Existing surface contexts

The audio architecture cannot yet refine the material taxonomy:

| Existing context | Current supported meaning | Audio relationship |
| --- | --- | --- |
| normal road `(2,2,2,2)` | controlled reference context; `.25` normal coefficient in the restored tactile lookup, with stage exceptions | no identified native audio selector |
| Tulip Garden `0x100000` | controlled cobblestone context; `.71` tactile coefficient | audibly distinguishable at runtime; construction UNKNOWN |
| B02/B03 `0x800/0x400` and related mixed states | striped runoff context in those captures | no evidence it enters the same audio selector as cobblestone |
| B04/B05 `4`, `8`, `0x2000` and mixed states | controlled rough/sand context, conservatively named | no distinct sound ID, loop, or modulation proven |
| B06 `8` re-entry context | off-surface/re-entry observation | no native transition sound branch proven |

These names remain local controlled-context descriptions. No raw ID is globally
renamed from audio evidence.

### T5 gates

- **T5 decision:** D — NO DEFENSIBLE SURFACE-AUDIO LINEAGE IDENTIFIED.
- **Presentation classification:** 6 — INSUFFICIENT EVIDENCE.
- **Road 2.0 consequence:** surface semantics remain established, but audio
  timing and construction remain unresolved.
- **Periodic model:** R4.2F-P remains passive research only. The absence of a
  discrete cross-speed sequence plus the unresolved construction path weakens,
  rather than strengthens, the case for periodic Road.
- **Active Road:** remains paused.
- **Audio-fed FFB:** prohibited; no audio observation enters Force.

### Minimum future probe

Static evidence is exhausted at the current boundary. If separately approved,
the narrowest useful runtime probe is observation-only:

1. Observe packed requests at `0x004249F0`, queued requests at `0x00424940`,
   and the lower request boundary at `0x0042F1A0`.
2. Record request command/ID, lifecycle flags, caller/return site, and frame.
3. Associate each request with the already-known four surface fields,
   surface-change mask, speed, gear, Impact, and steering context.
4. Compare stable normal-road and stable all-four-`0x100000` occupancy.
5. Only if a surface-specific ID/caller is demonstrated, trace that caller
   backward to its selector and forward to volume/pitch/buffer controls.

This must be a versioned developer-only observation. It must not alter sound,
telemetry semantics, Force, DirectInput, or use audio to generate wheel output.
No probe or physical test is implemented or requested by T5.

### Unresolved questions

- Does the audible cobblestone texture use the packed request dispatcher or a
  lower continuous-buffer system?
- Is it a loop embedded in a stage/sound bank rather than a car-owned request?
- Are speed and surface identity applied when selecting or updating a buffer
  below `0x0042F1A0`?
- Are entry/exit handled by explicit commands or by continuous mixing state?
- Do runoff and rough/off-road contexts share one selector with different
  assets, or use independent presentation systems?

### Credits and reference context

The synchronized cobblestone evidence and native executable trace are the
project's independent work. GATS remains useful experiential context. THP32 is
**EXTERNAL REFERENCE — NOT INTEGRATED**. No external source, offset, sound ID,
constant, timing value, effect parameter, or audio behavior was integrated.

ALPHA/Force Character, Event 2.0, and R5/AER remain separate lanes.

## Part II — Development journey recap

We could clearly hear cobblestone texture, but driving essentially the same
piece of road at nearly four times the speed did not reveal a stable sequence
of individual audio events. Instead of inventing a frequency, we went back
into the game to determine how its surface audio is actually constructed.

The game has a normal sound-request system with sound IDs and commands for
playing, stopping, and routing sounds. It also has several places that read or
copy the four surface states. What we could not prove is a connection between
them. The only rhythmic sound path already understood is Pegasus clopping, and
that remains a useful example of what an unrelated scripted loop looks like.

So T5 closes without pretending that an audible texture is a known Road
waveform. The next useful step, if approved later, is a small observation-only
probe at the game's own sound-request boundary. That would tell us which native
request and caller actually change when the car enters cobblestone. Until that
ownership is proven, periodic Road stays passive and Active Road stays paused.
