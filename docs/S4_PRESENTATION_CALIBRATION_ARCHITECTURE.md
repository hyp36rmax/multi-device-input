# S4 HYP36R Presentation Calibration Architecture

S4 defines a future presentation layer. It does not activate amplification,
change Force, alter DirectInput, implement protection, create device profiles,
or authorize physical testing.

The frozen references are:

- Force 2.0 Foundation: `42118c029340db95e5f77539b8696bc6b824a08d`
- Force 2.1 Research Reference: `8c5d68d92b19a21a09b3840413b8a453520517cc`
- S2 passive telemetry: `df5cd28867dec44d8fa29685d4dea64e853d55af`
- S3 offline study: `S3_PASSIVE_INFORMATION_AMPLIFICATION_HEADROOM_STUDY.md`

## A. S3 findings

The two representative Dino captures contain substantial normalized software
headroom. S3 describes 1.00–1.30x as observed software comfort territory and
1.40–1.60x as headroom-aware territory. Above that, pre-tanh scaling
increasingly compresses strong information while post-tanh scaling consumes
transient reserve more directly. M5 post-tanh replay begins clipping at 2.00x.

These observations are feasibility constraints, not defaults, user settings,
hardware-safe limits, or perceptual recommendations. The most important S3
finding is that global amplification preserves M5 proportionally but does not
make its small distinction inherently meaningful or perceptible. Global
presence and information contrast are therefore separate concepts.

## B. Presentation-layer responsibilities

Presentation calibration sits between the validated Force model and the future
safety/headroom envelope:

```text
vehicle information
    -> Force model
    -> presentation calibration
    -> safety / headroom envelope
    -> device calibration
    -> existing output conditioning
    -> hardware
```

Presentation owns how clearly existing information is communicated. It may
shape overall presence, validated contrast, and dynamic expression. It may not
create vehicle state or redefine LOAD, RELEASE, FREE, BITE, restoration,
native validity, M5 context, front/rear meaning, or vehicle direction.

Its output is a desired normalized communication request. Presentation does
not decide whether that request is safe for software or hardware. The future
safety layer decides what normalized request can be delivered, and device
calibration later maps that safe request to a verified device response.

## C. Global-presence architecture

Global Presence controls the strength of the complete validated HYP36R Force
presentation. It acts on the composed information model without changing
component meaning, sign, phase, ordering, eligibility, or authority.

A future interface should be conceptually explicit:

```text
forceModelComposite
    -> applyGlobalPresence(profile, userStrengthIntent)
    -> globalPresentationRequest
```

The interface must expose both input and request. Its implementation must not
be defined as an undocumented `ffb_raw * gain`. S3 shows why placement matters:
pre-tanh gain compresses strong information; post-tanh gain consumes boundary
reserve. The presentation request must remain visible before any safety or
device mapping changes it.

Global Presence should be smooth, monotonic, sign-preserving, finite, and
deterministic. Zero Force-model input must remain zero. It may not turn an
ineligible or invalid channel into output.

## D. Information-contrast architecture

Information Contrast controls the distinction of validated secondary
information relative to the primary M4 foundation. It is a bounded delta, not
a second master gain:

```text
primary M4 presentation
    + eligible confidence-weighted secondary contrast delta
    = presentation-composed request
```

The contrast path receives the original validated secondary signal plus its
authority metadata: confidence, activity, phase eligibility, contamination,
and governing M4 state. It returns a requested delta and a reason. It cannot
operate from the final aggregate alone because the source, confidence, and
governing context would already be lost.

Candidate future approaches are:

| Approach | Benefit | Primary risk | S4 assessment |
| --- | --- | --- | --- |
| Linear secondary gain | Predictable and easy to replay | Amplifies noise and remains insensitive to context | Insufficient alone |
| Response-curve shaping | Expands meaningful midrange distinctions while preserving zero | Can exaggerate or flatten ordering if poorly formed | Viable with monotonic bounds |
| Event-local contrast expansion | Targets validated M5 activity rather than the full drive | Can create gain steps or manufactured events | Viable only with smooth entry/release |
| Confidence-weighted scaling | Suppresses low-confidence and contaminated state | Confidence instability can modulate output | Required input, not a complete method |
| Bounded secondary budget | Guarantees subordination and reserve visibility | A fixed budget may be wrong across contexts | Required architectural boundary; values remain unknown |

