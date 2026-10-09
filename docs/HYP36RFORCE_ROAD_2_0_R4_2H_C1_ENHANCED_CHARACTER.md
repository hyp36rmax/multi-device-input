# HYP36Rforce Road 2.0 R4.2H-C1 — Enhanced Surface Character

C1 builds on the physically accepted V0 aperiodic foundation. It keeps the fixed 120 Hz update, deterministic xorshift sequence and seed, two-filter aperiodic texture, DC blocker, native authorization, four-corner occupancy envelope, transition shaping, Road Detail amount stage, and final Road-channel safety stages.

## Signal architecture and ownership

```text
native four-corner surface tuple                       [NATIVE]
native L/R effect magnitude                            [NATIVE]
    -> Road policy authorization and occupancy          [DERIVED]
    -> conservative authority presentation curve        [SYNTHETIC PRESENTATION]
    -> preserved V0 aperiodic base                      [SYNTHETIC PRESENTATION]
    -> bounded evidence-class character                 [SYNTHETIC PRESENTATION]
    -> +/-0.06 internal clamp and 0.90/s slew
    -> existing Road Detail amount
    -> development gain 4x / 6x / 8x / 10x
    -> +/-0.25 Road-channel clamp and 0.90/s slew
    -> existing Force composition and DirectInput path
```

Native amplitude remains authoritative. C1 presents it as `0.70a + 0.30sqrt(a)`, so low authorized amplitudes receive a conservative lift while stronger surfaces remain stronger. Zero authority remains exactly zero.

## Surface classification

The classifier uses only controlled native raw values already established by accepted captures:

| Presentation archetype | Supported raw context | Character |
|---|---|---|
| Hard / uneven | `0x100000` | Preserved irregular base plus a low-authority, deterministically modulated tight component |
| Soft / rough | `4`, `8`, `0x2000` | Preserved irregular base plus a slower coarse component |
| Striped / runoff | `0x400`, `0x800` | Preserved irregular base plus a sharper bounded high-pass component |
| Generic Enhanced | Authorized unmatched or ambiguous tuple | Preserved aperiodic base only |
| None | Reference surface or no authorization | Zero output |

Grass and sand are not separated. Existing evidence supports a combined rough/off-road context but not a reliable material distinction. No resistance component is implemented: the current signed ConstantForce channel cannot express drag or weight without risking persistent steering direction. The debug value remains zero and records this boundary explicitly.

## Safety and isolation

- Character components are deterministic, bounded fractions of the existing internal `0.06` ceiling, and gated by native authorization and occupancy.
- Entry and class changes pass through the existing slew protection; authorization loss clears Road immediately.
- The development stage accepts only 4x, 6x, 8x, or 10x and cannot bypass the `0.25` Road-channel hard limit or `0.90/s` slew limit.
- Classic still selects the original Reference+ Road value and cannot read Enhanced output.
- Directional and Impact inputs and equations are unchanged.
- No Spring, Damper, vehicle-physics change, baseline noise, or third-party constant is introduced.

## Accepted-capture replay

The updated deterministic replay was run against CST01–CST03 and B01–B06. Values below are final normalized Road contributions after the development stage at the recorded/reference Road amount. `auth` is authorized rows at approximately 60 Hz.

| Capture / archetype | Auth | 4x RMS/max | 6x RMS/max | 8x RMS/max | 10x RMS/max | Mean/DC range | Clamp |
|---|---:|---:|---:|---:|---:|---:|---:|
| CST01 normal / none | 0 | 0/0 | 0/0 | 0/0 | 0/0 | 0 | 0 |
| CST02 cobble / hard | 451 | .002788/.013409 | .004015/.020114 | .005049/.024188 | .005941/.029243 | -.000093 to -.000010 | 0 |
| CST03 normal / none | 0 | 0/0 | 0/0 | 0/0 | 0/0 | 0 | 0 |
| B01 asphalt / none | 0 | 0/0 | 0/0 | 0/0 | 0/0 | 0 | 0 |
| B02 runoff / striped | 173 | .002813/.017949 | .003994/.026924 | .004904/.035898 | .005762/.044873 | -.000121 to -.000046 | 0 |
| B03 runoff / mostly striped | 175 | .002287/.016794 | .003264/.024562 | .004130/.032239 | .004833/.039127 | -.000278 to -.000031 | 0 |
| B04 rough / soft | 943 | .004840/.016546 | .006907/.023382 | .008660/.027170 | .010192/.033392 | 0 to .000162 | 0 |
| B05 rough / mostly soft | 467 | .006687/.031608 | .008886/.047412 | .010637/.063216 | .011872/.070983 | .000245 to .000549 | 0 |
| B06 re-entry / mixed | 488 | .004510/.023458 | .006106/.035188 | .007513/.046917 | .008596/.045565 | -.000177 to -.000063 | 0 |

All three normal-road controls remained exactly zero at all gains. No hard clamp occurred. Slew activity increased normally with gain and remained bounded. Repeat replay produced byte-identical deterministic hashes.

## Debug observability

Debug → Road 2.0 (Enhanced) C1 reports mode, raw and presented authority, four-corner tuple, occupancy target/envelope, transition state, selected archetype, raw/conditioned generator state, aperiodic base, character, explicit zero resistance, pre-safety contribution, internal clamp/slew state, Road Detail scale, development gain, pre/post-gain Road, final Road, and final Road-channel clamp/slew state.

## Physical DD2 gate

Set Road Detail to its Recommended 50%. Select Enhanced and drive normal road, Tulip Garden cobblestone, the repeatable rough/off-road area, and convenient striped runoff. Compare 4x, 6x, 8x, and 10x without changing wheel settings. Report the first clearly perceptible gain, preferred gain, character of each supported surface, normal-road cleanliness, steering or Impact interference, harshness/chatter, and any clipping or oscillation. Do not raise Road Detail to 100% until the Recommended comparison is complete.

THP32 remains **EXTERNAL REFERENCE — NOT INTEGRATED**. C1 uses no THP32 code, constants, frequencies, effect parameters, or equations.
