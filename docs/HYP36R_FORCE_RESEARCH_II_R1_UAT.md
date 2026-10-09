# HYP36R Force Research II — R1 Windows schema UAT

## Purpose

R1-UAT validates the `HYP36R_RESEARCH_II_R1` measurement system in the real
Windows game. It is not the R2 driving campaign and changes no Force behavior.
The production v1.0.0 release and tag remain unchanged.

## Permanent UAT artifact convention

When a milestone needs tangible runtime testing, its artifact contains one
top-level `UAT/` directory. Everything inside that directory is copied directly
into the target application's root. Milestones without tangible testing do not
create a UAT directory.

This artifact contains exactly:

```text
UAT/
  OR2006C2C.exe
  dinput8.dll
  OutRun2006Tweaks.ini
  OutRun2006Tweaks.user.ini
  README.md
  LICENSE.md
```

The controlled user INI enables telemetry with scenario `R1_SCHEMA_UAT`, keeps
Reference+ active, and fixes Strength, Steering Load, Road Detail and Impact at
100%. It does not amplify or retune Force.

## Runtime evidence layout

Only when telemetry is enabled and a capture starts, runtime creates:

```text
HYP36R/Research/<sanitized-scenario>/
  telemetry_<timestamp>[_suffix].csv
  session_<timestamp>[_suffix].txt
```

The human-readable scenario remains in metadata. The directory component
allows only letters, digits, hyphens and underscores, is limited to 64
characters, and cannot express traversal, an absolute path, nested folders or
a reserved Windows device name. Empty scenarios become `UNSPECIFIED`.

CSV and session metadata paths are selected together. If either target already
exists, a deterministic suffix is added, so neither file is overwritten.

When telemetry is disabled, the writer returns before directory selection or
creation. Normal runtime therefore creates no `HYP36R` research tree.

## Session metadata

The session file records available runtime provenance: product, product
version, build commit, schema, original scenario and notes, Force profile,
Presence, Contrast, Strength, three Force Character levels, inversion, start
and end times, samples, effective frequency, maximum sample gap, write failures
and the paired CSV filename.

## Physical procedure

1. Copy everything inside `UAT/` into the OutRun game directory.
2. Launch the game and confirm normal Reference+ operation.
3. Open F11 → Debug → FFB Telemetry and start a new capture.
4. Drive normally for 30–60 seconds with ordinary steering,
   acceleration/deceleration and one ordinary gear change if practical.
5. Stop the capture, exit normally, and return the complete
   `HYP36R/Research/R1_SCHEMA_UAT/` folder.

Do not deliberately seek runoff, sand, curbs, impacts, drift, extreme steering
or high gains during this validation.
