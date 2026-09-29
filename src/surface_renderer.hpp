#pragma once

#include <string_view>

namespace HYP36RSurfaceRenderer
{
	inline constexpr float MaximumMagnitude = 0.12f;
	inline constexpr float MinimumFrequencyHz = 18.0f;
	inline constexpr float MaximumFrequencyHz = 42.0f;
	inline constexpr int DefaultStrengthPercent = 25;
	inline constexpr int MaximumStrengthPercent = 50;

	enum class Renderer { Directional, Surface };
	struct Input
	{
		Renderer renderer = Renderer::Directional;
		float directionalRoad = 0.0f;
		float normalizedSpeed = 0.0f;
		int strengthPercent = DefaultStrengthPercent;
		bool inGameplay = false;
		bool ffbEnabled = false;
		bool periodicSupported = false;
	};
	struct Request
	{
		Renderer renderer = Renderer::Directional;
		float directionalRoad = 0.0f;
		float sourceRoad = 0.0f;
		float sourceMagnitude = 0.0f;
		float requestedMagnitude = 0.0f;
		float boundedMagnitude = 0.0f;
		float frequencyHz = MinimumFrequencyHz;
		bool boundActive = false;
		bool active = false;
	};

	Renderer renderer_from_string(std::string_view value) noexcept;
	const char* renderer_name(Renderer renderer) noexcept;
	Request evaluate(const Input& input) noexcept;
}
