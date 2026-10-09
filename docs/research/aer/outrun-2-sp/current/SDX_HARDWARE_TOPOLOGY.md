# OutRun 2 SP SDX Steering Hardware Topology

## Evidence boundary

This note reconciles Sega's original SDX cabinet documentation with the verified
DVP-0015A `Jennifer` executable and the first DEV 5 runtime capture. It describes
the steering communication topology required by the game. It does not infer
motor torque, firmware waveform behavior, or safe physical drive commands.

Primary cabinet reference: Sega *OutRun 2 Special Tours SDX Owner's Manual*,
420-6958-01/02, assembly section 6 and section 21 wiring diagrams. The manual's
cockpit assembly procedure identifies two white motor-driver assemblies in the
front cabinet. The harness tagged `L` connects to the near driver and the harness
tagged `R` connects to the far driver. The game test initializes and tests the
left and right steering-wheel motors independently. Motion is documented and
tested through the separate actuator-control system.

## Physical and logical topology

```text
one SDX two-seat cockpit / one Jennifer instance
    |
    +-- Lindbergh SERIAL0 / hardcom channel 0
    |       |
    |       +-- one seven-byte transaction
    |               +-- logical steering channel 0 -> left motor driver
    |               +-- logical steering channel 1 -> right motor driver
    |
    +-- steering-position input 0 -> native analog channel 8
    +-- steering-position input 1 -> native analog channel 12
    |
    +-- separate actuator-control path -> ride/motion system
```

The two physical steering motors correspond to two motor-driver assemblies and
two game-side logical steering channels. They do not use two independent
Jennifer serial APIs: the executable combines both logical requests into one
seven-byte packet sent through `hardcomSend(0, ...)`.

## Jennifer configuration ownership

`CabinetCtrl_InitWork()` at `0x08103D48` calls `asgGetCabinetType()` and selects
the communication contract:

| Cabinet-type result | Logical channels | Packet size |
| --- | ---: | ---: |
| `0` | 1 | 4 bytes |
| `2` or `3` | 2 | 7 bytes |
| other/invalid | 0 | 0 bytes |

For the SDX runtime, the game selected two channels and a seven-byte packet.
This conclusion is independently confirmed by the captured first write.

`steerReqSendA()` at `0x08105A48` accepts a target channel. For target 0 it
places the requested command in slot 0 and `0x7D 00 00` in slot 1. For target 1
it reverses those roles. `steerReqSendOut()` at `0x08105AD2` marks bit 7 of the
first command, copies the configured payload, calculates the XOR byte, and
sends the configured packet length through hardcom channel 0.

The original response reader, `CabinetCtrlMIDI_Input()` at `0x08105BFC`, waits
for one response byte per configured logical channel. Therefore a two-channel
request requires two response bytes before the native initializer can evaluate
the selected channel's status.

## Captured first initialization request

The first DEV 5 write occurred after `CabinetCtrl_InitDriver()` began and was
observed through `steerReqSendOut()` and `hardcomSend()`:

```text
FF 00 00  7D 00 00  02
| slot 0 | | slot 1 | checksum
```

- slot 0 is logical request `0x7F 00 00`, with the transport marker applied;
- slot 1 is the native inactive-slot request `0x7D 00 00`;
- `0x7F ^ 0x7D == 0x02`, so the original XOR is valid.

This exactly matches native driver state 2, where board/channel 0 is selected
and the first `0x7F` probe is sent. It is not a zero/deactivation frame and does
not match the separately instrumented seven-byte-zero shutdown signature.

## First failed condition and state 11

The original DEV 5 launcher selected one virtual channel, so the transport
expected four bytes. Its first native seven-byte write exceeded that frame size,
failed with `ENOBUFS`, entered the virtual fault lifecycle, and returned `-1` to
the original `hardcomSend()` caller. No frame was accepted and no virtual reply
was queued.

Jennifer then entered state 3 without a readable response. The 16-tick response
window expired, state 2 retried the probe, and the native retry counter eventually
exceeded its two-attempt allowance. The original code set the channel-specific
drive-board error and entered terminal driver state 11. Because state 12 was
never reached, `CabinetCtrl_Check()` remained in state 0 and could not install
`CabinetCtrl_Main()`.

## Correct virtual contract

DEV 5 must expose two logical channels and accept the original seven-byte frame.
It must return two response bytes per transaction, keep per-channel initialization
and calibration state distinct, and synthesize the two steering-position inputs
independently. None of those requirements permits writing native state 12,
writing check state 2, or installing a callback from the loader.

The response values used by DEV 5 remain documented assumptions derived from
Jennifer's acceptance conditions rather than recovered Sega firmware. Physical
motor behavior and response timing remain outside the validated contract.

