# Multi Input v1.5 release checklist

**RELEASE CANDIDATE — RELEASE ACTION NOT YET AUTHORIZED.** Do not create a tag,
merge to master, or publish a GitHub Release until hyp36rmax reviews the exact
artifact and explicitly authorizes release.

## Identity

- [ ] The RC and final product identity is `1.5.0`; engineering build/commit
  metadata remains available separately and is not appended to player-facing
  release identity.
- [ ] Windows version resources identify Multi Input `1.5.0` and separately
  retain the OutRun2006Tweaks `0.6.1.0` foundation attribution.
- [ ] The proposed final tag, archive name, CI artifact, and release notes all
  agree on v1.5. Do not change the source identity to final before approval.

## Authoritative seven-file package

The Windows workflow must stage exactly these seven files. It must fail if one is
missing or any extra file appears.

| File | Purpose |
| --- | --- |
| `OR2006C2C.exe` | Supported replacement executable; required SHA-256 `68ceb386829066f8455b9d027320af962584321f3e2e8a79c72841495a6134c3` |
| `dinput8.dll` | Multi Input runtime wrapper |
| `OutRun2006Tweaks.ini` | Shipped base settings |
| `OutRun2006Tweaks.lods.ini` | Shipped LOD settings |
| `README.md` | Install, setup, safety, upgrade, and troubleshooting guidance |
| `LICENSE.md` | Upstream MIT license and copyright notice |
| `RELEASE_NOTES.md` | Frozen v1.5 release notes copied from the repository release-note source |

`OutRun2006Tweaks.user.ini` is not a package file. Helper/test executables,
research tools, launchers, background services, and `docs/research/` are also
excluded.

- [ ] Required Win32 Release CI passes tests, supported-EXE hash verification,
  the exact-manifest assertion, and artifact upload on the approved commit.
- [ ] Download the exact candidate artifact. List all six files, verify the EXE
  hash, and record SHA-256 for the archive and DLL.
- [ ] Confirm the packaged README and license open correctly. The package does
  not include the repository `docs/` directory.

## Release-candidate physical validation

- [ ] Fresh extraction into a legitimate installed game folder launches using
  the included `OR2006C2C.exe`, without manual INI edits.
- [ ] Startup shows the expected Multi Input candidate identity and visible
  OutRun2006Tweaks v0.6.1.0 by emoose attribution.
- [ ] **Options → Controller** opens the Multi Input controller overlay.
- [ ] Quick Setup completes and saves a basic configuration.
- [ ] Manual Bindings can add, remove, and invert an assignment.
- [ ] A wheel plus separate pedals and shifter work simultaneously.
- [ ] Bindings, device assignments, and the resolved FFB endpoint persist after
  a normal exit and restart.
- [ ] Test Left and Test Right are brief and move in the expected direction.
- [ ] Invert Wheel reverses direction correctly.
- [ ] Classic Road drives normally and retains shared Surface.
- [ ] Enhanced Road drives normally and retains shared Surface.
- [ ] Surface at 0%, 50%, and 100% changes only Surface intensity as expected.
- [ ] Automatic Bump is present without exposing separate player tuning.
- [ ] Re-detect Wheel refreshes the FFB endpoint without losing bindings.
- [ ] General Capture starts, records, stops, and uses its automatic filename
  under `Telemetry/General Capture/`.
- [ ] Guided UAT starts and writes research output under
  `Telemetry/Research/`.
- [ ] A gamepad completes a basic menu and driving smoke test.
- [ ] Normal exit and relaunch complete without a crash or stale force.
- [ ] The tested files are byte-for-byte the exact packaged artifact selected
  for release.

Stop immediately if wheel behavior is unsafe. Use a conservative wheel-side
torque limit during validation.

## Documentation and release copy

- [ ] Read the packaged [README](../README.md) as a first-time player. Confirm
  install, Options → Controller setup, FFB safety, VC++ guidance, upgrade
  behavior, telemetry, credits, and troubleshooting are clear.
- [ ] Review the [release draft](RELEASE_DRAFT.md) for natural player-facing
  language, v1.5 accuracy, upstream attribution, and no deferred research
  advertised as shipping functionality.
- [ ] Confirm all public copy uses **HYP36rforce FFB** and keeps Multi Input
  `1.5.x` separate from upstream `0.6.1.0`.
- [ ] Confirm Unlock All, SimHub, active pedals, motion, bass shakers,
  leaderboards, material-specific Surface profiles, AER expansion, and other
  post-v1.5 research are absent from shipping claims.

## Protected release path

- [ ] Use the protected path: `feature branch → pull request → required Win32
  CI → resolved conversations → master`.
- [ ] Do not bypass repository protections or add bypass actors.
- [ ] After the approved merge, create the protected final `v*` tag from the
  verified release commit only.
- [ ] Obtain explicit final approval from hyp36rmax before creating the tag or
  GitHub Release.

This checklist prepares and verifies a release; it does not authorize one.
