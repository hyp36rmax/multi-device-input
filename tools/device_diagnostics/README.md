# HYP36rforce Device Diagnostics 0.2.0-dev.3

Standalone Windows utility for checking USB input devices, Multi-Input assignments, and native DirectInput force-feedback support. It does not require OutRun 2006 and does not modify game settings.

The application header and About panel show the independent diagnostic version and the seven-character source commit used to compile it. Exported TXT and JSON reports retain the full source commit and the independently versioned report schema.

## Start

Run `HYP36rforce-Device-Diagnostics.exe`, then select **Initialize Devices**. The utility automatically checks WGI, RawInput, DirectInput, and XInput in separate ten-second observation sessions before discovering native DirectInput FFB endpoints. A responsive progress screen identifies the active stage, completed stages, discovered devices, delayed arrivals, and estimated remaining time. Successful initialization offers **Start Quick Setup** or **Not Now**; it never starts capturing automatically. Quick Setup covers seven gameplay bindings: steering, accelerator, brake, separate shift controls, Start/Menu, and Back. Each input gets a six-second detection window with Continue, Retry, Skip, Back, and Cancel. FFB is resolved separately during initialization. After the bindings, an optional single-click check shows a countdown and runs one 1.2-second, 20%-limited shake before asking whether it was felt. Controls from separate USB devices form one diagnostic profile without changing OutRun bindings.

The application checks SDL and native DirectInput independently. The Devices page keeps **Connected Devices** separate from **Input Backend Compatibility** and provides **Re-run Backend Tests**, which uses the same isolated test sequence without replacing the last complete results until the new run finishes. Duplicate-name interfaces retain their VID/PID, SDL instance, or numbered DirectInput interface identity so Fanatec dual interfaces remain distinguishable.

## Force-feedback safety

Motor output is disabled by default. The utility restores a previously selected FFB identity or auto-selects the only valid endpoint; multiple endpoints require confirmation. Enable the visible authorization checkbox, then hold **Test Left**, **Test Right**, or **Hold to Shake**. One shared linear Strength request spans 20–100% (2000–10000 DirectInput nominal units); the separately displayed 20% physical-test ceiling remains enforced pending a dedicated safety review. Shake alternates the same constant-force path at an internal 10 Hz diagnostic rate and uses that shared request. **Invert FFB** reverses all three directions without changing magnitude. Releasing the control, changing focus, disconnecting the device, or reaching the 1.5-second maximum ends the effect. The existing **Re-detect** control stops and releases effects, refreshes SDL input devices and native DirectInput FFB endpoints, reconciles the saved endpoint, and requires fresh movement from the Quick Setup steering assignment.

## Compare FFB Response

**Compare FFB Response** performs a motor-free software assessment, then guides you through legacy effect recreation and persistent `SetParameters` updates using the proven directional constant-force engine. After each bounded test, output stops and the active timer freezes while you answer **Yes**, **No**, or **Unsure**; you may retry only that method before continuing. Zero-magnitude sequence points stop output instead of attempting to create a zero-force effect. Both methods use the same bounded 15 Hz request sequence with a minimum one-second zero-force interval between them. STOP, focus loss, disconnect, watchdog, or unsafe shutdown cancels the active sequence. Results and exports keep DirectInput API evidence separate from the user's physical observation; an unsuccessful method does not mean the wheel is defective.

## Loaded Steering Response

**Loaded Steering Response** is a guided development-only comparison for sustained cornering resistance. It inherits the existing Quick Setup **Steering** assignment and displays its resolved device, axis, live position, and activity without adding another binding or selector. The live position and readiness evidence respond only to that assigned axis; movement must span at least 2500 raw SDL axis units to reject idle noise. Input Test remains the full device monitor, Quick Setup remains the assignment owner, and the independently selected native DirectInput endpoint continues to own FFB output. If the assigned interface changes or becomes unavailable, use **Re-detect** in FFB Test and move the wheel again; the utility never silently substitutes another input.

The tester chooses a held steering direction and selects **START TEST** separately for each method. A six-second motor-off countdown leaves both hands available before the diagnostic repeats the preserved bounded resistance pattern for up to 15 seconds. The request opposes the held turn, ramps gradually to the existing 20% nominal ceiling, holds briefly, makes one small gradual reduction/restoration, and ramps back to zero. It never reverses the requested direction.

**Reset Test** safely stops and releases any active effect, clears both method results and observations, and returns the workflow to Test 1 of 2. It preserves a still-valid Quick Setup steering assignment, the selected FFB endpoint, and held-turn direction, but never starts force automatically; Legacy still requires a fresh **START TEST** authorization.

