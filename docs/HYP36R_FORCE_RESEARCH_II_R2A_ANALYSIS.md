# HYP36R Force Research II — R2-A vehicle and Force baseline

## Decision

**R2-A COMPLETE WITH OPTIONAL TARGETED REPEAT.** The seven accepted captures
establish the requested vehicle/Force baseline. A07 contains repeated FREE to
recovering to LOAD transitions, but no active BITE. A BITE-specific A07 repeat
would improve that scenario; it is not required to preserve the broader R2-A
gate because active BITE is already captured in A04 and A05.

This is an analysis result, not a Force change or a final Steering Load ceiling.
R2-B remains a separate future campaign.

## Attempt inventory

| Scenario | Attempt | Review | Primary use |
| --- | ---: | --- | --- |
| A01 | 1 | accepted | yes |
| A02 | 1 | accepted | yes |
| A03 | 1 | accepted | yes |
| A04 | 1 | accepted | yes |
| A05 | 1 | retry | secondary only |
| A05 | 2 | accepted | yes |
| A06 | 1 | retry | secondary only |
| A06 | 2 | retry | secondary only |
| A06 | 3 | cancelled at 11.03 s | secondary only |
| A06 | 3 | accepted | yes |
| A07 | 1 | accepted | yes |

The cancelled and accepted A06 files are unique even though both metadata
records say attempt 3. Status and filename keep them distinguishable. The
unreviewed 11:26 A01 CSV has no matching session record and is excluded.

## Capture quality

| Accepted scenario | Duration | Samples | Effective rate | Median / P95 / max gap | Integrity | Result |
| --- | ---: | ---: | ---: | --- | --- | --- |
| A01 | 19.99 s | 1,131 | 56.58 Hz | 15.20 / 20.29 / 23.72 ms | 0 malformed, 0 writes failed | CLEAN |
| A02 | 17.98 s | 1,067 | 59.34 Hz | 15.22 / 20.35 / 24.31 ms | 0 malformed, 0 writes failed | CLEAN |
| A03 | 17.99 s | 1,055 | 58.64 Hz | 15.22 / 20.35 / 23.71 ms | 0 malformed, 0 writes failed | CLEAN |
| A04 | 19.99 s | 1,183 | 59.19 Hz | 15.23 / 20.35 / 23.72 ms | 0 malformed, 0 writes failed | CLEAN |
| A05 | 14.99 s | 856 | 57.11 Hz | 15.20 / 20.29 / 23.91 ms | 0 malformed, 0 writes failed | CLEAN WITH EXPECTED VALIDITY BLANKS |
| A06 | 17.99 s | 1,060 | 58.92 Hz | 15.27 / 20.38 / 24.35 ms | 0 malformed, 0 writes failed | CLEAN |
| A07 | 15.00 s | 891 | 59.39 Hz | 15.23 / 20.28 / 24.03 ms | 0 malformed, 0 writes failed | CLEAN |

No CSV contains a literal NaN or infinity in an emitted numeric value. `xforce`
is deliberately unavailable in all accepted rows. The first row of each file
has no previous-surface values and no response rate. A05 also suppresses three
native-response values on 30 invalid-state rows. These are schema-defined
unavailable values, not corrupt samples.

## Reference+ reconstruction and Force Character identity

All accepted files report Reference+ at Presence 1.44 and Contrast 4. Strength,
Steering Load, Road Detail and Impact remain 100 percent. Invert remains true.
Directional, Road and Impact pre/post-gain values are identical on every row.

| Scenario | Pre-tanh max error | Post-tanh / pre-drive max error | `ffb_raw` max error | DirectInput unclamped max error | Final request max error |
| --- | ---: | ---: | ---: | ---: | ---: |
| A01 | 1.00e-7 | 1.37e-7 | 5.64e-7 | 1.37e-7 | 1.0002e-4 |
| A02 | 1.00e-7 | 1.33e-7 | 5.59e-7 | 1.33e-7 | 9.9976e-5 |
| A03 | 1.00e-7 | 1.38e-7 | 5.67e-7 | 1.38e-7 | 9.9940e-5 |
| A04 | 1.00e-7 | 1.58e-7 | 6.01e-7 | 1.58e-7 | 1.0003e-4 |
| A05 | 1.00e-7 | 1.71e-7 | 5.74e-7 | 1.71e-7 | 1.0001e-4 |
| A06 | 1.00e-7 | 1.54e-7 | 5.86e-7 | 1.54e-7 | 9.9904e-5 |
| A07 | 1.00e-7 | 1.61e-7 | 5.65e-7 | 1.61e-7 | 9.9866e-5 |

