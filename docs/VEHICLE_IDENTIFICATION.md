# OutRun 2006 C2C vehicle identification

The native player-car value `Game::pl_car()->car_kind_11` identifies 15 Ferrari
models across two ID groups. This mapping was established through physical
in-game selection and telemetry validation. Color choices do not receive
separate IDs; they retain the ID of their model and class.

| Native ID | Vehicle | Class |
| ---: | --- | --- |
| 0 | Ferrari F50 | Intermediate A |
| 1 | Ferrari Dino 246 GTS | Novice |
| 2 | Ferrari 288 GTO | Intermediate B |
| 3 | Ferrari 512 BB | Professional |
| 4 | Ferrari 365 GTS/4 Daytona | Novice |
| 5 | Ferrari Enzo Ferrari | Professional |
| 6 | Ferrari Testarossa | Intermediate B |
| 7 | Ferrari 360 Spider | Intermediate A |
| 8 | Ferrari F40 | Professional |
| 9 | Ferrari 250 GTO | Professional |
| 10 | Ferrari F355 Spider | Intermediate A |
| 11 | Ferrari 328 GTS | Intermediate B |
| 12 | Ferrari F430 | Professional |
| 13 | Ferrari 550 Barchetta | Professional |
| 14 | Ferrari SuperAmerica | Intermediate A |
| 15 | Ferrari F50 | OutRun |
| 16 | Ferrari Dino 246 GTS | OutRun |
| 17 | Ferrari 288 GTO | OutRun |
| 18 | Ferrari 512 BB | OutRun |
| 19 | Ferrari 365 GTS/4 Daytona | OutRun |
| 20 | Ferrari Enzo Ferrari | OutRun |
| 21 | Ferrari Testarossa | OutRun |
| 22 | Ferrari 360 Spider | OutRun |
| 23 | Ferrari F40 | OutRun |
| 24 | Ferrari 250 GTO | OutRun |
| 25 | Ferrari F355 Spider | OutRun |
| 26 | Ferrari 328 GTS | OutRun |
| 27 | Ferrari F430 Spider | OutRun |
| 28 | Ferrari 550 Barchetta | OutRun |
| 29 | Ferrari SuperAmerica | OutRun |

`CarIdentity` is the single runtime resolver used by the telemetry overlay,
General Capture filenames, and session metadata. Player-facing output includes
the model and class, while CSV metadata retains the authoritative numeric ID.
Filename sanitization preserves spaces, parentheses, and class labels, and
replaces characters such as the slash in `GTS/4` with a safe underscore.

IDs 30 and above have not been observed. An unknown native ID is displayed as
`Car #<ID>` rather than being assigned a guessed model. If gameplay or the
player-car object is unavailable, presentation uses `Car unavailable` and the
numeric metadata remains unavailable.
