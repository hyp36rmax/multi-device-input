#include "surface_renderer.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>

namespace HYP36RSurfaceRenderer
{
	std::string normalize(std::string_view value)
	{
		std::string normalized(value);
		for (char& c : normalized) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
		return normalized;
	}

	Renderer renderer_from_string(std::string_view value) noexcept
	{
		const auto normalized = normalize(value);
		return normalized == "SURFACE" || normalized == "SURFACE_EXPERIMENTAL"
			? Renderer::Surface : Renderer::Directional;
	}

	Waveform waveform_from_string(std::string_view value) noexcept
	{
		const auto normalized = normalize(value);
		if (normalized == "TRIANGLE") return Waveform::Triangle;
		if (normalized == "SQUARE") return Waveform::Square;
		return Waveform::Sine;
	}

	const char* waveform_name(Waveform waveform) noexcept
	{
		switch (waveform) { case Waveform::Triangle: return "Triangle"; case Waveform::Square: return "Square"; default: return "Sine"; }
	}

	FrequencyProfile frequency_profile_from_string(std::string_view value) noexcept
	{
		const auto normalized = normalize(value);
		if (normalized == "LOW") return FrequencyProfile::Low;
		if (normalized == "MEDIUM") return FrequencyProfile::Medium;
		if (normalized == "HIGH") return FrequencyProfile::High;
		return FrequencyProfile::Reference;
	}

	const char* frequency_profile_name(FrequencyProfile profile) noexcept
	{
		switch (profile) { case FrequencyProfile::Low: return "Low"; case FrequencyProfile::Medium: return "Medium"; case FrequencyProfile::High: return "High"; default: return "Reference"; }
	}

	FrequencyRange frequency_range(FrequencyProfile profile) noexcept
	{
		switch (profile) {
		case FrequencyProfile::Low: return { 12.0f, 30.0f };
		case FrequencyProfile::Medium: return { 24.0f, 48.0f };
		case FrequencyProfile::High: return { 30.0f, 60.0f };
		default: return { MinimumFrequencyHz, MaximumFrequencyHz };
		}
	}

	const char* renderer_name(Renderer renderer) noexcept
	{
		return renderer == Renderer::Surface ? "Surface (2.0 Experimental)" : "Directional (1.5 Reference)";
	}

	int sanitize_amplitude_ceiling_percent(int value) noexcept
	{
		for (int candidate : AmplitudeCeilingPercents)
			if (value == candidate) return candidate;
		return DefaultAmplitudeCeilingPercent;
	}

	Request evaluate(const Input& input) noexcept
	{
		Request out{};
		out.renderer = input.renderer;
		out.strengthPercent = (std::clamp)(input.strengthPercent, 0, MaximumStrengthPercent);
		out.amplitudeCeilingPercent = sanitize_amplitude_ceiling_percent(input.amplitudeCeilingPercent);
		out.waveform = input.waveform;
		out.frequencyProfile = input.frequencyProfile;
		out.directionalRoad = std::isfinite(input.directionalRoad) ? input.directionalRoad : 0.0f;
		out.sourceRoad = out.directionalRoad;
		out.sourceMagnitude = std::abs(out.directionalRoad);
		const float speed = (std::clamp)(std::isfinite(input.normalizedSpeed) ? input.normalizedSpeed : 0.0f, 0.0f, 1.0f);
		const auto range = frequency_range(input.frequencyProfile);
		out.frequencyHz = range.minimumHz + speed * (range.maximumHz - range.minimumHz);
		if (input.renderer != Renderer::Surface) return out;
		out.directionalRoad = 0.0f;
		out.requestedMagnitude = out.sourceMagnitude * (float(out.strengthPercent) / 100.0f);
		const float ceiling = float(out.amplitudeCeilingPercent) / 100.0f;
		out.boundedMagnitude = (std::clamp)(out.requestedMagnitude, 0.0f, ceiling);
		out.boundActive = out.boundedMagnitude != out.requestedMagnitude;
		out.active = input.inGameplay && input.ffbEnabled && input.periodicSupported &&
			out.boundedMagnitude > 0.0001f;
		return out;
	}
}
