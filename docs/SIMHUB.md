# SimHub live telemetry

## Development status

HYP36rforce SimHub Integration is an optional, experimental feature developed
exclusively for OutRun 2006 C2C Multi-Input v2.0. It is not included in v1.5.0
or v1.5.1, and v2.0 has not been publicly released. Live UDP telemetry works;
Digital Dash import and rendering and additional telemetry signals still need
further validation.

Install the development package beside `OR2006C2C.exe`, start the game, then
enable **Settings → Gameplay → SimHub Telemetry**. The game registers the
packaged external-simulation definition for the current Windows user when
SimHub is installed and sends localhost UDP telemetry at approximately 60 Hz.
No network address, port, or connection setup is required.

This first live contract publishes verified steering, vehicle identity, stage,
and pre-render native-effect evidence for road and impact activity. Native
speed units and gear encoding are not yet validated as standard SimHub units,
so `SpeedKmh` remains `0` and `Gear` remains empty rather than presenting
guessed data. Pause state is likewise neutral until its lifecycle mapping is
validated. The Debug screen reports local transmission and registration state;
it does not claim that SimHub is receiving packets.

SimHub owns dashboard, bass-shaker, wind, waveform, mixing, and channel-routing
configuration. This adapter does not alter or suppress HYP36rforce wheel FFB.
