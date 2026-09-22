#pragma once

#include <algorithm>
#include <cmath>

namespace HYP36RForceCharacter
{
	constexpr int DefaultPercent = 100;
	constexpr int MaximumPercent = 100;
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

	inline int clamp_percent(int percent)
	{
		return (std::clamp)(percent, 0, MaximumPercent);
	}

	inline float finite_or_zero(float value)
	{
		return std::isfinite(value) ? value : 0.0f;
	}

	// Presentation only: each gain touches its own already-resolved channel.
	inline Channels apply(Channels source, Percentages percentages)
	{
		return {
			finite_or_zero(source.directional) * (float(clamp_percent(percentages.steeringLoad)) / 100.0f),
			finite_or_zero(source.road) * (float(clamp_percent(percentages.roadDetail)) / 100.0f),
			finite_or_zero(source.impact) * (float(clamp_percent(percentages.impact)) / 100.0f)
		};
	}
}
