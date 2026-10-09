# AER-02G Virtual Drive-Board Runtime Integration Specification

## Status and scope

Target: OutRun 2 SP SDX Rev A, DVP-0015A

Executable: `Jennifer`

SHA-256: `f16fc04d836a2bd8e401d8f987d4fe694fa16e18a9f870da6623d7f884911075`

This is a design contract. It adds no runtime integration, patches, serial routing, physical output, host FFB, or executable modification.

## Non-negotiable native ownership

The accepted runtime sequence is:

```text
CabinetCtrl_InitDriver()                    0x08103EAA
  -> native states 0..10
  -> native response validation
  -> native steering-position search
  -> native state 12
CabinetCtrl_Check()                         0x0810477E
  -> native check table
  -> native check state 2
ChangeNowEventCtrlFunc(CabinetCtrl_Main)    0x0806D050
CabinetCtrl_Main()                          0x081048B2
  -> DrCtrlDataSet()
  -> DrCtrlMoveSend()
  -> steerReqSendOut()
  -> hardcomSend()
```

The integration must not write driver state `0x0866D460`, write check state, install `CabinetCtrl_Main()`, or invoke output functions directly. Callback installation is valid only as the original consequence of completed check state 1.

## Existing LinuxLoader integration surface

| Concern | Existing owner | Design use |
| --- | --- | --- |
| Game/revision classification | `config.h`, `gameData.c`, `mainShared.c`, `config.c` | First eligibility layer: `OUTRUN_2_SP_SDX_SBMB_REVA`, DVP-0015A, clean CRC |
| Patch selection | `patching/patch.c` | Suppress the conflicting bypass/emulator patches only after complete eligibility succeeds |
| SERIAL0 open/routing | `redirections/filesystemShared.c::sharedOpen()` | Return a loader-owned virtual descriptor; never call the configured real serial path |
| `read`/`write`/`select`/`ioctl` | `filesystemShared.c` | Route virtual descriptor aliases to the research transport |
| Windows `writev`/`fwrite`/`dup` | `elfLoader/filesystemBridge.cpp` | Use the same authoritative descriptor registry and transport operations |
| Normal drive-board emulator | `hardware/lindbergh/driveBoard.c` | Reuse no mutable globals; retain unchanged as the disabled-mode path |
| Diagnostics/recording | `research/aer*` | Observe requests and lifecycle without becoming transport authority |
| Shutdown | Linux destructor and Windows loader shutdown ownership | Close transport once, clear queues/aliases, keep shutdown idempotent |

### Narrow boundary

Introduce one research transport interface behind the filesystem bridge rather than a second filesystem hook stack:

```c
eligible/open -> aerVirtualDriveboardOpen()
write family  -> aerVirtualDriveboardWrite()
read family   -> aerVirtualDriveboardRead()
select/ioctl  -> aerVirtualDriveboardReadable()
dup/close     -> aerVirtualDriveboardAlias()/Close()
shutdown      -> aerVirtualDriveboardShutdown()
```

These names are illustrative, not implementation in this milestone. The interface owns no game addresses and performs no native state transition.

## Revision-locking contract

Activation requires every check below. They are conjunctive, not alternatives.

1. `AER_VIRTUAL_DRIVEBOARD=1` parses as explicitly enabled.
2. Game identity resolves to `OUTRUN_2_SP_SDX_SBMB_REVA` and DVP-0015A, not either test executable or DVP-0015.
3. The clean ELF CRC is the established DVP-0015A `Jennifer` value, not merely a group match.
4. A loader-computed full SHA-256 equals the value above. An environment-provided hash is diagnostic metadata only and cannot authorize activation.
5. Required virtual addresses and original instruction bytes match a versioned manifest before any research-related patch is selected.
6. Required filesystem symbols are intercepted and the platform supports the complete descriptor contract.
7. `SKIP_OUTRUN_CABINET_CHECK=0` and normal `EMULATE_DRIVEBOARD=0`.
8. Board count is one or two and all research configuration is valid.

The byte manifest covers the initializer, actuator check, native callback/output entries, and every current DVP-0015A patch site relevant to activation or steering output. The following bytes were read from the verified `Jennifer` ELF identified above; addresses are original virtual addresses and lengths follow complete instruction or data-entry boundaries.

