# HYP36R Force 2.0 R4.2W — Road waveform and direction research

R4.2W resolves a missing architectural question left by the cobblestone study:
Candidate A measures **how much** continuous surface activity exists, but it
does not say which direction a steering-wheel force should take. The accepted
R2-A, R2-B, R2-C and R4.2C captures were replayed offline. No runtime Force,
DirectInput, controller, telemetry-schema or player-setting path changed.

## Decision

**Selection result: MORE EMPIRICAL EVIDENCE REQUIRED.**

No mapped native value is presently supported as a signed road carrier. A
first-difference residual and a causal local-mean/high-pass residual can both
make Candidate A bipolar mathematically, but the available stable cobblestone
record does not establish that either residual represents road texture rather
than envelope and speed change. Neither is promoted to a versioned waveform
contract.

Road 2.0 therefore remains passive. R4.2A must not resume yet. Candidate A is
still the validated continuous-activity envelope and remains useful research
state; it is not valid ConstantForce input by itself.

## Native source lineage

The restored path begins in `CalcVibrationValues(EVWORK_CAR*)` in
`src/hooks_forcefeedback.cpp`. It is the restored Xbox routine identified in
source as originating at Xbox address `0x114C60`. The routine reads multiple
vehicle/event fields, builds two accumulated motor intents, uses absolute
value for at least one relative-motion term, applies positive additions and
caps, multiplies by the non-negative vibration multiplier, and stores the
results in `VibrationLeftMotor` and `VibrationRightMotor`.

Those floats are passed to `SetVibration`, clamped to `[0,1]`, scaled to the
Xbox `WORD` motor range and forwarded as left/right motor speeds. They are also
copied unchanged into `HYP36R_SIGNAL_STATE_V1` as `effectLeft` and
`effectRight`. Road policy retains their channel identity. Candidate A is the
first Road presentation stage that explicitly applies magnitude semantics:

```text
left  = clamp01(abs(effectLeft))
right = clamp01(abs(effectRight))
Candidate A = clamp01(sqrt((left^2 + right^2) / 2))
```

The `abs` calls in Candidate A do not discard a supported physical polarity.
The upstream restored values are controller-motor amplitudes by construction,
not signed steering effects. Their L/R identity is real and preserved, but it
does not establish steering-wheel direction or physical handedness. Temporal
phase suitable for signed wheel force is already absent at this interface.

Current v1 Road is separate: it takes the right motor magnitude, applies Road
gain and a synthetic 6 Hz sine carrier. That sine supplies direction, but its
frequency and phase do not come from the captured road evidence and therefore
cannot be adopted as a Reference+ Road 2.0 fact.

## Upstream signed-carrier search

The already mapped telemetry/native lineage was screened before considering a
derived carrier:

| Candidate source | Finding | Result |
| --- | --- | --- |
| Restored left/right motor values | Non-negative Xbox motor amplitudes with useful channel identity | Envelope evidence only |
| Native rise / combined effect | Magnitude/change evidence; not naturally bipolar | Not a signed carrier |
| Four surface fields | Categorical occupancy and transitions | No temporal polarity |
| Native `1D0`, `1D4`, `1DC`, `1E0`, `1E4`, `264`, `268` | Either inactive in the controlled window or vehicle-motion state; no validated road-specific zero-mean behavior | Rejected |
| D38–D48 steering-state family | Valuable native steering/handling response, but related to steering authority and vehicle dynamics | Prohibited for Road polarity |
| Candidate A channel contrast | L/R amplitude relationship, not proven physical handedness | Preserved as context only |

No native candidate met all requirements: continuous-surface correlation,
natural bipolarity, quiet normal-road behavior, independence from steering and
load, and a defensible physical lineage. This is a negative result, not proof
that the game contains no earlier signed surface signal; it means none exists
in the currently mapped and captured path.

## AC-coupled comparison

Two small, deterministic classes were evaluated offline against Candidate A.
No broad filter search was performed.

### A. First difference

```text
w[n] = envelope[n] - envelope[n-1]
```

This is bounded by the envelope bounds and has negligible long-term DC for a
bounded stable sequence. It is silent when the envelope is constant and adds
no fixed frequency. In the 438-sample stable cobblestone interval its median
absolute value was approximately `0.000033`, P95 `0.000247`, RMS `0.000732`,
with one boundary-scale maximum of `0.02138`. It therefore represented mostly
slow change and isolated edges, not sustained texture communication.

### B. Causal local-mean subtraction

The comparison used a simple causal exponential mean with a 250 ms research
time constant:

```text
mean[n] = mean[n-1] + alpha * (envelope[n] - mean[n-1])
w[n] = envelope[n] - mean[n]
```

