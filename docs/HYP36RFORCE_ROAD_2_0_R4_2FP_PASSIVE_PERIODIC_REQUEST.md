# HYP36Rforce Road 2.0 R4.2F-P — passive periodic request

**PART I — ENGINEERING RECORD**

## Scope and decision

R4.2F-P implements `HYP36R_ROAD_PERIODIC_V1_SHADOW` as an offline/passive
research contract. It terminates before Force composition and DirectInput.
Current Road and all hardware-selected output remain authoritative.

**Timing decision: C — BOTH REMAIN PLAUSIBLE; MORE EVIDENCE REQUIRED.**

**Active-Road gate: ACTIVE ROAD REMAINS PAUSED.**

No UAT or physical test is requested. This milestone is Road 2.0 only; other
lanes remain **CROSS-LANE CANDIDATES — NOT INTEGRATED**.

## Contract and inputs

The fixed-size `HYP36R_ROAD_PERIODIC_V1_SHADOW` contract consumes the existing
passive Candidate A presentation plus explicit research timing:

- Road-policy validity, confidence, exclusions, and Candidate A authority;
- session and frame identity;
- valid frame delta time; and
- non-negative vehicle-speed magnitude.

It does not read game memory, steering sign, Directional sign, total-force
sign, surface-group sign, Impact sign, or external implementation state.

The output preserves availability/validity, reset reason, provenance,
confidence, source/conditioned/proposed amplitude, speed, valid delta time,
accumulated distance, time and distance phase, both zero-mean requests,
envelope state, pre/post-bound requests, and bound activity.

## Research representation

Both models use a mathematically transparent sine solely to test phase/rate
topology. It is **SYNTHETIC RESEARCH PRESENTATION STATE**, not recovered OutRun
physics, product tuning, or external-source behavior.

- Time domain advances one normalized cycle per second.
- Distance domain advances one normalized cycle per captured speed-distance
  unit.

Both normalizations are explicitly **NOT PHYSICAL / NOT PRODUCT PARAMETERS**.
They make deterministic comparison possible without claiming Hz or wavelength.

Candidate A remains source amplitude. A single bounded 50 ms research envelope
compares minimally conditioned authority while preserving source amplitude
separately. This constant prevents boundary spikes in the topology test; it is
not selected feel or final attack/release tuning. Road inactivity fails closed
immediately, so the envelope cannot leak onto normal road.

## Timing, speed, and reset behavior

Distance accumulation is `max(speed, 0) * valid_dt`. Distance phase stops at
zero speed; time phase continues only while Road remains authorized. Neither
model invents low-speed amplitude.

Zero, negative, non-finite, or greater-than-100-ms delta time fails closed.
Session start/change, unavailable source, non-finite amplitude, Road
inactivity, invalid timing, and timing discontinuity reset amplitude, phase,
distance, and request to zero. Absolute phase has no native meaning.

Surface transitions never create an amplitude or phase kick. Gear,
collision-candidate, and unknown-event exclusions are inherited from Road
Policy. Reference-surface steering and grip state cannot authorize the model.

## Passive bounds and performance

Source, conditioned, and proposed amplitude are bounded to `[0,1]`; requests
are independently bounded to `[-1,1]`. Pre-bound and post-bound values plus
bound flags remain separate. No current-Road `2.00x` ceiling is inherited.

The contract is trivially copyable, allocation-free, has no per-frame strings
or file I/O, and passes a 200,000-update deterministic smoke test. The runtime
Force hook neither includes nor evaluates it.

## Accepted-capture replay

`research/road2_periodic_replay.py` replayed the accepted R4.2C, R2-B and R2-C
captures. Its event gate mirrors Signal State's gear/rise classification. R2-A
was also screened: every periodic request on a stable reference surface was
zero; activity found elsewhere coincided with recorded non-reference surface
context rather than steering or grip state alone.

