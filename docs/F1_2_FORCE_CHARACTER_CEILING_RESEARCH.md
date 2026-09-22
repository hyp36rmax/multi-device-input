# F1.2: Force Character ceiling research

Starting point: `33bd3cf90aac53099856fb611429d5209ca9e0b6` (Run #106), branch `multi-device-input`. This is offline research. No player control, force equation, or device output was changed. Reference+ Presence stays at 1.44 and Strength stays at 100% for the reference condition.

## What is being scaled

Steering Load multiplies `presentation.hardwareDirectionalSelected`, after M4, BITE, optional M5, Reference+ selection and Presence 1.44. Road Detail multiplies the existing `road` term, derived from the game's right vibration motor through the existing six-hertz carrier and impact-edge suppression. Impact multiplies the existing vibration-rise-triggered, decaying `impact` term. `HYP36RForceCharacter::apply()` scales these three resolved terms independently. The selected request is `tanh(steering + road + impact) * outputRamp`. `WheelForceFeedback::drive()` then applies optional inversion, master Strength and the DirectInput nominal clamp. There is no independent wheel vibration channel. A channel gain cannot change M4/M5/BITE decisions, create a road effect, or alter an impact trigger, though changing the mixed output can change its sign near cancellation.

## Evidence and method

The synchronized capture `telemetry_20260921_075311 (S9).csv` contains 19,931 frames of Ferrari Dino 246 GTS on Sunny Beach, with road activity on 8,449 frames and impact activity on 6,225 frames. It covers normal, emerging, established and recovering directional phases, BITE and 1,347 M5I shadow-active frames. This was recorded at Presence **1.20**, not 1.44. The observed composer and output reconstruct from the recorded components to maximum absolute errors of 0.00000020 and 0.00000061 respectively. We then counterfactually set directional Presence to 1.44, held each recorded road/impact sample and inferred output ramp fixed, and swept the independent gains. This is an exact replay of the recorded mix under a different Presence, not a physical Run #106 capture. The local, parameterized replay is `research/f12_replay.py`; no private capture is committed.

For additional event coverage, `telemetry_20260919_191133 (M5C).csv` (4,605 frames, known Sunny Beach left-surface/runoff), `telemetry_20260919_164303 (M4GR1).csv` (13,083 frames, BITE restoration), `telemetry_20260920_202708 (S2 M5).csv` (6,715 frames, M5 context), and `telemetry_20260919_185007 (M5BC3).csv` (8,031 frames, combined four-corner contact) were inspected. These earlier captures confirm road/impact events and handling phases, but they precede the S9 presentation fields. They are **not** treated as independent 1.44 channel-output replays or as additional cars, courses, or hardware. The analyzed recordings are Dino/Sunny Beach dominated. No stripe/runoff force was invented.

At gain 1.00 on all channels, the 1.44 counterfactual has output-magnitude P95/P99/max `.439/.535/.643`, 1-second RMS P95/max `.380/.513`, and 3-second RMS P95/max `.327/.412`. Peak normalized software headroom is `.357`. Occupancy at or above `.75`, `.90`, `.98` is zero. The following rows keep the *other* two channels at 1.00. `R3` is maximum 3-second RMS; `>1` is percent of frames whose composer input exceeds unity before `tanh`. These are software measurements, not wheel torque.

| Channel | Gain | Output P95 / P99 / max | R3 max | Frames >=.75 | Pre-`tanh` >1 |
| --- | ---: | ---: | ---: | ---: | ---: |
| Steering | 0 / .50 / .75 | .069/.156/.286; .251/.323/.465; .348/.433/.558 | .092 / .242 / .327 | 0% | 0% |
| Steering | 1.00 / 1.10 / 1.20 / 1.30 | .439/.535/.643; .474/.572/.673; .507/.607/.704; .538/.640/.735 | .412 / .444 / .475 / .504 | 0% | 0% |
| Steering | 1.40 / 1.50 / 1.60 | .569/.671/.764; .599/.700/.789; .626/.727/.812 | .531 / .557 / .581 | .04% / .15% / .54% | .01% / .10% / .35% |
| Steering | 1.75 / 2.00 | .665/.764/.843; .723/.814/.883 | .618 / .673 | 1.47% / 3.81% | 1.05% / 3.28% |
| Road | 0 / .50 / .75 | .440/.534/.643; .438/.534/.643; .438/.534/.643 | .413 / .412 / .412 | 0% | 0% |
| Road | 1.00 / 1.10 / 1.20 / 1.30 | .439/.535/.643; .439/.535/.643; .439/.535/.643; .439/.535/.643 | .412 / .412 / .412 / .412 | 0% | 0% |
| Road | 1.40 / 1.50 / 1.60 | .440/.535/.643; .440/.535/.643; .440/.536/.645 | .412 / .412 / .412 | 0% | 0% |
| Road | 1.75 / 2.00 | .440/.537/.647; .441/.538/.652 | .412 / .412 | 0% | 0% |
| Impact | 0 / .50 / .75 | .417/.503/.600; .427/.517/.602; .433/.525/.618 | .394 / .402 / .407 | 0% | 0% |
| Impact | 1.00 / 1.10 / 1.20 / 1.30 | .439/.535/.643; .442/.538/.657; .445/.542/.670; .448/.547/.682 | .412 / .414 / .417 / .419 | 0% | 0% |
| Impact | 1.40 / 1.50 / 1.60 | .451/.551/.694; .455/.555/.706; .459/.560/.717 | .421 / .425 / .429 | 0% | 0% |
| Impact | 1.75 / 2.00 | .464/.568/.734; .472/.582/.759 | .435 / .444 | 0% / .01% | 0% |

Steering consumes the sustained reserve fastest: at 1.30 its 1-second RMS max is `.620`, at 1.50 `.680`, and at 2.00 `.797`. Road 2.00 raises frames where road amplitude exceeds directional from 1.35% to 2.78%, without showing meaningful global compression in this capture. Impact 2.00 raises frames where impact amplitude exceeds directional from 3.36% to 5.56% and maximum instantaneous output slew from 19.67 to 33.34 normalized units/s; a short transient is not automatically benign. All rows remained finite. Tiny mixed-output sign changes under individual gain changes occurred only around near-cancellation; the resolved directional sign itself is unchanged for every positive steering gain. Zero steering gain intentionally removes that channel. M4, M5, and BITE telemetry and secondary budget are upstream and unchanged by construction. No DirectInput clamp occurs for these replays at Strength 100%; this does not establish hardware behavior.

## Combination and decision

| Simultaneous Steering / Road / Impact | Output P95 / P99 / max | 1s RMS max | 3s RMS max | Frames >=.75 | Pre-`tanh` >1 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 1.00 / 1.00 / 1.00 | .439/.535/.643 | .513 | .412 | 0% | 0% |
| 1.20 / 1.30 / 1.20 | .512/.615/.724 | .591 | .478 | 0% | 0% |
| 1.30 / 1.50 / 1.30 | .546/.651/.759 | .627 | .509 | .02% | 0% |
| 1.30 / 1.50 / 1.50 | .551/.657/.778 | .631 | .512 | .06% | .03% |
| 1.40 / 1.50 / 1.40 | .578/.684/.789 | .660 | .538 | .12% | .09% |
| 1.50 / 2.00 / 2.00 | .621/.729/.851 | .699 | .573 | .63% | .49% |
| 2.00 / 2.00 / 2.00 | .737/.832/.910 | .810 | .683 | 4.33% | 3.90% |

The single-capture software boundary begins around Steering 1.40, where `tanh` input first exceeds unity; all channels at 1.30 do the same on only `.02%` of frames. This is a headroom observation, **not** a tested perceptual failure threshold. Road and Impact have no standalone compression boundary through 2.00 in this drive; Impact slew and dominance nevertheless grow. No normalization to a fixed post-`tanh` ceiling can recover lost relative channel contrast.

| Channel | Recommended | Highest studied | Validated software presentation ceiling | First observed headroom warning |
| --- | ---: | ---: | --- | --- |
| Steering Load | 1.00 | 2.00 | **Not established** | 1.40 in this capture |
| Road Detail | 1.00 | 2.00 | **Not established** | None through 2.00 here; surface diversity lacking |
| Impact | 1.00 | 2.00 | **Not established** | Combined 1.30/1.50/1.50 has first composer-over-unity frames; transient severity unvalidated |

The independent ceiling and aggregate-governor decision cannot responsibly be finalized from one car/course and an older-Presence recording. A provisional research envelope of 1.20 / 1.30 / 1.20 remains within the measured capture's `.75` occupancy and pre-`tanh` thresholds, including all-max, **but is not a validated player ceiling and must not ship as one**. No aggregate governor is indicated for that provisional combination in this capture; this is not option A for a future released maximum. For stronger future ceilings, decide whether all-max can stand without a governor on a broader replay. If not, define an explicit, observable aggregate budget after independent gains and before `tanh`, preserving the resolved directional authority and documenting how road/impact are attenuated. Never silently reduce one slider merely because another moved.

## Future UI and migration

Keep every player slider at 0–100 and Strength at 0–100 with no hidden reserve. Recommended internal gain is exactly 1.00 for each channel. A linear mapping for a subsequently validated channel ceiling `C` would be `gain = UI * C / 100`, with Recommended marker at `100/C` percent. For illustration only, a *hypothetical* ceiling of 1.25 puts the marker at 80%; it is not an F1.2 finding. A two-segment mapping can reserve 0–R for attenuation (`gain = UI/R`) and R–100 for extra emphasis (`gain = 1 + (C-1)*(UI-R)/(100-R)`). It offers precise adjustment near Recommended and an exact reset point. Prefer that simple piecewise mapping with a clearly drawn marker once ceilings and integer marker positions are chosen. With no validated `C`, the three marker UI values and final mapping are **undetermined**; current 100% still means 1.00 and remains the current default.

Future tooltips: “Adjusts steering and cornering load. The marker is the recommended balance; higher settings add emphasis.” “Adjusts existing road-surface feedback. The marker is the recommended balance; higher settings add emphasis.” “Adjusts impact feedback. The marker is the recommended balance; higher settings add emphasis.” These are proposed words, not implemented UI.

Persist a versioned presentation schema, not an unversioned reinterpreted percentage. Treat all existing `[Controls]` values as legacy attenuation, convert `oldPercent / 100` to internal gain, then encode that gain in the new schema. Existing saved 100 becomes exactly 1.00 at the eventual Recommended marker; 50 remains .50, 0 remains zero. Missing old keys inherit 1.00 rather than the new slider maximum. Save the new schema version and round-trip internal gain/marker behavior, including a later downgrade policy, before shipping. Reset should restore the three Recommended internal gains, while Strength remains 100. Do not change config handling in F1.2.

Next research needs synchronized Presence 1.44 captures across more cars and tracks, sustained left/right load, road surfaces and larger as well as mild collisions, plus a measured all-max replay for any candidate tuple. Run #106 broader combined UAT is still incomplete. Normalized software output cannot measure wheel torque, current, temperature or protection margin. Physical tests, if later authorized, must be separate from this offline study. The implementation milestone should wait for those observations and an approved ceiling/governor decision.
