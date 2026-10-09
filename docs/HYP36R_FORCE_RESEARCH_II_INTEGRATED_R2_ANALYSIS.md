# HYP36Rforce FFB Research II — integrated R2 analysis

**PART I — ENGINEERING RECORD**

## Decision

**R3 COMPLETE — READY FOR 2.0 ARCHITECTURE RESEARCH.** Physical R2 is complete.
The accepted R2-A/B/C campaign supports conservative candidate extended
presentation ranges for the current Reference+ architecture:

| Channel | Current recommended marker | Candidate 100% ceiling | Confidence |
|---|---:|---:|---|
| Steering Load | 1.00x | **1.30x** | MODERATE |
| Road Detail | 1.00x | **2.00x** | MODERATE for software headroom; LOW as a complete Road solution |
| Impact | 1.00x | **1.50x** | MODERATE |

These are offline candidates, not implementation decisions, defaults, hardware
torque limits, or proof of perceptual suitability on every wheel. Reference+
at 1.00x remains the recommended/default marker. This milestone changes no
runtime behavior and creates no UAT artifact.

The integrated architectural conclusion is separate: Road and Impact have more
software gain available, but both lose useful source identity before final
presentation. Gain cannot recover that information. HYP36Rforce FFB 2.0 research
should preserve spatial surface meaning and native event meaning before
designing new equations.

## Accepted R2 dataset

Only session records explicitly marked `accepted` are primary evidence.
Rejected, retry, cancelled, unmatched, and historical captures are excluded.
The integrated set contains 15 captures, 15,803 rows, and 263.13 seconds of
sampled in-game time.

| Scenario | Samples | Session duration/rate | Surface context | Quality | Primary purpose |
|---|---:|---:|---|---|---|
| A01 straight baseline | 1,131 | 20 s / 56.58 Hz | stable, one state | CLEAN | quiet LOAD/control |
| A02 progressive left | 1,067 | 18 s / 59.34 Hz | mixed, 8 transitions | CLEAN | left directional response |
| A03 progressive right | 1,055 | 18 s / 58.64 Hz | stable, one state | CLEAN | right directional response |
| A04 sustained high-load corner | 1,183 | 20 s / 59.19 Hz | mixed, 16 transitions | CLEAN | limiting load and BITE case |
| A05 drift initiation | 856 | 15 s / 57.11 Hz | mixed, 11 transitions | CLEAN, expected validity blanks | LOAD→RELEASE→FREE/BITE |
| A06 sustained drift | 1,060 | 18 s / 58.92 Hz | mixed, 18 transitions | CLEAN | sustained FREE/recovery |
| A07 release/recovery | 891 | 15 s / 59.39 Hz | mixed, 8 transitions | CLEAN | FREE→recovery→LOAD |
| B01 local asphalt control | 972 | 20 s / 60.01 Hz | stable `2/2/2/2` | USABLE WITH COVERAGE CAVEAT | local surface control |
| B02 striped partial | 580 | 18 s / 56.91 Hz | mixed, 24 transitions | USABLE WITH COVERAGE CAVEAT | partial occupancy |
| B03 striped full | 1,080 | 18 s / 60.03 Hz | mixed, 24 transitions | CLEAN | broad occupancy |
| B04 rough/sand partial | 1,080 | 18 s / 60.03 Hz | mixed, 42 transitions | CLEAN | continuous Road control |
| B05 rough/sand full | 1,080 | 18 s / 60.01 Hz | mixed, 33 transitions | CLEAN | broad rough/transients |
| B06 surface re-entry | 1,068 | 18 s / 59.37 Hz | mixed, 28 transitions | CLEAN | transition/recovery ordering |
| C01 gear shifts | 1,500 | 25 s / 60.03 Hz | stable `2/2/2/2` | CLEAN | canonical shift transient |
| C02 controlled impact | 1,200 | 20 s / 60.01 Hz | mixed, 30 transitions | CLEAN | physical-impact candidates |