| Address | Length | Original bytes | Owner | Existing loader action |
| --- | ---: | --- | --- | --- |
| `0x08105317` | 6 | `0f 84 6f 01 00 00` | `SetOutFactor()` | Base patch replaces the first five bytes with `e9 1f 00 00 00` |
| `0x08109593` | 2 | `78 43` | `hmmInitInternal()` | Base patch writes `90 90` |
| `0x08109597` | 2 | `78 3f` | `hmmInitInternal()` | Base patch writes `90 90` |
| `0x0810959d` | 2 | `7f 22` | `hmmInitInternal()` | Base patch changes the opcode to `77` |
| `0x081e2180` | 4 | `db 41 10 08` | `CabinetCtrl_InitDriver()` state table | Emulator patch changes its low word to `df 43` |
| `0x0810401b` | 1 | `0b` | `CabinetCtrl_InitDriver()` | Emulator patch changes driver state 11 to 12 |
| `0x08103eaa` | 6 | `55 89 e5 83 ec 28` | `CabinetCtrl_InitDriver()` entry | Cabinet bypass installs a return-one detour |
| `0x08105d88` | 6 | `55 89 e5 83 ec 18` | `hardacuIsInitEnd()` entry | Cabinet bypass/emulator installs a return-one detour |
| `0x0810477e` | 6 | `55 89 e5 83 ec 28` | `CabinetCtrl_Check()` entry | No base patch; native integrity requirement |
| `0x081048b2` | 6 | `55 89 e5 57 56 53` | `CabinetCtrl_Main()` entry | No base patch; native integrity requirement |
| `0x08104f02` | 6 | `55 89 e5 57 56 53` | `DrCtrlDataSet()` entry | No base patch; native integrity requirement |
| `0x081051f4` | 6 | `55 89 e5 83 ec 18` | `DrCtrlMoveSend()` entry | No base patch; native integrity requirement |
| `0x08105ad2` | 6 | `55 89 e5 57 56 53` | `steerReqSendOut()` entry | No base patch; native integrity requirement |
| `0x0810735e` | 6 | `55 89 e5 8b 4d 08` | `hardcomSend()` entry | No base patch; native integrity requirement |

The bootstrap maps each virtual address through the ELF32 `PT_LOAD` table and compares every byte before declaring eligibility. An unreadable, truncated, missing, or mismatched entry rejects the entire request. This verifies the clean file only; a future integration milestone must also define patch-selection ordering and expected in-memory bytes.

### Existing-patch conflict matrix

| Patch/hook group | Classification | Reason |
| --- | --- | --- |
| `SKIP_OUTRUN_CABINET_CHECK`: return-one hooks at `0x08103eaa` and `0x08105d88` | **CONFLICTING** | Replaces original initializer/actuator-check ownership and prevents proof of native progression. |
| `EMULATE_DRIVEBOARD`: table patch `0x081e2180`, state patch `0x0810401b`, return-one hook `0x08105d88` | **CONFLICTING** | Rewrites driver-state behavior and supplies the existing emulated activation path. |
| `SetOutFactor()` patch at `0x08105317` | **REQUIRES FURTHER EVIDENCE** | Does not replace initialization, but changes the original steering-output gate and therefore cannot yet be called native-equivalent. |
| `hmmInitInternal()` patches at `0x08109593`, `0x08109597`, `0x0810959d` | **COMPATIBLE** | Heap/runtime compatibility patches; no observed ownership of cabinet initialization, serial routing, or steering commands. |
| AER native-activation diagnostic observers | **REQUIRES FURTHER EVIDENCE** | Passive by design, but future hook chaining and verification order must be defined before sharing native entry sites. |
| Unrelated security, shader, and general compatibility patches | **COMPATIBLE** | No address overlap or established path into the verified cabinet/drive-board control flow. |

No patch is removed or changed by the bootstrap. Conflicting configuration is rejected rather than rewritten.

### Rejection

Failure occurs before virtual SERIAL0 routing or patch mutation. Log one stable reason code, leave existing user configuration unchanged, and run the loader's ordinary path. To avoid partial native activation, a configuration that requests the research mode but fails after any mutation is a fatal startup error, not a fallback.

## Configuration contract

`AER_VIRTUAL_DRIVEBOARD` is a default-off research setting. Environment syntax is proposed because existing AER modules use explicit environment gates; a later implementation may expose it in research configuration only if it retains identical semantics.

| Requested configuration | Result |
| --- | --- |
| Virtual off | Existing behavior unchanged |
| Virtual on, skip off, emulation off, identity valid | Eligible |
| Virtual on with `SKIP_OUTRUN_CABINET_CHECK=1` | Reject: bypass prevents native ownership |
| Virtual on with `EMULATE_DRIVEBOARD=1` | Reject: two transports would compete |
| Virtual on with physical serial passthrough requested | Reject: no physical fallback |
| Virtual on, identity or byte check fails | Reject before patch/routing mutation |

