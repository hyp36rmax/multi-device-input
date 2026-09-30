# Telemetry

The telemetry probe records one row from the hooked player-car update, normally
at OutRun's 60 Hz simulation cadence. It exists to connect native observations,
semantic state, force decisions, presentation, and the final software request
without guessing what reached hardware.

Telemetry is developer instrumentation. It does not alter force behavior and
its output values are not measured wheel torque.

The current 254-column append-only schema is
`HYP36R_RESEARCH_II_R1_PLAYER_SURFACE_V8`. It preserves the complete 253-column
Surface Amplitude V5 prefix, which preserves the complete 240-column Surface Renderer V4 prefix and the complete 233-column
User Configuration V3 prefix, which preserves the complete 228-column
Road Live V2 prefix, which preserves the complete 222-column
`HYP36R_RESEARCH_II_R1` prefix. The original neutral surface, corner-field, restored-vibration, gear,
Force Character and output additions are defined in
[HYP36R_FORCE_RESEARCH_II_R1.md](HYP36R_FORCE_RESEARCH_II_R1.md).

## Capturing a run

Add a single developer section to `OutRun2006Tweaks.user.ini`:

```ini
[Developer]
TelemetryEnabled = true
TelemetryTestScenario = descriptive_scenario_name
TelemetryNotes = car; route; wheel; hardware strength; game strength
```

The Debug tab provides **Start New Capture** and **Stop Capture**. Each capture
creates a unique CSV and session record under
`HYP36R/Research/<sanitized-scenario>/` beside the game. Rows are buffered and
flushed every 120 samples and again on normal shutdown. No research directory
or file is created while telemetry is disabled.

The file begins with metadata comments for probe version, tweaks version, game
executable timestamp, local start time, scenario, and notes. Use one file per
controlled scenario. If capture controls are missing, check first for duplicate
`[Developer]` sections or duplicate telemetry keys; that configuration error
previously looked like a UI defect.

Capture metadata records both the authoritative native `car_id` and its
human-readable `car_name`. The centralized, physically verified mapping is
documented in [CAR_ID_MAPPING.md](CAR_ID_MAPPING.md).

## Authoritative routing fields

The schema is append-only, so older research fields remain present. For the
current R1 schema, use this chain when answering what reached the wheel:

| Stage | Authoritative field |
| --- | --- |
| Legacy directional subtotal | `legacy_directional_component` |
| M4C unloaded directional | `m4c_unloaded_directional` |
| M4F restored directional | `bite_shadow_directional` |
| M4 unloading before restoration | `bite_shadow_current_m4c_unloading` |
| M4 unloading after restoration | `bite_shadow_unloading` |
| M5 mode and candidate selection | `m5j_mode`, `m5j_selected_directional` |
| M5 contribution applied by its experiment | `m5j_applied_modulation` |
| Presentation mode | `s9_mode` |
| Directional value before Presence | `s9_directional_pre_presence` |
| Directional value after Presence | `s9_directional_post_presence` |
| Final directional value selected for composition | `s9_hardware_directional_selected` |
| Directional + road + impact before `tanh` | `s2_composer_input` |
| Post-`tanh` value | `s2_post_tanh` |
| Normalized value passed to `drive()` | `ffb_raw` |
| Request after strength, inversion, and clamp | `ffb_final` |

## Live Enhanced Road calibration

The six fields appended by `HYP36R_RESEARCH_II_R1_ROAD_LIVE_V2` are:

```text
road_calibration_gain
road_pre_calibration
road_post_calibration
road_post_safety_ceiling
road_ceiling_active
road_slew_limiter_active
```

User Configuration V3 appends five fields after that unchanged 228-column
prefix:

```text
user_ffb_strength_percent
user_steering_load_percent
user_road_detail_percent
user_impact_percent
user_road_mode
```

These are presentation values from the same conversion used by the player FFB
menu and update on every sample. The older `steering_load_percent`,
`road_detail_percent`, and `impact_percent` fields remain canonical engineering
gains in hundredths; they are not reinterpreted or renamed.

