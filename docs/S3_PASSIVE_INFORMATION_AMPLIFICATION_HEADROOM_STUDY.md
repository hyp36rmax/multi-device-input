# S3 Passive Information Amplification Headroom Study

S3 is an offline replay study of the unchanged S2 output path. It does not
change Force, runtime gain, output conditioning, DirectInput, or hardware.

## A. Datasets used

| Capture | Scenario | Samples | Duration | Configuration |
| --- | --- | ---: | ---: | --- |
| `telemetry_20260920_202027 (S2 M4).csv` | `S2_DINO_M4_OUTPUT_EXPOSURE` | 6,238 | 103.74 s | Dino 246 GTS, Sunny Beach, DD2 50%, game FFB 100%, M4 only |
| `telemetry_20260920_202708 (S2 M5).csv` | `S2_DINO_M5_OUTPUT_EXPOSURE` | 6,715 | 111.70 s | Dino 246 GTS, Sunny Beach, DD2 50%, game FFB 100%, M5 lateral active |

The 1.00x replay reproduces the S2 summaries. M4 has mean magnitude 0.082,
P95 0.333, maximum 0.488, and maximum three-second RMS 0.331. M5 has mean
magnitude 0.082, P95 0.310, maximum 0.510, and maximum three-second RMS 0.271.

## B. Timing-artifact handling

Output and exposure statistics retain every recorded force sample. Only slew
rejects invalid timing. A valid interval is 8.33 through 33.33 milliseconds,
or one-half through twice the nominal 60 Hz period.

- M4: 6,224 valid slew intervals; 13 rejected.
- M5: 6,644 valid slew intervals; 70 rejected.
- The M5 0.517-second discontinuity and the short catch-up samples are rejected.

This prevents a capture-timing artifact from being described as a Force
transient. No recorded Force value is modified or removed from other metrics.

## C. Strategy definitions

### Strategy A: pre-tanh presentation scale

```text
replayed output = tanh(composer input * scale) * observed output ramp
```

The observed output ramp is reconstructed sample by sample from
`ffb_raw / s2_post_tanh` where the denominator is nonzero. This strategy keeps
the existing `tanh` downstream of presentation. It naturally prevents a hard
boundary crossing, but progressively compresses strong information.

### Strategy B: post-tanh, pre-drive presentation scale

```text
unbounded replay = s2_post_tanh * observed output ramp * scale
bounded replay = clamp(unbounded replay, -1, 1)
```

This is equivalent to scaling `ffb_raw` for the captured path. It preserves
linear proportionality until the normalized boundary, but consumes peak and
transient reserve faster.

### Strategy C: channel-aware presentation before the safety envelope

S1 suggests a possible future layer that presents directional load, bounded
impact, and detail with explicit channel visibility before a safety envelope
allocates reserve. This is architecturally stronger than blindly scaling a
final aggregate because it can preserve transient and detail budgets. S3 does
not replay it: choosing channel gains, priorities, or budgets would introduce
new policy not established by the captures.

## D. Replay scales

Both strategies were replayed at 1.00, 1.10, 1.20, 1.30, 1.40, 1.50, 1.60,
1.75, and 2.00x. These are simulation points only, not runtime settings or
hardware-safe recommendations.

## E. Comparison matrix

`O75`, `O90`, and `O98` are sample occupancy at or above the named magnitude.
`Clamp` is the fraction whose unbounded request would exceed 1.0. `Pre>1` is
the fraction whose pre-tanh composer magnitude exceeds 1.0.

