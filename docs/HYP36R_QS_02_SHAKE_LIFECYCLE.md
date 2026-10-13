# HYP36R-QS-02 Quick Setup Shake Lifecycle

## Purpose

Quick Setup ends with an optional physical force-feedback check. The check uses
the selected native DirectInput force-feedback endpoint; it does not assume the
device supplying steering input is also the motor-output endpoint.

The player must select **Test Shake** before any force is requested. The test
alternates a bounded constant-force direction every 50 milliseconds, producing
a 10 Hz cycle for 1.2 seconds. Requested magnitude remains capped at 20% of
DirectInput nominal output. Each active effect slice has a 250 millisecond
hardware timeout so a stalled update loop cannot leave stale output running.

## QS-02 root cause

The first Quick Setup integration advanced its shake scheduler only from
`InputManager_Update()`. That function runs inside the game's simulation-tick
loop. Quick Setup can remain visible on rendered frames where the paused game
executes no simulation tick, so the DirectInput effect could expire while the
software state remained `Running`. The UI then waited indefinitely and never
showed the physical-response question.

Activation failures were also changed back to `Ready` immediately. This hid
Acquire, actuator, effect-creation, and effect-start failures behind a button
that appeared to do nothing. Unlike the proven standalone diagnostic path, the
initial game integration did not explicitly download the effect before Start.

## Corrected lifecycle

While the final Quick Setup page is visible, it services the test-only FFB
scheduler independently of simulation ticks. The sequence is:

1. Verify the selected native DirectInput endpoint is ready.
2. Acquire it and enable its actuators.
3. Create the existing two-axis polar constant effect, with the established
   one-axis Cartesian fallback.
4. Explicitly download and start the effect.
5. Alternate direction at 50 millisecond half-periods.
6. Stop and release the effect after 1.2 seconds.
7. Ask the player whether the wheel physically responded.

Normal completion, activation failure, update failure, safety interruption,
and player cancellation are distinct outcomes. Only normal completion opens
the **Yes / No / Unsure** confirmation. A failed or interrupted test retains a
concise DirectInput status, then offers **Retry Test** and **Skip Verification**.
DirectInput acceptance is not treated as proof of physical torque.

## Safety and compatibility

The test remains explicitly activated, capped at 20%, and isolated from normal
driving force generation. Focus loss, device loss, FFB disablement, manual
cancellation, failed DirectInput calls, completion, and watchdog expiry all
stop and release the effect. Existing inversion, duplicate-interface selection,
two-axis compatibility, one-axis fallback, Test Left/Test Right behavior, and
production FFB delivery remain unchanged.

Automated tests validate policy and state progression without requesting
physical force. Final confirmation of felt wheel response necessarily remains a
hardware validation step.
