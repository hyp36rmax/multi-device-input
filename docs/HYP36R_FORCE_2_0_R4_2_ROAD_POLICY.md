# HYP36R Force 2.0 R4.2 Road policy

R4.2 asks what Road means before deciding how Road should feel at the wheel. It
implements `HYP36R_ROAD_POLICY_V1` as a passive interpretation of
`HYP36R_SIGNAL_STATE_V1`. It does not produce torque, modify current Road, apply
Road Detail, or participate in Reference+ composition.

## Responsibility

Road 2.0 owns only evidence about continuous surface activity, spatial surface
occupancy and surface transitions. Directional retains steering authority and
LOAD/RELEASE/FREE/BITE. Gear shifts and collision candidates remain discrete
events. Raw IDs do not imply a material or strength.

The policy consumes only Signal State:

- four canonical raw surfaces, previous values and change flags;
- neutral occupancy and established channel groups 0/2 and 1/3;
- independent restored native left, right and rise evidence;
- observation validity, freshness, confidence and provenance;
- gear/collision/unknown event identity for exclusion.

It does not read game memory. `field14` is not consumed. E8, EC and EE are not
consumed. Grip state is deliberately not an activity input.

## Policy contract

The fixed-size output retains three independent components:

1. `SpatialContext` preserves raw group values, within-group agreement,
   uniform/paired/general-mixed topology, distinct count and coverage relative
   to an established local reference.
2. `ContinuousActivity` preserves left/right/rise evidence and labels it only
   as inactive, a continuous candidate, excluded event or unavailable.
3. `TransitionEvent` preserves the exact previous/current arrays, changed mask
   and changed count. It does not synthesize a bump.

No component is convertible to a force scalar. Broad occupancy is not stronger
than partial occupancy by definition.

## Local reference and occupancy

After six consecutive valid uniform samples, the policy records that raw value
as a session-local reference. It then reports zero, one-to-three, or four
channels differing as `Reference`, `Partial`, or `Broad`. This permits a known
control-to-target replay to retain coverage without creating a global material
table.

The policy never relabels IDs `2`, `4`, `8`, `1024`, `2048`, `8192`, or an
unknown ID. In R2-B, `2` remains a strong local-asphalt candidate only.

A capture that starts on another uniform state establishes that state as its
local reference. This is intentionally cautious. For example, isolated B06
starts on all-`8`, so its later all-`2` return is represented as a broad change
from the capture's initial state, not globally declared “asphalt re-entry.”

## Continuous activity and isolation

Native L/R activity is only a continuous candidate when a valid non-reference
surface context exists. A stable reference surface does not become active just
because a vibration channel is nonzero. Gear, collision-candidate and unknown
native transients are marked `EventExcluded`; they cannot become continuous
Road activity. Changing grip state alone has no policy effect.

This is interpretation evidence, not physical texture truth. The native motor
signals are composite game intent, and confidence remains moderate at most.

## Failure behavior

Invalid, stale, unsupported, non-finite or insufficient-confidence inputs make
the affected policy component unavailable. A valid surface with no established
reference remains observable, but continuous activity stays inactive with an
explicit reason. There is no compatibility scalar inside the policy; current
v1 Road remains independently authoritative.

## Accepted R2-B replay

All 5,860 rows from the accepted `R2_B_UAT.zip` were replayed. Counts below are
policy observations, not presentation strengths.

| Scenario | Local reference | Raw patterns | Transition rows / first-last | Mixed rows | Partial / broad rows | Continuous candidates | Event-excluded rows | Interpretation |
| --- | ---: | ---: | --- | ---: | ---: | ---: | ---: | --- |
| B01 local asphalt control | 2 | 1 | 0 | 0 | 0 / 0 | 0 | 6 | Stable uniform reference; non-surface native events did not create Road activity |
| B02 striped partial | 2 | 14 | 24 / 3.606-8.572 s | 221 | 173 / 90 | 173 | 4 | Paired partial occupancy and staged return retained |
| B03 striped full | 2 | 17 | 24 / 4.461-16.691 s | 151 | 83 / 219 | 175 | 9 | Broader occupancy retained without a stronger-than-B02 rule |
| B04 rough/sand partial | 2 | 17 | 42 / 0.128-17.412 s | 826 | 231 / 742 | 943 | 2 | Persistent native activity retained separately from Impact |
| B05 rough/sand full | 2 | 27 | 33 / 1.232-10.432 s | 272 | 65 / 428 | 467 | 13 | Broad and mixed context retained; transient events excluded |
| B06 surface re-entry | 8 | 24 | 28 / 5.688-11.803 s | 209 | 103 / 636 | 230 | 11 | Surface transition timing retained independently of the later LOAD sequence |

