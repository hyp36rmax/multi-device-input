# Letting OutRun Complete Its Own Steering Startup

The DEV 4 capture finally made the activation failure concrete. The game repeatedly called its cabinet check, but its native driver state never moved away from zero. That meant it never completed the cabinet check, never registered `CabinetCtrl_Main()`, and never began producing its normal steering requests.

The reason is straightforward: LinuxLoader currently replaces the original initialization function with a shortcut that reports success. That satisfies the outer startup path, but it does not perform the work the original game associates with success. The original game expects a conversation with a drive board, a bounded steering-position search, configuration of one or two boards, and a final transition to driver state 12. The shortcut returns without establishing any of those conditions.

My preferred direction is not to write state 12 ourselves or call the steering functions manually. Both would skip internal state that the game expects to own. Instead, a research-only virtual drive board could let the original initialization function run and answer it through a controlled synthetic transport. If the conversation completes normally, the game would advance its own states, perform its own cabinet check, and register its own callback.

That idea has an important safety boundary. The original calibration sequence includes requests intended for a physical arcade steering mechanism. We understand how the game constructs those requests and how it samples the steering analog channel, but we do not yet know the authoritative firmware meaning or physical response of every command. A virtual implementation must therefore keep calibration entirely synthetic: no serial passthrough, no SDL force output, and no physical motor access. It would supply a modeled steering-position signal only inside the research boundary.

The offline model now proves that this architecture can represent:

- one or two virtual boards;
- original four- and seven-byte framing;
- the game's duplicated-bit response validation;
- delayed and missing acknowledgments;
- bounded response queues;
- malformed commands;
- disconnects, timeouts, pause/suspend, reset, and shutdown; and
- the original gate from driver state 12 through cabinet check state 2 to callback eligibility.

It does not prove that the assumed responses match Sega's original firmware. Those assumptions stay visible and replaceable. This is the distinction that keeps the next step honest: we have enough evidence to design a safe virtual boundary, but not enough to describe it as a recovered original drive board.

The smallest future implementation would be disabled by default and limited to the verified DVP-0015A executable. It would preserve the original game's state-machine ownership, synthesize steering-position feedback, prevent calibration requests from reaching hardware, and fail closed on any mismatch. Only after that implementation passed offline and platform tests would a new runtime artifact be justified.

For the full state, protocol, and safety analysis, see [AER-02F Virtual Drive-Board Technical Design](VIRTUAL_DRIVEBOARD_MODEL.md).
