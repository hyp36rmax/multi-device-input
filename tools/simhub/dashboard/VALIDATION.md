# SIMHUB-03A validation notes

## Verified root cause

The earlier `.simhubdash` archive placed its `.djson` and `.djson.metadata` files directly at the ZIP root. A dashboard exported by SimHub places those files inside a single dashboard-named root directory. The old archive was a valid ZIP but not a faithful SimHub export package, which explains why static ZIP checks passed while SimHub produced no import confirmation.

## Completed statically

- The corrected package uses one `HYP36rforce Digital Dash/` root directory containing the dashboard definition and metadata sidecar.
- Dashboard schema and metadata follow SimHub 9.13.2-generated files: standard root object, one screen with an item collection, typed text/rectangle widgets, widget lifecycle fields, unique names, and matching dimensions/version metadata.
- Widgets use SimHub typed `TextItem` and `RectangleItem` models.
- All six required custom-property bindings are present with the approved namespace.
- Standard speed and gear bindings provide explicit unavailable presentation.
- Steering, Road and Impact meters are bounded by their declared ranges.
- No additional UDP sender, protocol change, game binary, DLL, font or external artwork is included.
- Original supplied dashboard archive was not modified.
- Package generation is deterministic and checks both required entries.
- Static title, Speed, Gear, and Steering presentation remains visible without telemetry; unavailable values remain `--`.

## Requires real SimHub validation

- Import/open behavior in the installed SimHub 9.13.2 application remains **NEEDS VALIDATION**.
- Actual property-explorer names for the live external simulation.
- Live steering and stage updates; prior observation was steering `0` and stage `-1`.
- Visual scaling on physical secondary and compact dashboard displays.

The package must not be described as live-validated until these checks have been observed in SimHub.
