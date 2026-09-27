# HYP36R Force 2.0 — Alpha 1

This is a controlled wheel UAT build for the existing HYP36R Force Character
presentation. Reference+ remains the known-good baseline at `1.00x / 1.00x /
1.00x`. Alpha 1 only extends the research range around that baseline:

| Control | Range | Step | Recommended |
|---|---:|---:|---:|
| Steering Load | 0.00x–1.30x | 0.05x | 1.00x |
| Road Detail | 0.00x–2.00x | 0.10x | 1.00x |
| Impact | 0.00x–1.50x | 0.05x | 1.00x |

These are research ceilings, not targets. Road is the existing v1 Road carrier,
not Road2. Impact still includes the existing gear-shift path. Road2, event-aware
Impact, and AER remain inactive.

## Install

Copy all six files from the `UAT` folder into a clean OutRun 2006 Coast 2 Coast
game folder, replacing files when prompted. Launch the supplied
`OR2006C2C.exe`. The supplied executable is the supported build with SHA-256
`68ceb386829066f8455b9d027320af962584321f3e2e8a79c72841495a6134c3`.

## Wheel test

1. Open the F11 overlay, choose **Debug**, then expand **FFB Telemetry**.
2. Press **RESET TO REFERENCE+** and **Start Session**.
3. Drive a normal full route at `1.00x / 1.00x / 1.00x`, then choose
   **Finish Session** and **Accept**.
4. Between runs, change one channel at a time. Start a new session after every
   change. Suggested progressions are Steering `1.00, 1.10, 1.20, 1.30`, Road
   `1.00, 1.20, 1.40, 1.60, 1.80, 2.00`, and Impact `1.00` through `1.50` in
   `0.10` steps.
5. After choosing a preference for each channel, make one combined run. Use
   **Retry** only when a run was not representative.

Do not change settings while recording. The UI locks them until the session is
finished. Captures and session summaries are written under
`HYP36R/Research/HYP36R_2_ALPHA1_FORCE_CHARACTER`. Each summary records start
and end multipliers, Strength, profile, build commit, duration, maximum output,
and DirectInput clamp count.

If the wheel feels violent, oscillates, loses useful steering information, or
becomes uncomfortable, stop driving and use **RESET TO REFERENCE+**. Keep the
normal conservative torque setting on the wheel base.

## Offline gate

The 15 accepted controlled R2 captures (15,803 samples) were replayed through
the same channel composition before hardware UAT. `1.00 / 1.00 / 1.00`
reproduced recorded `force_pre_drive` within `1.72e-7`. Independent ceiling
peaks were `.76746` Steering, `.69712` Road, and `.73733` Impact, with zero
software clamps. The combined `1.30 / 2.00 / 1.50` replay peaked at `.81260`,
had `.03797%` occupancy above `.75`, and zero clamps. All results were finite
and reproduce the R3 decision.

This is not a production release and does not change v1.0.0.
