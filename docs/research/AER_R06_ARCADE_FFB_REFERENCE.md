# AER-R06 — Arcade FFB Reference

This reference connects the preserved OutRun 2 SP SDX research to the optional
HYP36rforce Arcade Experience without treating them as the same system. The
authoritative arcade evidence remains the LinuxLoader research revision linked
from [`aer/README.md`](aer/README.md). Its current synchronized copy is under
[`aer/outrun-2-sp/current/`](aer/outrun-2-sp/current/README.md).

## Verified game-side command path

```mermaid
flowchart LR
    Vehicle[Original vehicle and steering state] --> Native[Jennifer native steering logic]
    Native --> Callback[CabinetCtrl_Main callback]
    Callback --> Data[DrCtrlDataSet]
    Data --> Move[DrCtrlMoveSend]
    Move --> Frame[Single/dual-board serial frame]
    Frame --> Board[Sega drive board]
    Board --> Motor[Cabinet steering hardware]
```

Verified original-game evidence establishes the native callback pipeline,
continuous command family, event-pattern requests, framing, request queuing,
response validation, and game-side magnitudes. Live isolated research reached
driver state 12, cabinet-check state 2, and sustained original commands. The
preserved technical authority is
[`NATIVE_STEERING_TECHNICAL_REFERENCE.md`](aer/outrun-2-sp/current/NATIVE_STEERING_TECHNICAL_REFERENCE.md).

The following remain unknown without firmware or original-hardware measurement:

- command-to-motor-current transfer;
- physical torque and direction at the wheel;
- firmware waveform or pattern realization;
- mechanical filtering and cabinet-dependent feel;
- exact command-to-actuation latency.

Game-side values such as the recovered `4–15` range are command values, not Nm.
The hardware questions stay recorded in
[`HARDWARE_VALIDATION_REGISTER.md`](aer/outrun-2-sp/current/HARDWARE_VALIDATION_REGISTER.md).

## Original arcade and SDL/DirectInput boundaries

```mermaid
flowchart TB
    subgraph Arcade[Original Lindbergh cabinet]
      J[Jennifer request] --> Serial[Drive-board serial protocol]
      Serial --> Firmware[Unrecovered board firmware]
      Firmware --> Cabinet[Arcade motor and mechanics]
    end
    subgraph PC[HYP36rforce on PC]
      Evidence[OutRun 2006 game evidence] --> Model[Independent HYP36rforce interpretation]
      Model --> DI[DirectInput descriptors]
      DI --> Consumer[Consumer wheel driver/base]
    end
    Arcade -. research reference only .-> Model
```

There is no byte-for-byte or constant-for-constant translation from the Sega
protocol into DirectInput. AER uses independently observed PC game state and a
bounded modern presentation. The original command identifiers, addresses, and
drive-board constants are not imported into runtime HYP36rforce code.

## Provenance rules

- Arcade claims cite preserved executable/runtime evidence.
- Virtual-board behavior is labeled as a research model, not Sega firmware.
- HYP36rforce behavior is described as an independent interpretation.
- DirectInput success proves API acceptance only.
- Reference+ remains the comparison foundation and is not redefined by AER.
