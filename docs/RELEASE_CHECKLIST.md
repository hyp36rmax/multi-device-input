# Multi Input release checklist

**FINAL PREPARATION — RELEASE ACTION NOT YET AUTHORIZED.** This is an internal
gate, not release copy. Physical testing and the v1.0.0 version are approved.
Do not create a tag or GitHub Release until hyp36rmax reviews the final archive
and explicitly authorizes that action.

## Version decision

- [x] Set the approved **v1.0.0** version. `multi-device-v0.1.0` was an earlier
  public test release. The authoritative `cmake.toml` configuration and its
  generated `CMakeLists.txt` mirror feed the startup card and About screen
  through `src/product_identity.hpp.in`. Build SHA is separate.
- [x] Keep Windows resource metadata in `src/Resource.rc` and `src/resource.h`
  at upstream Tweaks `0.6.1.0`. That compatibility version is separate from
  the Multi Input product version; changing it is outside this release pass.
- [x] Align the proposed tag `v1.0.0`, archive
  `OutRun-2006-C2C-Multi-Input-v1.0.0.zip`, CI artifact label, and release
  notes. The old v0.1.0 auto-publication step has been removed. This workflow
  builds and uploads an artifact; it does not publish a Release.

## Intended six-file package

The current Windows workflow stages these six files and fails if one is
missing or an extra file appears. Preserve this manifest unless the owner
separately approves a package change.

| File | Purpose |
| --- | --- |
| `OR2006C2C.exe` | Supported replacement game executable, obtained from the established upstream source; required SHA-256 `68ceb386829066f8455b9d027320af962584321f3e2e8a79c72841495a6134c3` |
| `dinput8.dll` | Multi Input runtime wrapper |
| `OutRun2006Tweaks.ini` | Shipped base settings |
| `OutRun2006Tweaks.lods.ini` | Shipped LOD settings |
| `README.md` | Install, controls, safety, troubleshooting, online documentation |
| `LICENSE.md` | Upstream MIT license and copyright notice |

- [ ] Final Win32 Release CI passes tests, supported-EXE hash check, six-file
  assertion, and artifact upload on the exact approved commit.
- [ ] Download the exact artifact selected for publication. Independently list
  all six files, check the EXE hash, record SHA-256 for the archive and DLL,
  and confirm there are no helper/test EXEs, research tools, launcher, or
  background service.
- [ ] Verify the packaged README's online documentation links and local license
  link. The six-file package does **not** include the `docs/` directory.
- [x] Create the release archive from the asserted six-file staging directory,
  not `build/bin/*`.

## Fresh installation and player check

- [ ] Use a legitimate installed PC copy of OutRun 2006: Coast 2 Coast. Extract
  the complete package into the game's main folder, not a subfolder. Keep a
  backup of existing personal settings before replacing files.
- [ ] Launch the included `OR2006C2C.exe` without manual INI edits. Confirm
  the displayed Multi Input name/version and visible emoose attribution.
- [ ] Open **Options → Controller**; confirm the overlay opens. Connect a mixed
  device setup, complete Quick Setup, confirm each prompt, verify live inputs,
  and save bindings.
- [ ] Restart and confirm bindings and the resolved wheel persist. Verify
  Re-detect Wheel works after a device refresh without losing input bindings.
- [ ] Confirm the default player experience: Reference+; Strength 100%; Steering
  Load, Road Detail, and Impact each 100%; Invert Wheel Off. Use a conservative
  wheel-side torque setting. Verify each advanced control attenuates only its
  channel and that Reset to Defaults restores the intended settings.
- [ ] Check Invert Wheel, short Left/Right tests, race startup, normal driving,
  normal exit, and the final log. Stop immediately if wheel behavior is unsafe.
- [ ] Complete the remaining device and regression evidence in
  [V1 release issues](V1_RELEASE_ISSUES.md) before approving publication.
  Validation stays here, not in public release copy.

## Documentation and preservation

- [ ] Read the packaged [README](../README.md) as a first-time player. Confirm
  installation, in-game setup, VC++ x86 guidance, wheel detection, force
  direction, Re-detect, Reset to Defaults, credits, and license are clear.
- [ ] Check the [release draft](RELEASE_DRAFT.md) for accurate highlights,
  natural voice, upstream credit, and no internal milestone language or
  unvalidated hardware claims. Approve the final text and version together.
- [ ] Confirm Unlock All / ENTIRETY is absent from product and release copy;
  its documents remain historical feasibility research only.
- [ ] Archive the [46 inventoried raw telemetry CSVs](../research/README.md)
  from outside Git to a durable, access-controlled location. Record file
  checksums, verify a restore, and update the preservation index. This is a
  private provenance gate, not a player feature or release-package file.
- [ ] Obtain explicit final approval from hyp36rmax. Only then create the
  approved tag and GitHub Release using the verified six-file artifact.

This preparation does not create a tag or publish a GitHub Release. The final
artifact and physical startup-card layout still require verification before
the release action is authorized.
