#pragma once

#include <cstdint>

#include "road2_presentation.hpp"

namespace HYP36RRoad2Periodic
{
	inline constexpr uint32_t ContractVersion = 1;
	inline constexpr const char ContractName[] = "HYP36R_ROAD_PERIODIC_V1_SHADOW";

	enum class ResetReason : uint8_t {
		None, Explicit, SessionStart, SessionChanged, SourceUnavailable,
		RoadInactive, InvalidAmplitude, InvalidTiming, TimingDiscontinuity
	};
	enum class EnvelopeState : uint8_t { Quiet, Attack, Sustain, Release };
	enum class Provenance : uint8_t { None, RoadPresentationCandidateA };

	struct TimingInput
	{
		uint64_t sessionId = 0;
		uint64_t frameId = 0;
		float deltaTimeSeconds = 0.0f;
		float speedMagnitude = 0.0f;
	};

	struct Frame
	{
		uint32_t contractVersion = ContractVersion;
		uint32_t presentationVersion = HYP36RRoad2Presentation::PresentationVersion;
		uint64_t sessionId = 0;
		uint64_t frameId = 0;
		bool available = false;
		bool valid = false;
		ResetReason resetReason = ResetReason::SessionStart;
		EnvelopeState envelopeState = EnvelopeState::Quiet;
		Provenance provenance = Provenance::None;
		HYP36RSignalState::Confidence confidence = HYP36RSignalState::Confidence::Unknown;
		float sourceAmplitude = 0.0f;
		float conditionedAmplitude = 0.0f;
		float proposedAmplitude = 0.0f;
		float speedMagnitude = 0.0f;
		float validDeltaTime = 0.0f;
		float accumulatedDistance = 0.0f;
		float timePhase = 0.0f;
		float distancePhase = 0.0f;
		float timeRequest = 0.0f;
		float distanceRequest = 0.0f;
		float timePreBound = 0.0f;
		float distancePreBound = 0.0f;
		float timePostBound = 0.0f;
		float distancePostBound = 0.0f;
		bool timeBoundActive = false;
		bool distanceBoundActive = false;
	};

	class PassivePeriodicRequest
	{
	public:
		const Frame& evaluate(const HYP36RRoad2Presentation::Frame& road,
			const TimingInput& timing) noexcept;
		void reset(ResetReason reason = ResetReason::Explicit) noexcept;
		const Frame& frame() const noexcept { return current_; }

	private:
		static constexpr float MaxValidDeltaSeconds = 0.1f;
		static constexpr float ResearchEnvelopeSeconds = 0.05f;
		Frame current_{};
		uint64_t sessionId_ = 0;
		bool sessionEstablished_ = false;
		float envelope_ = 0.0f;
		float accumulatedDistance_ = 0.0f;
		float timePhase_ = 0.0f;
		float distancePhase_ = 0.0f;
		ResetReason pendingReset_ = ResetReason::SessionStart;
	};
}
