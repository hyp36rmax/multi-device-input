# HYP36R-QS-03 — Quick Setup FFB Initialization

## Reproduction

On a Fanatec DD2, Quick Setup could reach **Test Shake** before the selected
native DirectInput force-feedback endpoint was usable. Opening Force Feedback
Settings and selecting **Re-detect Wheel** restored the shake test. The same
sequence was reproduced in the v1.5.1 compatibility and v2.0 development lines.

## Existing Re-detect behavior

The existing Re-detect operation safely stops active effects, releases the
current DirectInput device, enumerates attached force-feedback endpoints,
opens and acquires a candidate, disables driver auto-centering, resets and
enables its actuators, discovers actuator axes and periodic capabilities, and
creates a minimal constant-force probe to verify effect readiness.

## QS-03 sequence

After the final Quick Setup binding, the FFB verification page performs one
internal refresh before enabling **Test Shake**. It reuses the existing device
open and readiness-probe implementation, but resolves only the exact stable
identity of the endpoint that was active when refresh began. Enumeration order
cannot redirect this operation to another interface.

The readiness states are Pending, Refreshing, Ready, Failed, and Skipped. A
failed initialization keeps Test Shake disabled, retains the DirectInput status,
and offers **Retry Initialization** or **Skip Verification**. Retry invokes the
same refresh path. Retrying the shake itself does not refresh the endpoint.

## Safety and preserved configuration

Refresh never starts a shake automatically. Quick Setup blocks this refresh
during gameplay, stops and releases active effects before device teardown, and
fails closed when the selected endpoint is missing. It does not change input
bindings, inversion, Force profile, Strength, Road, Impact, Surface, or Debug
settings. The QS-02 10 Hz, 1.2-second, 20% shake and 250 ms watchdog are
unchanged, as are production driving-force calculations and delivery.

## Remaining limitation

Automated tests validate state transitions, one-time invocation, retry/skip,
identity preservation policy, and the unchanged shake limits. Whether a driver
becomes physically responsive after re-enumeration remains a hardware UAT item.
