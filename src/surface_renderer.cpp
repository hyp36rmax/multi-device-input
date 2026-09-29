#include "surface_renderer.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>

namespace HYP36RSurfaceRenderer
{
	Renderer renderer_from_string(std::string_view value) noexcept
	{
		std::string normalized(value);
		for (char& c : normalized) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
		return normalized == "SURFACE" || normalized == "SURFACE_EXPERIMENTAL"
			? Renderer::Surface : Renderer::Directional;
	}

	const char* renderer_name(Renderer renderer) noexcept
	{
		return renderer == Renderer::Surface ? "Surface (2.0 Experimental)" : "Directional (1.5 Reference)";
	}

	Request evaluate(const Input& input) noexcept
	{
		Request out{};
		out.renderer = input.renderer;
		out.directionalRoad = std::isfinite(input.directionalRoad) ? input.directionalRoad : 0.0f;
		out.sourceRoad = out.directionalRoad;
		out.sourceMagnitude = std::abs(out.directionalRoad);
		const float speed = (std::clamp)(std::isfinite(input.normalizedSpeed) ? input.normalizedSpeed : 0.0f, 0.0f, 1.0f);
		out.frequencyHz = MinimumFrequencyHz + speed * (MaximumFrequencyHz - MinimumFrequencyHz);
		if (input.renderer != Renderer::Surface) return out;
		out.directionalRoad = 0.0f;
		const int strength = (std::clamp)(input.strengthPercent, 0, MaximumStrengthPercent);
		out.requestedMagnitude = out.sourceMagnitude * (float(strength) / 100.0f);
		out.boundedMagnitude = (std::clamp)(out.requestedMagnitude, 0.0f, MaximumMagnitude);
		out.boundActive = out.boundedMagnitude != out.requestedMagnitude;
		out.active = input.inGameplay && input.ffbEnabled && input.periodicSupported &&
			out.boundedMagnitude > 0.0001f;
		return out;
	}
}