B01 and B02 have less sampled in-game time than wall time, but their sampled
frames retain clean approximately 60 Hz spacing. Every accepted file remains
suitable for its offline replay purpose.

## Common replay method

Every channel and combined test uses the same recorded same-cycle values:

```text
recorded directional_pre_gain × Steering multiplier
recorded road_pre_gain        × Road multiplier
recorded impact_pre_gain      × Impact multiplier
        ↓
sum to replay pre-tanh composition
        ↓
tanh
        ↓
recorded output_ramp
        ↓
inversion and recorded 100% master Strength
        ↓
DirectInput request/clamp observation
```

Independent sweeps hold the other two multipliers at 1.00x. Combined tests
apply gains to the channels already selected by Reference+ and recompose them.
Replay does not recalculate or alter vehicle state, raw surfaces, M4, M5,
LOAD/RELEASE/FREE/BITE, event identity, output ramp, or device behavior.
Rolling RMS is calculated inside each continuous capture before aggregation.
All replay outputs are finite.

“Sign crossover” means the new **composed** output crosses zero relative to the
1.00x composition, normally where channels nearly cancel. Positive channel
gain never reverses that channel's own sign.

## Steering Load

R2-A's seven accepted captures provide the independent Steering replay. A02–A03
establish opposed directional response; A04 is the limiting sustained-load
case; A05–A07 cover drift, FREE, release, and recovery.

| Gain | P95 / P99 / max | 1 s RMS P95 / max | 3 s RMS P95 / max | Occupancy .75/.90/.98 | Pre-tanh >1 | Min headroom | Slew P99/max |
|---:|---|---|---|---|---:|---:|---|
| 1.00 | .371/.506/.682 | .317/.453 | .285/.344 | 0/0/0% | 0% | .318 | 4.23/18.14 |
| 1.10 | .403/.545/.713 | .343/.491 | .309/.371 | 0/0/0% | 0% | .287 | 4.30/17.86 |
| 1.20 | .434/.581/.741 | .368/.527 | .333/.399 | 0/0/0% | 0% | .259 | 4.45/17.58 |
| 1.30 | .463/.616/.767 | .392/.561 | .354/.425 | .055/0/0% | .028% | .233 | 4.67/17.54 |
| 1.40 | .492/.647/.791 | .414/.594 | .374/.450 | .083/0/0% | .069% | .209 | 4.90/17.50 |
| 1.50 | .520/.677/.813 | .435/.624 | .394/.473 | .110/0/0% | .097% | .187 | 4.99/17.46 |
| 1.60 | .548/.706/.832 | .457/.653 | .412/.496 | .276/0/0% | .166% | .168 | 5.02/17.42 |
| 1.75 | .586/.744/.858 | .486/.693 | .441/.527 | .884/0/0% | .649% | .142 | 5.14/18.21 |
| 2.00 | .645/.799/.893 | .533/.751 | .482/.574 | 2.209/0/0% | 1.657% | .107 | 5.71/20.38 |

No gain clamps. A04 remains the limiting case and increasingly enters `tanh`
compression above 1.30x. **Candidate Steering ceiling: 1.30x, MODERATE
confidence.** It is the upper edge of the clearly comfortable controlled
region and also survives the combined test. Confidence is not HIGH because the
extended value has not been physically compared across cars and wheel classes.

## Road Detail