| Run | Strategy | Scale | P95 | Max | P95 3s RMS | Max 3s RMS | O75 | O90 | O98 | Clamp | Pre>1 | Region |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| M4 | A | 1.00 | .333 | .488 | .274 | .331 | 0 | 0 | 0 | 0 | 0 | comfort |
| M4 | A | 1.10 | .363 | .528 | .299 | .361 | 0 | 0 | 0 | 0 | 0 | comfort |
| M4 | A | 1.20 | .393 | .565 | .323 | .389 | 0 | 0 | 0 | 0 | 0 | comfort |
| M4 | A | 1.30 | .422 | .600 | .347 | .416 | 0 | 0 | 0 | 0 | 0 | comfort |
| M4 | A | 1.40 | .450 | .634 | .370 | .443 | 0 | 0 | 0 | 0 | 0 | headroom-aware |
| M4 | A | 1.50 | .477 | .664 | .392 | .468 | 0 | 0 | 0 | 0 | 0 | headroom-aware |
| M4 | A | 1.60 | .503 | .693 | .414 | .493 | 0 | 0 | 0 | 0 | 0 | headroom-aware |
| M4 | A | 1.75 | .541 | .732 | .444 | .527 | 0 | 0 | 0 | 0 | 0 | compression |
| M4 | A | 2.00 | .599 | .789 | .491 | .580 | .14% | 0 | 0 | 0 | .10% | compression |
| M4 | B | 1.00 | .333 | .488 | .274 | .331 | 0 | 0 | 0 | 0 | 0 | comfort |
| M4 | B | 1.10 | .366 | .537 | .301 | .364 | 0 | 0 | 0 | 0 | 0 | comfort |
| M4 | B | 1.20 | .399 | .586 | .329 | .398 | 0 | 0 | 0 | 0 | 0 | comfort |
| M4 | B | 1.30 | .433 | .635 | .356 | .431 | 0 | 0 | 0 | 0 | 0 | comfort |
| M4 | B | 1.40 | .466 | .684 | .383 | .464 | 0 | 0 | 0 | 0 | 0 | headroom-aware |
| M4 | B | 1.50 | .499 | .732 | .411 | .497 | 0 | 0 | 0 | 0 | 0 | headroom-aware |
| M4 | B | 1.60 | .532 | .781 | .438 | .530 | .06% | 0 | 0 | 0 | 0 | headroom-aware |
| M4 | B | 1.75 | .582 | .854 | .479 | .580 | .38% | 0 | 0 | 0 | 0 | boundary approach |
| M4 | B | 2.00 | .666 | .977 | .548 | .663 | 1.99% | .14% | 0 | 0 | 0 | boundary approach |
| M5 | A | 1.00 | .310 | .510 | .229 | .271 | 0 | 0 | 0 | 0 | 0 | comfort |
| M5 | A | 1.10 | .339 | .550 | .250 | .295 | 0 | 0 | 0 | 0 | 0 | comfort |
| M5 | A | 1.20 | .367 | .588 | .271 | .319 | 0 | 0 | 0 | 0 | 0 | comfort |
| M5 | A | 1.30 | .395 | .624 | .291 | .341 | 0 | 0 | 0 | 0 | 0 | comfort |
| M5 | A | 1.40 | .421 | .657 | .311 | .363 | 0 | 0 | 0 | 0 | 0 | headroom-aware |
| M5 | A | 1.50 | .448 | .688 | .330 | .385 | 0 | 0 | 0 | 0 | 0 | headroom-aware |
| M5 | A | 1.60 | .473 | .716 | .349 | .405 | 0 | 0 | 0 | 0 | 0 | headroom-aware |
| M5 | A | 1.75 | .509 | .755 | .375 | .434 | .01% | 0 | 0 | 0 | 0 | compression |
| M5 | A | 2.00 | .566 | .809 | .418 | .479 | .21% | 0 | 0 | 0 | .13% | compression |
| M5 | B | 1.00 | .310 | .510 | .229 | .271 | 0 | 0 | 0 | 0 | 0 | comfort |
| M5 | B | 1.10 | .342 | .561 | .252 | .298 | 0 | 0 | 0 | 0 | 0 | comfort |
| M5 | B | 1.20 | .373 | .612 | .275 | .325 | 0 | 0 | 0 | 0 | 0 | comfort |
| M5 | B | 1.30 | .404 | .663 | .298 | .352 | 0 | 0 | 0 | 0 | 0 | comfort |
| M5 | B | 1.40 | .435 | .714 | .321 | .379 | 0 | 0 | 0 | 0 | 0 | headroom-aware |
| M5 | B | 1.50 | .466 | .765 | .344 | .407 | .01% | 0 | 0 | 0 | 0 | headroom-aware |
| M5 | B | 1.60 | .497 | .816 | .367 | .434 | .09% | 0 | 0 | 0 | 0 | headroom-aware |
| M5 | B | 1.75 | .543 | .892 | .401 | .474 | .43% | 0 | 0 | 0 | 0 | boundary approach |
| M5 | B | 2.00 | .621 | 1.000 | .459 | .542 | 2.19% | .21% | .04% | .01% | 0 | saturation onset |

