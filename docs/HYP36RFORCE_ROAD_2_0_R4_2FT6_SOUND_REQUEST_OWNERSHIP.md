# Road 2.0 R4.2F-T6 native sound-request ownership probe

## Part I — Engineering record

T5 found no defensible static path from OutRun's known surface state to its
native sound dispatcher. T6 therefore adds one narrow observation-only probe
at the three boundaries earned by that analysis. It does not broaden into the
mixer and it does not connect audio to HYP36Rforce FFB.

## Observation boundary

The developer probe observes the supported executable at `0x00424940`
(`SetSndQueue`), `0x004249F0` (`PrjSndRequest`), and `0x0042F1A0` (the lower
play/route boundary).

At each call it buffers the packed command, decoded sound ID and flags, caller
RVA, frame, monotonic session time, four native surface values and transition
mask, speed, gear and gear transition, post-gain Impact state, and steering.
Hook callbacks perform no file I/O and no allocation. The fixed buffer holds
16,384 records and stops at 45 seconds or when full.

The separate trace schema is `HYP36R_SOUND_REQUEST_TRACE_V1`; the public
222-column telemetry schema is unchanged. Metadata records product identity,
build commit, scenario, linked telemetry filename, completion state, count,
and drops.

## Developer workflow

The control lives under `F11 -> Debug -> Road Research`. Starting it opens the
normal research telemetry capture and the separate request trace. The future
run uses OutRun 2, Tulip Garden, immediately after the first corner: normal
road, partial entry, all four values at `0x100000`, partial exit, normal road.
Finish after the exit. Output is below
`HYP36R/Research/R4_2FT6_SOUND_REQUEST_OWNERSHIP/`.

No physical capture is authorized by this implementation milestone. Windows
CI and project-owner review must precede runtime UAT.

## Deterministic analysis

`research/sound_request_trace_analyze.py` validates the schema, segments the
longest complete occupancy into normal-before, entry, full cobblestone, exit,
and normal-after, then groups counts and rates by boundary, sound ID, caller,
surface tuple, and phase. It reports gear/Impact contamination and rejects
sound `0x8D` as the known Pegasus false lead.

Candidate labels are conservative. UI, music, voice, stage, collision, gear,
and unrelated vehicle ownership still require caller tracing or a controlled
follow-up. Temporal coincidence is not a semantic result.

## Decision gate

T6 is **pending runtime evidence**. A future accepted capture must return A,
surface-owned request; B, strong candidate needing one narrow follow-up; C, no
surface-specific request at these boundaries; or D, presentation below/outside
the observed boundaries. None authorizes Road output or audio-fed Force.

## Part II — Development journey recap

Static analysis could not connect the known cobblestone surface state to the
game's sound system. Rather than guessing, we instrumented only the native
sound-request boundaries actually identified and asked whether their traffic
changes when the car enters and leaves the known surface. The observation is
ready; the answer remains intentionally open until a controlled capture exists.

## Credits and reference context

GATS remains experiential/reference context. THP32 remains **EXTERNAL
REFERENCE — NOT INTEGRATED**. No external code, offsets, IDs, constants,
equations, or behavior were used by this probe.
