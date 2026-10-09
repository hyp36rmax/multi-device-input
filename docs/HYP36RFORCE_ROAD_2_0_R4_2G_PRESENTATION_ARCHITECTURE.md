# HYP36Rforce Road 2.0 R4.2G — native evidence to presentation architecture

## Part I — Engineering record

### Scope and decision

R4.2G closes the native-timing discovery phase and asks a narrower, more
honest question: how should HYP36Rforce communicate the native surface
information the PC game actually preserves?

**Architecture direction: E — HYBRID. Confidence: MODERATE.**

The preferred direction combines the proven native Road envelope, categorical
surface/material context, speed/context scaling, and transition/occupancy state
with a bounded HYP36Rforce presentation layer. If that layer supplies texture
timing or waveform character, those parts are explicitly synthetic. This is an
architecture decision only. It adds no runtime path, DirectInput effect, wheel
output, UAT, setting, or physical test.

### T6 closure

Accepted controlled T6 traversals reached complete four-corner `0x100000`
cobblestone occupancy. Observation at `SetSndQueue`, `PrjSndRequest`, and the
lower play/route boundary found no defensible surface-specific request,
lifecycle, or command relationship.

**T6 gate: C — NO SURFACE-SPECIFIC REQUEST AT THESE BOUNDARIES.**

Those dispatcher boundaries are closed. R4.2G does not descend into the mixer
or broaden instrumentation into the audio engine.

### Native Timing Discovery — closed

> No defensible native Road timing carrier was identified within the
> investigated PC paths after tactile lineage analysis, spatial analysis,
> cross-surface analysis, synchronized audio comparison, static audio tracing,
> and native sound-request instrumentation.

This is a bounded evidence statement. It does not claim that the executable
mathematically cannot contain timing information anywhere.

### Evidence boundary

| Road information | Ownership | Current support | Permitted architectural use |
| --- | --- | --- | --- |
| four raw per-corner surface states | Native | spatial occupancy and partial/full transitions are observed | identify material/context and occupancy |
| material lookup strength | Native-derived | strongest applicable positive coefficient is established | amplitude authority, not waveform |
| speed/context scaling | Native-derived | stable cobblestone envelope follows speed almost exactly | govern intensity/context |
| unsigned left/right motor envelope | Native-derived | mapped PC/Xbox tactile output is established | evidence and comparison authority |
| entry/exit/contact changes | Derived from native state | previous/current four-corner tuples and transition mask are observed | bounded onset/release structure |
| surface grouping and occupancy summary | Derived from native state | deterministic grouping is available | policy/gating; no invented physics |
| texture timing, phase, waveform, density, attack/release | Synthetic presentation if created | not supplied by recovered native state | allowed only when labeled HYP36Rforce presentation |
| final DirectInput topology | Synthetic presentation | not selected by R4.2G | later passive comparison only |

Proven native evidence supports surface identity, positive material response,
strongest-response selection, speed/context amplitude scaling, an unsigned
motor envelope, transitions, and four-corner spatial occupancy.

The evidence does not support a signed Road carrier, steering-wheel Road
waveform, spatial phase, wavelength, Road frequency, recoverable periodic
cobblestone or runoff cadence, audio-derived frequency, physical surface
spacing, audio-fed FFB, or a surface-owned T6 dispatcher request.

### R4.2F-P disposition

**C — RETAIN ONLY AS AN OPTIONAL SYNTHETIC PRESENTATION CANDIDATE.**

R4.2F-P was useful because it kept amplitude evidence separate from timing and
proved a passive topology could remain isolated. Later work did not validate
its time- or distance-domain phase as native-derived. Its implementation and
records remain research controls, but it is no longer the leading Road 2.0
architecture and must never be described as recovered native cadence. It stays
inactive.

### Candidate architecture assessment

| Candidate | What survives | Main weakness | Decision |
| --- | --- | --- | --- |
| A — native envelope refinement | game-authored material and speed amplitude; simplest and most portable | adds little texture character beyond current Reference+ Road and preserves the original strongest-response collapse | valid conservative control, not sufficient alone |
| B — contextual texture synthesis | material class and native amplitude can govern distinct, clearly synthetic character | can imply unsupported material physics if character policy is over-specific | viable component if generic and bounded |
| C — aperiodic/stochastic texture | avoids claiming a recovered fixed cadence; can express roughness without a persistent buzz | noise can feel artificial, mask weak evidence, vary across frame rates, or become non-repeatable | promising carrier family with deterministic seeding and strict bounds |
| D — event + envelope | transitions give native structure while continuous amplitude stays authoritative | transitions alone cannot represent sustained surface texture and must not borrow collision/gear semantics | valuable structural component, not a complete Road model |
| E — hybrid | combines native envelope/context, native-derived transitions, and explicitly synthetic aperiodic character | needs careful ownership telemetry, budgeting, deterministic behavior, and device validation | preferred direction |

Candidate F, closing active Road entirely, remains the correct fallback if a
passive hybrid cannot materially outperform the Reference+ experience without
confusing Directional or Impact.

### Material character, not invented frequency

Surface categories may govern presentation character without being assigned a
physical frequency. Candidate dimensions include bounded density, roughness,
impulse distribution, envelope response, attack/release, and occupancy-aware
balance. Values remain HYP36Rforce policy unless a native relationship is
proved. Labels such as “cobblestone = X Hz” or claims of physical stone spacing
are prohibited.