The two nonidentical drives identify approximately the same regime. Through
1.30x both remain well inside the software range. At 1.40–1.60x reserve is
still substantial, but high-output occupancy begins to become measurable.
At 1.75–2.00x the strategies diverge: A spends information quality through
nonlinear compression, while B spends boundary reserve.

The scale sequence in the compact sequence tables below is always:
`1.00, 1.10, 1.20, 1.30, 1.40, 1.50, 1.60, 1.75, 2.00x`.

| Run | Strategy | Recent-peak P95 by scale | Recent-peak maximum by scale | Occupancy >=.50 by scale |
| --- | --- | --- | --- | --- |
| M4 | A | .393, .427, .461, .493, .523, .553, .581, .621, .681 | .488, .528, .565, .600, .634, .664, .693, .732, .789 | 0, .10%, .40%, .98%, 2.08%, 3.40%, 5.21%, 7.37%, 9.07% |
| M4 | B | .393, .432, .471, .511, .550, .589, .628, .687, .785 | .488, .537, .586, .635, .684, .732, .781, .854, .977 | 0, .11%, .50%, 1.59%, 2.98%, 4.95%, 6.93%, 8.24%, 10.29% |
| M5 | A | .413, .449, .483, .516, .548, .578, .606, .646, .706 | .510, .550, .588, .624, .657, .688, .716, .755, .809 | .01%, .13%, .43%, 1.25%, 2.22%, 2.95%, 3.86%, 5.45%, 7.85% |
| M5 | B | .413, .455, .496, .537, .579, .620, .661, .723, .826 | .510, .561, .612, .663, .714, .765, .816, .892, 1.000 | .01%, .19%, .61%, 1.88%, 2.71%, 3.69%, 4.85%, 6.36%, 9.19% |

Recent peak uses the S2 30-sample window, approximately 500 ms at nominal
60 Hz. Its maximum necessarily equals the capture maximum because every
sample belongs to a recent-peak window.

## F. Output and headroom distributions

The following tables complete the output distribution requested for each
scale. Values are magnitude. Headroom is `1 - magnitude`; its P05 is the
high-output reserve indicator.

### M4

| Strategy | Scale | Mean | P50 | P75 | P90 | P99 | Head P05 | Head median | Head P95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| A | 1.00 | .082 | .037 | .120 | .253 | .398 | .667 | .963 | 1.000 |
| A | 1.10 | .090 | .041 | .132 | .277 | .433 | .637 | .959 | 1.000 |
| A | 1.20 | .098 | .045 | .143 | .301 | .467 | .607 | .955 | 1.000 |
| A | 1.30 | .106 | .048 | .155 | .325 | .499 | .578 | .952 | 1.000 |
| A | 1.40 | .113 | .052 | .167 | .347 | .530 | .550 | .948 | 1.000 |
| A | 1.50 | .121 | .056 | .179 | .370 | .560 | .523 | .944 | 1.000 |
| A | 1.60 | .128 | .060 | .190 | .392 | .588 | .497 | .940 | 1.000 |
| A | 1.75 | .139 | .065 | .208 | .425 | .628 | .459 | .935 | 1.000 |
| A | 2.00 | .156 | .074 | .236 | .476 | .687 | .401 | .926 | 1.000 |
| B | 1.00 | .082 | .037 | .120 | .253 | .398 | .667 | .963 | 1.000 |
| B | 1.10 | .091 | .041 | .132 | .279 | .438 | .634 | .959 | 1.000 |
| B | 1.20 | .099 | .045 | .144 | .304 | .478 | .601 | .955 | 1.000 |
| B | 1.30 | .107 | .048 | .156 | .329 | .518 | .567 | .952 | 1.000 |
| B | 1.40 | .115 | .052 | .168 | .355 | .558 | .534 | .948 | 1.000 |
| B | 1.50 | .124 | .056 | .180 | .380 | .597 | .501 | .944 | 1.000 |
| B | 1.60 | .132 | .060 | .192 | .405 | .637 | .468 | .940 | 1.000 |
| B | 1.75 | .144 | .065 | .210 | .443 | .697 | .418 | .935 | 1.000 |
| B | 2.00 | .165 | .075 | .240 | .507 | .796 | .334 | .925 | 1.000 |