| Gain | P95 / P99 / max | 1 s RMS P95 / max | 3 s RMS P95 / max | Occupancy .75/.90/.98 | Pre-tanh >1 | Min headroom | Slew P99/max |
|---:|---|---|---|---|---:|---:|---|
| 1.00 | .158/.233/.502 | .163/.285 | .141/.180 | 0/0/0% | 0% | .498 | 1.86/12.92 |
| 1.10 | .158/.233/.502 | .163/.285 | .141/.180 | 0/0/0% | 0% | .498 | 1.89/12.90 |
| 1.20 | .158/.233/.502 | .163/.285 | .141/.180 | 0/0/0% | 0% | .498 | 1.90/12.88 |
| 1.30 | .158/.232/.502 | .163/.285 | .141/.180 | 0/0/0% | 0% | .498 | 1.90/12.86 |
| 1.40 | .158/.232/.502 | .163/.285 | .141/.180 | 0/0/0% | 0% | .498 | 1.91/12.85 |
| 1.50 | .158/.231/.502 | .163/.285 | .141/.180 | 0/0/0% | 0% | .498 | 1.91/12.83 |
| 1.60 | .159/.231/.502 | .163/.285 | .141/.180 | 0/0/0% | 0% | .498 | 1.91/12.81 |
| 1.75 | .159/.231/.502 | .163/.285 | .141/.180 | 0/0/0% | 0% | .498 | 1.95/12.78 |
| 2.00 | .158/.234/.502 | .163/.285 | .141/.180 | 0/0/0% | 0% | .498 | 2.05/12.74 |

The small campaign-level movement reflects Road's sparse, low amplitude next to
Directional and Impact, not an absence of local surface change. **Candidate
current-architecture Road ceiling: 2.00x, MODERATE confidence for software
headroom.** It remains unclamped in the combined test. Confidence in 2.00x as a
complete player-facing Road solution is LOW because controlled R2 does not
establish perception across hardware and gain does not restore lost meaning.

### Road's information limit

```text
four native per-corner surface states
    ↓ all four remain observable and spatially independent
restored Xbox CalcVibrationValues()
    ↓ combines surface and other native state
left/right gamepad motor values
    ↓ HYP36R selects only the right motor for Road amplitude
one scalar envelope × synthetic 6 Hz carrier
    ↓ Road Detail gain
scalar Road contribution
```

Spatial occupancy and class identity cease to be independently represented in
the two restored motor composites. HYP36R narrows the result again to one motor
and adds a synthetic carrier. Current Road can communicate continuous activity,
but cannot say which corner, how many corners, or which source/context produced
it. Force 2.0 Road research must keep spatial occupancy, material/context
identity, continuous texture, physical surface events, and later grip/load
response distinct before selecting a presentation.

## Impact

C01 and C02 remain separate because C01 is the exact known-good shift reference
and C02 contains the larger physical-contact candidates.

| Gain | C01 P95/P99/max | C02 P95/P99/max | C02 1 s RMS P95/max | C02 3 s RMS P95/max | C02 min headroom | C02 slew P99/max |
|---:|---|---|---|---|---:|---|
| 1.00 | .137/.160/.228 | .238/.255/.373 | .220/.243 | .173/.185 | .627 | 4.24/16.19 |
| 1.10 | .137/.161/.236 | .239/.255/.387 | .220/.243 | .175/.185 | .613 | 4.34/17.74 |
| 1.20 | .137/.161/.243 | .239/.255/.401 | .220/.243 | .177/.185 | .599 | 4.74/19.28 |
| 1.30 | .137/.161/.250 | .240/.263/.414 | .223/.243 | .180/.185 | .586 | 5.13/20.81 |
| 1.40 | .139/.165/.257 | .241/.276/.427 | .225/.243 | .182/.188 | .573 | 5.52/22.33 |
| 1.50 | .142/.172/.265 | .241/.285/.440 | .227/.243 | .183/.191 | .560 | 5.91/23.84 |
| 1.60 | .143/.184/.272 | .242/.292/.453 | .231/.243 | .184/.194 | .547 | 6.29/25.33 |
| 1.75 | .144/.193/.282 | .245/.310/.472 | .235/.248 | .185/.198 | .528 | 6.76/27.55 |
| 2.00 | .149/.206/.300 | .249/.334/.503 | .239/.260 | .190/.206 | .497 | 6.37/31.18 |

