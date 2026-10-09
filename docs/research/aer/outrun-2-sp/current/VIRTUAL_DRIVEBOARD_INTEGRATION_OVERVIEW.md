# Letting OutRun Start Its Own Steering System Safely

I want OutRun 2 SP to reach its original steering system through the same sequence the arcade game used. The point is not to make LinuxLoader declare the game initialized, install a callback on its behalf, or translate the game into a new force-feedback model. The point is to give the original game a safe, synthetic drive board to talk to so it can complete its own initialization and later produce its own steering requests.

The current shortcut reports that initialization succeeded without performing the work OutRun associates with success. The game therefore never reaches driver state 12, never completes cabinet check state 2, and never installs `CabinetCtrl_Main()` through its normal event system. The offline AER-02D and AER-02F models reproduce that failure and show the lifecycle that would have to be restored.

## The proposed boundary

The safest design is a dedicated research transport behind LinuxLoader's existing filesystem bridge. When explicitly enabled for the one verified DVP-0015A executable, opening the game's first serial port would create a virtual endpoint instead of a real serial connection. The existing `read`, `write`, `select`, `ioctl`, `writev`, and `fwrite` interception paths would route that endpoint to bounded request and response queues.

The original game would still own:

- `CabinetCtrl_InitDriver()` and its states 0 through 12;
- request construction and response validation;
- the cabinet-check table and transition to check state 2;
- registration of `CabinetCtrl_Main()`;
- scheduling of `DrCtrlDataSet()`, `DrCtrlMoveSend()`, and later steering requests.

The loader would own only the synthetic endpoint and its safety policy. It would never write driver state 12, set check state 2, or install the callback directly.

## Why this is separate from normal emulation

The current drive-board emulator was built for existing loader behavior. It can report a serial descriptor as readable when no response is queued, and its read path can return one byte even when it did not supply a valid response. That behavior cannot be used to prove native initialization.

The research transport would use the same interception infrastructure but a separate mode and state object. Readability would mean that actual response bytes are queued. A read with no response would block or report the same no-data condition expected by the intercepted API; it would never manufacture a successful byte. Existing emulation would remain unchanged when the research feature is disabled.

## Revision lock and explicit configuration

The feature would be disabled by default and requested only with `AER_VIRTUAL_DRIVEBOARD=1`. A request is not enough to activate it. LinuxLoader would first verify all of the following:

- the game is OutRun 2 SP SDX Rev A, DVP-0015A;
- the detected game ID and clean executable CRC are the expected values;
- the full `Jennifer` SHA-256 is `f16fc04d836a2bd8e401d8f987d4fe694fa16e18a9f870da6623d7f884911075`;
- every native address needed by the design contains the expected original bytes;
- the configuration does not request the cabinet-check bypass or normal drive-board emulation.

Any mismatch would reject the research session before serial routing or patch selection changes. It would not silently alter existing preferences, partially enable native steering, or fall back to a physical serial port.

## Synthetic calibration, not physical calibration

OutRun's original startup searches for a steering position using bounded test levels and an analog steering source. We know the game's range, stepping, center comparison, and timeout structure, but we do not know the authoritative physical behavior of every board command.

The virtual board would therefore expose a deterministic synthetic steering position only during research initialization. It would start at the verified center reference, move within the native bounds in response to recognized calibration requests, and stop only when the original comparison logic converges. Invalid direction, impossible movement, an unknown request, or timeout would terminate the virtual session.

Nothing in this path would open a real drive-board endpoint, move a wheel, invoke SDL force feedback, or pass a calibration request to hardware. The synthetic model would remain clearly labeled as an assumption rather than recovered Sega firmware.

## Failure means stop

Executable mismatch, malformed traffic, missing replies, queue overflow, disconnect, calibration failure, or an illegal lifecycle transition would permanently fault that research session. Queues would be cleared, the endpoint would stop reporting readiness, and physical fallback would remain forbidden. Restarting the game and loader would be required before another attempt.

Pause and suspend would suppress runtime service without pretending the board disconnected. Shutdown would clear every queued byte and retire all duplicated descriptor aliases deterministically.

