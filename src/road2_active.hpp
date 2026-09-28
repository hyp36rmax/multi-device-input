#pragma once

#include <cstdint>
#include <string_view>

#include "road2_policy.hpp"
#include "road2_presentation.hpp"

namespace HYP36RRoad2Active
{
	inline constexpr uint32_t Version = 1;
	inline constexpr const char Name[] = "HYP36R_ROAD2_ACTIVE_V0";
	inline constexpr uint32_t DeterministicSeed = 0x48595036u;
	inline constexpr float InternalCeiling = 0.06f;
	inline constexpr float MaximumSlewPerSecond = 0.90f;

	enum class Mode : uint8_t { ReferencePlus, Experimental };
	enum class Phase : uint8_t { Quiet, Attack, Sustain, Release, FailSafe };
	enum class SafetyState : uint8_t { Clear, AuthorityZero, InvalidInput, OutputClamped, SlewLimited };

	struct Frame
	{
		uint32_t version = Version;
		uint64_t frameId = 0;
		Mode mode = Mode::ReferencePlus;
		Phase phase = Phase::Quiet;
		SafetyState safety = SafetyState::Clear;
		float nativeAuthority = 0.0f;
		float occupancyTarget = 0.0f;
		float occupancyEnvelope = 0.0f;
		float rawGenerator = 0.0f;
		float conditionedTexture = 0.0f;
		float unclampedContribution = 0.0f;
		float contribution = 0.0f;
		bool active = false;
		bool clamped = false;
		bool slewLimited = false;
		bool reset = false;
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
		float occupancyEnvelope_ = 0.0f;
		float previousContribution_ = 0.0f;
	};

	Mode mode_from_string(std::string_view value) noexcept;
	const char* mode_name(Mode mode) noexcept;
	const char* phase_name(Phase phase) noexcept;
	const char* safety_name(SafetyState safety) noexcept;
	float select_road(Mode mode, float referenceRoad, float experimentalRoad) noexcept;

	const Frame& evaluate(const HYP36RRoad2::Frame& policy,
		const HYP36RRoad2Presentation::Frame& presentation,
		float deltaTimeSeconds, Mode mode) noexcept;
	void reset() noexcept;
	const Frame& frame() noexcept;
}
