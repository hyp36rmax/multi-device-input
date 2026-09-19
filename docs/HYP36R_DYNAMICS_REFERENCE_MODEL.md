# HYP36R Dynamics Reference Model

The HYP36R Dynamics Reference Model (DRM) is a standing methodology for
interpreting observed OutRun 2006 state. It is not a replacement physics engine
and does not generate force. Theory informs interpretation; observed game
behavior remains authoritative.

The working architecture is:

```text
Observed game state
        ↓
Semantic vehicle state
        ↓
Dynamics Reference Model interpretation
        ↓
Grip envelope
LOAD → RELEASE → FREE → BITE → LOAD
        ↓
Force intent
        ↓
Composer
```

Relevant reference concepts include transient tire-force buildup, relaxation
and delayed load development, vehicle inertia during a slide, progressive load
reacquisition, combined-slip context, and chassis/load-transfer transients.
These concepts constrain plausible interpretations and force shapes; they do
not override measured game behavior or supply synthetic steering commands.

## Standing principles

**MORE INFORMATION, NOT MORE TORQUE.**

**CONTINUOUS VEHICLE COMMUNICATION, NOT MANUFACTURED EFFECTS.**

**HYP36R FORCE COMMUNICATES THE INERTIA OF GRIP; IT DOES NOT FIGHT THE SLIDE.**

**NATIVE VEHICLE DIRECTION INFORMS LOAD CAPABILITY, NOT STEERING COMMAND.**

**VEHICLE CHARACTER EMERGES FROM MEASURED GAME BEHAVIOR, INTERPRETED THROUGH
ESTABLISHED VEHICLE DYNAMICS.**

## Grip-envelope interpretation

- **LOAD:** stable directional authority.
- **RELEASE:** load capability begins falling; native divergence may lead the
  established synthetic event.
- **FREE:** meaningful dynamic displacement continues with reduced directional
  authority. FREE does not mean zero grip.
- **BITE:** directional load capability begins returning while substantial
  dynamic state may remain. BITE is not full grip or a wheel target.
- **RETURNED LOAD:** reference/response behavior and dynamic state stabilize.

Future vehicle-character parameters may describe load-build rate, release
rate, FREE persistence, BITE/rebuild rate, and recovery inertia. They must be
derived from measured game behavior rather than assigned as unvalidated
car-specific tuning.

M4G activates the exact M4F-validated BITE-informed unloading trajectory only
in Force 2.0 Active mode. It restores part of the directional load previously
removed by M4C, never exceeds Legacy directional magnitude, and does not add a
BITE effect, response-angle steering, response-rate force, or additional
torque. Legacy and Shadow hardware behavior remain unchanged.