The setting must not silently rewrite either existing option. Startup diagnostics report requested, eligible/rejected, identity results, transport mode, board count, and reason. They must not claim hardware authenticity.

## Eligibility bootstrap implemented by AER-02H.2

`aerVirtualDriveboardBootstrap.c` is an offline research boundary, not a LinuxLoader startup component. It evaluates one request exactly once unless explicitly reset. Eligibility requires the requested flag, DVP-0015A revision string, CRC `0x4debd5f0`, loader-side full-file SHA-256, all 14 original-byte entries, nonconflicting configuration, board count one or two, and the complete required bridge-capability mask.

Evaluation is atomic: the state exposes `eligible` only after all checks pass, and its mutation counter remains zero for success and every rejection. It allocates no descriptor, changes no patch, opens no serial endpoint, and has no fallback path. Default-off and rejected evaluations therefore leave runtime behavior untouched. Repeated evaluation is rejected; reset only clears the offline decision state.

The compile-time production identity remains the authoritative SHA-256. Tests may substitute a synthetic-fixture digest only when built with `AER_VDB_TESTING`.

## Filesystem bridge capability audit

The verified Jennifer import surface reaches `open`, `read`, `write`, `fwrite`, `select`, `ioctl`, and `close`. These form the required bootstrap capability mask. The current Windows bridge additionally intercepts `writev` and `dup`, so they are represented as optional capabilities for the later routing milestone.

Jennifer does not import `openat`, `dup2`, or `dup3`; no evidence currently justifies new global interception for those APIs. LinuxLoader has a native `openat` wrapper on Linux, but the Windows ELF bridge does not map it. Any later static or runtime evidence that one of these paths is reachable must expand both the capability gate and the transport tests before activation.

## AER-02H.2 automated evidence

The focused synthetic-ELF suite verifies default-off, explicit eligibility, wrong revision, CRC mismatch, SHA mismatch, missing executable, modified bytes, incomplete/truncated manifest, each configuration conflict, invalid board counts, missing required bridge capability, repeated evaluation, reset, and zero mutation throughout. A link/dependency check also rejects references from the bootstrap object to serial passthrough, SDL/evdev force feedback, motion output, actuator control, or runtime patch/hook functions.

Existing AER recorder, activation-diagnostic, native-activation, offline-activation, virtual-model, and virtual-infrastructure suites remain the regression authority. No runtime build file includes this bootstrap.

## AER-02H.3 filesystem bridge adapter

The research adapter now uses LinuxLoader's existing shared filesystem interception layer. It does not introduce another global hook stack. `sharedOpen()` owns SERIAL0 selection; `sharedRead()`, `sharedWrite()`, `sharedIoctl()`, `sharedSelect()`, and `sharedClose()` recognize only descriptors registered by the bounded AER registry. Linux `writev`, `fwrite`, and `dup` wrappers and their Windows C++ bridge equivalents call the same adapter operations.

Startup evaluates `AER_VIRTUAL_DRIVEBOARD=1` once through the AER-02H.2 bootstrap. Windows supplies the absolute loaded-ELF path; Linux resolves `/proc/self/exe`. A rejected explicit request makes SERIAL0 open fail with no physical fallback. With the setting absent, the added branches are dormant and the existing serial/emulator behavior remains authoritative.

An eligible SERIAL0 open creates one loader-owned backing descriptor and attaches one transport. Aliases share its queues and lifecycle. Closing one alias preserves the transport; final close clears it. The registry remains bounded and rejects reuse/overflow. SERIAL1 is unchanged.

### API coverage and platform boundary

| Operation | Integrated behavior |
| --- | --- |
| `open` / Linux `openat` | SERIAL0 enters the virtual path only after complete eligibility; rejected research requests cannot fall through to physical serial. |
| `read` | Copies only queued bytes; empty reads return `EAGAIN`; closed aliases return `EBADF`. |
| `write` | Uses bounded partial-frame assembly and reports accepted bytes exactly. Failed frames do not queue replies. |
| `writev` | Flattens vectors as one ordered logical stream through the same atomic transport operation. |
| `fwrite` | Returns completed elements, not byte counts. |
| `select` | Reports virtual read readiness only with queued bytes and write readiness only while the bounded queue can accept work. |
| `ioctl(FIONREAD)` | Returns the exact queued-byte count through an `int`; other virtual requests fail explicitly. |
| `dup` | Adds a bounded alias only after OS duplication succeeds. |
| `close` | Removes one alias and shuts down on final close. |

