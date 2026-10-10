# Property bindings

| Instrument | SimHub property | Current status |
| --- | --- | --- |
| Speed | `SpeedKmh` | Verified neutral (`0`); dashboard displays `-- km/h` |
| Gear | `Gear` | Verified neutral (empty); dashboard displays `--` |
| Steering | `DataCorePlugin.GameRawData.Custom_SteeringInput` | Contract verified; live gameplay update still requires physical confirmation |
| Car ID | `DataCorePlugin.GameRawData.Custom_CarID` | Contract and 30-car runtime mapping verified |
| Car name | `DataCorePlugin.GameRawData.Custom_CarName` | Uses adapter-provided centralized name; no dashboard mapping |
| Stage ID | `DataCorePlugin.GameRawData.Custom_StageID` | Contract verified; observed `-1` remains under investigation |
| Road activity | `DataCorePlugin.GameRawData.Custom_RoadActivity` | Native composite-effect evidence, bounded `0..1` |
| Impact intensity | `DataCorePlugin.GameRawData.Custom_ImpactIntensity` | Native composite-effect rise, bounded `0..1` |

The dashboard does not infer receiver connection, RPM, stage names, road roughness, suspension
motion, or any other unavailable physical telemetry.