### M5

| Strategy | Scale | Mean | P50 | P75 | P90 | P99 | Head P05 | Head median | Head P95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| A | 1.00 | .082 | .035 | .127 | .236 | .405 | .690 | .965 | 1.000 |
| A | 1.10 | .090 | .039 | .140 | .258 | .440 | .661 | .961 | 1.000 |
| A | 1.20 | .098 | .042 | .152 | .281 | .474 | .633 | .958 | 1.000 |
| A | 1.30 | .105 | .046 | .165 | .303 | .507 | .605 | .954 | 1.000 |
| A | 1.40 | .113 | .049 | .177 | .324 | .538 | .579 | .951 | 1.000 |
| A | 1.50 | .120 | .053 | .190 | .346 | .568 | .552 | .947 | 1.000 |
| A | 1.60 | .127 | .056 | .202 | .367 | .596 | .527 | .944 | 1.000 |
| A | 1.75 | .138 | .062 | .220 | .397 | .636 | .491 | .938 | 1.000 |
| A | 2.00 | .156 | .070 | .251 | .447 | .696 | .434 | .930 | 1.000 |
| B | 1.00 | .082 | .035 | .127 | .236 | .405 | .690 | .965 | 1.000 |
| B | 1.10 | .090 | .039 | .140 | .259 | .445 | .658 | .961 | 1.000 |
| B | 1.20 | .098 | .042 | .153 | .283 | .486 | .627 | .958 | 1.000 |
| B | 1.30 | .107 | .046 | .166 | .307 | .526 | .596 | .954 | 1.000 |
| B | 1.40 | .115 | .049 | .178 | .330 | .567 | .565 | .951 | 1.000 |
| B | 1.50 | .123 | .053 | .191 | .354 | .607 | .534 | .947 | 1.000 |
| B | 1.60 | .131 | .056 | .204 | .377 | .648 | .503 | .944 | 1.000 |
| B | 1.75 | .143 | .062 | .223 | .413 | .709 | .457 | .938 | 1.000 |
| B | 2.00 | .164 | .071 | .255 | .472 | .810 | .379 | .929 | 1.000 |

## G. Sustained exposure

### One-second RMS

| Run | Strategy | Scale sequence | Median RMS at 1.00→2.00x | P95 RMS at 1.00→2.00x | Maximum RMS at 1.00→2.00x |
| --- | --- | --- | --- | --- | --- |
| M4 | A | 1, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.75, 2 | .053, .058, .063, .069, .074, .079, .084, .092, .105 | .279, .305, .330, .354, .378, .401, .424, .456, .506 | .413, .449, .483, .516, .547, .577, .606, .645, .704 |
| M4 | B | same | .053, .058, .063, .069, .074, .079, .084, .092, .106 | .279, .307, .335, .363, .391, .419, .447, .488, .558 | .413, .455, .496, .537, .579, .620, .661, .724, .827 |
| M5 | A | same | .072, .079, .086, .093, .100, .107, .113, .124, .140 | .261, .285, .308, .331, .353, .374, .395, .425, .471 | .380, .413, .445, .476, .505, .533, .560, .597, .654 |
| M5 | B | same | .072, .079, .086, .093, .100, .107, .115, .125, .143 | .261, .287, .313, .339, .365, .391, .417, .456, .522 | .380, .418, .457, .495, .533, .571, .609, .666, .760 |

### Three-second RMS

| Run | Strategy | Median at 1.00→2.00x | P95 at 1.00→2.00x | Maximum at 1.00→2.00x |
| --- | --- | --- | --- | --- |
| M4 | A | .076, .084, .091, .098, .106, .113, .121, .131, .149 | .274, .299, .323, .347, .370, .392, .414, .444, .491 | .331, .361, .389, .416, .443, .468, .493, .527, .580 |
| M4 | B | .076, .084, .091, .099, .107, .114, .122, .133, .152 | .274, .301, .329, .356, .383, .411, .438, .479, .548 | .331, .364, .398, .431, .464, .497, .530, .580, .663 |
| M5 | A | .098, .108, .117, .127, .136, .145, .154, .168, .190 | .229, .250, .271, .291, .311, .330, .349, .375, .418 | .271, .295, .319, .341, .363, .385, .405, .434, .479 |
| M5 | B | .098, .108, .118, .128, .138, .147, .157, .172, .196 | .229, .252, .275, .298, .321, .344, .367, .401, .459 | .271, .298, .325, .352, .379, .407, .434, .474, .542 |

