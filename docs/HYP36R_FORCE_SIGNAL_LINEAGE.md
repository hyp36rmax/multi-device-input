# HYP36R Force signal lineage

## HYP36R Force overview

HYP36R Force is the project's independent force-feedback interpretation for
OutRun 2006. It runs once per hooked player-car update in
`Vibration::GamePlCar_Ctrl_Hook`, combines input state, vehicle motion and the
restored Xbox vibration result, then sends one normalized constant-force request
to `WheelForceFeedback::drive`.

The current model does not read a confirmed native steering-torque value. Its
main steering load is derived from controller input, vehicle velocity and the
car orientation matrix. Road and impact cues use the output of the restored
Xbox `CalcVibrationValues` routine, which is itself a composite calculated from
multiple values in `EVWORK_CAR`.

## Signal lineage

Unless a row names another function, its variables and transformations are in
`src/hooks_forcefeedback.cpp::Vibration::GamePlCar_Ctrl_Hook`. The table keeps
OutRun-provided state, project input state, derived estimates and synthetic
force terms separate even when they occur in the same function.

| Signal | Source and immediate data | Classification | Transformation and purpose | X-Force relevance | Confidence |
| --- | --- | --- | --- | --- | --- |
| `steering` | `InputManager_SteeringValue()` in `GamePlCar_Ctrl_Hook`; ultimately `InputManager::GetVolume(ADChannel::Steering) / 127.0f` | Observed game state: normalized input-layer state, not vehicle physics | Clamped to `[-1, 1]`. Represents the player's current steering command after the input system's binding and calibration path. | It may correlate with a steering-force signal because both respond to steering, but it is an input command rather than native force. | Confirmed |
| `steeringDelta` | Current `steering - previousSteering` | Derived from observed game state | Per-update change in normalized steering input. Used as the damping input. It is not time-normalized or a native steering velocity. | Could reproduce some steering-rate behavior associated with a force signal, but contains no native force information. | Confirmed |
| `speed` | `car->spd_mb_20` in `GamePlCar_Ctrl_Hook` | Derived from observed game state | `sqrt(x² + y² + z²)`. Magnitude of OutRun's native three-component motion value. Its physical unit is not established. | Vehicle speed would normally influence steering force, but this is supporting state rather than X-Force itself. | Strongly inferred |
| `normalizedSpeed` | Derived from `speed` | Derived from observed game state | `clamp(speed / 1.5, 0, 1)`. Places the observed motion magnitude on the model's working scale. | No direct relationship; it shapes other force terms. | Confirmed |
| `centeringAuthority` (`authority` in diagnostics) | Derived from `normalizedSpeed` | Synthetic FFB behavior | `min(1, normalizedSpeed / 0.25) * (0.35 + 0.65 * normalizedSpeed)`. Fades centering in with vehicle speed. | It may imitate the speed-dependent behavior of a native steering-force signal, but it is created entirely by HYP36R Force. | Confirmed |
| `lateralSpeed` | Dot product of `car->spd_mb_20` and the first row of `car->matrix_70` | Derived from observed game state | `spd.x * _11 + spd.y * _12 + spd.z * _13`. Intended to isolate motion along the car's presumed lateral basis. | It could contain vehicle-state information similar to inputs behind a native steering-force calculation. It is not a force value. | Strongly inferred; the basis interpretation still needs physical validation |
| `slipRatio` (`slip` in diagnostics) | Derived from `lateralSpeed` and `speed` | Derived from observed game state | When `speed > 0.02`, `clamp(abs(lateralSpeed) / speed, 0, 1)`; otherwise zero. Intended as a stable body-slip proxy. | It can reproduce some loss-of-grip behavior that X-Force may have reflected, but it is independently derived kinematics. | Strongly inferred |
| `gripLoss` | Derived from `slipRatio` | Derived from observed game state | `clamp((slipRatio - 0.08) / 0.55, 0, 1)`. Converts the slip proxy into a progressive unloading amount. | Behavioral resemblance is possible, but this threshold curve is HYP36R Force logic. | Confirmed |
| `gripScale` | `gripLoss` and `WheelFFBGripLossStrength` | Synthetic FFB behavior plus configuration | `1 - gripLoss * WheelFFBGripLossStrength`. Reduces the centering term during a slide. | May imitate force unloading observed historically, but it is not native telemetry. | Confirmed |
| `spring` | `steering`, `WheelFFBSpringStrength`, `centeringAuthority`, `gripScale` | Synthetic FFB behavior | `-steering * springStrength * centeringAuthority * gripScale`. Provides the model's speed-sensitive centering backbone. | This is the closest HYP36R Force component in purpose to an aligning-force signal, but it is fully composed by this project. | Confirmed |
| `damper` | `steeringDelta` and `WheelFFBDamperStrength` | Synthetic FFB behavior | `-steeringDelta * damperStrength * 4`. Opposes rapid changes in steering input. | It could resemble a rate-sensitive part of native steering force, but it is synthetic. | Confirmed |
| `VibrationLeftMotor`, `VibrationRightMotor` | Written by `CalcVibrationValues(car)` restored from the Xbox C2C routine at `0x114C60` | Derived from observed game state / synthetic gamepad vibration | The restored routine reads many `EVWORK_CAR` values, including speed, state flags, four `water_flag_24C` entries, gear-related state and anonymous fields such as offsets `0x1D0`, `0x1D4`, `0x1DC`, `0x1E0`, `0x1E4`, `0x264` and `0x268`. It produces the two gamepad-motor levels. | This is the only current path that consumes several anonymous internal driving-state fields. One or more may overlap information Howard investigated, but the composite motor outputs are not established as X-Force. | Confirmed as an Xbox vibration calculation; individual anonymous field meanings are unknown |
| `vibration` | `max(VibrationLeftMotor, VibrationRightMotor)` | Derived from observed game state | Clamped to `[0, 1]`. Used only to detect a sudden rise for an impact cue. | It may carry indirect information from a steering-force-related field used inside the Xbox routine, but that cannot be separated from its other inputs. | Possible |
| `vibrationRise` | Current `vibration - previousVibration` | Derived from observed game state | `max(0, vibration - previousVibration)`. A rise above `0.12` triggers the HYP36R impact envelope. | No evidence of direct equivalence. It is a change detector over a composite rumble signal. | Confirmed |
| `impactDirection` | Current `steering`, otherwise alternating stored sign | Synthetic FFB behavior | Opposes steering when `abs(steering) > 0.05`; alternates at center. Prevents repeated centered impacts from creating one directional bias. | None as a native signal. | Confirmed |
| `impactForce` / `impact` | `vibrationRise`, `vibration`, `WheelFFBImpactStrength`, `impactDirection` | Synthetic FFB behavior | On a qualifying rise: `direction * impactStrength * min(0.55, rise * 0.65 + vibration * 0.20)`. The stored value decays by multiplying by `0.90` each update. | Its trigger can indirectly contain native game-state information through Xbox vibration, but the wheel impulse is HYP36R Force behavior. | Confirmed |
| `roadCarrier` | `steady_clock` time | Synthetic FFB behavior | `sin(seconds * 37.6991118)`, a 6 Hz carrier. | None. | Confirmed |
| `road` | `VibrationRightMotor`, `WheelFFBRoadStrength`, `roadCarrier` | Synthetic FFB behavior driven by a derived game signal | When `vibrationRise <= 0.12`: `clamp(rightMotor, 0, 1) * roadStrength * 0.25 * roadCarrier`; otherwise zero for that update. Intended to turn the lighter Xbox vibration channel into wheel texture without masking impact onset. | The envelope may indirectly reflect native surface or vehicle-state inputs, but the 6 Hz wheel texture is synthetic. | Confirmed |
| `outputRamp` | Persistent hook-local state and update cadence | Synthetic FFB behavior | Reset to zero after a gap over 500 ms, then increased by `1/30` per hooked update to a maximum of one. Prevents an abrupt return to full force. | None. | Confirmed |
| `force` | `spring + damper + impact + road`, then `outputRamp` | Synthetic FFB behavior: HYP36R Force output | `tanh(spring + damper + impact + road) * outputRamp`. `tanh` provides soft saturation before the device layer. | It may feel similar to behavior Howard observed, but it is an independent composition and must not be called X-Force. | Confirmed |
| `WheelFFBStrength` | `Settings::WheelFFBStrength` in `wheel_force_feedback.cpp` | Configuration/user input | Integer percent from `0` to `150`; current fresh-install default is `70`. It scales the normalized HYP36R Force request at the DirectInput boundary. | None. | Confirmed |
| `magnitude` / final DirectInput requested output | `WheelForceFeedback::drive(float normalizedForce)` | Synthetic FFB behavior plus configuration | Optional inversion is applied first. Then `LONG(normalizedForce * WheelFFBStrength * 100)` is clamped to `[-10000, 10000]`. Two-axis effects encode sign as polar direction and use absolute magnitude; the one-axis fallback uses signed Cartesian magnitude. | This is the final device command derived from HYP36R Force, not a native OutRun or X-Force value. | Confirmed |
| `xforce` in TP-01 | No source assigned | Unknown / needs validation | Left unavailable and emitted as an empty CSV cell. No current field is relabelled as X-Force. | This is the explicit future observation target. | Unknown |