The preferred future concept combines a monotonic response curve,
confidence/eligibility weighting, and a bounded secondary budget. It must not
amplify missing, invalid, low-confidence, contaminated, or phase-ineligible
information.

## E. M4 and M5 hierarchy

M4 remains authoritative. Presentation must preserve its directional sign,
grip-envelope phase, RELEASE, FREE, BITE, and restoration progression.

M5 remains a subordinate characterization delta. It may distinguish an
eligible M4 request, but it may not:

- create Force when the governing M4 value is zero;
- reverse the governing M4 sign;
- exceed the governing M4 magnitude boundary;
- survive an M4 FREE or BITE veto that currently suppresses it;
- change M4 state transitions or thresholds;
- become an independent steering source.

Cross-car validation remains required before M5 can become a production
default. Presentation cannot compensate for incomplete M5 normalization.

## F. Presentation-budget model

The presentation budget is a normalized, device-independent limit on how much
secondary information may modify the primary request. S4 assigns no values.

Conceptually, the allowed secondary delta is the minimum of:

1. the contrast delta requested from validated information;
2. the M4 subordination bound;
3. the presentation profile's secondary-information budget;
4. the normalized presentation ceiling defined by the layer contract.

The presentation layer should report its requested delta before safety. The
safety layer remains authoritative over deliverable normalized headroom and
may grant less when dynamic reserve is unavailable. Presentation must never
silently borrow impact or detail reserve, bypass the envelope, or interpret
unused range as hardware permission.

The budget is not a fixed gain. It is a maximum modification allowance with
observable input, request, granted amount, and limiting reason.

## G. Noise and confidence handling

Contrast eligibility should require all relevant validation gates to pass:

- the secondary signal is finite and currently valid;
- activity exceeds its established information floor or deadband;
- confidence meets the future validated requirement;
- phase permits the secondary interpretation;
- surface contamination does not invalidate it;
- the governing M4 state permits a subordinate delta.

Below the information floor, output should remain at or near zero rather than
expanding noise. Eligibility transitions must be smoothed without inventing a
new force event. Confidence can reduce an otherwise valid request; it cannot
increase semantic authority. Missing confidence data must fail back to the
unchanged Reference behavior.

S4 does not define thresholds, curve coefficients, hysteresis, or transition
times. Those require passive replay evidence before implementation.

## H. Pre-tanh and post-tanh placement

Global Presence and Information Contrast should not be forced into one
mathematical location.

### Global Presence

The preferred architectural placement is after the Force channels have been
composed semantically but before the safety/headroom envelope. Its exact
nonlinear transfer remains a calibration decision. A pure pre-tanh scale is
not preferred because strong information progressively loses contrast. A pure
post-tanh scale is not sufficient because it consumes transient reserve
without channel awareness.

The future global transfer should be an explicit, replayable presentation
curve with a known input/output relationship. It may use the existing tanh as
part of output conditioning, but it must not hide presentation policy inside
that conditioner.

### Information Contrast

Contrast belongs before final aggregate nonlinearity and before safety, while
channel identity and confidence are still available. The future composer
should expose the primary request and secondary delta separately. This allows
the safety layer to preserve or reduce them intentionally instead of applying
unknown compression to an opaque total.

### Ordering

```text
M4 primary request -------------------------------+
                                                   |
M5 validated state -> contrast curve -> budget ----+-> presentation composite
road detail request -------------------------------+            |
impact transient request --------------------------+            v
                                                    safety/headroom envelope
```

This diagram shows visibility, not an allocation policy. S4 does not change
the current composer or select budgets.

## I. Dynamic-range preservation

Presentation should preserve the relationships that carry meaning:

- small road information should not disappear under sustained directional
  load merely because global presence increased;