The verified Jennifer import table does not reach `dup2` or `dup3`; neither is added. Linux's existing `openat` wrapper reaches `sharedOpen` for serial paths. Windows ELF imports are mapped through `filesystemBridge.cpp`, whose Linux ABI fd sets remain distinct from Winsock descriptors.

### Isolation and present limitation

The adapter links only to the bootstrap and isolated transport. Static symbol checks prohibit serial passthrough helpers, SDL/evdev FFB, motion output, cabinet callbacks, native state functions, and hardcom ownership. Failure after routing faults the research transport and never switches to another endpoint.

Unknown response bytes remain test-only synthetic assumptions. The adapter does not generate firmware responses on its own. Consequently it cannot advance native initialization, and the existing cabinet/drive-board patches remain unchanged. This milestone proves the communication ABI and isolation boundary, not functioning native FFB.

The focused suite covers default-off and rejected bootstrap states, attachment, shared aliases, ordered partial writes, `writev`, `fwrite`, empty and partial reads, queued-byte reporting, readiness, queue overflow, timeout, disconnect, repeated shutdown, final-close cleanup, and prohibited link dependencies. Both platform workflows run the bootstrap and bridge suites before their authoritative 32-bit builds.

## Virtual transport contract

### Lifecycle

```text
disabled -> eligible -> idle -> initializing -> configuring
         -> calibrating -> ready -> shutdown
any active state -> fault
```

`fault` and `shutdown` are terminal for a process. Repeated initialization is permitted only through an explicit reset while the transport remains idle and has no descriptors, queued bytes, or retained calibration state. After activation begins, failure requires a full research-session restart.

### Queues and ordering

- Fixed-capacity request and response queues; no unbounded allocation.
- Whole frames are validated before state mutation.
- Single-board four-byte and dual-board seven-byte framing follows the AER-02F model.
- Partial writes accumulate only up to one maximum frame and have a bounded completion timeout.
- Requests are processed in write order; responses are read in queue order.
- One native response byte is supplied per configured board when the original protocol requests it.
- Queue overflow, an unknown command in initialization, bad XOR, or illegal frame length faults the session.
- Runtime steering requests after readiness may be recorded passively, but receive no host-FFB conversion or physical forwarding.

### Assumption boundary

Packet framing and native response-byte validation are established. The status returned for each request and its timing are not fully established. A response-policy table must therefore mark each entry `verified`, `structurally inferred`, or `synthetic assumption`. Implementation cannot promote an assumed response to verified evidence.

## API behavior

All calls first resolve the descriptor through one alias registry. Closing the final alias shuts the endpoint; closing one duplicate does not invalidate the others.

### `open`

For eligible research mode, `/dev/ttyS0` and `/dev/tts/0` return a loader-owned virtual descriptor. No call may open `SERIAL_1_PATH` or another physical endpoint. Repeated opens either return aliases to the same session or fail deterministically; they never create competing boards.

### `select`

- Readable only when at least one response byte is queued for that descriptor.
- Writable only while the request queue can accept data and lifecycle permits writes.
- Preserve unrelated descriptors and return the number of ready descriptors, not unconditional `1`.
- Respect zero, finite, and null timeouts without busy readiness.
- Fault/closed descriptors use normal error/exception semantics rather than fake readable data.

### `ioctl`

For the game's bytes-available query (`FIONREAD`/Linux request `0x541B`), write the exact queued response-byte count and return success. Zero queued bytes is a successful query returning zero. Unsupported requests either reproduce the harmless serial contract established by static analysis or fail with the platform-equivalent error; they cannot report invented readiness.

### `read` and `readv`

- Consume at most the requested queued bytes and return the number actually copied.
- Never return `1` without writing one byte.
- Empty nonblocking reads return the correct no-data error; blocking behavior follows the intercepted descriptor contract.
- Partial reads retain remaining bytes and readiness.
- Invalid pointers/counts and closed descriptors return the matching API error without state mutation.

### `write`, `__write`, and `writev`

- Return only bytes accepted by the bounded assembler/queue.
- `writev` is one logical byte stream; it must not expose intermediate fragments as separate transactions.
- Partial acceptance is reported accurately; failed writes do not enqueue or mutate lifecycle state.
- A complete validated frame produces at most the response(s) declared by the active response policy.
- Recorder observation wraps the authoritative result and does not change it.

