#pragma once

#include <string_view>

namespace HYP36RPresentation
{
	enum class Profile { Reference };
	enum class SoftwareRegion { Comfort, HeadroomAware, CompressionRisk, SaturationRisk };

	struct Inputs
	{
		float directionalPrimary = 0.0f;
		float directionalSecondaryRaw = 0.0f;
		float legacyDirectional = 0.0f;
		float road = 0.0f;
		float impact = 0.0f;
		float vibration = 0.0f;
		bool secondaryEligible = false;
	};

	struct Frame
	{
		Profile profile = Profile::Reference;
		float presence = 1.0f;
		float contrast = 0.0f;
		float secondaryBudgetFraction = 0.05f;
		float directionalPrimary = 0.0f;
		float secondaryRaw = 0.0f;
		float secondaryRequested = 0.0f;
		float secondaryPermitted = 0.0f;
		float directionalRequest = 0.0f;
		float roadRequest = 0.0f;
		float impactRequest = 0.0f;
		float vibrationRequest = 0.0f;
		bool secondaryBudgetActive = false;
		bool legacyBoundaryActive = false;
		bool fallbackActive = false;
		SoftwareRegion softwareRegion = SoftwareRegion::Comfort;
	};

	// Resolve hidden research settings once, after all INI files have loaded.
	// Missing or invalid values select the neutral Reference configuration.
	void initialize();

	// Passive observer only. Its result is consumed by telemetry after drive().
	const Frame& evaluate(const Inputs& inputs);
	void reset();
	const Frame& frame();
	const char* profile_name(Profile profile);
	const char* software_region_name(SoftwareRegion region);
}