Surface Renderer V4 appends seven research fields:

```text
road_renderer
surface_source
surface_requested_magnitude
surface_bounded_magnitude
surface_frequency_hz
surface_effect_active
surface_capability
```

These fields originally described the isolated experimental periodic renderer.
Their historical meaning is unchanged. Current Player Surface runs alongside
Directional Road and uses the shared calibrated pre-directional-conditioning
source for its independently bounded periodic effect.

Surface Amplitude V5 appends `surface_strength_percent` and
`surface_amplitude_ceiling_percent`. Both are recorded per sample so a live
research adjustment is unambiguous without restarting the capture.

Surface Controls V6 appends `surface_waveform` and
`surface_frequency_profile`. Together with the existing live Hz field, these
identify the selected periodic topology and speed-responsive profile for every
sample without reconstructing either from UI state.

Surface Bump V7 appends nine fields describing the independent transient path:
`surface_bump_transient_metric`, `surface_bump_threshold`,
`surface_bump_candidate`, `surface_bump_triggered`,
`surface_bump_requested_magnitude`, `surface_bump_bounded_magnitude`,
`surface_bump_effect_active`, `surface_bump_duration_ms`, and
`surface_bump_cooldown_remaining_s`. The existing `road_post_calibration` field
is the shared calibrated branch source; Road continues through its directional
ceiling and slew fields while Surface now consumes that shared source directly.

Player Surface V8 appends `user_surface_percent`, sourced from the same
persisted 0–100 player setting displayed by the Force Feedback menu. Normal
Enhanced Road resolves to the centralized 30x calibration; historical schemas
and captures retain their original calibration meaning.

`surface_amplitude_ceiling_percent` remains the authoritative per-sample
Texture envelope. With no research override it records 12 for Classic and 18
for Enhanced. A deliberate Debug/UAT override records its selected 12, 18 or
25 value. The Bump fields keep the same mode-independent calibration.

They are copied from the same per-frame calibration selection and
`HYP36RRoad2Active::GainFrame` used by the hardware path. They are observations,
not offline reconstructions. The complete Road lineage is:

```text
road_pre_gain
  -> Road Detail player scaling
road_pre_calibration
  -> normal Enhanced x30, or active Debug x8/x10/x15/x20/x25/x30 override
road_post_calibration
  -> +/-0.25 Road safety ceiling
road_post_safety_ceiling
  -> 0.90/second Road slew limiter
road_post_gain
  -> directional_post_gain + impact_post_gain
composer_pre_tanh
  -> tanh
composer_post_tanh
  -> output_ramp
force_pre_drive / ffb_raw
  -> master strength + inversion
directinput_unclamped_request
  -> DirectInput clamp
ffb_final
```

`road_ceiling_active` and `road_slew_limiter_active` identify intervention in
their respective stages. `road_calibration_gain` is recorded on every sample,
so changing Debug Road Detail Authority during one capture is visible without
restarting telemetry.

`hardware_selected_directional` remains the correct pre-presentation M4/M5
routing field. Once S9 presentation is enabled,
`s9_hardware_directional_selected` is the final directional authority.

## Field families

### Session and base output

`timestamp`, `frame`, `elapsed_time`, `speed`, `steering_input`, the four
surface fields, `ffb_raw`, `ffb_final`, and `ffb_master` provide the common
timeline. `xforce` intentionally remains empty because no observed field has
been proven equivalent to Howard Casto's historical X-Force signal.

### Native steering response

`candidate_D38` through `candidate_D48` preserve the raw research values.
`state_validity`, `steering_reference_rad`, `response_angle_rad`,
`response_rate_rad_s`, `reference_response_error_rad`,
`corrected_reference_rad`, `response_authority`, `overshoot_attenuation`, and
the transition/rate-validity fields are the interpreted state. The synthetic
lateral-speed, slip-ratio, and grip-loss columns remain explicitly separate.

### M4 grip envelope