## Current force equation

The current composition is:

```text
normalized controller steering ───────────────┐
                                               ├─ synthetic spring
native spd_mb_20 ── speed ── normalizedSpeed ─┤
                                               │
native spd_mb_20 + matrix_70 ── slipRatio ─────┘
                                      │
                                      └─ gripLoss ── gripScale

steering change ───────────────────────── synthetic damper

EVWORK_CAR ── restored Xbox CalcVibrationValues
                 ├─ vibration rise ─────── synthetic impact
                 └─ right motor envelope ─ synthetic 6 Hz road texture

HYP36R Force raw = tanh(spring + damper + impact + road) * outputRamp

final requested = clamp(
    optional_invert(HYP36R Force raw) * WheelFFBStrength * 100,
    -10000,
    10000)
```

The DirectInput layer submits that final request through a constant-force
effect. Compatible two-axis devices are updated normally. The single-axis
compatibility path recreates the constant-force effect at approximately 15 Hz.
Neither path changes the force equation.

## Native versus derived state

### Directly observed from OutRun or the input layer

- `car->spd_mb_20`: native three-component vehicle motion value.
- `car->matrix_70`: native car transformation/orientation matrix.
- `InputManager::GetVolume(ADChannel::Steering)`: the project's current
  normalized steering-input path.