The roughly `1e-4` final-request delta is the expected conversion to the
integer DirectInput nominal range of -10,000 to 10,000. The floating-point
composition path remains at the expected R1 precision.

## Scenario findings

### A01 straight baseline

A01 is surface-stable with one raw surface state and no transition. It remains
in LOAD/normal for all 1,131 frames. Response authority is 1.0 throughout,
there is no BITE and no applied M5 modulation. Steering is not perfectly zero
(`abs P95 .142`, range `-.055` to `.181`), but directional output stays quiet
(`abs P95 .058`) and final pre-drive output remains low (`abs P95 .062`, max
`.118`). Road is zero. One gear transition and a small Impact/vibration tail
occur, so the quietest baseline windows should exclude that event.

The neutral corner baseline is stable: field14 is 2 and fieldE8 is 1 at every
corner. fieldEC and fieldEE vary in paired groups but show effectively no
correlation with the small steering movement.

### A02 progressive left and A03 progressive right

The pair gives useful directional comparison evidence. A02 steering is mostly
negative and its directional request mostly positive. A03 steering is mostly
positive and its directional request mostly negative. Steering-to-directional
correlation is `-.983` in A02 and `-.998` in A03. Magnitudes are comparable:
directional abs P95 is `.153` versus `.172`, and final abs P95 is `.154` versus
`.184`.

A03 is fully surface-stable. A02 has eight surface transitions in two short
clusters at 2.42–2.75 s and 6.07–6.45 s, outside its only RELEASE interval at
1.07–1.55 s. This supports directional symmetry, but it does not establish
physical corner ordering. fieldEC and fieldEE change sign/distribution between
the captures and preserve close 0/1 and 2/3 pairing. That is a useful
direction-sensitive correlation candidate, not a wheel-position label.

### A04 sustained high load

| Measure | Result |
| --- | ---: |
| Peak steering input | 1.000 |
| Directional abs P95 / P99 / max | .448 / .604 / .646 |
| Directional 1 s RMS P95 / max | .386 / .411 |
| Directional 3 s RMS P95 / max | .339 / .363 |
| Final abs P95 / P99 / max | .452 / .553 / .682 |
| Final 1 s RMS P95 / max | .372 / .423 |
| Final 3 s RMS P95 / max | .333 / .357 |
| Occupancy at .75 / .90 / .98 | 0 / 0 / 0 |
| Minimum normalized headroom | .318 |
| DirectInput clamp frames | 0 |

A04 contains 10.24 s of FREE/established state, 2.21 s recovering, 0.52 s
RELEASE/emerging and 220 active-BITE frames. Its longest continuous interval
above the scenario's directional P75 is about .96 s.

Directional-channel analysis is valid across the capture because it is logged
before Road and Impact composition. Final-output analysis is mixed: raw surface
transitions occur at 7.99–8.42 s and 18.47–19.42 s; Road is active on 700 frames,
Impact on 533 and restored vibration on 755. Surface-stable FREE windows include
3.29–7.94 s and 14.99–18.32 s, but they are not Road/Impact-free. The clean
0.29–3.07 s window is useful as a composition control, not sustained FREE.

### A05 drift initiation

**TARGET CAPTURED.** The accepted attempt contains LOAD → RELEASE → FREE,
followed by recovering behavior and active BITE. FREE occupies 538 frames
(9.42 s), RELEASE 15 frames (.26 s), recovering 40 frames (.70 s), and active
BITE 189 frames. The principal FREE interval begins at 3.79 s. Surface changes
appear later at 9.64–12.64 s, so the initial LOAD/RELEASE/FREE transition is
separable. The rejected attempt remains secondary evidence only.

### A06 sustained drift

**TARGET CAPTURED.** The accepted attempt contains 583 FREE frames (9.90 s)
across several sustained intervals, including 14.03–17.99 s. RELEASE covers 69
frames (1.17 s), recovering 117 frames (1.99 s), and BITE never activates.
M5 intent is active on 970 frames and the passive M5 shadow on 66, while the
authoritative `M4_ONLY` route applies zero M5 modulation. Surface changes are
concentrated around 8.79–9.59 s plus two later events, leaving useful stable
drift intervals. Rejected and cancelled attempts are not needed for the result.

