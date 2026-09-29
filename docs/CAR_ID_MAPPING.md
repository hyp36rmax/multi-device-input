# OutRun 2006 C2C Player Car ID Mapping

This table maps the native player-car value `Game::pl_car()->car_kind_11` to a
friendly vehicle name. The complete 0–14 mapping was established through
physical in-game car selection and telemetry validation.

| ID | Car |
| ---: | --- |
| 0 | F50 |
| 1 | Dino 246 GTS |
| 2 | 288 GTO |
| 3 | 512 BB |
| 4 | 365 GTS/4 Daytona |
| 5 | Enzo Ferrari |
| 6 | Testarossa |
| 7 | 360 Spider |
| 8 | F40 |
| 9 | 250 GTO |
| 10 | F355 Spider |
| 11 | 328 GTS |
| 12 | F430 |
| 13 | 550 Barchetta |
| 14 | SuperAmerica |

The numeric ID remains the authoritative engineering and game-state value.
Player-facing telemetry, Guided UAT, and reports should use the friendly name
resolved centrally by `CarIdentity`. Unknown IDs must be shown neutrally as
`Car <ID>` and must never be assigned a guessed vehicle name.

This mapping is the reusable reference for future telemetry, UAT, game-context,
and research work. Do not duplicate the table in individual presentation or
reporting components.
