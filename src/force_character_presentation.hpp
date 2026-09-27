#pragma once

#include <algorithm>
#include <cmath>

namespace HYP36RForceCharacter
{
	constexpr int DefaultPercent = 100;
	constexpr int SteeringMaximumPercent = 130;
	constexpr int RoadMaximumPercent = 200;
	constexpr int ImpactMaximumPercent = 150;
	constexpr int PlayerUiMaximumPercent = 100;
	constexpr bool HasIndependentWheelVibration = false;

	struct Percentages
	{
		int steeringLoad = DefaultPercent;
		int roadDetail = DefaultPercent;
		int impact = DefaultPercent;
	};

	struct Channels
	{
		float directional = 0.0f;
		float road = 0.0f;
		float impact = 0.0f;
	};

	inline int clamp_percent(int percent, int maximum)
	{
		return (std::clamp)(percent, 0, maximum);
	}

	// The persisted/runtime value remains the engineering multiplier in
	// hundredths (100 == 1.00x). The normal Advanced UI is only a 0-100
	// presentation over each independently validated ceiling.
	inline int to_player_percent(int canonicalPercent, int canonicalMaximum)
	{
		const int clamped = clamp_percent(canonicalPercent, canonicalMaximum);
		return (std::clamp)(int(std::lround((double(clamped) * PlayerUiMaximumPercent) /
			double(canonicalMaximum))), 0, PlayerUiMaximumPercent);
	}

	inline int recommended_player_percent(int canonicalMaximum)
	{
		return to_player_percent(DefaultPercent, canonicalMaximum);
	}

	inline int from_player_percent(int playerPercent, int canonicalMaximum)
	{
		const int clamped = (std::clamp)(playerPercent, 0, PlayerUiMaximumPercent);
		// The rounded UI positions for 1.00x are explicit snap points. This keeps
		// Steering 77% and Impact 67% from becoming 1.001x/1.005x equivalents.
		if (clamped == recommended_player_percent(canonicalMaximum))
			return DefaultPercent;
		return clamp_percent(int(std::lround((double(clamped) * canonicalMaximum) /
			double(PlayerUiMaximumPercent))), canonicalMaximum);
	}

	inline float finite_or_zero(float value)
	{
		return std::isfinite(value) ? value : 0.0f;
	}

	// Presentation only: each gain touches its own already-resolved channel.
	inline Channels apply(Channels source, Percentages percentages)
	{
		return {
			finite_or_zero(source.directional) * (float(clamp_percent(percentages.steeringLoad, SteeringMaximumPercent)) / 100.0f),
			finite_or_zero(source.road) * (float(clamp_percent(percentages.roadDetail, RoadMaximumPercent)) / 100.0f),
			finite_or_zero(source.impact) * (float(clamp_percent(percentages.impact, ImpactMaximumPercent)) / 100.0f)
		};
	}
}