The original Logitech RS50 V8 CSV is not stored in this repository, so P11-E retains P11-B's explicitly labeled conservative synthetic fixture rather than claiming telemetry provenance. Legacy recreation and Persistent dynamic receive the exact same 21 requests at the same 66 ms target interval, separated by at least one second of zero force and requiring separate authorization. After each method, the tester records Yes/No/Unsure, any combination of Smooth/Pulsing/Flutter/Chatter/Clicking/Other/Unsure, and when the sensation occurred. The three visible **Unsure** controls are independent; timing Unsure remains explicit evidence and mutually exclusive with specific phases. These observations are not torque measurements and the test does not claim to reproduce tire grip loss.

## Reports

**Export Report** writes paired `.txt` and `.json` files to the resolved Windows `Documents\HYP36rforce Device Diagnostics\Exports` folder, including redirected Documents/OneDrive locations. Filenames use the Windows-reported selected wheel name and local timestamp, with `_02`, `_03`, and later suffixes preventing overwrites. The application confirms the filename and provides **Open Exports Folder**. Reports stay local. Diagnostic input assignments include the SDL GUID, VID/PID, available device serial/path, and deterministic duplicate occurrence needed to explain reconnection behavior; reports are never uploaded automatically.

## M10 steering and directional compatibility

Quick Setup now treats completion of the guided workflow separately from its seven binding outcomes. Each step is reported as assigned, skipped, or unresolved, while FFB readiness remains independent. Steering accepts meaningful movement above a noise-aware minimum instead of requiring half of the raw axis range. It considers axes only, requires one dominant candidate, explains noise or ambiguity rejection, and still requires explicit confirmation. Diagnostic profiles use serial or path when available and otherwise use GUID, VID/PID, and a deterministic duplicate occurrence; legacy transient profiles are retained only for safe same-session use and must be recaptured after re-enumeration.

**Directional FFB Compatibility** is a development-only four-pulse comparison. Method A submits the existing two-axis polar left and right requests. Method B submits equivalent bounded signed Cartesian requests through the first actuator axis actually enumerated by DirectInput. Output stops before the tester records Left, Right, No movement, or Unsure. The export keeps API acceptance separate from that observation and does not select a production FFB strategy.

The normal directional, shake, delivery, and loaded-steering tests retain the proven DirectInput descriptor sequence: two-axis polar first, followed only after safe rejection by one-axis signed Cartesian on the canonical `DIJOYSTATE2` X offset. Every attempted descriptor and lifecycle result is exported. The enumerated-axis M10 comparison remains isolated from this compatibility fallback.

P10 reports begin with a plain-language summary and keep current output state separate from earlier requests. They report selected strength, last nonzero nominal request, last nonzero safety-limited request, current output, and the unchanged 20% ceiling separately. Simulated and physical comparisons are labeled independently. Physical attempts retain create, start, update, stop, release, replacement, final-cleanup, API, and user-confirmation evidence; retried attempts remain in the technical detail while the latest accepted attempt drives the summary.

P11-E extends the Loaded Steering Response report with its Quick Setup assignment source, resolved device and axis, live-position/activity evidence, rediscovery and Reset Test events, 15-second active duration, repetition count and boundaries, requested and actual timing, fixture provenance, held and resistance directions, signed requests, DirectInput results, lifecycle counts, safety shutdown reason, multiple sensation classifications, and specific or explicitly unsure observation timing. Software assessment, physical hardware execution, observed steering input, API acceptance, and tester-reported sensation remain separate.

## Known limitations

- Physical torque cannot be measured; requested DirectInput magnitude is recorded instead.
- Device acceptance by DirectInput does not prove that torque was physically felt; hardware direction remains a manual validation item.
- Unvalidated spring, steering-load, damper, Road, Surface, Bump/Kerb, Impact, Grip Loss, and combined-effect modules are hidden from the player-facing test screen. Shake is only an alternating constant-force diagnostic and does not reproduce Surface 2.0.
- WGI testing uses SDL's Windows.Gaming.Input backend so all SDL backends can be compared consistently.
- Screen-reader/UI Automation support is not implemented in this development build.
- Comparison results apply only to the selected interface and test session. The P07 DD2 result was not sufficient evidence of incompatibility: the old comparison used a separate one-axis path and attempted effect creation at zero magnitude. P08 removes those confounders, but physical response remains unverified until hardware validation.
- Full-range 100% nominal calculation is supported and tested, but physical output remains capped at 20% until a separately reviewed safety policy and controlled hardware validation approve any increase.
- A backend comparison takes at least ten seconds per SDL backend to observe delayed arrivals.
