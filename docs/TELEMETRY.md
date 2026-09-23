# Telemetry

The telemetry probe records one row from the hooked player-car update, normally
at OutRun's 60 Hz simulation cadence. It exists to connect native observations,
semantic state, force decisions, presentation, and the final software request
without guessing what reached hardware.

Telemetry is developer instrumentation. It does not alter force behavior and
its output values are not measured wheel torque.

The current append-only schema is `HYP36R_RESEARCH_II_R1`. Its neutral surface,
corner-field, restored-vibration, gear, Force Character and output additions
are defined in
[HYP36R_FORCE_RESEARCH_II_R1.md](HYP36R_FORCE_RESEARCH_II_R1.md).

## Capturing a run

Add a single developer section to `OutRun2006Tweaks.user.ini`:

```ini
[Developer]
TelemetryEnabled = true
TelemetryTestScenario = descriptive_scenario_name
TelemetryNotes = car; route; wheel; hardware strength; game strength
```

The Debug tab provides **Start New Capture** and **Stop Capture**. Each new file
is named `telemetry_YYYYMMDD_HHMMSS.csv` beside `dinput8.dll`. Rows are buffered
and flushed every 120 samples and again on normal shutdown.

The file begins with metadata comments for probe version, tweaks version, game
executable timestamp, local start time, scenario, and notes. Use one file per
controlled scenario. If capture controls are missing, check first for duplicate
`[Developer]` sections or duplicate telemetry keys; that configuration error
previously looked like a UI defect.

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
