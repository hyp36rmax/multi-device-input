# S7 near-zero secondary-budget study

S7 asks whether the S6 primary-relative budget needs special behavior as M4
directional authority approaches zero. This is offline policy analysis only.
No source, Force, presentation, telemetry, build, or hardware behavior changes.

Baseline `be82dd811f887616c31c5d141b2d764d305e942d` passed Windows CI in run
73.

## Data

The study reused the S2 M5, M5J active, and M5I shadow captures. The first step
was measuring eligible primary magnitude before selecting curve thresholds.

| Capture | Eligible samples | Minimum | P05 | Median | Maximum | Samples <=.05 | Samples <=.10 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| S2 M5 | 243 | .0736 | .1396 | .2120 | .4519 | 0 | 3 (1.23%) |
| M5J | 450 | .0150 | .1647 | .2589 | .4751 | 3 (.67%) | 7 (1.56%) |
| M5I | 441 | .0775 | .1553 | .2292 | .4641 | 0 | 2 (.45%) |

Eligible M5 information does not normally occur near zero primary authority.
Only M5J contains eligible samples below .05, and only one is below .02. The
existing M5 activity, confidence, phase, contamination, FREE, and BITE gates
already exclude almost all near-zero transitions.

## Curves compared

All candidates retain S6 ordering: M5 eligibility, secondary budget, sign
preservation, and Legacy authority. Two, five, and ten percent remain research
budget probes; no production fraction is selected.

**Linear reference**

```text
budget = fraction * abs(primary)
```

**Deadband with activation transition**

```text
primary <= .05: weight = 0
.05 < primary < .10: weight = (primary - .05) / .05
primary >= .10: weight = 1
budget = fraction * abs(primary) * weight
```

**Smoothstep taper**

```text
x = clamp(abs(primary) / .10, 0, 1)
weight = x² * (3 - 2x)
budget = fraction * abs(primary) * weight
```

**Quadratic taper**

```text
x = clamp(abs(primary) / .10, 0, 1)
weight = x²
budget = fraction * abs(primary) * weight
```

The .05/.10 interval is a research probe chosen after observing the lower tail,
not a proposed threshold. All four curves are nonnegative, monotonic, equal
zero at zero, and never exceed the linear primary-relative budget.

## Replay results

The strongest useful discriminator is the M5J stress case: Contrast 8 with a
five-percent research budget. S2 M5 and M5I have no eligible samples low enough
for any curve to differ at that point.

| Curve | M5J samples affected | Fully suppressed | Median permitted | P95 permitted | Maximum permitted | Legacy interventions |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Linear | 0 | 0 | .001657 | .007421 | .018963 | 2 |
| Deadband | 3 (.67%) | 3 | .001657 | .007421 | .018963 | 2 |
| Smoothstep | 1 (.22%) | 0 | .001657 | .007421 | .018963 | 2 |
| Quadratic | 2 (.44%) | 0 | .001657 | .007421 | .018963 | 2 |

Whole-capture permitted percentiles remain identical because the affected
events are both rare and small. The Legacy interventions are the same two S5
boundary cases already resolved by S6 and are unrelated to near-zero behavior.

At the tighter two-percent research probe, Contrast 8 causes 33 ordinary
linear budget interventions in M5J. Deadband affects three additional samples,
smoothstep two additional samples, and quadratic three additional samples.
This still does not reveal a near-zero failure; it only shows that the added
curves impose extra restrictions.

No curve creates force from zero, reverses primary sign, exceeds its selected
primary-relative budget, or bypasses Legacy authority. Contrast 0 remains
identity behavior.

## Rare-event trace

The only M5J eligible samples below .05 are:

| Time | Primary | Raw M5 delta | Confidence | Lateral activity |
| ---: | ---: | ---: | ---: | ---: |
| 31.381 s | .02195 | -.0000293 | .937 | .799 |
| 31.398 s | .04551 | -.0001009 | .918 | .782 |
| 79.632 s | -.01499 | .0000019 | .232 | .816 |

The first two are high-confidence validated RELEASE samples. The deadband
removes them solely because primary is small; the current evidence does not
show they are noise. At Contrast 8 and the five-percent probe:

| Primary | Requested secondary | Linear | Deadband | Smoothstep | Quadratic |
| ---: | ---: | ---: | ---: | ---: | ---: |
| .02195 | -.0002344 | -.0002344 | 0 | -.0001354 | -.0000529 |
| .04551 | -.0008072 | -.0008072 | 0 | -.0008072 | -.0004712 |
| -.01499 | .0000152 | .0000152 | 0 | .0000152 | .0000152 |

The lowest event is already negligible under linear policy. A hard activation
floor suppresses valid information but does not solve an observed output
problem. The tapers either leave the tiny request unchanged or reduce another
already-small request without a measurable aggregate benefit.

The lowest event also occurs naturally. Over the preceding frames, primary
moves through `.0672`, `.0501`, `.0264`, and `.00575` while the authoritative
M5 delta remains exactly zero. M5 becomes eligible only after primary reaches
`-.01499`, and the raw delta is just `.0000019`. Existing M5 gating, not an
extra budget curve, controls emergence.

## Decision

**Keep the linear primary-relative budget.**

The near-zero concern is theoretical in the available captures, not a
practical failure. Linear policy already:

- reaches zero continuously with primary authority;
- cannot create secondary force from zero;
- preserves strict proportionality and monotonicity;
- retains validated low-primary information;
- requires no new activation thresholds;
- works with the independent Legacy authority boundary.

The deadband is rejected because it discards three validated samples and adds
an unsupported threshold. Smoothstep and quadratic tapering are rejected for
now because they add policy and tuning surface while affecting at most two
stress samples and producing no demonstrated continuity, noise, or exposure
benefit.

This decision can be revisited if broader captures show eligible M5 noise,
chatter, or perceptible switching near zero. The taper candidates remain
mathematically valid fallback options; current evidence simply does not
justify them.

## Remaining uncertainty and S8

S7 cannot determine a human perceptual floor. It also cannot establish whether
the three low-primary deltas are useful to a driver, merely undetectable, or
undesirable. Choosing a curve because it produces a larger or smaller number
would overstate the evidence.

Recommended S8: **cross-car secondary-authority distribution study**. Before
any active presentation work, repeat offline distribution and policy replay
using existing or future authorized captures from materially different cars.
The purpose is to learn whether Dino-derived M5 eligibility, primary-relative
behavior, and the linear near-zero conclusion remain stable. S8 should not
select production Presence, Contrast, or budget values and should not connect
presentation to hardware.
