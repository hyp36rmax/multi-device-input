# SimHub live telemetry

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
