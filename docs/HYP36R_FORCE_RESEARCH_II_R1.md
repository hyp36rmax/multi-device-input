# HYP36R Force Research II — R1 passive observation schema

## Boundary

R1 starts from R0 commit `67b6f2ba38ae81c41310b7bdb1c4d654c3c7b6af`
on `multi-device-input`. Released v1.0.0 remains commit
`8d67c16792a386d5c3a03d42f2316115bb516abc`.

This is developer instrumentation only. `TelemetryEnabled` remains false by
default. No observation is read by HYP36R Force, presentation, Road, Impact,
device selection, or DirectInput output.

Schema identity is `HYP36R_RESEARCH_II_R1`. All S9 columns retain their names
and meaning; R1 columns are appended.

## Added observations

### Surface and native corner state

- `surface0_previous` through `surface3_previous`
- `surface0_changed` through `surface3_changed`
- `corner0_field14` through `corner3_field14`
- `corner0_fieldE8` through `corner3_fieldE8`
- `corner0_fieldEC` through `corner3_fieldEC`
- `corner0_fieldEE` through `corner3_fieldEE`

The four existing `surface_0..3` values remain authoritative current raw
classifications. R1 does not assign wheel positions or material identities.
The new corner names are deliberately tied to structure offsets. `fieldE8` is
accepted only when finite; the existing exact pointer-topology check still
guards every corner read. A failed prerequisite leaves the complete corner
group empty.

### Restored native PC effect path and gear event

- `vibration_left_raw`, `vibration_right_raw`, `vibration_combined_raw`
- `vibration_rise`
- `gear_current`, `gear_previous_native`, `gear_transition`

These values show what the restored C2C vibration routine supplied before
HYP36R Road and Impact interpretation. `gear_transition` records the same
`cur_gear_208 != dword1D8` comparison used by that routine. It does not add a
new shift effect or alter its strength.

### Force Character and composition

- `directional_pre_gain`, `road_pre_gain`, `impact_pre_gain`
- `directional_post_gain`, `road_post_gain`, `impact_post_gain`
- `steering_load_percent`, `road_detail_percent`, `impact_percent`
- `output_ramp`, `composer_pre_tanh`, `composer_post_tanh`
- `force_pre_drive`, `ffb_invert_enabled`
- `directinput_unclamped_request`

The pre-gain directional value is the already resolved Reference/Reference+
selection. The three post-gain channels are the exact values summed by the
active composer. Therefore current output can be reconstructed as:

```text
composer_pre_tanh = directional_post_gain
                  + road_post_gain
                  + impact_post_gain

composer_post_tanh = tanh(composer_pre_tanh)
force_pre_drive = composer_post_tanh * output_ramp

directinput_unclamped_request =
    (ffb_invert_enabled ? -force_pre_drive : force_pre_drive) * ffb_master

ffb_final = clamp(directinput_unclamped_request, -1, 1)
```

Existing presentation columns retain Reference+ identity, Presence 1.44,
Contrast, M4 primary, M5 secondary request/budget and fallback state. Existing
vehicle, LOAD/RELEASE/FREE/BITE, M4/M5, exposure and final-request fields are
unchanged.

No separate “pre-presentation composed force” is added because the active
architecture applies presentation to the directional channel before channel
composition. Inventing such a stage would not describe a real runtime value.

## Offline replay readiness

`research_telemetry_replay_test` proves:

- identity reconstruction from logged post-gain channels through `tanh` and
  output ramp;
- independent Steering, Road and Impact counterfactual gains from 1.00 through
  2.00 without changing raw channel inputs;
- an all-channel counterfactual sweep;
- four independent surface-change flags without spatial collapse; and
- finite replay output across the requested candidate sweep.

This is dataset readiness, not a ceiling or safety decision. Physical gain is
never emitted by the replay helper.

## Performance and failure behavior

Rows remain buffered and flush every 120 samples. No per-row disk flush or
offline analysis was added. Session-close logging reports sample count,
effective frequency, maximum observed sample gap, and CSV write failures.
Write failure never enters the Force path.

Telemetry-off execution returns before session creation, string formatting,
or CSV work. The added row values already exist in the active update, apart
from validated corner members read at the existing four-corner observation
site while telemetry research is active.

## Safety and next gate

R1 adds no hook. It uses the existing player-car telemetry call and final
`drive()` observer. It does not own or modify Force output. Existing finite
guards, exact four-corner pointer validation, supported executable boundary,
and disabled-by-default setting remain in force.

R2 may begin only after Windows CI is green and the controlled research
artifact proves the schema in a short non-campaign capture. The 15-scenario
physical campaign is not part of R1.
