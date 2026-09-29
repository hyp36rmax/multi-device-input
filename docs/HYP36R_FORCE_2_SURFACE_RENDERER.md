# HYP36R Force 2.0 Experimental Surface Renderer

This removable research prototype compares the established Road 1.5
directional renderer with a dedicated DirectInput periodic renderer. It does
not classify stages or surfaces and contains no Deep Lake-specific behavior.

## Modes

- **Directional (1.5 Reference)** preserves the established Road source,
  player Road Detail, calibration, ceiling, slew, composer, `tanh`, and
  constant-force output without changes.
- **Surface (2.0 Experimental)** removes the final Road channel from the
  directional composer and uses that same Road value to control a separate
  DirectInput sine effect. Steering and Impact remain in the normal composer.

The selection is Debug-only and defaults to Directional.

## Initial request policy

The absolute final Road value supplies the periodic magnitude envelope. The
default research strength is 25%, adjustable from 0–50% in Debug. Requested
magnitude is hard-limited to 0.12 of DirectInput nominal output. The period is
derived from normalized vehicle speed:

```text
frequencyHz = 18 + clamp(normalizedSpeed, 0, 1) * 24
```

This gives a conservative 18–42 Hz range. It is a research mapping, not a
validated physical road wavelength.

The effect uses standard DirectInput `GUID_Sine`. At device initialization the
backend enumerates periodic effects and records whether Sine and dynamic
type-specific parameter updates are supported. Unsupported devices remain
safe and receive no periodic request.

The effect stops immediately when Surface mode is disabled, Force Feedback is
disabled, focus is lost, gameplay ends, the device is refreshed/disconnected,
or the update watchdog expires.
