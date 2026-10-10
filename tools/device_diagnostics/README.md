# HYP36rforce Device Diagnostics 0.1.0-dev

Standalone Windows utility for checking USB input devices, Multi-Input assignments, and native DirectInput force-feedback support. It does not require OutRun 2006 and does not modify game settings.

## Start

Run `HYP36rforce-Device-Diagnostics.exe`, then select **Initialize Devices**. The application checks SDL and native DirectInput independently. Use **Devices → Discovery Details** to run the complete ten-second-per-backend comparison.

## Force-feedback safety

Motor output is disabled by default. Select a native FFB device, enable the visible authorization checkbox, then hold **Run Test**. Releasing the control, changing focus, disconnecting the device, reaching the short timeout, or pressing **STOP** ends the effect. The utility applies a separate 20% DirectInput nominal ceiling regardless of the displayed strength.

## Reports

**Export Report** writes `.txt` and `.json` files to `Documents\HYP36rforce Device Diagnostics\Reports`. Reports stay local and exclude device paths, serial numbers, and DirectInput GUIDs.

## Known limitations

- Physical torque cannot be measured; requested DirectInput magnitude is recorded instead.
- WGI testing uses SDL's Windows.Gaming.Input backend so all SDL backends can be compared consistently.
- Screen-reader/UI Automation support is not implemented in this development build.
- A backend comparison takes at least ten seconds per SDL backend to observe delayed arrivals.
