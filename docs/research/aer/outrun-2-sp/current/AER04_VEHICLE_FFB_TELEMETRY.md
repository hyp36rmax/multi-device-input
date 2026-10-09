# AER-04 — Native FFB and Vehicle Telemetry

## Purpose

AER-04 adds a passive, separately versioned observation stream beside the recovered native steering pipeline. It does not calculate modern FFB, call original game functions, or interpret virtual-board responses as Sega firmware behavior.

Schema: `AER_VEHICLE_FFB_V2` (corrected EVWORK_CAR pointer). Existing V1 recordings remain historically preserved but their road/direction fields are not valid EVWORK_CAR observations.

## Observation points

```text
CabinetCtrl_Main
    → DrCtrlDataSet (original executes)
        → passive vehicle snapshot
    → DrCtrlMoveSend (original executes)
        → passive scheduling marker
    → steerReqSendOut
        → passive logical-command snapshot
    → hardcomSend / raw recorder
```

All timestamps come from the same monotonic clock used by `AER_DRIVEBOARD_RAW_V1`. The vehicle snapshot is taken immediately after the original `DrCtrlDataSet`; the command row uses the most recent snapshot. This preserves ordering but does not claim zero delay between calculation, scheduling, and transport.

## Verified fields

| CSV field | Original source | Representation | Validity | Evidence |
|---|---|---|---|---|
| `front_tire_direction_s16` | `CalcTireDirection()` → `EVWORK_CAR+0x054` | Signed-16 angle, π/32768 | Valid with verified `EVWORK_CAR` pointer (`car` argument), and flag `0x2` | Confirmed average front-tire direction |
| `road_aggregate_mask` | `EVWORK_CAR+0x3FC` | 32-bit one-hot/contact-family mask | Flag `0x4` | Confirmed transition input |
| `front_left_road_mask` | `EVWORK_CAR+0x404` | 32-bit one-hot classification mask | Flag `0x4` | Confirmed front contact |
| `front_right_road_mask` | `EVWORK_CAR+0x408` | 32-bit one-hot classification mask | Flag `0x4` | Confirmed front contact |
| `threshold_state_f32` | `EVWORK_CAR+0x3AC` | Native float; semantics unresolved | Flag `0x4` | Confirmed calculation/selection input, meaning unknown |
| `logical_channel` and command bytes | `steerReqSendOut()` input | Original three-byte logical request per channel | Flag `0x8` | Confirmed game-side request |

`DrCtrlDataSet()` receives separate `car` (EVWORK_CAR) and `carWork` (CAR_WORK) arguments. AER-04 V1 mistakenly applied EVWORK_CAR offsets to `carWork`. V2 reads the five EVWORK_CAR offsets from `car`; the CAR_WORK pointer is not used for those fields. Flag `0x1` identifies a non-null EVWORK_CAR pointer. Correct field values still require validation in the next live V2 capture. Reads happen only in the verified DVP-0015A observer hook.

## Explicitly unavailable fields

Vehicle identity, speed, raw player steering, vehicle orientation, track position, wall-contact state, and vehicle-contact state are written as `-1`. Existing evidence does not yet establish safe source offsets and representations for this executable. AER-04 does not guess them.

## Correlation capability

Only V2 synchronized streams can directly compare the observed front-tire direction and three road masks with the selected logical request. V1 road masks are invalid for surface attribution and are excluded by the analyzer. It can identify exact classification transitions around Pattern 10 and the road-mask conditions surrounding translated Pattern 13 or ambiguous zero-translated patterns.

It cannot yet call an event a kerb, bump, cobblestone, wall impact, or vehicle impact unless the observed classification is joined to independently established course evidence. Tulip Garden's ordinal-20 bridge-road mapping remains the strongest course-specific reference, but track position is not yet captured.

## Live AER-04 findings and transport follow-up

The first synchronized V1 session `20261008-190100-758af1d8` reported 58,126 telemetry observations, 16,557 raw recorder events and **zero dropped raw records**. The native pipeline reached driver state 12/check state 2 and continued executing its original callback. These counts are historical capture evidence, not verification of the invalid V1 EVWORK_CAR fields.

The early AER-03 and AER-04 sessions both showed virtual transport failure after roughly 500 accepted writes. The captured sequence `80 00 00 00 00 00 00` followed by `87 00 7F 07 00 7F 00` caused the virtual model to leave READY on the first zero command and reject the subsequent `0x07` request. The `fab8780` correction keeps the transport READY when a runtime zero request occurs, with a regression replay of the original seven-byte frames. **No subsequent live capture has yet established continuous acceptance or fault-free shutdown.** The earlier command observer continued running even after writes failed, so callback counts must not be mistaken for accepted virtual frames.

Road-classification and tire-direction conclusions based on `AER_VEHICLE_FFB_V1` are superseded by the pointer-lineage correction documented above. Existing V1 raw FFB commands remain analyzable with the corrected `0x0B` decoder. A live `AER_VEHICLE_FFB_V2` session is still needed for exact kerb/texture event attribution.

## Capture integrity corrections

The successful AER-03 recording reached virtual lifecycle `FAULT` only after the ready transport stopped receiving traffic long enough to exceed its general 900-tick watchdog. The watchdog is now restricted to initialization/configuration/calibration; a ready but idle board remains ready. Physical disconnect and malformed traffic still fail closed.

The 28 AER-03 recorder losses came from production `pthread_mutex_trylock()` contention, not malformed frames. Production capture now takes the recorder's short queue mutex normally. Queue-full behavior and overflow accounting remain intact and tested.

These corrections do not change Jennifer's state ownership, responses, steering calculations, serial routing, or physical-output isolation.

## Corrected magnitude interpretation

Original continuous `0x0B` gameplay magnitude is decoded from the high five bits of logical request byte 1 (`value_a >> 3`), bounded to the verified 4–15 game-side range. Earlier analyzer versions incorrectly used byte 2 as magnitude. For continuous requests, the analyzer no longer labels byte-1 bit `0x10` as an independent direction bit because it overlaps the shifted magnitude field; polarity needs separate opcode-level verification. Discrete `0x7B` pattern direction interpretation is unchanged. No decoded magnitude is a physical torque measurement.

## Prior captures

Preserve original V1 CSV files and recorded bytes unchanged. Re-analyzing their raw native command packets with the corrected magnitude decoder is supported, but the missing correctly sampled EVWORK_CAR road masks cannot be recovered from V1. V2 output uses `vehicle_ffb_v2.csv`; a new live capture is only necessary for confirmed road-contact correlation, not for the command re-analysis.

## Completed V2 runtime result

Capture `20261008-200325-6ba226b8` is the first complete synchronized V2 evidence set: 20,341 raw records, zero drops, 72,714 vehicle/FFB rows, and 40,380 logical send rows. The corrected decoder observes 1,692 continuous magnitude requests spanning 4–11 with 1,687 transitions. This replaces the earlier apparent fixed-magnitude result.

The analyzer now prefers `vehicle_ffb_v2.csv` and emits `aer_profile_evidence_matrix.json`. The matrices establish coexistence of pattern requests, symmetric/asymmetric front-contact masks, and changing front-tire direction. They do not independently name a material or collision type.