### `fwrite`

The bridge must resolve `FILE* -> fileno -> descriptor registry`, pass the complete element stream through the same transport, and return completed elements, not raw bytes. Partial final elements are not reported as complete. Buffered flushing cannot bypass shutdown/fault checks.

### `dup`

Register the returned descriptor as an alias only after OS duplication succeeds. Aliases share queues, lifecycle, flags, and readiness. A future `dup2` interception is required if the target executable imports or reaches it; absence must be proven from the verified import/call surface before implementation.

### Failed operations

Failed or zero-length operations remain observable to AER recording but cannot synthesize responses. `errno` and return types must match the platform-facing Linux ABI used by the bridge.

## Virtual steering sensor

### Established native inputs

- Initialization state 8 samples the configured steering analog channel.
- It compares the sample against the stored center reference at `0x0866C3F0`.
- The configured source is selected from the game's analog-channel table near `0x0866C3E8`.
- Signed test level changes in steps of 10 and remains in `[-96, 96]`.
- Native convergence advances to state 9; 900 ticks without convergence calls `CabinetCtrl_Off()` and enters state 11.

### Synthetic contract

Each virtual board owns `{position, center, target, last_request, update_count}`. Reset sets `position == center` and clears motion. Recognized calibration requests may choose a bounded synthetic target based on request direction. On each deterministic transport step, position moves toward target by a fixed model increment and is clamped to the native analog range used by the game-side input layer.

The exact increment and request-direction mapping remain unverified and must be parameters in tests, not described as Sega measurements. The implementation must demonstrate that:

- the original state-8 comparison converges under a declared synthetic policy;
- reversing or impossible movement does not converge;
- position cannot escape the original input range;
- no physical input is read and no output API is called;
- reset and shutdown restore center and clear target state;
- the game's original 900-tick timeout remains authoritative.

The virtual sensor supplies only the analog value required by the original game. It does not write driver/check states or declare calibration complete.

## Physical-output isolation

Eligibility establishes a hard routing invariant before native initialization is restored:

1. SERIAL0 is backed only by the loader-owned virtual descriptor.
2. Physical serial paths cannot be opened by virtual transport code.
3. Initialization and runtime requests cannot call `sdlFfbDriveboard()`, evdev FF APIs, motion-board APIs, or passthrough routines.
4. No error path falls through to normal `write`, configured serial paths, or a real endpoint.
5. Native runtime requests may be copied to the AER recorder only after their original API result is determined.
6. Compile/link tests must show the virtual transport has no physical-output dependencies; runtime tests must inject all I/O.

The implementation should place hardware sinks behind an interface unavailable to the research transport target rather than rely solely on conditionals.

## Failure and recovery

| Failure | Required action |
| --- | --- |
| Identity/revision/byte mismatch | Reject before mutations; ordinary loader behavior remains intact |
| Configuration conflict/invalid board count | Reject before mutations |
| Missing/malformed/unexpected response | Fault, clear queues, make endpoint unavailable |
| Response/calibration timeout | Let original game take its error path; transport then faults |
| Queue/partial-frame overflow | Fault without processing the frame |
| Illegal lifecycle transition | Fault and record prior/current/requested states |
| Disconnect/final close | Fault if active; otherwise clean shutdown |
| Pause/suspend | Preserve queues; native event scheduling suppresses service; do not fabricate progress |
| Game shutdown | Idempotently clear queues, descriptors, sensor state, and diagnostics |
| Repeated initialization after active failure | Reject until process restart |

Once patch selection or virtual routing begins, fallback to the normal emulator or physical serial path is forbidden. A partial activation failure exits or leaves the game on its original error path; it does not force native success.

## Architecture comparison

| Option | Original-flow fidelity | Isolation | Compatibility | Complexity | Reversibility/testing | Assessment |
| --- | --- | --- | --- | --- | --- | --- |
| A. Modify existing SERIAL0 emulation path | Medium; easy to inherit global-response shortcuts | Weakest unless extensively split | Risks every emulator user | Moderate initially, high regression cost | Poor separation | Reject |
| B. Dedicated research transport behind filesystem bridge | High; original game sees normal serial APIs | Strong, explicit descriptor type and no hardware sink | Default path can remain byte-for-byte unchanged | Moderate | Best injection/unit-test boundary | Recommend |
| C. Add native-init mode inside current emulator | Medium-high | Better than A but shares mutable globals and `sdlFfbDriveboard()` reachability | Conditional complexity in existing emulator | High | Testable but entangled | Do not prefer |

