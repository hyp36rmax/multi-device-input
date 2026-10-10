# HYP36rforce Device Diagnostics 0.1.0-dev

Standalone Windows utility for checking USB input devices, Multi-Input assignments, and native DirectInput force-feedback support. It does not require OutRun 2006 and does not modify game settings.

## Start

Run `HYP36rforce-Device-Diagnostics.exe`, then select **Initialize Devices**. The utility automatically checks WGI, RawInput, DirectInput, and XInput in separate ten-second observation sessions before discovering native DirectInput FFB endpoints. A responsive progress screen identifies the active stage, completed stages, discovered devices, delayed arrivals, and estimated remaining time. Successful initialization offers **Start Quick Setup** or **Not Now**; it never starts capturing automatically. Quick Setup covers seven gameplay bindings: steering, accelerator, brake, separate shift controls, Start/Menu, and Back. Each input gets a six-second detection window with Continue, Retry, Skip, Back, and Cancel. FFB is resolved separately during initialization. After the bindings, an optional single-click check shows a countdown and runs one 1.2-second, 20%-limited shake before asking whether it was felt. Controls from separate USB devices form one diagnostic profile without changing OutRun bindings.

The application checks SDL and native DirectInput independently. The Devices page keeps **Connected Devices** separate from **Input Backend Compatibility** and provides **Re-run Backend Tests**, which uses the same isolated test sequence without replacing the last complete results until the new run finishes. Duplicate-name interfaces retain their VID/PID, SDL instance, or numbered DirectInput interface identity so Fanatec dual interfaces remain distinguishable.

## Force-feedback safety

Motor output is disabled by default. The utility restores a previously selected FFB identity or auto-selects the only valid endpoint; multiple endpoints require confirmation. Enable the visible authorization checkbox, then hold **Test Left**, **Test Right**, or **Hold to Shake**. One shared linear Strength request spans 20–100% (2000–10000 DirectInput nominal units); the separately displayed 20% physical-test ceiling remains enforced pending a dedicated safety review. Shake alternates the same constant-force path at an internal 10 Hz diagnostic rate and uses that shared request. **Invert FFB** reverses all three directions without changing magnitude. Releasing the control, changing focus, disconnecting the device, or reaching the 1.5-second maximum ends the effect. Re-detect stops and releases effects before reacquiring, retains zero force, and disarms authorization.

## Compare FFB Response

**Compare FFB Response** performs a motor-free software assessment, then guides you through legacy effect recreation and persistent `SetParameters` updates using the proven directional constant-force engine. After each bounded test, output stops and the active timer freezes while you answer **Yes**, **No**, or **Unsure**; you may retry only that method before continuing. Zero-magnitude sequence points stop output instead of attempting to create a zero-force effect. Both methods use the same bounded 15 Hz request sequence with a minimum one-second zero-force interval between them. STOP, focus loss, disconnect, watchdog, or unsafe shutdown cancels the active sequence. Results and exports keep DirectInput API evidence separate from the user's physical observation; an unsuccessful method does not mean the wheel is defective.

## Reports

**Export Report** writes paired `.txt` and `.json` files to the resolved Windows `Documents\HYP36rforce Device Diagnostics\Exports` folder, including redirected Documents/OneDrive locations. Filenames use the Windows-reported selected wheel name and local timestamp, with `_02`, `_03`, and later suffixes preventing overwrites. The application confirms the filename and provides **Open Exports Folder**. Reports stay local and exclude device paths, serial numbers, DirectInput GUIDs, and private filesystem paths.

P10 reports begin with a plain-language summary and keep current output state separate from earlier requests. They report selected strength, last nonzero nominal request, last nonzero safety-limited request, current output, and the unchanged 20% ceiling separately. Simulated and physical comparisons are labeled independently. Physical attempts retain create, start, update, stop, release, replacement, final-cleanup, API, and user-confirmation evidence; retried attempts remain in the technical detail while the latest accepted attempt drives the summary.

## Known limitations

- Physical torque cannot be measured; requested DirectInput magnitude is recorded instead.
- Device acceptance by DirectInput does not prove that torque was physically felt; hardware direction remains a manual validation item.
- Unvalidated spring, steering-load, damper, Road, Surface, Bump/Kerb, Impact, Grip Loss, and combined-effect modules are hidden from the player-facing test screen. Shake is only an alternating constant-force diagnostic and does not reproduce Surface 2.0.
- WGI testing uses SDL's Windows.Gaming.Input backend so all SDL backends can be compared consistently.
- Screen-reader/UI Automation support is not implemented in this development build.
- Comparison results apply only to the selected interface and test session. The P07 DD2 result was not sufficient evidence of incompatibility: the old comparison used a separate one-axis path and attempted effect creation at zero magnitude. P08 removes those confounders, but physical response remains unverified until hardware validation.
- Full-range 100% nominal calculation is supported and tested, but physical output remains capped at 20% until a separately reviewed safety policy and controlled hardware validation approve any increase.
- A backend comparison takes at least ten seconds per SDL backend to observe delayed arrivals.
