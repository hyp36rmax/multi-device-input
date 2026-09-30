#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "road2_policy.hpp"
#include "road2_presentation.hpp"

namespace HYP36RRoad2Active
{
	inline constexpr uint32_t Version = 2;
	inline constexpr const char Name[] = "HYP36R_ROAD2_ENHANCED_C1";
	inline constexpr uint32_t DeterministicSeed = 0x48595036u;
	inline constexpr float InternalCeiling = 0.06f;
	inline constexpr float MaximumSlewPerSecond = 0.90f;
	inline constexpr float RoadChannelSafetyCeiling = 0.25f;
	inline constexpr int ShippingCalibrationGain = 30;
	inline constexpr int DefaultDebugAuthorityGain = 10;
	inline constexpr std::array<int, 6> DebugAuthorityGains{ 8, 10, 15, 20, 25, 30 };

	enum class Mode : uint8_t { ReferencePlus, Experimental };
	enum class Phase : uint8_t { Quiet, Attack, Sustain, Release, FailSafe };
	enum class SafetyState : uint8_t { Clear, AuthorityZero, InvalidInput, OutputClamped, SlewLimited };
	enum class SurfaceArchetype : uint8_t { None, HardUneven, SoftRough, StripedRunoff, GenericEnhanced };

	struct Frame
	{
		uint32_t version = Version;
		uint64_t frameId = 0;
		Mode mode = Mode::ReferencePlus;
		Phase phase = Phase::Quiet;
		SafetyState safety = SafetyState::Clear;
		SurfaceArchetype archetype = SurfaceArchetype::None;
		float nativeAuthority = 0.0f;
		float presentationAuthority = 0.0f;
		float occupancyTarget = 0.0f;
		float occupancyEnvelope = 0.0f;
		float rawGenerator = 0.0f;
		float conditionedTexture = 0.0f;
		float aperiodicBase = 0.0f;
		float characterComponent = 0.0f;
		float resistanceComponent = 0.0f;
		float preSafetyContribution = 0.0f;
		float unclampedContribution = 0.0f;
		float contribution = 0.0f;
		bool active = false;
		bool clamped = false;
		bool slewLimited = false;
		bool reset = false;
	};

	struct GainFrame
	{
		int developmentGain = 1;
		float roadDetailScale = 1.0f;
		float preGainRoad = 0.0f;
		float postGainRoad = 0.0f;
		float boundedRoad = 0.0f;
		float finalRoad = 0.0f;
		bool clamped = false;
		bool slewLimited = false;
		bool invalidGainFallback = false;
	};

	class Generator
	{
	public:
		const Frame& evaluate(const HYP36RRoad2::Frame& policy,
			const HYP36RRoad2Presentation::Frame& presentation,
			float deltaTimeSeconds, Mode mode) noexcept;
		void reset() noexcept;
		const Frame& frame() const noexcept { return current_; }

	private:
		uint32_t next_random() noexcept;
		void advance_generator() noexcept;
		Frame fail_safe(Mode mode, uint64_t frameId) noexcept;

		Frame current_{};
		uint32_t randomState_ = DeterministicSeed;
		double fixedAccumulator_ = 0.0;
		float fastLowPass_ = 0.0f;
		float slowLowPass_ = 0.0f;
		float dcPreviousInput_ = 0.0f;
		float dcPreviousOutput_ = 0.0f;
		float whiteNoise_ = 0.0f;
		float roughLowPass_ = 0.0f;
		float characterPhase_ = 0.0f;
		float occupancyEnvelope_ = 0.0f;
		float previousContribution_ = 0.0f;
	};

	class DevelopmentGainStage
	{
	public:
		const GainFrame& evaluate(float roadAfterDetail, float roadDetailScale,
			float deltaTimeSeconds, int requestedGain, bool nativeAuthorized) noexcept;
		void reset() noexcept;
		const GainFrame& frame() const noexcept { return current_; }

	private:
		GainFrame current_{};
		float previousRoad_ = 0.0f;
	};

	Mode mode_from_string(std::string_view value) noexcept;
	const char* mode_name(Mode mode) noexcept;
	const char* phase_name(Phase phase) noexcept;
	const char* safety_name(SafetyState safety) noexcept;
	const char* archetype_name(SurfaceArchetype archetype) noexcept;
	SurfaceArchetype classify_surface(const HYP36RRoad2::Frame& policy) noexcept;
	float select_road(Mode mode, float referenceRoad, float experimentalRoad) noexcept;
	int sanitize_development_gain(int requestedGain) noexcept;
	int sanitize_debug_authority_gain(int requestedGain) noexcept;
	int resolve_calibration_gain(bool debugAuthorityEnabled, int debugAuthorityGain) noexcept;
	float resolve_surface_source(float generatedSurface, float roadDetailScale) noexcept;

	const Frame& evaluate(const HYP36RRoad2::Frame& policy,
		const HYP36RRoad2Presentation::Frame& presentation,
		float deltaTimeSeconds, Mode mode) noexcept;
	void reset() noexcept;
	const Frame& frame() noexcept;
	const GainFrame& evaluate_gain(float roadAfterDetail, float roadDetailScale,
		float deltaTimeSeconds, int requestedGain, bool nativeAuthorized) noexcept;
	void reset_gain() noexcept;
	const GainFrame& gain_frame() noexcept;
}