Deterministic seeded aperiodic texture is more consistent with current evidence
than a fixed periodic oscillator. It avoids a single artificial buzz and makes
replay reproducible. Its risks are audible/feelable randomness, bandwidth and
update-rate sensitivity, device inconsistency, and accidental directional
bias. A zero-mean construction, deterministic seed lifecycle, bounded slew and
energy, authoritative inactivity reset, and identical replay are requirements,
not optional polish.

### Four-corner preservation

Road 2.0 should preserve the native four-corner tuple through the semantic
layer instead of immediately collapsing it to the maximum coefficient.
Occupancy count, material mixture, and partial/full transition state can
improve authorization, confidence, onset, and release. Left/right occupancy may
inform neutral texture balance only after passive validation; it cannot assign
steering torque direction. Front/rear order may inform transition sequencing,
but not suspension travel, tire load, or physical bump position. The native
context is faithful; a physical interpretation beyond it is not.

### DirectInput topology assessment

- **ConstantForce modulation:** flexible enough for an aperiodic zero-mean
  request, but it shares the directional channel, consumes headroom, and risks
  bias or clipping. It is not selected without proof of clean composition.
- **Periodic effect:** device-separated and naturally centered, but represents
  an oscillator. With no native rate or phase it has no remaining claim as a
  native Road topology. It may remain a synthetic comparison/fallback only.
- **Spring/damper modulation:** changes steering weight or resistance and
  conflicts with Directional ownership. It remains rejected for Road.
- **Existing supported mechanisms in combination:** potentially portable, but
  effect slots, start/update latency, unsupported-device fallback, teardown,
  clipping, and DD-wheel behavior require passive and later hardware gates.

R4.2G deliberately does not select the final DirectInput mechanism. Semantic
authority must be validated before an effect class is chosen.

### Synthesis governance

The governing rule is:

> **Native state owns whether Road is present, what material/context is active,
> and how much authority is available. Derived policy may organize that state.
> HYP36Rforce may own how the authorized information is presented, provided
> every synthetic component is explicit, bounded, deterministic, removable,
> and unable to create Road without native authorization.**

Every component must remain classifiable as Native, Derived, or Synthetic
Presentation in telemetry, replay, documentation, and code ownership.

### Reference+ relationship and player controls

Reference+ remains the proven product baseline and comparison control. A future
Road 2.0 candidate should initially be an optional passive/shadow policy, then
an explicitly selectable advanced presentation only after evidence and hardware
validation. Replacement is considered only if it wins that comparison without
regression; version marketing is irrelevant.

Player controls remain simple: the existing Road Detail 0–100% surface should
remain the primary control. No frequency, wavelength, seed, noise-density, or
effect-topology slider is justified. Engineering parameters belong in
developer telemetry and replay until a player-facing need is demonstrated.

### Safety and failure policy

Any later candidate must remain bounded inside existing composition headroom,
preserve Directional and Impact ownership, produce no activity on stable normal
road, stop immediately and smoothly when source validity is lost, remain
zero-mean where a bipolar carrier exists, avoid sign or DC steering bias,
handle unsupported effect classes safely, and fall back to Reference+ without
corrupting device state. R4.2G establishes no new ceiling.

### Smallest future passive prototype

The next milestone, if approved, is a replay-first **passive hybrid
presentation shadow**. It should consume only the proven Road semantic state
and emit no DirectInput request. Per frame it should record:

1. native surface tuple, coefficient/envelope, speed/context, and validity;
2. derived occupancy/material group, transition phase, and authority;
3. a deterministic zero-mean aperiodic presentation request;
4. separate A-envelope, C-aperiodic, D-transition, and E-hybrid shadow values;
5. energy, peak, slew, DC bias, active duty, budget use, and reset reason;
6. unchanged Reference+ Road, Directional, Impact, final Force, and hardware
   request for comparison.

No new physical capture is required to design or replay this prototype. It
should first use accepted Road/control captures. It advances only if it remains
silent on normal road and exclusions, is deterministic, adds no DC bias, stays
inside a predeclared secondary budget, preserves transitions without impulses,
and shows materially clearer separation of supported material contexts than
the envelope-only control. Failure closes active Road 2.0 rather than prompting
arbitrary tuning.

**Active Road gate: CLOSED.** It may reopen only after passive architecture,
replay, ownership, safety, and project-owner review succeed.

## Part II — Development journey recap

We started by asking whether the original game contained richer Road timing
that could be restored to a modern wheel. We followed the controller signal,
tested different surfaces, reconstructed distance, synchronized game audio,
compared the same road at nearly four times the speed, traced the sound system,
and instrumented native sound requests. The evidence consistently preserved
surface identity and amplitude—but not a recoverable timing waveform.

That changed the design question from “what frequency should we restore?” to
“how should we faithfully present the native surface information the game
actually provides?” The answer is not to relabel a synthetic oscillator as
native. It is to preserve native authority and context, then evaluate a clearly
owned, bounded presentation layer against the Reference+ baseline. If that
layer cannot improve communication cleanly, current Road remains the answer.

## Credits and reference context

The native lineage, controlled captures, synchronized comparisons, and T6
request-boundary result are independent HYP36Rforce research. GATS remains
experiential/reference context. THP32 remains **EXTERNAL REFERENCE — NOT
INTEGRATED**. No external code, offsets, constants, equations, frequencies,
wavelengths, effect parameters, or behavior fill the unresolved evidence.

ALPHA / Force Character, Event 2.0, and R5/AER remain separate lanes.
