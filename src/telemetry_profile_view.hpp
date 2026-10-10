#pragma once

#include <string_view>

namespace HYP36RTelemetryView
{
	inline constexpr std::string_view TelemetrySchemaName = "HYP36R_RESEARCH_II_R1_AER_OUTPUT_V11";
	inline constexpr unsigned TelemetryColumnCount = 294;
	enum class ForceProfile { ReferencePlus, ArcadeExperience };

	struct Visibility
	{
		ForceProfile profile = ForceProfile::ReferencePlus;
		bool showReferenceSettings = true;
		bool showAerSettings = false;
		bool showSurfaceSettings = false;
	};

	inline ForceProfile force_profile_from_selection(int selection) noexcept
	{
		return selection == 1 ? ForceProfile::ArcadeExperience : ForceProfile::ReferencePlus;
	}

	inline const char* force_profile_name(ForceProfile profile) noexcept
	{
		return profile == ForceProfile::ArcadeExperience ? "Arcade Experience" : "Reference+";
	}

	inline const char* force_profile_name(bool aerSelected) noexcept
	{
		return force_profile_name(aerSelected ? ForceProfile::ArcadeExperience : ForceProfile::ReferencePlus);
	}

	inline bool surface_renderer_selected(std::string_view renderer) noexcept
	{
		return renderer == "Surface" || renderer == "SURFACE";
	}

	inline Visibility resolve(int profileSelection, std::string_view renderer,
		bool surfaceEffectActive) noexcept
	{
		const auto profile = force_profile_from_selection(profileSelection);
		const bool surfaceActive = surface_renderer_selected(renderer) && surfaceEffectActive;
		return { profile, profile == ForceProfile::ReferencePlus,
			profile == ForceProfile::ArcadeExperience, surfaceActive };
	}
}
