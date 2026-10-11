# HYP36R-DEBUG-02 — Debug Input Backend Controls

The v2 Engineering Workspace forward-ports the reviewed selector presentation
from v1.5.1 commit `3a51410` without merging its delayed-discovery or automatic
startup-recovery implementation.

## Ownership and lifecycle

`Developer/InputBackendOverride` stores one of `AUTOMATIC`, `WGI`,
`DIRECTINPUT`, `RAWINPUT`, or `XINPUT`. `InputManager::init()` resolves this
setting before any SDL hint or `SDL_Init` call. Automatic delegates to the
unchanged v2 production policy: the existing player `Controls/InputBackend`
preference wins when explicit; otherwise an attached FFB device selects SDL
DirectInput and other systems select Windows.Gaming.Input.

The Debug Apply button only saves the developer override. It does not switch
the running SDL backend, reload bindings, touch FFB configuration, or restart
the injected process. Apply & Restart is deliberately disabled because safe
shutdown and relaunch ownership is not available.

The UI keeps the startup override and currently saved override separate, so
`Restart Required` is derived without pretending that a newly saved value is
active. Invalid stored values fail safely to Automatic and are logged.

## Isolation

No U03/U04 resolver, automatic fallback, retry schedule, manufacturer rule,
VID/PID exception, or delayed device recovery is included. Native wheel FFB
continues through its independent DirectInput output system. Input bindings,
Surface, Road Research, telemetry, and AER force behavior are unchanged.