Every Impact replay has zero `.75/.90/.98` occupancy, zero pre-`tanh` over-one
frames, zero clamps, and finite output. **Candidate current-architecture Impact
ceiling: 1.50x, MODERATE confidence.** This preserves substantial software
headroom and avoids using the highest tested transient slew as the first
candidate.

The architecture caveat controls the interpretation: any scalar Impact gain
also amplifies the canonical gear shift. At 1.50x, the exact `.07735` canonical
shift carrier becomes `.11603` before composition. That does not prove the
shift would feel wrong, but it does mean collision emphasis cannot be changed
independently while the common scalar path remains. Current 1.00x shift feel is
the known-good baseline and is not retuned here. Force 2.0 can preserve that
baseline while researching separate event presentation.

## Combined-channel analysis

The candidate configuration is Steering `1.30x`, Road `2.00x`, Impact `1.50x`.
Each accepted row retains its naturally occurring channel combination.

| Configuration | P95/P99/max | 1 s RMS P95/max | 3 s RMS P95/max | Occupancy .75/.90/.98 | Pre-tanh >1 | Min headroom | Slew P99/max | Crossovers |
|---|---|---|---|---|---:|---:|---|---:|
| Baseline 1/1/1 | .262/.457/.682 | .269/.453 | .242/.344 | 0/0/0% | 0% | .318 | 3.48/18.14 | 0 |
| Steering+Road 1.3/2/1 | .333/.565/.779 | .337/.561 | .305/.425 | .032/0/0% | .025% | .221 | 4.13/17.54 | 82 |
| Steering+Impact 1.3/1/1.5 | .338/.566/.810 | .340/.561 | .307/.430 | .038/0/0% | .032% | .190 | 4.55/25.24 | 50 |
| Road+Impact 1/2/1.5 | .277/.462/.741 | .274/.453 | .245/.358 | 0/0/0% | 0% | .259 | 4.34/26.00 | 156 |
| **All candidates 1.3/2/1.5** | **.342/.568/.813** | **.340/.561** | **.307/.430** | **.038/0/0%** | **.038%** | **.187** | **4.92/25.24** | **66** |

The all-candidate replay is finite and unclamped. Steering remains the largest
absolute channel on 89.77% of rows, Impact on 6.83%, Road on 1.59%, with 1.80%
ties. Road does not become a global force driver. Impact increases transient
slew but not sustained high occupancy. Crossovers are low-magnitude cancellation
changes, not channel sign inversions.

Grid checks around the candidate remain unclamped: 1.4/2/1.5 peaks at `.832`,
1.5/2/1.5 at `.850`, and 1.3/2/1.75 at `.828`. These do not justify raising the
candidate; Steering above 1.30 is already headroom-aware and Impact above 1.50
further magnifies the known-good shift.

## Candidate player mapping and defaults

For an eventual extended slider, candidate linear internal mappings are:

```text
internal gain = player_percent / 100 × channel_ceiling
```

| Channel | 0% | Recommended marker | 100% | Recommended marker position |
|---|---:|---:|---:|---:|
| Steering | 0x | 1.00x | 1.30x | 76.9% |
| Road | 0x | 1.00x | 2.00x | 50.0% |
| Impact | 0x | 1.00x | 1.50x | 66.7% |

The mapping is a candidate specification only. Reference+ at 1.00x remains the
default/recommended marker for every channel. Extended range is optional player
emphasis; research maxima do not become defaults.

## Impact and event architecture

```text
native event meaning
    ↓ gear mismatch, surface/native state and other event sources exist here
restored left/right vibration signature
    ↓ channel pattern retains partial evidence
max(left,right)
    ↓ left/right and source pattern collapse
positive rise > .12
    ↓ steering-selected direction + bounded kick
single impactForce × .90/update
    ↓ Impact gain
scalar Impact
```

