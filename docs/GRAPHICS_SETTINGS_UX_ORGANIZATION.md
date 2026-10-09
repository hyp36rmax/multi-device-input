# Graphics settings UX organization

This inventory records the Graphics page contract at the start of the UX-only organization milestone (`b9bde6a`). The underlying `Setting` objects remain authoritative. This change adds only menu categories, player-facing labels, and concise tooltips.

`RestoreJPClarissa` remains canonically stored in `[Misc]`; the UI already presented it on the Graphics page. `TextureBaseFolder` and `DrawDistanceBehind` remain hidden and are not part of the rendered inventory.

## Inventory and presentation map

| Previous label | Canonical INI | Type | Default / valid values | Restart | Existing description (condensed) | Verified behavior | New label | Category | New tooltip |
|---|---|---|---|---|---|---|---|---|---|
| UIScalingMode | `[Graphics] UIScalingMode` | int choice | `1`; 0 Vanilla stretch, 1 scaled/no stretch, 2 centered 4:3 | Conditional: crossing Vanilla/non-Vanilla only | Adjusts UI scaling | Fits the 4:3 UI to the display using the selected geometry | Interface Scaling | Display | Changes how the 4:3 interface is fitted to widescreen displays. |
| UILetterboxing | `[Graphics] UILetterboxing` | int choice | `1`; 0 off, 1 menus, 2 always | No | Adds 4:3 menu letterboxing outside Vanilla mode | Controls side bars for non-stretched UI modes | Menu Letterboxing | Display | Adds side bars to menus when a non-stretched interface mode is used. |
| ScreenEdgeCullFix | `[Graphics] ScreenEdgeCullFix` | bool | `true` | No | Fixes early object disappearance at non-4:3 ratios | Adjusts screen-edge culling for widescreen views | Widescreen Edge Culling Fix | Display | Keeps stage objects visible until they actually leave a widescreen view. |
| AnisotropicFiltering | `[Graphics] AnisotropicFiltering` | int | `16`; 0–16 | No | 1–16; 0 leaves game default | Sets anisotropic sampling; 0 leaves native filtering untouched | Texture Filtering | Image Quality | Improves the clarity of stage textures viewed at an angle. Set 0 to leave filtering at the game's default. |
| ReflectionResolution | `[Graphics] ReflectionResolution` | int | `1024`; 0–8192 | Yes | Sets car-reflection resolution | Sets the car reflection cubemap dimensions | Car Reflection Resolution | Image Quality | Changes the texture resolution used for reflections on cars. |
| ReflectionUpdateRate | `[Graphics] ReflectionUpdateRate` | float | `0.5`; 0.0–1.0 | No | Fraction of reflection redrawn per frame | Accumulates and redraws the corresponding cubemap faces each frame | Car Reflection Update Rate | Image Quality | Controls how much of each car reflection is redrawn per frame; 1 updates all faces every frame. |
| TransparencySupersampling | `[Graphics] TransparencySupersampling` | bool | `true` | Yes | Enables transparency supersampling | Enables vendor-specific transparency anti-aliasing paths | Transparency Anti-Aliasing | Image Quality | Smooths jagged edges on transparent details such as trackside barriers and cloth. |
| DrawDistanceIncrease | `[Graphics] DrawDistanceIncrease` | int | `0`; 0–1024 | No | Increases where stage models begin drawing | Extends forward stage-section drawing with existing exclusions | Stage Draw Distance | Image Quality | Makes stage models begin drawing farther ahead; high values can expose incorrect lower-detail models. |
| DisableVehicleLODs | `[Graphics] DisableVehicleLODs` | bool | `true` | No | Disables vehicle LODs | Keeps vehicle rendering on the high-detail model | Full-Detail Vehicles | Image Quality | Keeps vehicles on their highest-detail model instead of switching to lower-detail versions. |
| DisableStageCulling | `[Graphics] DisableStageCulling` | bool | `true` | No | Disables culling of certain stage objects | Bypasses the associated distant-stage culling check | Keep Distant Stage Objects | Image Quality | Keeps certain distant stage objects from being hidden by the game's culling checks. |
| FixZBufferPrecision | `[Graphics] FixZBufferPrecision` | bool | `true` | Yes | Reduces z-fighting and distant drawing issues | Applies the existing depth-buffer precision patches | Depth Precision Fix | Image Quality | Improves depth precision to reduce overlapping-surface flicker and distant drawing errors. |
| SceneTextureReplacement | `[Graphics] SceneTextureReplacement` | bool | `true` | No | Loads matching stage replacement textures | Enables hash/dimension-matched stage texture replacement | Scene Texture Replacements | Image Quality | Loads matching replacement textures for stages from the configured texture-pack folder. |
| SkyGlowFactor | `[Graphics] SkyGlowFactor` | int | `4`; 0–16 | No | Restores console glow; value divides resolution | Enables the glow path and sets its buffer divisor | Sky Glow Resolution | Effects | Enables console-style sky and track glow and sets its render resolution. Higher values use a lower-resolution glow buffer; 0 disables it. |
| SkyGlowTwoStep | `[Graphics] SkyGlowTwoStep` | bool | `true` | No | Two-step reduction lowers glow aliasing | Selects the two-stage glow reduction path | Sky Glow Smoothing | Effects | Processes glow reduction in two stages to reduce aliasing at higher Sky Glow Resolution values. |
| RestoreXboxBrightness | `[Graphics] RestoreXboxBrightness` | bool | `false` | No | Restores Xbox HDR brightness | Restores the Xbox-style HDR brightness pass | Xbox HDR Brightness | Effects | Restores the Xbox HDR brightness pass, which brightens most scenes. |
| CarBaseShadowOpacity | `[Graphics] CarBaseShadowOpacity` | float | `1.0`; 0.0–1.0 | No | Restores and scales the player-car base shadow | Draws the restored base shadow at the selected opacity | Car Base Shadow | Effects | Restores the soft base shadow beneath the player car and controls its opacity. |
| HD Interface | `[Graphics] UITextureReplacement` | bool | `true` | Yes | Enables higher-resolution interface textures | Enables installed UI texture replacements | HD Interface | Presentation | Enables higher-resolution interface textures when a compatible texture pack is installed. |
| UseHiDefCharacters | `[Graphics] UseHiDefCharacters` | bool | `true` | Yes | Forces hi-def Alberto/Jennifer/Clarissa | Selects the game's high-detail character assets during play | Hi-Def Characters | Presentation | Uses the game's higher-detail Alberto, Jennifer and Clarissa models during gameplay. |
| Japanese Clarissa | `[Misc] RestoreJPClarissa` | bool | `false` | Yes | Restores original Japanese Clarissa presentation | Applies the existing Japanese Clarissa model/presentation patches | Japanese Clarissa | Presentation | Uses Clarissa's original Japanese character presentation instead of the regional variant. |
| SceneTextureExtract | `[Graphics] SceneTextureExtract` | bool | `false` | No | Dumps original stage textures when loaded | Writes stage textures for texture-pack authoring | Extract Scene Textures | Texture Tools | Writes original stage textures to disk for texture-pack creation. |
| UITextureExtract | `[Graphics] UITextureExtract` | bool | `false` | No | Dumps original UI textures when loaded | Writes UI textures for texture-pack authoring | Extract Interface Textures | Texture Tools | Writes original interface textures to disk for texture-pack creation. |
| EnableTextureCache | `[Graphics] EnableTextureCache` | bool | `true` | Yes | Caches replacement textures on another thread | Preloads stage replacements through the existing cache path | Texture Replacement Cache | Texture Tools | Preloads stage replacement textures on a background thread, which can reduce loading hitches. |
| UseNewTextureAllocator | `[Graphics] UseNewTextureAllocator` | bool | `true` | Yes | Uses the faster simplified texture allocator | Selects the mod's existing simplified texture-loading path | Simplified Texture Loader | Texture Tools | Uses the mod's simplified texture loader in place of the game's original loader. |

## Why Texture Tools is separate

Extraction, caching, and allocator selection control texture-pack authoring or loading infrastructure rather than the image itself. A small fifth group keeps those advanced tools visible without presenting them as image-quality adjustments.

## Compatibility boundary

- Canonical sections and keys are unchanged.
- Types, defaults, ranges, named choices, current values, save/load behavior, and restart declarations are unchanged.
- Graphics hooks and their initialization are unchanged.
- Existing INI files require no migration.
- Unknown future Graphics settings remain visible under an automatic `OTHER` heading until presentation metadata is assigned.

## Optional HD Interface textures

HD Interface textures are optional. **Install HD Textures** downloads, validates,
and installs the supported community package. The Graphics page reports
**Installed** only while the required package files are present on disk.

Installation and use are separate: the **HD Interface** checkbox can switch an
installed package off to use the original game interface, then back on without
downloading it again. Missing or incomplete required files return the control to
the install/retry state rather than relying on a saved flag.
