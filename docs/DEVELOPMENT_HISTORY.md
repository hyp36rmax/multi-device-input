# Development history

This is the shortened engineering history of the multi-device and HYP36R Force
work. The milestone documents remain in the repository when the full experiment
or replay result matters. This account keeps the wrong turns that changed the
architecture rather than presenting the current design as inevitable.

## Making modern controls usable

The fork began with a practical problem: OutRun expected a much simpler input
layout than a current wheel setup. A base, rim, pedals, shifter, button boxes,
and gamepad could appear as separate USB devices, and some wheel bases exposed
two interfaces with the same name. Players were compensating with vJoy and
manual configuration.

The first development sequence added raw device discovery, device-aware
bindings, in-game calibration, guided Quick Setup, stable identity matching,
and then a dedicated DirectInput FFB backend. Early Fanatec testing made it
clear that input and force output were not always the same interface. The
device selector was changed from name/index assumptions to capability testing
and usable-interface selection. Dual-axis and single-axis effect paths were
both retained because drivers differed in what they accepted.

Quick Setup also exposed a human-factors bug: input detection was fast enough
to skip steps. A six-second capture and explicit confirmation were added. The
goal settled early: normal users should be able to connect devices, configure
them, test them, and drive without editing an INI file.

The first live wheel model was deliberately conventional: speed-scaled
centering, steering-input damping, derived slip unloading, vibration-triggered
impact, and a road carrier driven by the restored Xbox vibration channel. It
worked, but most of its driving semantics were derived or synthetic. That
limitation led to the telemetry program.

## Diagnostics changed the process

Startup and FFB compatibility work accumulated several diagnostic commits
before the project understood which information was actually useful. Hook-by-
hook logging, exception addresses, linker maps, and optional-hook isolation
were introduced while investigating a Windows `0xc0000142` startup failure.
The eventual fix on the affected system was repairing current Microsoft Visual
C++ x86 and x64 redistributables. The failure happened before the patch could
always initialize its logger; it was not evidence of an FFB defect. Runtime
logging was later reduced to keep normal logs readable while preserving useful
startup and device diagnostics.

Windows CI also caught integration assumptions that a macOS development host
could not. New translation units initially inherited Direct3D types without
the Windows definitions that supplied `BYTE`; adding `Windows.h` then exposed
the familiar `min` macro collision with `std::numeric_limits::min()`. The
project adopted `WIN32_LEAN_AND_MEAN` and `NOMINMAX` consistently. Later M5
work repeated the lesson once for a new observer, and one build failed because
the new M5I source existed locally but was not listed in the Windows build.
These failures established a simple rule: every research milestone gets a
separate commit and an exact Win32 Release CI check before physical testing.

## From candidate fields to vehicle state

TP-01 began as an observation-only probe. Anonymous values used by OutRun's
restored Xbox vibration routine were logged without assigning meaning, and the
`xforce` column was intentionally left empty. Controlled capture management
was added after an early configuration lookup error prevented the intended
metadata and controls from appearing reliably.

The TP-02 and M3 work traced the native steering-response state around D38-D48
and `field_32`. Stationary and moving captures separated steering reference,
vehicle response, response increment, correction, authority, attenuation, and
transition suppression. The interpreter stayed passive until the temporal
relationships were understood. The project did not rename a similar-looking
value to X-Force; resemblance was not lineage.

## M4 and the grip envelope

M4 started as a passive composer, then enabled one behavior at a time. M4C
used native reference/response divergence to remove up to 25 percent of the
existing directional load. M4E separated credible vehicle-led convergence
from driver-led convergence. M4F replayed a bounded BITE restoration path
without touching hardware. M4G finally selected that validated restoration in
Active mode.

The first M4G runtime review appeared to show a serious routing defect:
restoration changed in telemetry while the reported active directional value
remained identical to M4C. The code trace showed that hardware had actually
received the restored M4F value. Two old telemetry columns still described the
M4C composer stage and their names implied more authority than they had.
M4G-R1 added explicit M4C, M4F, and hardware-selected fields. Run #58 then
proved exact equality between the hardware selection and M4F restoration on
every frame. The correction changed observability, not force.