R2-C proves controlled gear and physical-contact candidates are distinguishable
upstream. Current Impact cannot identify its cause after the max/rise/envelope
stage. Force 2.0 research should preserve supported gear, collision, surface
transient, and other native event classes long enough to choose presentation.
It should use current shift feel as the gear reference, not treat every event as
an anonymous amplitude or invent unsupported labels.

## Native per-corner candidates

| Field | Integrated evidence | Current classification | Next independent question |
|---|---|---|---|
| `field14` | 23,440/23,440 R2-B comparisons exactly match raw surface state through a distinct address/path | mirrors the same underlying surface game state | establish writer/copy direction before removing either observation |
| `fieldE8` | neutral 1.0; changes to 0.7/mixed in controlled rough/re-entry contexts; may lag raw changes | surface/contact-context candidate, not identity alias | isolate contact loss/re-entry at constant raw class and low vehicle motion |
| `fieldEC` | paired 0/1 and 2/3 behavior; signed distribution changes with direction and expands under load/drift | dynamic vehicle-state correlation candidate | repeat opposed steady corners on one stable surface and compare phase/scale |
| `fieldEE` | related but not identical to EC; larger excursions in drift/recovery and continues after surface stability | dynamic/drift/recovery correlation candidate | isolate drift entry, hold, and recovery without surface/event contamination |

No suspension, tire-load, grip, or material physics names are assigned.

## LOAD, RELEASE, FREE, and BITE

- **LOAD** is strongly supported by straight/progressive controls and recovery
  returns. It is the normal M4 phase, not a generic “car has grip” label.
- **RELEASE** is supported as the emerging transition observed before FREE and
  during reversals. It is brief and can differ from the operator's maneuver
  label.
- **FREE** is strongly supported by sustained A04/A05/A06 intervals. It is the
  established M4 unloading state, not proof of one physical drift angle.
- **BITE** is strongly supported where the actual classifier activates in A04
  and A05. A07 was a recovery scenario but did not activate BITE; maneuver name
  and classifier state must remain separate.
- Recovery transitions back toward LOAD are established in A04–A07. Surface
  transitions can overlap individual sequences, but uncontaminated intervals
  also exist.

The model is ready for state-interpretation research, not a logic change. An
A07 BITE-specific repeat remains optional and is not a blocker.

## Signal identity map

| Signal class | Raw/earliest supported source | Current HYP36R destination | Identity status |
|---|---|---|---|
| Vehicle/directional | steering, speed, native response state, M4/M5 | Reference+ directional | partially preserved through interpreted state |
| Surface identity | four `water_flag_24C` states / mirrored `field14` | observed only; indirect effect on Road | collapsed before Road |
| Continuous surface activity | restored motor levels, especially right | synthetic Road carrier | scalar activity retained, source/spatial identity collapsed |
| Surface transition | four explicit change streams | telemetry/context; may affect restored effects | preserved in telemetry, not a separate wheel channel |
| Gear event | native gear mismatch and exact left-only motor signature | common Impact | available upstream, collapsed in Impact |
| Physical-impact event | bilateral restored vibration/rise in controlled capture | common Impact | available as controlled upstream pattern, collapsed in Impact |
| Grip/load state | native steering-response state, M4/BITE interpretation | directional authority/unloading | interpreted state preserved; physical semantics bounded |
| Per-corner dynamic candidates | E8/EC/EE | passive telemetry/context | preserved raw, semantics unresolved |

## Future pedals, haptics, and motion

Future outputs should consume the underlying event/vehicle signals where
possible, not reverse-engineer them from final wheel torque.

| Output use | Candidate evidence | Confidence |
|---|---|---|
| Haptic shift cue | native gear event | HIGH event identity; presentation untested |
| Collision/body haptic | controlled bilateral transient candidates | MODERATE; event classifier needs broader proof |
| Surface transducers | spatial raw state plus continuous activity | MODERATE; material map and presentation remain open |
| Active-pedal cue | gear event and validated vehicle-state transitions | LOW–MODERATE; no pedal protocol or causal pedal model exists |
| Motion load/release | directional state, M4 phases, native dynamic candidates | MODERATE for state timing, LOW for platform equations |
| Per-corner motion/texture | four surface states and E8/EC/EE candidates | LOW until dynamic semantics are independently validated |

