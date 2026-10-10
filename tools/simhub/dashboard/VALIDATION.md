# Validation notes

## Completed statically

- SimHub 9.13.2 dashboard schema and metadata match the supplied template.
- Widgets use SimHub typed `TextItem` and `RectangleItem` models.
- All six required custom-property bindings are present with the approved namespace.
- Standard speed and gear bindings provide explicit unavailable presentation.
- Steering, Road and Impact meters are bounded by their declared ranges.
- No additional UDP sender, protocol change, game binary, DLL, font or external artwork is included.
- Original supplied dashboard archive was not modified.

## Requires real SimHub validation

- Import/open behavior in the installed SimHub 9.13.2 application.
- Actual property-explorer names for the live external simulation.
- Live steering and stage updates; prior observation was steering `0` and stage `-1`.
- Visual scaling on physical secondary and compact dashboard displays.

The package must not be described as live-validated until these checks have been observed in SimHub.
