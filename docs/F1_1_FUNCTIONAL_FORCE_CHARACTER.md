# F1.1: Force Character without changing Force interpretation

F1 was physically stable on the restored Run #78 / Reference+ lineage. Its three visible Force Character controls still changed the older force-generation tunings. F1.1 corrects that placement: the model and its inputs stay untouched, and these controls now scale resolved output channels only. The 100% setting is identity. Unlock/save work remains deferred.

## What the stable build actually produces

| Player channel | Existing source and last independent value | Composition and output |
| --- | --- | --- |
| Steering Load | Game steering, speed and vehicle state feed Legacy, M4/BITE and optional M5. `HYP36RPresentation::evaluate()` selects `presentation.hardwareDirectionalSelected`. | Gain is applied to that selected directional value, after all interpretation and eligibility decisions. |
| Road Detail | The game's right vibration motor is rendered by the existing six-hertz road carrier, with the existing `WheelFFBRoadStrength` tuning and impact-edge suppression. The independent value is `road`. | Gain is applied to `road`, after its generation. It does not create new surface information. |
| Impact | A rise in the game's vibration signal triggers the existing decaying `impactForce`, with the existing `WheelFFBImpactStrength` tuning. The independent value is `impact`. | Gain is applied to `impact`, after trigger/timing logic. |
| Vibration | `vibration = max(VibrationLeftMotor, VibrationRightMotor)` is an observed game signal. It is used to generate road and impacts and is also passed to the game's Xbox vibration path. | There is **no independent wheel-vibration force** after road and impact are formed. The wheel uses one composed DirectInput constant force. F1.1-R1 removes the nonfunctional player slider; this remains a documented backlog item, not a fourth independent gain. |

Actual ordering is game/native observations → existing Legacy/M4/M5 and road/impact generation → Reference+ directional selection → three independent Force Character gains → existing `tanh(directional + impact + road)` → existing output ramp → `WheelForceFeedback::drive()` → existing inversion and master Strength → DirectInput nominal clamp/request. The old force model, Reference+ equations, road/impact timing, ramp, inversion and device code are not changed by the channel feature. The S2 observer and telemetry continue to see the final selected directional and composed request on their established paths.

## Player limits and persistence

Master Strength is now 0–100%, with a 100% fresh-install default; the old 70% default and 150% normal-UI ceiling were inconsistent with this milestone. Its implementation remains the existing global DirectInput scaling. Steering Load, Road Detail and Impact each persist independently under `[Controls]`, default to 100%, and clamp to 0–100%. Missing keys retain 100%; malformed numeric text leaves the existing/default value through the settings reader. The software replay/headroom studies in S3/S5 concern global presentation and do not prove safe *per-channel* amplification. Accordingly, 110/120/130/140/150% test inputs clamp to 100% and no channel overdrive is shipped yet. Normalized software limits do not establish physical wheel torque.

Reset restores the three functional presentation gains, master Strength, Invert Wheel Off and a legacy disabled-FFB override. An explicit Developer profile override is left alone; on a fresh install Reference+ is already the default. Controller, telemetry and other Developer settings are untouched.

## Regression evidence

The standalone Force Character test runs in Win32 Release CI but is excluded from the six-file player package. It checks 100% exact channel identity and the corresponding composed `tanh` result; one-at-a-time 0% removal with other channels unchanged; 50% scaling; 75/100/110/120/130/140/150% replay; out-of-range clamping; finite output; sign preservation; and default percentages. Its Vibration assertion records that no independent wheel channel exists. The source diff confines runtime force changes to a three-channel multiplier after presentation selection. At 100% the multiplier is 1, so Reference+ reaches the existing composition unchanged. The left/right diagnostic tests do not use these gains and retain their 20% request cap.
