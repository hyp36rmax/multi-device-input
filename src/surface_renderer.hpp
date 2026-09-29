#pragma once

#include <string_view>
#include <array>

namespace HYP36RSurfaceRenderer
{
	inline constexpr std::array<int, 7> AmplitudeCeilingPercents{ 12, 18, 24, 30, 36, 42, 50 };
	inline constexpr int DefaultAmplitudeCeilingPercent = 12;
	inline constexpr float MinimumFrequencyHz = 18.0f;
	inline constexpr float MaximumFrequencyHz = 42.0f;
	inline constexpr int DefaultStrengthPercent = 100;
	inline constexpr int MaximumStrengthPercent = 100;

	enum class Renderer { Directional, Surface };
	struct Input
	{
		Renderer renderer = Renderer::Directional;
		float directionalRoad = 0.0f;
		float normalizedSpeed = 0.0f;
		int strengthPercent = DefaultStrengthPercent;
		int amplitudeCeilingPercent = DefaultAmplitudeCeilingPercent;
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
		int strengthPercent = DefaultStrengthPercent;
		int amplitudeCeilingPercent = DefaultAmplitudeCeilingPercent;
		bool boundActive = false;
		bool active = false;
	};

	Renderer renderer_from_string(std::string_view value) noexcept;
	const char* renderer_name(Renderer renderer) noexcept;
	int sanitize_amplitude_ceiling_percent(int value) noexcept;
	Request evaluate(const Input& input) noexcept;
}