Even at 1.60x, maximum three-second RMS stays at or below .530 across both
strategies and captures. Sustained exposure is therefore not near the
normalized boundary in the studied drives.

## H. Transient reserve

For samples in the highest five percent of one-second RMS windows, median
headroom remains substantial. The minimum is the worst instantaneous reserve
inside those windows, including impacts.

| Run | Strategy | Scale | Median reserve in high-RMS windows | Minimum reserve | Minimum reserve with impact active |
| --- | --- | ---: | ---: | ---: | ---: |
| M4 | A | 1.30 | .606 | .400 | .400 |
| M4 | A | 1.60 | .528 | .307 | .307 |
| M4 | A | 2.00 | .441 | .211 | .211 |
| M4 | B | 1.30 | .593 | .365 | .365 |
| M4 | B | 1.60 | .500 | .219 | .219 |
| M4 | B | 1.75 | .453 | .146 | .146 |
| M4 | B | 2.00 | .375 | .023 | .023 |
| M5 | A | 1.30 | .743 | .376 | .376 |
| M5 | A | 1.60 | .686 | .284 | .284 |
| M5 | A | 2.00 | .608 | .191 | .191 |
| M5 | B | 1.30 | .745 | .337 | .337 |
| M5 | B | 1.60 | .686 | .184 | .184 |
| M5 | B | 1.75 | .657 | .108 | .108 |
| M5 | B | 2.00 | .608 | 0 | 0 |

Strategy B at 2.00x leaves essentially no reserve for the strongest captured
impact combination. Strategy A retains boundary reserve by compressing that
combination instead. A future operating point should not be selected merely
because its average or RMS remains low.

## I. Tanh compression

For Strategy A, the local response retained relative to ideal linear scaling
is `sech²(scale * composerInput)`. This is a common local multiplier on all
additive channels at a sample, so sign and ordering remain intact while strong
moments lose contrast.

| Run | Scale | P05 local response retained | Median retained | Worst retained |
| --- | ---: | ---: | ---: | ---: |
| M4 | 1.00 | 88.9% | 99.9% | 76.2% |
| M4 | 1.30 | 82.2% | 99.8% | 63.9% |
| M4 | 1.50 | 77.3% | 99.7% | 55.9% |
| M4 | 1.60 | 74.7% | 99.6% | 52.0% |
| M4 | 1.75 | 70.7% | 99.6% | 46.3% |
| M4 | 2.00 | 64.1% | 99.4% | 37.8% |
| M5 | 1.00 | 90.4% | 99.9% | 74.0% |
| M5 | 1.30 | 84.4% | 99.8% | 61.1% |
| M5 | 1.50 | 80.0% | 99.7% | 52.7% |
| M5 | 1.60 | 77.6% | 99.7% | 48.7% |
| M5 | 1.75 | 74.1% | 99.6% | 43.0% |
| M5 | 2.00 | 67.9% | 99.5% | 34.5% |

The median remains nearly linear because most samples are small. The relevant
loss appears in high-information samples: by 1.75–2.00x their incremental
contrast is materially compressed even though clipping never occurs.

## J. Clipping and boundary analysis

- Strategy A produces no boundary or clamp event at any replay scale. This is
  not free headroom; `tanh` has converted boundary pressure into compression.
- Strategy B produces no clamp event through 1.75x in either capture.
- At 2.00x, M4 reaches .977 without clipping. M5 reaches the boundary, with
  .01% clamp requirement, .04% occupancy at or above .98, and .21% occupancy
  at or above .90.
- No capture supports normalizing repeated boundary contact as acceptable.

Near-boundary occupancy is otherwise zero. There is no broad saturation
region in the tested range, only the onset of saturation in M5 Strategy B at
2.00x.

## Valid-dt slew distribution

