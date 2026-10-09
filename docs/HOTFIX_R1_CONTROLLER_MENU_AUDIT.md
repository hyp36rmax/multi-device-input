# Hotfix R1: controller ownership and menu-action audit

This is an investigation record, not a claim that P1/P2 or the two menu actions are fixed. The supported `OR2006C2C.exe` inspected here has SHA-256 `68ceb386829066f8455b9d027320af962584321f3e2e8a79c72841495a6134c3`.

**Project decision:** P1/P2 ownership is a *possible* multi-gamepad limitation, not a reproduced bug. Do not add seat selectors, change controller routing, or alter existing multi-device bindings on this evidence alone. Revisit only with a concrete report showing the connected devices, intended controls, actual behavior, and restart/reconnect sequence. This decision leaves the existing wheel, pedal, shifter, and gamepad setup unchanged.

## Controller ownership

`InputManager` currently opens every SDL joystick, but sends generic `[Gamepad]` bindings through one `primaryControllerIndex`. The first gamepad to enumerate becomes primary. That index is transient and is not persisted. This is a potential ambiguity when two standard gamepads are connected; it is not evidence that a user's existing multi-device setup is broken. Device-specific bindings carry GUID, occurrence, VID/PID, serial, and path. A matching serial or path is preferred, while the existing binding matcher can fall back to GUID plus occurrence when exact identity is unavailable. No identity or binding behavior is changed here.

The current Controllers page lists and tests devices. It has no P1/P2 assignment action. `docs/multi_device_input.md` explicitly describes aggregating all bindings into OutRun's *single-player state*. In the validated EXE, the selector used by native `SwitchOn`/`SwitchNow` at `0x47F110` is simply `xor eax,eax; ret` (player 0). A P2 identity record by itself would not create an independent game-input route. Adding one would be a separate product decision, not a routine hotfix.

## Menu-action evidence

The reported Sign In and Change Car Class mapping problems do not yet have a reproducible screen/button sequence. The second-hand report only says the user could not map them. Both remain **open for reproduction**, not confirmed defects in the native action path. No replacement keyboard shortcut or global button mapping should be added on that evidence.

Native `SwitchOn` at `0x4536F0` reads the game's pressed mask; `SwitchNow` at `0x4536C0` reads its held mask. Multi Input replaces both reads with its current unified switch masks and also writes a raw DirectInput mask after `ReadIO`. This is the narrow existing input boundary, but the native front end applies context rules after reading it.

The current binding table calls bit `0x8000` `SignIn`. The supported EXE's front-end code at `0x445875` checks `0x20` then `0x10`; under menu/object-state gates, the latter is translated into a `0x8000` front-end event at `0x4458C2`. Other state checks later at `0x445928`–`0x4459DE` gate its action. The relation between this path and the reported Sign In UI action is not yet established well enough to patch or bypass it. The current default binds F1 and the left shoulder to both `X` and `SignIn`; that duplication must be evaluated before changing it.

At `0x48F5F0` the EXE translates switch presses into front-end event numbers. A `0x2000` press becomes event 5, while `0x8000` becomes event 8. The current mod separately writes raw `0x2000` when acceleration reaches 50%, with a code comment tying this to the Sumo car-list toggle. That establishes a candidate input edge, not proof of the complete Change Car Class action or of safe behavior in every menu. It should not be turned into a global binding until the car-selection consumer and gating are traced.

No native cheat, force, save, or menu behavior was modified in this audit.
