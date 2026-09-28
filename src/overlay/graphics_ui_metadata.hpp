#pragma once

#include <array>
#include <string_view>

namespace GraphicsUi
{
struct Metadata
{
	std::string_view key;
	std::string_view category;
	std::string_view label;
	std::string_view tooltip;
};

// Presentation metadata only. Canonical Setting objects continue to own their
// INI sections, keys, values, ranges and restart behavior.
inline constexpr std::array Settings{
	Metadata{ "UIScalingMode", "DISPLAY", "Interface Scaling", "Changes how the 4:3 interface is fitted to widescreen displays." },
	Metadata{ "UILetterboxing", "DISPLAY", "Menu Letterboxing", "Adds side bars to menus when a non-stretched interface mode is used." },
	Metadata{ "ScreenEdgeCullFix", "DISPLAY", "Widescreen Edge Culling Fix", "Keeps stage objects visible until they actually leave a widescreen view." },

	Metadata{ "AnisotropicFiltering", "IMAGE QUALITY", "Texture Filtering", "Improves the clarity of stage textures viewed at an angle. Set 0 to leave filtering at the game's default." },
	Metadata{ "ReflectionResolution", "IMAGE QUALITY", "Car Reflection Resolution", "Changes the texture resolution used for reflections on cars." },
	Metadata{ "ReflectionUpdateRate", "IMAGE QUALITY", "Car Reflection Update Rate", "Controls how much of each car reflection is redrawn per frame; 1 updates all faces every frame." },
	Metadata{ "TransparencySupersampling", "IMAGE QUALITY", "Transparency Anti-Aliasing", "Smooths jagged edges on transparent details such as trackside barriers and cloth." },
	Metadata{ "DrawDistanceIncrease", "IMAGE QUALITY", "Stage Draw Distance", "Makes stage models begin drawing farther ahead; high values can expose incorrect lower-detail models." },
	Metadata{ "DisableVehicleLODs", "IMAGE QUALITY", "Full-Detail Vehicles", "Keeps vehicles on their highest-detail model instead of switching to lower-detail versions." },
	Metadata{ "DisableStageCulling", "IMAGE QUALITY", "Keep Distant Stage Objects", "Keeps certain distant stage objects from being hidden by the game's culling checks." },
	Metadata{ "FixZBufferPrecision", "IMAGE QUALITY", "Depth Precision Fix", "Improves depth precision to reduce overlapping-surface flicker and distant drawing errors." },
	Metadata{ "SceneTextureReplacement", "IMAGE QUALITY", "Scene Texture Replacements", "Loads matching replacement textures for stages from the configured texture-pack folder." },

	Metadata{ "SkyGlowFactor", "EFFECTS", "Sky Glow Resolution", "Enables console-style sky and track glow and sets its render resolution. Higher values use a lower-resolution glow buffer; 0 disables it." },
	Metadata{ "SkyGlowTwoStep", "EFFECTS", "Sky Glow Smoothing", "Processes glow reduction in two stages to reduce aliasing at higher Sky Glow Resolution values." },
	Metadata{ "RestoreXboxBrightness", "EFFECTS", "Xbox HDR Brightness", "Restores the Xbox HDR brightness pass, which brightens most scenes." },
	Metadata{ "CarBaseShadowOpacity", "EFFECTS", "Car Base Shadow", "Restores the soft base shadow beneath the player car and controls its opacity." },

	Metadata{ "UITextureReplacement", "PRESENTATION", "HD Interface", "Enables higher-resolution interface textures when a compatible texture pack is installed." },
	Metadata{ "UseHiDefCharacters", "PRESENTATION", "Hi-Def Characters", "Uses the game's higher-detail Alberto, Jennifer and Clarissa models during gameplay." },
	Metadata{ "RestoreJPClarissa", "PRESENTATION", "Japanese Clarissa", "Uses Clarissa's original Japanese character presentation instead of the regional variant." },

	Metadata{ "SceneTextureExtract", "TEXTURE TOOLS", "Extract Scene Textures", "Writes original stage textures to disk for texture-pack creation." },
	Metadata{ "UITextureExtract", "TEXTURE TOOLS", "Extract Interface Textures", "Writes original interface textures to disk for texture-pack creation." },
	Metadata{ "EnableTextureCache", "TEXTURE TOOLS", "Texture Replacement Cache", "Preloads stage replacement textures on a background thread, which can reduce loading hitches." },
	Metadata{ "UseNewTextureAllocator", "TEXTURE TOOLS", "Simplified Texture Loader", "Uses the mod's simplified texture loader in place of the game's original loader." },
};

inline constexpr std::array Categories{
	std::string_view("DISPLAY"),
	std::string_view("IMAGE QUALITY"),
	std::string_view("EFFECTS"),
	std::string_view("PRESENTATION"),
	std::string_view("TEXTURE TOOLS"),
};
}