Slew is normalized output per second, not motor torque slew.

| Run | Strategy | Scale | Median | P95 | P99 | Maximum |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| M4 | A | 1.00 | .185 | 1.429 | 3.941 | 16.717 |
| M4 | A | 1.30 | .240 | 1.803 | 4.942 | 21.144 |
| M4 | A | 1.60 | .293 | 2.156 | 5.798 | 25.215 |
| M4 | A | 2.00 | .363 | 2.562 | 6.733 | 29.986 |
| M4 | B | 1.00 | .185 | 1.429 | 3.941 | 16.717 |
| M4 | B | 1.30 | .241 | 1.858 | 5.123 | 21.732 |
| M4 | B | 1.60 | .296 | 2.287 | 6.306 | 26.747 |
| M4 | B | 2.00 | .371 | 2.859 | 7.882 | 33.433 |
| M5 | A | 1.00 | .203 | 1.556 | 4.229 | 17.958 |
| M5 | A | 1.30 | .262 | 1.940 | 5.414 | 22.204 |
| M5 | A | 1.60 | .321 | 2.299 | 6.503 | 25.729 |
| M5 | A | 2.00 | .395 | 2.780 | 7.483 | 29.287 |
| M5 | B | 1.00 | .203 | 1.556 | 4.229 | 17.958 |
| M5 | B | 1.30 | .264 | 2.023 | 5.498 | 23.345 |
| M5 | B | 1.60 | .325 | 2.489 | 6.767 | 28.733 |
| M5 | B | 2.00 | .407 | 3.112 | 8.458 | 35.916 |

Strategy B scales valid slew almost exactly with gain until its boundary.
Strategy A increasingly compresses high-slew events. Nothing here establishes
physical wheel slew or torque.

For completeness, the intermediate-scale valid-dt slew values are:

| Run | Strategy | Metric | Values at 1.00→2.00x |
| --- | --- | --- | --- |
| M4 | A | median | .185, .204, .222, .240, .258, .276, .293, .320, .363 |
| M4 | A | P95 | 1.429, 1.555, 1.677, 1.803, 1.925, 2.046, 2.156, 2.317, 2.562 |
| M4 | A | P99 | 3.941, 4.257, 4.603, 4.942, 5.267, 5.489, 5.798, 6.181, 6.733 |
| M4 | A | max | 16.717, 18.215, 19.700, 21.144, 22.546, 23.904, 25.215, 27.095, 29.986 |
| M4 | B | median | .185, .204, .222, .241, .259, .278, .296, .324, .371 |
| M4 | B | P95 | 1.429, 1.572, 1.715, 1.858, 2.001, 2.144, 2.287, 2.501, 2.859 |
| M4 | B | P99 | 3.941, 4.335, 4.729, 5.123, 5.517, 5.912, 6.306, 6.897, 7.882 |
| M4 | B | max | 16.717, 18.388, 20.060, 21.732, 23.403, 25.075, 26.747, 29.254, 33.433 |
| M5 | A | median | .203, .223, .243, .262, .282, .301, .321, .348, .395 |
| M5 | A | P95 | 1.556, 1.689, 1.815, 1.940, 2.063, 2.187, 2.299, 2.487, 2.780 |
| M5 | A | P99 | 4.229, 4.598, 5.002, 5.414, 5.759, 6.160, 6.503, 6.821, 7.483 |
| M5 | A | max | 17.958, 19.450, 20.867, 22.204, 23.461, 24.636, 25.729, 27.214, 29.287 |
| M5 | B | median | .203, .224, .244, .264, .285, .305, .325, .356, .407 |
| M5 | B | P95 | 1.556, 1.711, 1.867, 2.023, 2.178, 2.334, 2.489, 2.723, 3.112 |
| M5 | B | P99 | 4.229, 4.652, 5.075, 5.498, 5.921, 6.344, 6.767, 7.401, 8.458 |
| M5 | B | max | 17.958, 19.754, 21.549, 23.345, 25.141, 26.937, 28.733, 31.426, 35.916 |

## K. Channel preservation

The captures contain these observed component ranges:

