#pragma once

#include <string_view>
#include <array>

namespace HYP36RSurfaceRenderer
{
	inline constexpr std::array<int, 3> AmplitudeCeilingPercents{ 12, 18, 25 };
	inline constexpr int NormalAmplitudeCeilingPercent = 18;
	inline constexpr int DefaultAmplitudeCeilingPercent = NormalAmplitudeCeilingPercent;
	inline constexpr int MaximumAmplitudeCeilingPercent = 25;
	inline constexpr float MinimumFrequencyHz = 18.0f;
	inline constexpr float MaximumFrequencyHz = 42.0f;
	inline constexpr int DefaultStrengthPercent = 100;
	inline constexpr int MaximumStrengthPercent = 100;
	inline constexpr int DefaultPlayerSurfacePercent = 50;
	inline constexpr float BumpThreshold = 0.020f;
	inline constexpr int BumpStrengthPercent = 30;
	inline constexpr int BumpDurationMilliseconds = 60;
	inline constexpr int BumpCooldownMilliseconds = 120;
	inline constexpr float BumpMaximumMagnitude = 0.30f;

	enum class Renderer { Directional, Surface };
	enum class Waveform { Sine, Triangle, Square };
	enum class FrequencyProfile { Low, Reference, Medium, High };
	inline constexpr Renderer DefaultRenderer = Renderer::Surface;
	inline constexpr std::string_view DefaultRendererSetting = "SURFACE";
	inline constexpr Waveform DefaultWaveform = Waveform::Triangle;
	struct FrequencyRange { float minimumHz; float maximumHz; };
	struct Input
	{
		Renderer renderer = Renderer::Directional;
		float directionalRoad = 0.0f;
		float normalizedSpeed = 0.0f;
		int strengthPercent = DefaultStrengthPercent;
		int amplitudeCeilingPercent = DefaultAmplitudeCeilingPercent;
		Waveform waveform = DefaultWaveform;
		FrequencyProfile frequencyProfile = FrequencyProfile::Reference;
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
		Waveform waveform = DefaultWaveform;
		FrequencyProfile frequencyProfile = FrequencyProfile::Reference;
		bool boundActive = false;
		bool active = false;
	};

	Renderer renderer_from_string(std::string_view value) noexcept;
	const char* renderer_name(Renderer renderer) noexcept;
	const char* renderer_setting_value(Renderer renderer) noexcept;
	bool renderer_transition_requires_stop(Renderer active, Renderer selected) noexcept;
	Waveform waveform_from_string(std::string_view value) noexcept;
	const char* waveform_name(Waveform waveform) noexcept;
	FrequencyProfile frequency_profile_from_string(std::string_view value) noexcept;
	const char* frequency_profile_name(FrequencyProfile profile) noexcept;
	FrequencyRange frequency_range(FrequencyProfile profile) noexcept;
	int sanitize_amplitude_ceiling_percent(int value) noexcept;
	int resolve_amplitude_ceiling_percent(bool researchOverrideEnabled,
		int researchCeilingPercent) noexcept;
	int sanitize_player_surface_percent(int value) noexcept;
	float player_texture_scale(int surfacePercent) noexcept;
	float player_bump_strength_percent(int surfacePercent) noexcept;
	bool resolve_bump_enabled(int surfacePercent, bool legacyResearchEnabled) noexcept;
	float bound_bump_transport_magnitude(float magnitude) noexcept;
	Request evaluate(const Input& input) noexcept;

	struct BumpInput
	{
		float surfaceSource = 0.0f;
		float deltaTimeSeconds = 0.0f;
		float threshold = BumpThreshold;
		float strengthPercent = float(BumpStrengthPercent);
		int durationMilliseconds = BumpDurationMilliseconds;
		int cooldownMilliseconds = BumpCooldownMilliseconds;
		bool enabled = false;
	};
	struct BumpFrame
	{
		float sourceDelta = 0.0f;
		float transientMetric = 0.0f;
		float threshold = 0.02f;
		float requestedMagnitude = 0.0f;
		float boundedMagnitude = 0.0f;
		int durationMilliseconds = 60;
		float cooldownRemainingSeconds = 0.0f;
		bool candidate = false;
		bool triggered = false;
	};
	class BumpDetector
	{
	public:
		const BumpFrame& evaluate(const BumpInput& input) noexcept;
		void reset() noexcept { previousSource_ = 0.0f; initialized_ = false; cooldownRemaining_ = 0.0f; current_ = {}; }
		const BumpFrame& frame() const noexcept { return current_; }
	private:
		float previousSource_ = 0.0f;
		float cooldownRemaining_ = 0.0f;
		bool initialized_ = false;
		BumpFrame current_{};
	};
}