`composer_*` and `intent_*` describe native availability, phase, and the first
Force 2.0 intent. The `bite_*` fields explain candidate BITE and whether
reconvergence is vehicle-led. `bite_shadow_*` records the restoration state,
rate, limiter, and abort path. Use the authoritative directional and unloading
fields in the table above for routing conclusions.

### Native four-corner and M5

The raw corner fields record `+0x28`, `+0xAC`, and `+0xB0` for four corners.
`fc_*` fields contain the conditioned FL/FR/RL/RR and aggregate context.
`m5_intent_*` describes contextual interpretation. `m5i_*` records the
prospective lateral-context contribution and its gates. `m5j_*` records the
controlled active selection.

The corner units remain unknown. Normalized M5 values are research context,
not tire-force or grip percentages.

### Software exposure

`s2_*` records instantaneous magnitude, recent peak, one- and three-second
RMS, occupancy at several normalized thresholds, output slew, and boundary
flags. These fields measure normalized software exposure. They say nothing
directly about torque, motor current, temperature, fatigue, or a wheel's
protection threshold.

### Presentation

`presentation_*` records the Reference profile, Presence, Contrast, M4 primary,
raw/requested/permitted M5 secondary, road, impact, vibration, budget and
Legacy-boundary intervention, fallback, and descriptive software region.
`s9_*` then proves the active hardware selection from that same policy frame.

## Historical fields that can mislead

The M4G Run #57 capture appeared to show that BITE restoration was calculated
but not routed to hardware. The force path was correct. The misleading fields,
`active_directional_component` and `active_unloading_applied`, described the
earlier M4C composer stage rather than the later M4F selection. M4G-R1 added
`m4c_unloaded_directional`, `hardware_selected_directional`, and
`hardware_selected_unloading` so the route could be proven directly. The old
columns remain for compatibility, not authority.

M5J produced a different observability lesson. The first intended active A/B
captures recorded `m5j_mode = m4_only` throughout even though the M5I shadow
calculation was active. This time telemetry exposed a real mode-selection
problem: the developer setting was resolved before the effective INI values
were loaded. M5J-R1 moved resolution to the initialized settings path and added
a startup mode diagnostic. A scenario label never proves a mode was active;
the authoritative mode and route fields do.

Before S9, `hardware_selected_directional` was the final directional selection.
S9 added a later presentation stage, so analyses of current captures must use
the `s9_*` selector before reconstructing `s2_composer_input` and `ffb_raw`.

## Reading a capture safely

Start with metadata and confirm the requested scenario, car, route, hardware,
and strength. Then verify the recorded mode fields instead of trusting the
filename. Trace the authoritative routing chain in order. Check finite values,
sign preservation, zero-origin behavior, M5 budget and Legacy authority, FREE
and BITE vetoes, fallback state, and the final composition equation.

Compare controlled runs only when car, route, device settings, game strength,
and driving sequence are reasonably matched. Keep physical observations beside
the capture; telemetry can prove routing and software exposure, but it cannot
substitute for what the driver or hardware experienced.

The detailed historical schema remains in
[telemetry_probe.md](telemetry_probe.md), and the controlled driving procedures
remain in [TELEMETRY_TEST_PROTOCOL.md](TELEMETRY_TEST_PROTOCOL.md).

## Regular Telemetry and Guided UAT

Regular Telemetry is the stable, general-purpose recorder. Enabling telemetry
automatically shows its compact overlay; a freeform capture starts immediately,
has no time limit, and retains Scenario and Notes. Its
`HYP36R_RESEARCH_II_R1_PLAYER_SURFACE_V8` is 254 columns and retains the
earlier 253-column schema as an unchanged prefix.

Guided UAT is a separate experience built on the same recorder. The first
protocol, `UAT_ROAD_CALIBRATION_SWEEP_V1`, measures ×8, ×10, ×15, ×20, ×25 and
×30 in order. Each stage validates the authoritative runtime multiplier, gives
the tester a 15-second warm-up, records a 36-second measured window, and stores
configuration mismatch and subjective assessment information in session
metadata. Warm-up is excluded from measured data. Guided UAT temporarily owns
the visible overlay and never starts a second recorder.