- BITE progression and returned load must remain monotonic and recognizable;
- directional-load differences must retain sign and ordering;
- impacts remain bounded transients rather than a new sustained baseline;
- M5 distinctions remain subordinate but observable when eligible.

The future layer should avoid early aggregation when channel identity is still
needed for preservation. Any nonlinear curve must be tested for local response
retention, not only final clipping. Headroom evaluation must examine both
sustained load and a transient arriving on top of that load.

## J. Channel hierarchy

Priority describes information preservation, not unconditional amplitude:

1. **Primary vehicle information:** M4 directional/grip envelope and its state
   progression remain authoritative.
2. **Secondary validated information:** M5 context may distinguish the primary
   request within eligibility and budget bounds.
3. **Transient information:** impact may request reserved short-duration
   expression but cannot bypass normalized limits.
4. **Texture information:** road and tire vibration retain a small detail path
   rather than being erased by aggregate compression.

The future safety layer should know the origin of each request. It may reduce
presentation pressure, but it must not reorder semantics, turn a subordinate
channel into authority, or normalize repeated boundary contact.

## K. Reference and Arcade profile support

Profiles share one vehicle-information pipeline and one Force model:

```text
same observations -> same HYP36R Force semantics -> selected presentation
```

**Reference** is the validated unchanged presentation and mandatory fallback.
It preserves current behavior when no other profile has sufficient evidence.

**Arcade** is a future presentation character. AER may eventually inform its
contrast, response, and dynamic expression after direct validation. Arcade may
not replace native semantics, modify M4/M5 detection, or rely on an
unvalidated third-party interpreted model.

A profile contains presentation policy only. It does not duplicate Force
equations or device calibration and cannot bypass safety.

## L. Force Strength architecture

The normal user should see one simple `Force Strength` control. Conceptually it
expresses desired overall presence, not vehicle physics or device torque.

Internally, the request is divided by responsibility:

- presentation interprets the user's desired normalized presence while
  preserving information relationships;
- the safety envelope constrains the resulting normalized request;
- device calibration maps the safe normalized result to a verified device;
- existing output conditioning submits the final request.

The UI should not expose two strength sliders for presentation and hardware.
If a device requires explicit calibration, that belongs in optional advanced
setup and must not silently rewrite the profile. Force Strength must not alter
M4/M5 thresholds, confidence, vehicle character, or detection behavior.

The current master-strength behavior remains unchanged in S4. Future work must
define migration and compatibility before moving any responsibility.

## M. Device-calibration boundary

Presentation remains normalized and device-independent. It cannot assume that
a DD2 percentage, belt-wheel response, or DirectInput magnitude corresponds to
a universal physical torque.

Device calibration receives a safe normalized request and maps it using
verified device-class or device-specific evidence. It owns physical capability,
usable response range, and conservative unknown-device behavior. It does not
own Force semantics, profile character, or M5 contrast.

No device profile or hardware threshold is defined in S4.

## N. Perceptual-calibration methodology

A future calibration study should identify three distinct human boundaries:

1. **Perceptual floor:** the lowest level where a validated distinction is
   consistently detected above noise.
2. **Preferred range:** the interval where the distinction is clear, natural,
   and useful without masking other information.
3. **Manufactured-effect boundary:** the point where the presentation feels
   exaggerated, disconnected, or event-generating rather than informative.

The method should use controlled A/B trials with the same car, route, device,
driver settings, and labeled event windows. Global Presence and Information
Contrast must be varied independently. Trials should include zero-information
controls so participants cannot reward amplified noise, and repeated blinded
orders to reduce expectation bias.

Telemetry should confirm eligibility, requested contrast, granted contrast,
headroom, and final output for every rated event. Results must be separated by
device class and then cross-checked across cars before production decisions.
S4 assigns no test values and authorizes no physical UAT.

## O. Fail-safe model

Any missing, invalid, nonfinite, unsupported, or version-incompatible
presentation configuration falls back to the unchanged Reference path.

