# INPUT-COMPAT-U01 controlled hardware validation

This development build adds universal, bounded SDL discovery recovery. It does not change force-feedback behavior or rewrite bindings.

Keep the same USB port, driver, game installation, bindings, and FFB configuration for every test. Fully exit and restart the game between tests. Preserve `OutRun2006Tweaks.log` from each run.

## Test A — Automatic

```ini
[Controls]
InputBackend=0

[Developer]
InputBackendOverride=AUTOMATIC
```

Confirm the log reports the requested and resolved backend, initial enumeration, delayed snapshots at approximately 1, 3, and 5 seconds, and a successful device registration. Verify steering, accelerator, brake, buttons, and binding availability.

## Test B — Explicit DirectInput

```ini
[Controls]
InputBackend=2

[Developer]
InputBackendOverride=DIRECTINPUT
```

Repeat the same checks. The backend remains fixed for the complete game session.

## Test C — Explicit Windows.Gaming.Input

```ini
[Controls]
InputBackend=0

[Developer]
InputBackendOverride=WGI
```

Repeat the same checks, including delayed discovery. This is a developer validation override and does not change the player-facing `InputBackend` numbering.

## Cross-hardware regression

Repeat Test A with available Fanatec DD2, Thrustmaster T818, separate USB pedals, shifters, button boxes, and mixed-device configurations. Confirm each SDL instance is registered once, existing bindings remain intact, removal/reconnection is handled, and native DirectInput FFB behavior is unchanged.

A successful test demonstrates compatibility only for the tested configuration. It does not establish universal hardware compatibility.