**Recommendation: Option B.** It extends the existing filesystem interception boundary but gives native activation a dedicated transport implementation. This preserves existing emulation, isolates hardware dependencies, and permits the AER-02F model to become a replaceable response policy rather than new global emulator behavior.

## Automated validation plan

Before any runtime artifact, implementation tests must cover:

1. Revision manifest: valid identity plus every individual failure and atomic rejection.
2. Configuration matrix above and proof that rejected configuration changes no patch/routing state.
3. Single/dual framing, partial assembly, ordering, capacity, checksum, timeout, disconnect, reset, and shutdown.
4. Exact `select`, `ioctl`, `read`, `readv`, `write`, `writev`, `fwrite`, `dup`, close, and error contracts.
5. Synthetic calibration convergence, wrong-direction failure, bounds, timeout, and reset.
6. Original activation-model progression to state 12/check 2 without direct assignments.
7. Callback gating through original check completion only.
8. Pause/suspend and shutdown ownership.
9. Link/dependency test proving no serial passthrough, SDL/evdev FFB, or motion symbol is reachable.
10. Existing raw recorder, activation diagnostics, native activation, normal emulator, and non-OutRun regressions.
11. Linux 32-bit and Windows i686 builds before any controlled artifact.

Current AER-02D/AER-02F tests validate lifecycle logic, bounded queues, framing, synthetic assumptions, failure cases, callback gates, and physical-isolation intent. They cannot prove bridge ABI behavior, exact patch bytes, SHA calculation on both platforms, original firmware replies, or runtime compatibility.

## Unresolved evidence

- `AER-HW-RESP-001`: exact status byte per original request and timing.
- `AER-HW-CAL-001`: firmware and physical calibration semantics.
- `AER-HW-POWER-001`: physical meaning of power/configuration codes.
- `AER-HW-DUAL-001`: board roles and independent response behavior.
- `AER-HW-CADENCE-001`: authoritative callback cadence after activation.
- Exact original byte manifest for every conflicting patch site.
- Whether the verified executable uses `dup2`, buffered `fwrite`, or another reachable I/O path requiring interception.
- Platform-correct blocking and `errno` behavior for the current synthetic hook descriptor.

These gaps allow a default-off implementation skeleton and injected integration tests, but not a claim of hardware-faithful board emulation or a gameplay-ready build.

## Implementation gate

Implementation is justified only as the next **research skeleton milestone**: revision verifier, configuration arbiter, descriptor/transport interface, synthetic sensor, and fully injected tests. Restoring the native initializer or producing a DEV build remains gated on passing those tests and closing the original-byte manifest. No physical output or host FFB belongs in that milestone.

## AER-02H.1 implemented infrastructure

The isolated `aerVirtualDriveboard` research module now implements the skeleton described above without entering the production build graph:

- a streaming, loader-computed SHA-256 file verifier;
- exact revision and CRC checks;
- caller-supplied original-byte manifest validation that rejects missing, incomplete, or mismatched entries;
- a configuration arbiter covering bypass, emulator, physical-serial, board-count, and verification conflicts;
- an eight-slot descriptor registry with alias ownership, generation increments on reuse, and final-close shutdown;
- fixed-capacity request/response queues;
- four-byte single-board and seven-byte dual-board framing with XOR validation;
- atomic `writev` assembly, bounded partial writes, partial reads, element-count `fwrite`, queued-byte readiness, and assumed-response injection;
- terminal fault behavior for checksum errors, overflow, timeout, and disconnect;
- a deterministic synthetic sensor using step 10, `[-96,96]`, and a 900-tick bound; and
- idempotent shutdown.

The response policy remains injectable: the module never assigns a firmware meaning to a request. It does not call the filesystem bridge, serial passthrough, SDL/evdev FFB, or motion APIs. A link-symbol test rejects those dependencies.

### Deliberate non-integration

The module is not referenced by LinuxLoader's runtime sources or CMake target. `AER_VIRTUAL_DRIVEBOARD` is therefore a documented future setting, not an active environment switch. No active bridge API is changed and no serial descriptor is routed to this module.

The production original-byte manifest remains incomplete. Tests may supply synthetic byte arrays and a test-only expected hash to validate verifier behavior, but non-test compilation always requires the fixed `Jennifer` SHA-256. Real activation must remain unavailable until the verified byte manifest is completed and reviewed.