That episode is why routing fields are now named by their exact boundary and
why a derived conclusion is not accepted when the selected hardware value can
be logged directly.

## M5 and four-corner state

Static research found four repeated native corner blocks. The first telemetry
kept neutral corner numbers and neutral offset names. Controlled captures
established the front/rear pair, and a known-left surface test established the
final FL/FR/RL/RR ordering. `+0x28` began as a possible suspension quantity;
captures supported a reference-relative displacement/loading context, but not
literal suspension travel or load. The `+0xAC` and `+0xB0` channels became
lateral and longitudinal contact-plane response candidates, still with unknown
units.

M5D organized the raw values into descriptive corner, axle, side, and surface
context. M5F explored which parts were relevant to the existing M4 force
intent. M5I tested a small passive lateral-context characterization during
developing RELEASE. It remained subordinate to M4 and had no steering direction
of its own.

The first M5J physical A/B test seemed indistinguishable. Telemetry supplied
the reason: both captures recorded `m5j_mode = m4_only`, while the M5I shadow
calculation had been running normally. The INI text was valid; the mode was
being resolved before effective settings initialization and retained its
default. M5J-R1 moved resolution to the correct initialization point and added
an explicit startup diagnostic. A second UAT confirmed that M5 reached hardware
only on eligible RELEASE frames. The effect was perceptible but subtle, so
amplitude was deferred rather than increased to force a result.

Force 2.1 was frozen at `8c5d68d92b19a21a09b3840413b8a453520517cc`.
Cross-car validation remained blocked by locked vehicles, and the team rejected
per-car compensation without evidence.

## Presentation and the S-series

The next question was whether the useful information was too quiet rather than
wrong. S1 separated force information, presentation, software headroom,
hardware safety, and device calibration. That separation mattered because an
earlier high-strength DD2 test caused the wheel to enter a protection or power-
shutdown condition. It proved that this device/setup could be driven into an
undesirable state; it did not establish a universal torque limit or software
threshold.

S2 measured the existing normalized output. S3 replayed amplification and found
substantial software headroom, with roughly 1.00–1.30 behaving as observed
comfort territory and 1.40–1.60 becoming headroom-aware. S4 and S5 separated
Global Presence from Information Contrast and proved their different effects
in replay. S6 established authority and budgets. S7 tested more elaborate
near-zero curves but found no demonstrated failure in the simpler linear rule,
so complexity was rejected. S8 put the same policy into the runtime passively.

S9 activated Reference+ with Presence 1.20, Contrast 4, and a five-percent
primary-relative M5 budget. Physical UAT found clearly greater steering
presence while retaining natural behavior. The 19,931-frame capture showed no
high-occupancy exposure, sign reversal, zero-origin creation, budget violation,
Legacy-authority violation, or FREE/BITE leakage. More intensity appeared
possible, but final calibration was deliberately deferred.

S10 graduated the presentation foundation at
`7036d3d8b6cfd4af6454636ce3e0c1c1bed2b35b`; Windows run #77 passed. Reference
remains the validated comparison and fail-safe. Reference+ remains a validated
experimental foundation, not a production-safe claim.

## Where this leaves the project

The code now has a traced native-state layer, a validated M4 grip envelope, a
bounded M5 research contribution, explicit routing telemetry, and separate
presentation policy. It still lacks universal cross-car M5 evidence, physical
hardware safety calibration, device-specific mapping, final intensity, a
validated Arcade Experience Reference, and proof that visually distinctive
surfaces such as the Sunny Beach striped runoff carry the expected native
disturbance.

Those are open questions rather than omissions to conceal. The milestone files
indexed in [README.md](README.md) retain the detailed experiments, replay
tables, and procedures behind this shorter account.