It creates no periodic oscillator and its output is bounded when the envelope
is bounded. In stable cobblestone it remained small (mean about `0.000125`,
P95 absolute `0.00349`, maximum `0.00371`, RMS `0.00134`). Its low crossing
rate and shape primarily reflected removal of the slow envelope trend. Because
R4.2C already established envelope-to-speed correlation of essentially 1.0,
this cannot yet be interpreted as an independent road waveform. The 250 ms
value is a comparison parameter, not a selected product constant.

First difference is the cleaner mathematical zero-mean operation. Local-mean
subtraction is the leading *research* candidate because it retains more source
variation during sustained activity. Neither is an evidence-backed wheel
waveform, so no `HYP36R_ROAD_WAVEFORM_V1_SHADOW` contract was created.

## Controlled evidence results

### Cobblestone

Candidate A remained active on every sample of the 438-sample stable interval,
but its source channels held an almost exact 4:1 ratio and its envelope was
effectively proportional to speed. Both AC methods were bipolar and avoided a
persistent one-direction force in the mathematical sense. Neither revealed an
independent repeating structure or source-driven polarity that can presently
be attributed to cobblestone texture.

### Normal road

CST01 and CST03 remain the decisive controls. Road policy makes Candidate A
zero on both reference-surface captures, so both derived candidates are also
zero after reset. They do not manufacture normal-road oscillation.

### Rough and sand

B04 and B05 contain sustained Candidate A evidence. Both derived candidates
produce bipolar residuals without material-specific tuning. Their activity is
again tied to changes in the unsigned envelope; the data does not establish
that residual sign corresponds to physical surface direction. Generalization
of activity succeeds, but semantic validation of waveform direction does not.

### Gear and collision

R2-C C01 gear events are excluded by Road policy, leaving zero waveform input.
The classified collision frames in C02 are likewise excluded. C02 also
contains legitimate non-reference surface context outside those event frames;
that context must not be mislabeled as collision leakage. Any future stateful
waveform must reset or hold safely across exclusions so an event cannot create
filter ringing or trailing Road force.

### Grip and steering

The R2-A controls include steering, corner load, drift, release and recovery.
Where the established surface remains the reference, Candidate A and both
derived candidates remain zero. No steering sign, Directional sign, total
Force sign, centering sign, yaw proxy or spatial group sign was used.

## Envelope and waveform roles

The evidence supports this separation:

```text
Candidate A = activity/envelope authority (unsigned)
waveform    = unresolved zero-mean temporal carrier
```

A future carrier must not be multiplied blindly by Candidate A if it already
contains envelope amplitude; doing so could square the activity and distort
surface relationships. Normalization and authority must be specified only
after the carrier lineage is validated.

## Periodic effects, tactile output and motion

A DirectInput periodic effect is not evidence-backed at this stage. Selecting
frequency, phase and waveform shape would manufacture temporal behavior that
R4.2C did not measure. Future arcade-hardware evidence may justify revisiting
that conclusion, but the PC evidence does not.

Candidate A may already be a useful amplitude/control signal for a future bass
shaker path because tactile hardware does not require it to become signed
steering authority. No shaker output is implemented here.

Road waveform is not a motion-platform signal. A future motion policy should
consume the underlying validated vehicle, surface and event states directly,
with its own semantics and safety limits.

## AER implication

The PC lineage provides a useful future comparison point: the restored Xbox
endpoint exposes unsigned motor amplitudes, and no mapped PC-side precursor is
currently validated as signed road direction. An arcade drive-board study can
ask whether original hardware received a signed temporal command or generated
one locally. R4.2W makes no claim about arcade behavior.

## Regression and implementation boundary

No passive runtime component was added because the evidence does not support a
versioned waveform contract. The public 222-column telemetry schema is
unchanged. Existing replay tests continue to require exact Reference+ equality
through Directional, Road, Impact, composition, tanh, pre-drive and final
DirectInput request. Existing passive Road policy/presentation remains
fixed-size, allocation-free and covered by its 200,000-update smoke test.

If a future passive waveform is introduced, observability should identify its
input envelope, baseline/DC estimate, signed residual, reset/exclusion state
and selected normalization without changing hardware output during validation.

## Next gate

Keep active Road paused. The next useful step is one narrowly scoped evidence
campaign capable of separating speed trend from texture timing—for example a
longer constant-speed surface pass with matched normal control, while capturing
an earlier pre-motor native source if static lineage identifies one. The gate
is a repeatable bipolar source component that is quiet on normal road and
isolated from gear, collision and steering/load behavior. Until then, there is
no defensible ConstantForce waveform for Road 2.0.

## R4.2N lineage closure

R4.2N subsequently traced all four surface states through `sub_1149C0()` into
a positive material coefficient, followed by native speed-like scaling and
composite Xbox motor presentation. No signed surface waveform exists in that
mapped path. The R4.2W active-Road decision is unchanged; see
`HYP36R_FORCE_2_0_R4_2N_NATIVE_SURFACE_LINEAGE.md`.
