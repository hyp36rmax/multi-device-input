# Presentation and safety

HYP36R separates the information in the force model from how strongly and
clearly that information is presented. This avoids using gain to compensate
for uncertain vehicle semantics and avoids treating available software range
as proof of hardware safety.

```text
observed vehicle state
        -> force information
        -> presentation
        -> software conditioning
        -> device calibration
        -> hardware
```

## Force information

M4 determines directional authority through LOAD, RELEASE, FREE, BITE, and
returned load. Eligible M5 state may characterize that existing M4 directional
value. These layers decide what information belongs in the force request.
Presentation may amplify or clarify their result, but it may not create a
steering direction, bypass a BITE/FREE veto, reinterpret native state, or make
an invalid contribution eligible.

## Global Presence and Information Contrast

**Global Presence** scales the complete selected directional request. It changes
how present the steering load feels without changing M4/M5 semantics.

**Information Contrast** scales only an eligible M5 delta before its policy
boundaries. It changes the separation of secondary information from the M4
primary. Contrast cannot supply direction and does nothing when M5 is
ineligible.

Keeping the controls separate lets replay distinguish "the whole directional
signal is too quiet" from "the secondary information is hard to notice."

## Secondary authority

Reference+ applies secondary information in this order:

```text
eligible M5 delta * Contrast
        -> five-percent linear budget relative to abs(M4 primary)
        -> no force from zero
        -> preserve M4 sign
        -> remain inside Legacy directional authority
        -> add to M4 primary
        -> apply Presence
```

The five-percent rule limits what M5 can contribute before Presence. The Legacy
boundary limits the secondary contribution; it does not reduce an already-
authoritative M4 primary merely because the Legacy comparison is smaller on a
particular frame.

S7 replayed deadband, smoothstep, floor-plus-linear, and confidence-shaped
near-zero alternatives. The evidence did not show a practical near-zero
failure in the linear rule. The simpler policy was retained rather than adding
an equation without an observed problem.

## Current modes

### Reference

Reference is the validated permanent comparison and fail-safe:

- M4-only directional selection;
- Presence 1.00;
- no M5 secondary presentation;
- unchanged road, impact, vibration, and output conditioning.

Missing, invalid, or nonfinite presentation state falls back to Reference.

### Reference+ experimental

Reference+ is the current validated experimental foundation:

- M4 primary plus eligible, policy-permitted M5 secondary;
- Presence 1.20;
- Contrast 4;
- five-percent linear primary-relative secondary budget;
- Legacy directional authority retained;
- road, impact, and vibration expressed exactly as Reference.

Physical S9 testing found more perceptible steering presence while natural
behavior was retained. It felt like a useful foundation, and additional
intensity appeared possible. That observation is not approval for another gain
increase. Final intensity remains deferred.

## Software headroom

S2/S3 measure normalized software output before hardware interpretation.
Reference+ physical telemetry recorded 19,931 frames with instantaneous
magnitude P95 `0.378874`, P99 `0.468083`, and maximum `0.588180`. One-second
RMS reached P95 `0.327202` and maximum `0.446214`; three-second RMS reached P95
`0.279331` and maximum `0.356084`. Three-second occupancy at 0.75, 0.90, and
0.98 was zero.

The S3 replay described approximately 1.00–1.30 as observed software comfort
territory and 1.40–1.60 as headroom-aware territory. These ranges describe the
captured normalized signal. They are not recommended wheel gains, torque
limits, or certification.

## Hardware safety is a separate problem

A normalized request does not reveal motor torque, driver filtering, current,
temperature, clipping inside the wheel, fatigue, or protection behavior. One
Fanatec DD2 entered a protection or power-shutdown condition during earlier
high-output testing. That observation is important and remains unresolved, but
it is evidence about one device/setup, not a universal software threshold.

The current project therefore does not claim a physical hardware safety
envelope. Direct-drive users should begin with conservative hardware torque and
game strength. Device calibration must eventually map normalized output to
known hardware behavior without changing the vehicle-information model.

Future safety work should consider transient peak, sustained RMS and occupancy,
output slew, clipping/compression, driver rejection, focus and device loss,
thermal or protection behavior, and a conservative return-to-zero path. It
must be validated per meaningful device class before becoming a product claim.

## Calibration boundary

Hardware strength and in-game strength are user/device controls. They do not
validate M4 or M5 semantics. A future calibration layer may provide safe
starting points or device-aware mapping, but it should remain downstream of
Reference/Reference+ and should never disguise an information-model change as
a wheel preset.

AER, the future Arcade Experience Reference, belongs to presentation research.
Validated original arcade behavior may inform presence, contrast, texture, or
impact character. It may not redefine native state, the grip envelope, or
driver-versus-vehicle convergence. No Arcade profile is currently implemented.

The detailed S-series studies remain available as historical research records.
Current mode behavior and force routing are summarized in
[HYP36R_FORCE.md](HYP36R_FORCE.md); future work is tracked in
[ROADMAP.md](ROADMAP.md).