| Scenario | Activity coverage | Time mean / RMS | Distance mean / RMS | Zero crossings time / distance | Normal/event false positives |
| --- | ---: | ---: | ---: | ---: | ---: |
| CST01 normal control | 0.0% | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 |
| CST02 cobblestone | 52.0% | .000371 / .014725 | .000215 / .014939 | 15 / 3 | 0 / 0 |
| CST03 normal return | 0.0% | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 |
| B02 striped partial | 29.8% | .004763 / .026076 | .003356 / .024563 | 4 / 3 | 0 / 0 |
| B03 striped broad | 16.2% | .005815 / .023029 | .005510 / .023578 | 2 / 2 | 0 / 0 |
| B04 rough/sand partial | 87.3% | .001892 / .041064 | .004662 / .040149 | 30 / 15 | 0 / 0 |
| B05 rough/sand broad | 43.2% | .018167 / .072632 | .016432 / .072065 | 10 / 10 | 0 / 0 |
| B06 re-entry | 45.7% | .001683 / .043932 | .003740 / .044823 | 15 / 8 | 0 / 0 |
| C01 gear | 0.0% | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 |

C02 contains legitimate non-reference surface activity outside its excluded
collision frames, matching the earlier R2-C finding; collision candidates
themselves created no periodic Road request. Bound occupancy was zero in every
scenario. Repeated replay produced identical deterministic digests.

The small nonzero finite-window means occur in short/interrupted active windows
whose phase starts from zero; neither model creates a persistent one-direction
request in long sustained deterministic testing. A 6,000-frame constant
activity test produced absolute mean below `1e-4` for both models.

## Time-versus-distance result

Both models satisfy the passive safety and ownership gates. Distance phase has
the stronger physical topology because it stops with the car and responds to
travel. Time phase is the simpler frame-rate-independent control. The accepted
captures do not provide a physical wavelength or another ground truth that can
judge their different crossing counts.

Selecting distance merely because it appears physically plausible would exceed
the evidence. Selecting time would silently make texture rate independent of
travel. Decision C is therefore required.

## Current-Force equivalence

Deterministic tests prove identical Directional, current Road, Impact,
composition, `tanh`, and pre-drive values before and after passive evaluation.
Source inspection asserts that `hooks_forcefeedback.cpp` has no periodic-shadow
include or call. No DirectInput ConstantForce, periodic, spring, damper, or
other hardware effect is created.

The public 222-column telemetry schema is unchanged. A future runtime-shadow
proposal would need a separately approved, versioned observability extension;
none is deployed here.

## Unresolved questions

- No physical spatial wavelength or fixed frequency is established.
- No evidence selects time or distance timing conclusively.
- The research envelope is not product attack/release tuning.
- Cross-device periodic capability and perceptibility remain untested.
- A later authority budget requires physical validation independent of current
  Road/Force Character ceilings.
- A wheel-specific retail force path could provide timing evidence, but belongs
  to R5/AER and is not integrated here.

## Credits & Reference Context

GATS helped surface experiential Road-perception questions. THP32 supplied an
external example of alternate wheel-effect topology. Both remain reference
context only. **EXTERNAL REFERENCE — NOT INTEGRATED.** No external code,
equation, constant, rate, phase, direction, threshold, table, offset, tuning,
or effect parameter appears in this model.

## Next recommendation

Keep active Road paused. The next Road 2.0 step should be one narrowly scoped
independent timing-evidence study capable of supplying or rejecting a spatial
rate reference without wheel output. Until then, retain both passive models as
research comparisons and current Road as the product authority.

## Part II — Development Journey Recap

### What were we trying to understand?

R4.2F identified periodic presentation as the best topology to study, but it
did not know how fast that waveform should move. R4.2F-P built the smallest
passive model needed to compare time-based and distance-based phase safely.

### What did we test?

We replayed normal road, cobblestone, rough/sand, partial and broad runoff,
re-entry, gear, collision, and stable-surface steering/grip evidence. We also
tested bad timing, state loss, session changes, zero speed, bounds,
determinism, and exact current-Force equivalence.

### What did we find?

Both models can remain bounded, zero-mean, quiet on normal road, isolated from
events and steering, and deterministic. Distance timing stops with the car;
time timing provides a useful independent control. Their different crossing
rates cannot be judged without a real spatial or temporal reference.

### What did we deliberately not conclude?

We did not select a product frequency, wavelength, waveform, phase, amplitude,
attack/release, device fallback, or Road 2.0 wheel effect. The normalized rates
exist only to exercise architecture.

### What happens next?

Active Road stays paused. The project needs one independent source of timing
meaning before either passive model can advance toward a conservative hardware
hypothesis.