The event-exclusion count includes gear and qualifying unknown/collision
transients. It deliberately does not change current Impact or gear presentation.

## Information preservation versus v1

For every accepted row, the policy retained the four-value surface tuple,
0/2 and 1/3 group values, occupancy topology, exact transition mask, native L/R
evidence, validity and provenance. The accepted data contained 1 to 27 distinct
surface tuples per scenario. Current v1 Road retained one signed scalar and no
recoverable surface tuple, group, transition mask, or separate L/R identity.

This does not claim the policy is more physically accurate. It establishes that
the interpretation boundary preserves evidence v1 necessarily discards before
future presentation is chosen.

## Tests and output isolation

Deterministic fixtures cover all six R2-B scenarios, both established side
groups, uniform/partial/broad topology, transitions and re-entry, continuous
activity, unknown IDs, provenance, gear/collision/unknown exclusion, grip
isolation, stale/invalid/non-finite rejection and clean-state determinism.

Reference+ replay requires exact equality before and after policy evaluation
for Directional, Road, Impact, composition, tanh, pre-drive and final request.
A source guard requires this order:

```text
WheelForceFeedback::drive()
  -> Signal State update
  -> Road 2.0 passive policy
  -> STOP
```

The policy frame is fixed-size and trivially copyable. It performs no runtime
allocation, file I/O, formatting or offline analysis. A 200,000-update smoke
test runs in the normal regression suite.

## Open questions

- A session-local reference cannot identify the global meaning of a capture
  that starts away from a known control surface.
- Native effect amplitude is not yet separated into physical texture, chassis
  motion and other restored-game intent.
- The established 0/2 and 1/3 groups remain neutrally named because R2-B did
  not independently prove physical handedness.
- No evidence yet selects a torque carrier, spatial-to-wheel mapping, envelope,
  frequency, transition presentation, or player ceiling.

The first presentation prototype should remain passive and replay-only. It
should compare at least two bounded renderings—continuous envelope and spatial
imbalance—while keeping transition events separate and retaining current v1
Road as the fallback/reference channel.

## R4.2C evidence-gate closure

R4.2C later validated the continuous-candidate path against a controlled
cobblestone section. Its stable interval contained 438 consecutive uniform
surface samples with continuous bilateral native activity, zero event
contamination, and zero Candidate A activity on both normal-road controls.

That result approves only a future strictly bounded, symmetric Candidate A
prototype at conservative independent authority, subject to resolving how an
unsigned envelope can become zero-mean wheel output. R4.2W subsequently found
no validated native signed carrier and no AC-coupled derivation that the
available captures can distinguish from envelope/speed change. Active Road is
therefore paused. Physical handedness, spatial torque, waveform direction,
transition presentation and material-specific gain remain unresolved and
inactive. This policy remains passive; neither milestone changed a policy
equation or Force-output path. See
`HYP36R_FORCE_2_0_R4_2C_COBBLESTONE_ANALYSIS.md` and
`HYP36R_FORCE_2_0_R4_2W_ROAD_WAVEFORM.md`.

R4.2N later closed the mapped pre-motor lineage: the four raw surface states
select positive material coefficients through `sub_1149C0()`, the strongest
coefficient is scaled by native speed-like state, and other vehicle/event
branches join before unsigned Xbox motor output. This supports Road policy's
separation of categorical surface context from composite activity, but supplies
no signed wheel carrier. See
`HYP36R_FORCE_2_0_R4_2N_NATIVE_SURFACE_LINEAGE.md`.
