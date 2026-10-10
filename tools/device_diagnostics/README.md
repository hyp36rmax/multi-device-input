# HYP36rforce Device Diagnostics 0.1.0-dev

Standalone Windows utility for checking USB input devices, Multi-Input assignments, and native DirectInput force-feedback support. It does not require OutRun 2006 and does not modify game settings.

## Start

Run `HYP36rforce-Device-Diagnostics.exe`, then select **Initialize Devices**. Successful initialization offers **Start Quick Setup** or **Not Now**; it never starts capturing automatically. Quick Setup covers steering, accelerator, brake, separate shift controls, Start/Menu, Back, and the FFB device. Each input gets a six-second detection window with Continue, Retry, Skip, Back, and Cancel. Controls from separate USB devices form one diagnostic profile without changing OutRun bindings.

The application checks SDL and native DirectInput independently. Use **Devices → Discovery Details** to run the complete ten-second-per-backend comparison. Duplicate-name interfaces retain their VID/PID, SDL instance, or numbered DirectInput interface identity so Fanatec dual interfaces remain distinguishable.

## Force-feedback safety

Motor output is disabled by default. The utility restores a previously selected FFB identity or auto-selects the only valid endpoint; multiple endpoints require confirmation. Enable the visible authorization checkbox, then hold **Test Left**, **Test Right**, or **Hold to Test**. Releasing the control, changing focus, disconnecting the device, reaching the 1.5-second maximum, or pressing **STOP** ends the effect. The utility applies a separate 20% DirectInput nominal ceiling regardless of the displayed strength. Re-detect stops and releases effects before reacquiring, and retains zero force throughout recovery.

## Reports

**Export Report** writes paired `.txt` and `.json` files to `Documents\HYP36rforce Device Diagnostics\Exports`. Filenames use the Windows-reported selected wheel name and local timestamp. Reports stay local and exclude device paths, serial numbers, and DirectInput GUIDs.

## Known limitations

- Physical torque cannot be measured; requested DirectInput magnitude is recorded instead.
- Device acceptance by DirectInput does not prove that torque was physically felt; hardware direction remains a manual validation item.
- Steering Load, Road Detail, Shake, Bump/Kerb, Impact, Grip Loss, and Combined Effects are bounded synthetic diagnostics rather than full game-state reproductions.
- WGI testing uses SDL's Windows.Gaming.Input backend so all SDL backends can be compared consistently.
- Screen-reader/UI Automation support is not implemented in this development build.
- A backend comparison takes at least ten seconds per SDL backend to observe delayed arrivals.
