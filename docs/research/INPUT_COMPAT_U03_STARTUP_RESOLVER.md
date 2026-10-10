# INPUT-COMPAT-U03 — Startup Backend Resolver Audit

## Scope

This audit covers startup controller discovery on `dev/v1.5.1-input-compatibility`. It does not change binding semantics, native wheel force feedback, or the active SDL backend after startup.

## Existing selection lineage

The historical automatic policy introduced in commit `31081c3` chooses SDL DirectInput when a native force-feedback controller is attached and Windows.Gaming.Input otherwise. This is a preference based on native FFB attachment; it is not proof that SDL DirectInput successfully opened every non-FFB controller.

INPUT-COMPAT-U01 added stable enumeration, duplicate suppression, explicit lifecycle states, and bounded rediscovery at 1, 3, and 5 seconds. INPUT-COMPAT-U02 added startup-only developer overrides for Automatic, WGI, DirectInput, RawInput, and XInput, with active/requested backend diagnostics.

## Community evidence

The reported Sora controller case is consistent with a backend-specific startup discovery failure: the controller was not usable under the automatically selected DirectInput path, then became available when SDL was explicitly started with Windows.Gaming.Input. This establishes WGI as a useful compatibility path for that hardware. It does not, by itself, establish a safe universal rule for every controller or prove that backend switching after SDL initialization is safe.

## Lifecycle and safety audit

SDL's Windows input-driver hint must be selected before controller subsystem initialization. Once initialized, the process owns open controller handles, registered bindings, hot-plug state, and window/event integration through that backend. Replacing the backend would require tearing down and rebuilding that process-global state inside an injected game process.

The current codebase has no demonstrated transaction that can:

1. suspend input without losing gameplay ownership;
2. close every SDL controller and subsystem resource;
3. change the backend hint;
4. reinitialize SDL and re-enumerate;
5. restore bindings and assignments atomically;
6. prove native DirectInput FFB remains unaffected; and
7. roll back safely if the alternate backend also fails.

A standalone probe would not be equivalent to the injected runtime because it would not share the game's window, SDL lifecycle, device handles, or timing. An in-process probe would itself require the unsafe teardown/reinitialization operation under investigation.

## Decision

Automatic backend probing or fallback is **not enabled** in U03. The current automatic policy remains unchanged. This avoids speculative backend switching and preserves existing mixed-device and native FFB behavior.

The supported compatibility path is the existing explicit WGI startup override. The Debug selector makes that path available without manual INI editing. Applying a different requested backend requires a normal game restart; the active backend remains locked for the session.

Startup diagnostics now report:

- requested Controls backend;
- requested developer override;
- selected session backend;
- the exact selection reason;
- discovery state and device counts; and
- that native DirectInput FFB is independent of SDL input-backend selection.

No manufacturer, product-name, VID/PID, or controller-specific exception is used.

## Future automatic-resolver gate

A future universal fallback requires one of the following before implementation:

- an isolated pre-game helper whose result is demonstrably equivalent to the injected SDL environment; or
- a proven in-process SDL lifecycle transaction with atomic binding restoration, native-FFB isolation, rollback, and physical validation across mixed wheel/controller/pedal configurations.

Until that evidence exists, explicit startup selection is safer and more reviewable than an automatic backend switch.

## Validation boundary

Automated tests cover backend resolution, explicit overrides, invalid-value fallback, decision reasons, bounded recovery, and registry lifecycle behavior. Physical UAT remains necessary to confirm the Sora controller under WGI and the Fanatec DD2 native FFB path in the same build.