## AER comparison readiness

R2 provides a strong PC-side reference: steering/directional response, native
state timing, per-corner surface identity, restored PC vibration behavior,
canonical gear events, controlled physical transients, Road/Impact composition,
and final DirectInput request are synchronized. It does **not** identify an
official arcade/AER force law.

Keep three branches separate:

1. HYP36R's physics/state-derived directional interpretation.
2. Native PC/restored Xbox effect behavior.
3. Future official arcade/AER observations.

Highest-value AER questions are the official force signal's direction, scale,
timing, speed/load dependency, surface response, gear/collision treatment, and
whether it preserves event or spatial identity before cabinet output.

## Force 2.0 scorecard

| Goal | Status | Reason |
|---:|---|---|
| 1. Spatial/per-corner surface behavior understood | **UNDERSTANDING ESTABLISHED** | four independent states, side grouping, partial/broad occupancy and field14 mirroring established |
| 2. Improved Road based on those signals | **READY FOR IMPLEMENTATION RESEARCH** | information-loss boundary is known; no new Road exists |
| 3. Validated extended Force Character ranges | **EVIDENCE OBTAINED** | controlled offline candidates exist; no raised-gain physical validation or implementation |
| 4. Deeper validated native physics incorporated | **EVIDENCE OBTAINED** | candidates and state lineage exist; no new native field is incorporated |
| 5. Improved LOAD/RELEASE/FREE/BITE interpretation | **READY FOR IMPLEMENTATION RESEARCH** | states/transitions have controlled evidence; logic is unchanged |
| 6. Separate texture, physical events and grip | **UNDERSTANDING ESTABLISHED** | independent evidence and current collapse boundaries are mapped |
| 7. AER second FFB experience | **NOT STARTED** | PC comparison reference is ready; no official AER evidence is integrated |
| 8. Active pedals/haptics/motion architecture | **READY FOR IMPLEMENTATION RESEARCH** | source signal classes are mapped; protocols and output models do not exist |

## Release boundary

| Release | Appropriate work |
|---|---|
| v1.0.x hotfix | verified crashes, compatibility regressions, packaging or device defects that preserve current interpretation |
| v1.5 polish | controller mapping cleanup, Multi Input UX, diagnostics, upstream integration, and small evidenced presentation fixes that do not depend on Research II architecture |
| v2.0 | extended-range implementation based on R3, spatial Road, event-aware Impact, native per-corner/state interpretation, state-model refinement, AER, and future-output signal architecture |

If a change depends on Research II's surface, event, or native-physics
conclusions, it belongs to 2.0.

## Physical testing and next milestone

**NO MORE PHYSICAL TESTING REQUIRED.** R3 exposes no specific missing physical
evidence that blocks architecture research. The optional A07 BITE repeat and
future raised-gain/wheel-class UAT are validation opportunities, not current
research blockers.

The next milestone should be **R4 — HYP36Rforce FFB 2.0 signal-preservation
architecture**, design/research only. It should specify boundaries and passive
observation interfaces for spatial Road, event-aware Impact, per-corner dynamic
validation, and state-model refinement. It must not begin active equations or
runtime output until each proposed source retains a clear evidence lineage.

## Part II — Development Journey Recap

R2 brought the vehicle, surface, and transient campaigns together without
turning every interesting signal into Force. Steering Load had controlled room
for bounded development. Road proved to be limited by lost identity as well as
magnitude. Impact contained distinguishable event classes upstream even though
the current presentation combined them.

The important result was architectural: preserve information before tuning its
presentation. The study did not make offline ceilings into defaults, prove
universal tire physics, or integrate Road 2.0, Event 2.0, or AER. Those became
separate lanes with explicit validation gates.