| Failure | Required behavior |
| --- | --- |
| Missing profile | Use Reference; report fallback reason |
| Unknown profile | Reject it; use Reference |
| Invalid/nonfinite parameter | Reject the affected profile; use Reference |
| Unsupported profile version | Use Reference; report incompatibility |
| Invalid secondary state | Request zero secondary contrast; preserve M4 |
| Presentation computation becomes nonfinite | Reject the sample's presentation change and preserve a finite Reference request |

Presentation failure may not disable M4, enable a secondary channel, retain a
stale delta, or bypass existing zero-force device/focus protections.

## P. Future telemetry requirements

Future implementation must use one authoritative value for each stage:

| Field | Meaning |
| --- | --- |
| `force_model_primary` | Exact M4 primary request before presentation |
| `force_model_secondary` | Exact eligible secondary information before contrast |
| `force_model_impact` | Impact request before presentation/safety |
| `force_model_detail` | Road/detail request before presentation/safety |
| `presentation_profile` | Selected Reference/Arcade profile |
| `global_presence_request` | Requested global presentation parameter/result |
| `contrast_eligibility` | Whether secondary contrast is allowed |
| `contrast_confidence` | Confidence used by presentation |
| `contrast_requested_delta` | Delta requested before budget/safety |
| `contrast_budget_limit` | Current presentation-budget limit |
| `contrast_budgeted_delta` | Delta after the presentation budget |
| `presentation_composed_output` | Exact request sent to safety |
| `safety_envelope_output` | Exact normalized output granted by safety |
| `safety_intervention_reason` | None, transient reserve, sustained exposure, boundary, or fault |
| `device_calibrated_output` | Exact request after device mapping |
| `final_directinput_request` | Final submitted DirectInput magnitude |

The schema should also record curve/profile versions, units, windows, and
fallback state in capture metadata. A field must describe the value that
actually reaches the next layer; shadow and hardware-selected values must not
share ambiguous names.

S4 adds none of these fields.

## Q. UX implications

Normal setup should remain concise:

```text
Force Profile:  Reference | Arcade
Force Strength: simple normalized control
Advanced:       optional calibration and diagnostics
```

Reference remains available and is the fallback. Arcade remains unavailable
until validated. Normal users should not see tanh scale, contrast budget, M5
gain, RMS, occupancy, limiter state, or headroom calculations.

Diagnostics may explain that a profile fell back or a device is using generic
calibration, but the user should not need to edit an INI file to recover valid
Force.

## R. Evidence boundaries

S4 is architecture based on S1, S2, and two S3 Dino captures. It does not
establish:

- a production gain or default;
- a perceptual floor, preferred range, or manufactured-effect boundary;
- an M5 contrast curve, budget, threshold, or cross-car normalization;
- an Arcade presentation or validated AER dataset;
- a safety limiter or channel allocation algorithm;
- a device profile, physical torque mapping, or hardware-safe limit;
- that S3 covers every car, route, surface, impact, frame rate, device, driver,
  firmware, or user.

The approximate 1.30x software comfort observation must not appear as a user
setting or default. Presentation cannot turn incomplete cross-car evidence into
production authority.

## S. Recommended S5 milestone

**S5 — Passive Presentation Request and Contrast Replay Model.**

S5 should remain offline/passive. It should specify versioned Reference
identity behavior and evaluate candidate monotonic contrast curves against the
existing S2 captures, with explicit M4 subordination, confidence gates,
secondary budgets, transient reserve, and zero-information controls.

Acceptance should require:

- Reference reproduces the current path exactly;
- zero or ineligible secondary information produces zero contrast delta;
- M4 sign, magnitude authority, FREE, BITE, and restoration remain unchanged;
- candidate contrast preserves ordering and cannot create output from zero;
- all requested and budgeted values are replay-observable;
- no production constants, runtime amplification, limiter, device profile, or
  physical test is introduced.

Only after a passive candidate survives replay should a separate milestone
design runtime telemetry or bounded UAT.

## T. Documentation changes

S4 adds this architecture document and preserves the S3 replay study as its
evidence base. No source, configuration, telemetry schema, Force equation,
DirectInput path, build input, or hardware setting changes are part of S4.
