# DEV 5 Virtual Drive-Board Assumptions

DEV 5 is a research transport for the verified OutRun 2 SP SDX Rev A `Jennifer`
executable. It preserves the original game's initialization, cabinet-check,
callback-installation, and steering-command ownership. It does not reproduce
Sega drive-board firmware or physical motor behavior.

## Runtime-confirmed packet topology

The first native write recovered from the original executable was:

`FF 00 00 7D 00 00 02`

It is a valid seven-byte original-protocol frame: two three-byte command slots
followed by their XOR checksum (`0x7f ^ 0x7d == 0x02`). The first slot is the
native `0x7f` probe and the second is the native `0x7d` neutral/idle request.
This proves that Jennifer uses the two-slot transport contract during this
initialization path. Sega's SDX cabinet documentation independently identifies
the two L/R motor-driver assemblies; Jennifer combines their two logical command
slots into one SERIAL0 packet. See [SDX Steering Hardware Topology](SDX_HARDWARE_TOPOLOGY.md).

The initial DEV 5 package selected the four-byte single-slot transport. It
therefore rejected this first seven-byte write before command policy evaluation,
accepted zero frames, returned `-1` to Jennifer, and entered the transport fault
state. The launcher now selects the two-slot contract. Regression coverage
retains the captured bytes verbatim and proves both the former rejection and
the corrected acceptance/response behavior.

## Synthetic response policy

The original executable establishes which parsed status advances each game-side
state, but it does not establish the real board's raw firmware timing. DEV 5 uses
the following explicit assumptions at the isolated virtual SERIAL0 boundary:

| Native request | Virtual response | Evidence boundary |
| --- | --- | --- |
| `0x7f` probe | parsed status 0 (`0x00`) | Any parser-valid non-`0xee` reply advances the probe state. |
| `0x01/0x30/0x7f` readiness request | parsed status 1 (`0x11`) | Native state 4 explicitly requires status 1. |
| `0x7c`, `0x7d` | parsed status 0 (`0x00`) | Native state 5 requires status 0; `0x7d` is its retry/neutral request. |
| `0x7a`, `0x03`, `0x06`, `0x08` | parsed status 0 (`0x00`) | Each native configuration substate advances only on status 0. |
| `0x00`, `0x04`, `0x70` calibration family | parsed status 0 (`0x00`) | Replies are drainable; native analog comparison, not the reply, owns convergence. |
| `0x1d`, `0x1e` cabinet-check table | parsed status 0 (`0x00`) | The original check drains responses and installs its callback after table completion. |
| post-check runtime request | parsed status 0 (`0x00`) | Acknowledgment only; payload meaning is not interpreted by the virtual board. |

An unknown request before the cabinet-check phase faults the virtual session.
Runtime payloads after the check are recorded as opaque original-game traffic.

## Synthetic steering calibration

Static analysis of native state 8 confirms that a zero offset from the stored
steering center is the game's convergence condition. When the native `0x04`
calibration request reaches the virtual board, DEV 5 writes the centered value
to the loader-owned JVS analog state. The next native initializer invocation
performs the comparison and decides whether to enter state 9. DEV 5 never writes
driver state 12, check state 2, or the event callback pointer.

This centered sample is a declared synthetic safety mechanism. It is not a
measurement of original cabinet movement, steering torque, response time, or
firmware behavior.

## Isolation

The feature is off unless explicitly requested, requires the exact verified
executable hash and original-byte manifest, rejects the existing cabinet bypass
and drive-board emulator, and refuses physical serial passthrough. Its transport
does not call SDL/evdev force feedback, motion output, or a physical serial
fallback. Gameplay validation can establish that the original game-side command
pipeline activates; it cannot establish physical arcade-force equivalence.
