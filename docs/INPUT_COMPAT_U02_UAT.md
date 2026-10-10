# INPUT-COMPAT-U02 universal backend validation

This v1.5.1 development build exposes the existing startup backend override in **Debug → Input Backend Diagnostics**. It does not switch the active SDL session, change bindings, or change native DirectInput force feedback.

Keep the USB ports, drivers, bindings, and FFB settings unchanged. For each test, select the backend, choose **Apply**, close OutRun normally, relaunch it, then preserve `OutRun2006Tweaks.log`. **Apply & Restart** is intentionally unavailable because a safe restart from the injected DLL cannot be guaranteed.

## Sora Test A — Automatic

Select **Automatic**, Apply, and relaunch. For Sora's current FFB-wheel configuration, the expected active input backend is **SDL DirectInput**. Confirm the Debug page distinguishes Active and Requested backend, then verify initial enumeration, the bounded 1/3/5-second delayed discovery pass, registration, steering, pedals, and buttons.

## Sora Test B — SDL DirectInput

Select **SDL DirectInput**, Apply, and relaunch. Confirm Requested and Active both show SDL DirectInput and Status shows Applied. Compare the device count, interface details, registration result, and control response with Test A.

## Sora Test C — Windows.Gaming.Input

Select **Windows.Gaming.Input**, Apply, and relaunch. Confirm Requested and Active match. Observe whether the T300RS appears initially or during bounded delayed discovery and whether it becomes available for bindings. Do not infer SDL readiness from the separate FFB device count.

## Cross-hardware matrix

Repeat Automatic first, then explicit backends only when investigating a problem, with available Fanatec DD2, Thrustmaster T818/T300RS, Logitech, Moza, Simucube, separate pedals, shifters, button boxes, and mixed-device layouts. Confirm existing bindings persist, each SDL instance registers once, hot-plug remains operational, and native FFB behavior is unchanged.

Record the active backend, requested backend, Status, input count, FFB count, Last Discovery, and the privacy-safe Discovery Details. A passing observation applies only to the tested hardware, driver, USB topology, and backend.