### A07 release and recovery

**PARTIAL TARGET — USEFUL BUT BITE-SPECIFIC REPEAT RECOMMENDED.** A07 contains
multiple FREE → recovering → LOAD sequences. FREE covers 175 frames (2.95 s),
recovering 141 (2.37 s), RELEASE 97 (1.63 s), and LOAD 478 (8.05 s). It shows
steering reversal/recentering, full unloading up to .25, restored directional
authority and return to normal from 12.59–15.00 s. No active BITE occurs; only
19 BITE-candidate frames are present. Surface transitions at 8.85–9.55 s overlap
one recovery sequence, but earlier and later recoveries remain usable.

An optional repeat should establish a stable drift on clean asphalt, then
smoothly reduce countersteer and throttle so the car re-aligns without a surface
crossing or impact, and continue driving long enough to hold normal load after
the recovery. It should not repeat the rest of R2-A.

## State evidence matrix

M4 phase terminology is mapped as normal = LOAD, emerging = RELEASE and
established = FREE. BITE is taken only from `bite_active`, not inferred from the
intended maneuver.

| Scenario | LOAD | RELEASE | FREE | BITE | Observed transition evidence |
| --- | ---: | ---: | ---: | ---: | --- |
| A01 | 1,131 / 19.99 s | 0 | 0 | 0 | quiet LOAD control |
| A02 | 1,037 / 17.48 s | 30 / .51 s | 0 | 0 | LOAD → RELEASE → LOAD |
| A03 | 1,027 / 17.51 s | 28 / .48 s | 0 | 0 | LOAD → RELEASE at capture end |
| A04 | 415 / 7.01 s | 31 / .52 s | 606 / 10.24 s | 220 / 3.72 s | repeated LOAD/RELEASE/FREE/recovery, BITE |
| A05 | 263 / 4.61 s | 15 / .26 s | 538 / 9.42 s | 189 / 3.31 s | LOAD → RELEASE → FREE → recovery/BITE |
| A06 | 291 / 4.94 s | 69 / 1.17 s | 583 / 9.90 s | 0 | sustained FREE and repeated recovery |
| A07 | 478 / 8.05 s | 97 / 1.63 s | 175 / 2.95 s | 0 | repeated FREE → recovery → LOAD |

The dataset covers every foundational state. A07 does not independently prove
BITE, but A04 and A05 do.

## Interim Steering-only gain sweep

Road and Impact remain at 1.00. Values are normalized software output. They do
not represent wheel torque and do not select a production ceiling.

| Steering gain | P95 / P99 / max | 1 s RMS P95 / max | 3 s RMS P95 / max | Occupancy .75 / .90 / .98 | Min headroom | Pre-tanh over unity | Clamp |
| ---: | --- | --- | --- | --- | ---: | ---: | ---: |
| 1.00 | .371 / .506 / .682 | .324 / .453 | .281 / .358 | 0 / 0 / 0 | .318 | 0 | 0 |
| 1.10 | .403 / .545 / .713 | .350 / .491 | .304 / .388 | 0 / 0 / 0 | .287 | 0 | 0 |
| 1.20 | .434 / .581 / .741 | .375 / .528 | .328 / .417 | 0 / 0 / 0 | .259 | 0 | 0 |
| 1.30 | .463 / .616 / .767 | .399 / .562 | .350 / .444 | .055% / 0 / 0 | .233 | .028% | 0 |
| 1.40 | .492 / .647 / .791 | .422 / .594 | .370 / .470 | .083% / 0 / 0 | .209 | .069% | 0 |
| 1.50 | .520 / .677 / .813 | .446 / .625 | .390 / .495 | .110% / 0 / 0 | .187 | .097% | 0 |
| 1.60 | .548 / .706 / .832 | .467 / .654 | .409 / .518 | .276% / 0 / 0 | .168 | .166% | 0 |
| 1.75 | .586 / .744 / .858 | .497 / .694 | .438 / .551 | .884% / 0 / 0 | .142 | .649% | 0 |
| 2.00 | .645 / .799 / .893 | .544 / .752 | .479 / .600 | 2.209% / 0 / 0 | .107 | 1.657% | 0 |

