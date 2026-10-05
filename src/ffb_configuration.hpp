#pragma once

#include <filesystem>
#include <string>
#include <string_view>

#include "force_character_presentation.hpp"
#include "surface_renderer.hpp"

namespace HYP36RFFBConfiguration
{
	inline constexpr int CurrentMigrationVersion = 150;
	inline constexpr bool DefaultEnabled = true;
	inline constexpr int DefaultStrength = 100;
	inline constexpr bool DefaultInvert = false;
	inline constexpr std::string_view DefaultRoadMode = "ROAD2_EXPERIMENTAL";
	inline constexpr std::string_view DefaultForceMode = "Active";
	inline constexpr std::string_view DefaultPresentationMode = "REFERENCE_PLUS_EXPERIMENTAL";
	inline constexpr int DefaultRoadAuthorityGain = 10;

	struct PlayerState
	{
		bool enabled = DefaultEnabled;
		int strength = DefaultStrength;
		int steeringLoad = HYP36RForceCharacter::DefaultPercent;
		int roadDetail = HYP36RForceCharacter::DefaultPercent;
		int impact = HYP36RForceCharacter::DefaultPercent;
		int surface = HYP36RSurfaceRenderer::DefaultPlayerSurfacePercent;
		std::string roadMode = std::string(DefaultRoadMode);
		bool invert = DefaultInvert;
		std::string forceMode = std::string(DefaultForceMode);
		std::string presentationMode = std::string(DefaultPresentationMode);
	};

	struct ResearchState
	{
		bool roadAuthority = false;
		int roadAuthorityGain = DefaultRoadAuthorityGain;
		std::string roadRenderer = "SURFACE";
		int surfaceRendererStrength = 100;
		bool textureCeilingOverride = false;
		int textureCeiling = HYP36RSurfaceRenderer::NormalAmplitudeCeilingPercent;
		std::string waveform = "TRIANGLE";
		std::string frequency = "REFERENCE";
		int preferredAmplitude = 0;
		std::string preferredWaveform = "UNSET";
		std::string preferredFrequency = "UNSET";
		bool retiredBumpEnabled = true;
		float retiredBumpThreshold = HYP36RSurfaceRenderer::BumpThreshold;
		int retiredBumpStrength = HYP36RSurfaceRenderer::BumpStrengthPercent;
		int retiredBumpDuration = HYP36RSurfaceRenderer::BumpDurationMilliseconds;
	};

	struct State
	{
		int migrationVersion = 0;
		PlayerState player{};
		ResearchState research{};
	};

	inline bool migrate_v15(State& state)
	{
		if (state.migrationVersion >= CurrentMigrationVersion)
			return false;
		// Disable stale release-affecting research state without erasing harmless
		// stored choices such as the Road authority multiplier or UAT preferences.
		// A tester can deliberately re-enable those after this one-time migration.
		state.research.roadAuthority = false;
		state.research.textureCeilingOverride = false;
		state.research.textureCeiling = HYP36RSurfaceRenderer::NormalAmplitudeCeilingPercent;
		state.research.waveform = "TRIANGLE";
		state.research.frequency = "REFERENCE";
		state.research.retiredBumpEnabled = true;
		state.research.retiredBumpThreshold = HYP36RSurfaceRenderer::BumpThreshold;
		state.research.retiredBumpStrength = HYP36RSurfaceRenderer::BumpStrengthPercent;
		state.research.retiredBumpDuration = HYP36RSurfaceRenderer::BumpDurationMilliseconds;
		state.migrationVersion = CurrentMigrationVersion;
		return true;
	}

	inline void reset_player(State& state)
	{
		state.player = PlayerState{};
		state.research = ResearchState{};
	}

	inline void apply_reference_plus_force_character(PlayerState& player)
	{
		const auto reference = HYP36RForceCharacter::reference_plus();
		player.steeringLoad = reference.steeringLoad;
		player.roadDetail = reference.roadDetail;
		player.impact = reference.impact;
		player.surface = HYP36RSurfaceRenderer::DefaultPlayerSurfacePercent;
	}

	inline void reset_debug(State& state)
	{
		state.research = ResearchState{};
	}

	bool migrate_v15_settings(const std::filesystem::path& userIniPath);
	void apply_reference_plus_force_character_settings(const std::filesystem::path& userIniPath);
	void reset_player_settings(const std::filesystem::path& userIniPath);
	void reset_debug_settings(const std::filesystem::path& userIniPath);
}
