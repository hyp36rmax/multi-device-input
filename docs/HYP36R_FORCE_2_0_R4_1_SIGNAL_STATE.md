# HYP36R Force 2.0 R4.1 Signal State

R4.1 implements the first versioned Signal State contract described by the R4
architecture. It is an observation and interpretation layer only. Reference+
v1 remains the sole owner of wheel output.

## Contract

The internal version is `HYP36R_SIGNAL_STATE_V1` (`SchemaVersion == 1`). A
fixed-size `Frame` preserves:

- current runtime steering, speed and normalized speed, plus available native
  response angle, rate, reference/response error and authority;
- four independent raw surface values, their previous values and per-corner
  transition flags;
- neutral all-same/mixed, distinct-count and established 0/2 and 1/3 pair
  relationships, without assigning surface material names or wheel ownership;
- independent restored native left, right, combined and rise channels;
- previous/current gear and gear transition;
- `LOAD`, `RELEASE`, `FREE`, `RECOVERING` and `BITE` state identity without
  changing the existing classifier;
- four-corner E8, EC and EE values under neutral candidate names only.

Each observation has compact metadata for validity, provenance, frame identity,
age, confidence and fallback reason. The contract contains no dynamic strings,
containers, file access or output-device types.

## Surface source identity

The established `water_flag_24C[0..3]` values are canonical surface state.
Native four-corner `field14` is not exposed as another semantic input. When it
is available, R4.1 records only whether it matched the canonical values and a
four-bit mismatch mask. This preserves validation/provenance without creating
two consumers for one underlying state.

Raw surface values remain raw. R4.1 does not globally name them asphalt, sand,
curb, grass or any other material.

## Passive interpretation

`RoadShadowIntent` preserves continuous native-effect activity, neutral spatial
occupancy and surface-transition evidence. `EventShadowIntent` preserves gear,
cautious collision-candidate and unknown-native-event identity plus its source
left/right/rise evidence. Neither contract contains a force, gain, presentation
level or output request.

Gear transition has priority over transient classification. A non-gear event is
only a collision candidate when a strong rise, bilateral activity and the
existing impact detector agree. A strong rise without that evidence remains an
unknown native event.

The R2-C known gear fixture is retained in code and tests as left `0.14`, right
`0`, rise `0.14`, existing absolute Impact presentation approximately
`0.07735`. R4.1 records this fixture; it does not recreate its force.

## Validity, freshness and failure

Missing optional observations begin unavailable. Unsupported sources and
non-finite values are marked explicitly. A snapshot older than its caller's
frame budget becomes stale and clears semantically active shadow intent. No
invalid, stale or unsupported shadow state falls back into Reference+.

Signal State failure therefore means the shadow state is unavailable. It does
not change the v1 fallback, force profile or wheel request.

## Ownership and lifetime

The player-car control hook is the single writer. It constructs one fixed-size
frame per in-game update after the established hardware force has already been
sent. The frame remains owned by the Signal State module until the next update
or reset. Same-thread readers may take the current frame or a freshness-checked
copy. Cross-thread readers require external synchronization; R4.1 does not
claim lock-free publication.

This ownership is game-state oriented, not wheel oriented. Future pedals,
haptics, motion or AER policy can consume the same validated state without
changing its source identity, but none is implemented here.

## Shadow/output boundary

The runtime order is structural:

```text
Reference+ v1 composition
  -> tanh
  -> output ramp
  -> WheelForceFeedback::drive()
  -> R4.1 Signal State update
  -> passive Road/Event shadow intent
  -> STOP
```

The R4.1 update occurs after `drive()` and its return value is ignored. Shadow
intent types are not convertible to a scalar and no v1 composition API accepts
them. Regression tests replay identical v1 Directional, Road and Impact inputs
before and after Signal State observation and require exact equality through
composition, tanh, pre-drive and final strength/inversion request.

## Validation

The deterministic test covers version identity, four-surface independence and
transitions, mixed/full occupancy, field14 validation, L/R preservation, gear
and collision/unknown classification, all established grip/load states,
E8/EC/EE neutral preservation, unsupported/non-finite/stale failure, clean-state
determinism and exact v1 output equivalence.

Representative R2 fixtures are replayed without new driving:

- R2-A: the established load, release, free, recovery and bite identities;
- R2-B: partial `{2, 2048, 2, 2048}` and full `{4, 4, 4, 4}` occupancy;
- R2-C: the known gear fixture and a controlled bilateral collision candidate.

The frame and inputs are trivially copyable and bounded (`Frame <= 1024` bytes).
The test also runs 200,000 updates as an overhead smoke measurement. CI timing is
informational rather than a hardware-dependent pass threshold.

## Known limits

- E8, EC and EE remain unclassified candidates.
- Spatial 0/2 and 1/3 pairing is preserved without asserting physical
  left/right wheel ownership.
- Collision is a confidence-bearing candidate, not a universal classifier.
- No complete surface-material table exists.
- No public telemetry schema changes in R4.1.
- No Road 2.0, event-aware Impact, AER or external-device output is active.

R4.2 should consume this contract only in passive offline/shadow policy work
until a separately reviewed activation milestone proves identity, safety and v1
fallback behavior.