| Capture | Component | Active samples | Active P50 magnitude | Active P95 | Maximum |
| --- | --- | ---: | ---: | ---: | ---: |
| M4 | directional | 5,854 | .038080 | .338894 | .443875 |
| M4 | road | 1,258 | .007512 | .023364 | .029594 |
| M4 | impact | 3,753 | .000608 | .062654 | .290063 |
| M5 | directional | 6,367 | .037249 | .295020 | .454417 |
| M5 | road | 2,508 | .007452 | .023461 | .056874 |
| M5 | impact | 4,082 | .000776 | .070288 | .290063 |

Both global strategies preserve channel sign and additive ordering before
their nonlinear or boundary stage. They differ in information preservation:

- Strategy A applies the same instantaneous nonlinear slope to directional,
  road, impact, and M5 information. At high aggregate load, a newly arriving
  impact or road increment receives less output contrast. It preserves the
  boundary by compressing channel differences.
- Strategy B preserves exact proportional relationships until clipping. At
  2.00x, its strongest M5 sample clips, so the incremental relationship is no
  longer preserved there.
- Neither global strategy reserves room specifically for impact or protects
  road detail against a high directional baseline. Strategy C is the future
  architectural route for testing that requirement without changing Force
  semantics.

## L. M5 implications

M5 modulation is active on 243 of 6,715 samples (3.62%). Among active samples,
its median magnitude is .000911, P95 is .002987, and maximum is .005524.

Global presentation scaling preserves this distinction proportionally under
Strategy B until clipping. Strategy A subjects it to the same aggregate-load
compression as every other channel. Even a theoretical 2.00x scale leaves the
maximum direct modulation near .011 before nonlinear interaction. S3 therefore
does not establish that global amplification makes M5 perceptually meaningful.
It also does not justify a separate M5 gain. The two drives are not identical,
so differences between their aggregate distributions cannot be attributed to
M5 alone.

## M. Software-zone definitions

These are descriptive software regions for these captures, not hardware-safe
zones or approved settings.

### Software comfort region

Through approximately 1.30x in both strategies: maximum output remains at or
below .663, high-output occupancy is zero, and substantial transient reserve
remains. Nonlinear loss grows under Strategy A but is not yet dominant.

### Software headroom-aware region

Approximately 1.40–1.60x: sustained RMS remains moderate and no clipping
occurs, but high-output occupancy begins and worst-case transient reserve
declines. A future presentation design should observe reserve rather than
assuming average output describes it.

### Software compression / boundary-approach region

At approximately 1.75–2.00x, Strategy A materially compresses high-information
increments, while Strategy B approaches the normalized boundary. These are two
different forms of information loss pressure.

### Software saturation onset

Only M5 Strategy B at 2.00x demonstrates actual clamp requirement, and only on
.01% of samples. The captures do not establish a broad saturation region.

## N. Evidence boundaries

S3 establishes software-side replay behavior for two Dino captures. It does
not establish:

- a perceptual floor, preferred feel, or manufactured-effect boundary;
- a production presentation scale;
- a DD2 or other wheel's torque, thermal, PSU, protection, or safe sustained
  physical output;
- motor torque slew from normalized software slew;
- that M4/M5 distribution differences were caused by M5;
- that a scale safe in these captures covers every car, route, impact, surface,
  frame rate, driver, firmware, or device;
- channel budgets or limiter thresholds.

The historical DD2 shutdown remains device-specific evidence only.

## O. Recommended next direction

**B. S4 — Presentation Calibration Architecture.**

Do not activate amplification yet. S4 should define a passive, inspectable
presentation interface and calibration method, with Reference remaining the
unchanged default. It should preserve channel provenance and expose the exact
presentation request to the existing safety/headroom observer. Any later
active UAT would still require separately bounded human, device, and failure
testing.

## P. Rationale

Both captures show useful software headroom before protection would need to
intervene. Through 1.30x, neither strategy approaches the normalized boundary;
through 1.60x, neither clips and sustained exposure remains moderate. A dynamic
protection model is therefore not the immediate blocker.

The more important unresolved choice is how presentation should preserve
information. Pre-tanh scaling trades peaks for nonlinear compression;
post-tanh scaling preserves proportionality but consumes transient reserve;
aggregate scaling cannot explicitly protect impact or detail. Establishing the
presentation interface and its calibration evidence is the necessary next
step before choosing gain, allocating channels, or designing protection.