A04 is the limiting accepted capture. At 2.00 its P95/P99/max become
`.727/.842/.893`, 3 s RMS P95/max `.539/.579`, .75 occupancy 4.65 percent and
pre-tanh-over-unity occupancy 4.40 percent. No replay is nonfinite or reaches a
DirectInput clamp, and positive steering gain does not reverse the directional
channel sign.

For this dataset, 1.00–1.30 is clearly comfortable in normalized software
headroom. 1.40–1.75 is headroom-aware and increasingly compressed in the A04
high-load tail. 2.00 has no hard clamp but produces meaningful `tanh`
compression and materially higher sustained exposure. Physical testing across
cars, longer clean high-load corners and wheel classes remains necessary before
any final ceiling.

## Neutral per-corner correlation candidates

| Field | R2-A observation | Classification |
| --- | --- | --- |
| field14 | Constant 2 in A01/A03; discrete powers/flags appear in A02, A04–A07, sometimes very large and often near changed surface/context | surface/context-sensitive candidate; not a continuous load measure |
| fieldE8 | Essentially 1 at all four neutral corners in every accepted capture | no useful pattern yet |
| fieldEC | 0/1 and 2/3 remain closely paired; sign/distribution changes between A02 and A03 and amplitude expands strongly in load/drift | direction- and dynamic-load correlation candidate |
| fieldEE | 0/1 differ from fieldEC while 2/3 track fieldEC; direction changes between A02/A03 and amplitude expands in A04–A07 | direction/drift/recovery correlation candidate |

No field is renamed and no physical corner, load, suspension or material
semantic is assigned.

## Surface, gear and transient audit

| Scenario | Unique raw surface states | Transitions | Window class | Gear changes | Road active frames | Impact active frames | Restored vibration active frames |
| --- | ---: | ---: | --- | ---: | ---: | ---: | ---: |
| A01 | 1 | 0 | SURFACE-STABLE | 1 | 0 | 254 | 130 |
| A02 | 7 | 8 | SURFACE-MIXED; primary RELEASE separable | 4 | 0 | 492 | 14 |
| A03 | 1 | 0 | SURFACE-STABLE | 3 | 0 | 488 | 13 |
| A04 | 13 | 16 | SURFACE-MIXED; stable high-load subwindows exist | 2 | 700 | 533 | 755 |
| A05 | 9 | 11 | SURFACE-MIXED; initial transition separable | 0 | 662 | 445 | 694 |
| A06 | 14 | 18 | SURFACE-MIXED; stable drift subwindows exist | 3 | 655 | 613 | 777 |
| A07 | 7 | 8 | SURFACE-MIXED; two recoveries separable | 1 | 286 | 643 | 400 |

Impact activity here includes the existing PC effect/interpretation channel; it
is not proof of collisions. Gear events are sparse and can be segmented rather
than invalidating a scenario. A04 final-output ceiling work must account for
its mixed Road/Impact content; the independently logged directional channel
remains suitable for Steering-only replay.

## M4, M5 and native-PC observations

M4 authority stays within 0–1 and selected unloading stays within 0–.25 in all
accepted captures. Reference+ never falls back. No secondary-budget or Legacy
boundary intervention occurs. M5 intent responds across the maneuver set, and
the passive M5 shadow activates in A02–A07, most often in A06/A07. The runtime
route remains `M4_ONLY`, so applied M5 modulation is zero on every frame. This
is expected and shows no M4/M5 authority violation.

The restored native PC observation channels provide useful future comparison
evidence: A01/A03 are clean control contexts; A02/A03 provide opposed steering;
A04 supplies sustained load plus strong PC Road/vibration activity; A05/A06
supply transition and sustained FREE; A07 supplies recovery. Gear transitions
are explicitly located. These records can later compare native PC state,
HYP36R interpretation and HYP36R output with an official/arcade reference. They
do not identify AER behavior or justify an arcade semantic today.

## R2-B readiness

R2-A is sufficient to proceed when the project owner chooses. R2-B should keep
the planned local sequence: asphalt control, partial striped runoff, full
striped runoff, partial rough/sand, full rough/sand and surface re-entry. Its
central question remains whether useful per-corner information is lost before
Road Detail when only part of the car changes surface.

No tangible repeat is required by this gate, so no new UAT artifact is created.