- `EVWORK_CAR` fields consumed by the restored Xbox vibration routine,
  including the four neutral `water_flag_24C` indexes.

### Derived by HYP36R Force

- speed magnitude and normalized speed;
- lateral-speed projection and slip ratio;
- grip-loss amount and grip scale;
- vibration-rise detection.

### Synthetic force behavior

- speed-dependent centering authority;
- spring and damper terms;
- impact direction, magnitude and decay;
- 6 Hz road carrier;
- output ramp and `tanh` soft saturation;
- inversion, master scaling and DirectInput clamping.

No current HYP36R Force variable is a confirmed native steering torque, tire
force, aligning torque or Howard Casto X-Force value.

## Howard Casto historical context

Howard Casto's early OutRun 2006 reverse-engineering work demonstrated that the
game exposes meaningful steering-force and vehicle-state information beyond the
original PC input implementation. His published experimentation, including the
signal he described as “X-Force,” inspired further investigation into OutRun's
internal driving state. HYP36R Force is an independent implementation and does
not assume its derived force model is equivalent to Howard's X-Force.

The answer from the current code is **partially, with insufficient evidence of
identity**. HYP36R Force already derives force-relevant behavior from native
vehicle motion and orientation. Its restored Xbox vibration dependency also
reads anonymous internal fields that may contain related driving-state
information. However, the main spring, damping and grip behavior is constructed
by HYP36R Force, and no field has been connected to Howard's historical
X-Force signal.

## Open questions

- Validate the physical axis and scale represented by `spd_mb_20`.
- Validate that the first row of `matrix_70` is consistently the lateral basis
  across cars, stages and direction changes.
- Establish what the anonymous Xbox vibration inputs at `0x1D0`, `0x1D4`,
  `0x1DC`, `0x1E0`, `0x1E4`, `0x264` and `0x268` represent.
- Determine which native inputs cause each Xbox motor channel to change during
  steering, surface transitions, collisions and sliding.
- Compare TP-01 observations with Howard Casto's published X-Force behavior and
  test conditions before proposing any field relationship.
- Distinguish final requested DirectInput output from updates actually accepted
  by different wheel drivers; the current telemetry records the request, not
  measured wheel torque.
- Keep `water_flag_24C[0..3]` neutral until physical testing establishes their
  ordering.
