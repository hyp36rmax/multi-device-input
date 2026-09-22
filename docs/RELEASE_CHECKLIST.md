# Multi Input release checklist

**PREPARED — AWAITING FINAL APPROVAL.** This is an internal gate, not release
copy. Do not tag or publish until hyp36rmax explicitly approves the final
release. Only after that approval and every gate below passes may this status
become **READY TO PUBLISH**.

## Version decision

- [ ] Approve a version. Recommendation: **v1.0.0** for the first *full product*
  release after final validation. `multi-device-v0.1.0` already exists, so this
  is not literally the first public Multi Input release. Do not assign v1.0.0
  merely because the feature set is complete; the owner must approve it.
- [ ] Replace the current `dev-<short Git SHA>` presentation with the approved
  version in the authoritative `cmake.toml` configuration, regenerate its
  `CMakeLists.txt` mirror, then check startup and About display the same value.
  `src/product_identity.hpp.in` consumes that value; it is not a second version
  to edit independently. The current Git-derived version remains until this
  step is approved.
- [ ] Review Windows resource metadata in `src/Resource.rc` and `src/resource.h`.
  It currently reports upstream Tweaks `0.6.1.0`; keep upstream protocol and
  compatibility versioning distinct from the Multi Input product version.
  Decide the release metadata policy rather than silently changing these
  fields.
- [ ] Set the approved name, version, tag, archive filename, artifact label,
  and notes consistently. Current CI artifacts use
  `multi-input-test-<run>-<SHA>`; they are test artifacts, not a public release.
  The workflow still contains an old v0.1.0 auto-publication step keyed to the
  exact commit message `Prepare v0.1.0 release`. Review or retire it in the
  separately approved release workflow before publishing. Do **not** use that
  message for a preparation commit.

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
- [ ] Keep the release archive separate from `build/bin/*`; the old v0.1.0
  packaging step is not the approved six-file package path.

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

Current preparation changed documentation only. It did not finalize a version,
change package contents, create a tag, or publish a release.