## What this milestone does not prove

The design can preserve native ownership and prevent physical output, but offline tests cannot prove that our assumed replies match an original Sega drive board. Exact response meanings, response timing, calibration firmware behavior, dual-board roles, and the runtime callback cadence remain research questions.

The next justified step is not a player build. It is a default-off implementation skeleton with revision checks, transport isolation, and automated proof that no physical or host-FFB path is reachable. Only after that boundary is independently validated should the native initializer be allowed to communicate with it in a research artifact.

## Infrastructure now available

AER-02H.1 adds that skeleton as an isolated research module. It can hash an executable, validate target metadata and a caller-supplied original-byte manifest, reject conflicting configuration, manage bounded virtual descriptor aliases, assemble and validate request frames, queue assumed responses, model readiness and partial reads, and exercise a deterministic synthetic steering sensor.

The module is compiled only by its offline test runner. LinuxLoader does not initialize it, the filesystem bridge does not route to it, and the original cabinet-check bypass remains unchanged. The original-byte manifest is deliberately not populated because the complete verified bytes have not yet been established. That makes the target verifier fail closed for any attempted real activation.

This is useful progress without pretending the board is live. We now have testable pieces for identity, configuration, transport, descriptors, calibration, and shutdown, while the dangerous integration step remains unavailable.

## The eligibility boundary is now concrete

AER-02H.2 fills in the previously missing clean-executable manifest. I verified the full `Jennifer` identity and the original bytes at every current cabinet/drive-board patch site, plus the native check, main-loop, command-construction, steering-request, and serial-send entries. The bootstrap reads those locations through the executable's own ELF segment map. A missing byte, a changed instruction, a wrong revision or CRC, or a different full-file hash makes the complete request ineligible.

This strictness matters because recognizing the game by name is not enough. A nearby revision can place valid-looking code at the wrong address, and an existing loader option can deliberately replace the exact native path being studied. The bootstrap therefore also rejects cabinet-check bypass, the normal drive-board emulator, physical serial passthrough, invalid board counts, or an incomplete filesystem bridge. It does not silently turn any of those settings off.

Passing the check still does not start the steering system. The new module is compiled only by offline tests. It does not participate in loader startup, select patches, allocate descriptors, route serial calls, touch game memory, or reach any physical or host force-feedback output. It records only an atomic decision: everything is eligible, or nothing changes.

The audit also narrowed the future bridge surface. The verified game imports the ordinary open/read/write/fwrite/select/ioctl/close family. LinuxLoader already has optional writev and descriptor-duplication support, while the original executable supplies no evidence that openat, dup2, or dup3 needs a new interception path. I am leaving those unsupported paths alone until original evidence says otherwise.

One uncertainty remains intentionally visible: the loader's existing `SetOutFactor()` modification changes an original steering-output gate. It is not an initializer bypass, but its effect on future native-output fidelity needs separate evidence. The next milestone should resolve patch and observer ordering and prove the bridge ABI without yet activating native initialization.

## The bridge is connected, but the game is still not activated

AER-02H.3 connects the isolated board to LinuxLoader's existing file-operation boundary. I kept one ownership path: the same open, read, write, readiness, duplication, and close interception already used by the loader now recognizes a bounded research descriptor only after the complete revision and configuration check succeeds.

The important safety behavior is what happens when anything is wrong. An explicit research request that fails verification cannot continue into the configured physical serial path. It receives no half-created descriptor and does not silently turn on the normal emulator. With research mode absent, ordinary LinuxLoader behavior remains unchanged.

The adapter also behaves like a real byte endpoint rather than the older always-ready shortcut. Empty response queues are not readable, `FIONREAD` reports the actual byte count, scattered writes keep their order, partial frames stay bounded, and duplicated descriptors share one board state until the final alias closes.

This still does not start Sega's steering system. The adapter invents no board reply, changes no cabinet patch, forces no driver/check state, installs no game callback, and has no route to a motor or host FFB API. Automated Windows and Linux builds now exercise the same research boundary, but successful compilation will establish platform compatibility only—not arcade hardware authenticity or native FFB operation.
