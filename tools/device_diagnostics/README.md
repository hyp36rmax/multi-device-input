# HYP36rforce Device Diagnostics 0.1.0-dev

Standalone Windows utility for checking USB input devices, Multi-Input assignments, and native DirectInput force-feedback support. It does not require OutRun 2006 and does not modify game settings.

## Start

Run `HYP36rforce-Device-Diagnostics.exe`, then select **Initialize Devices**. The utility automatically checks WGI, RawInput, DirectInput, and XInput in separate ten-second observation sessions before discovering native DirectInput FFB endpoints. A responsive progress screen identifies the active stage, completed stages, discovered devices, delayed arrivals, and estimated remaining time. Successful initialization offers **Start Quick Setup** or **Not Now**; it never starts capturing automatically. Quick Setup covers steering, accelerator, brake, separate shift controls, Start/Menu, Back, and the FFB device. Each input gets a six-second detection window with Continue, Retry, Skip, Back, and Cancel. Controls from separate USB devices form one diagnostic profile without changing OutRun bindings.

The application checks SDL and native DirectInput independently. The Devices page keeps **Connected Devices** separate from **Input Backend Compatibility** and provides **Re-run Backend Tests**, which uses the same isolated test sequence without replacing the last complete results until the new run finishes. Duplicate-name interfaces retain their VID/PID, SDL instance, or numbered DirectInput interface identity so Fanatec dual interfaces remain distinguishable.

## Force-feedback safety

Motor output is disabled by default. The utility restores a previously selected FFB identity or auto-selects the only valid endpoint; multiple endpoints require confirmation. Enable the visible authorization checkbox, then hold **Test Left**, **Test Right**, or **Hold to Shake**. Shake alternates the same constant-force path at an internal 10 Hz diagnostic rate. **Invert FFB** reverses all three directions without changing magnitude. Releasing the control, changing focus, disconnecting the device, reaching the 1.5-second maximum, or pressing **STOP** ends the effect. The utility applies a separate 20% DirectInput nominal ceiling regardless of the displayed strength. Re-detect stops and releases effects before reacquiring, retains zero force, and disarms authorization.

## Reports

**Export Report** writes paired `.txt` and `.json` files to the resolved Windows `Documents\HYP36rforce Device Diagnostics\Exports` folder, including redirected Documents/OneDrive locations. Filenames use the Windows-reported selected wheel name and local timestamp, with `_02`, `_03`, and later suffixes preventing overwrites. The application confirms the filename and provides **Open Exports Folder**. Reports stay local and exclude device paths, serial numbers, DirectInput GUIDs, and private filesystem paths.

## Known limitations

- Physical torque cannot be measured; requested DirectInput magnitude is recorded instead.
- Device acceptance by DirectInput does not prove that torque was physically felt; hardware direction remains a manual validation item.
- Unvalidated spring, steering-load, damper, Road, Surface, Bump/Kerb, Impact, Grip Loss, and combined-effect modules are hidden from the player-facing test screen. Shake is only an alternating constant-force diagnostic and does not reproduce Surface 2.0.
- WGI testing uses SDL's Windows.Gaming.Input backend so all SDL backends can be compared consistently.
- Screen-reader/UI Automation support is not implemented in this development build.
- A backend comparison takes at least ten seconds per SDL backend to observe delayed arrivals.
