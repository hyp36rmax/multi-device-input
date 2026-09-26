#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "road2_policy.hpp"

namespace HYP36RRoad2Presentation
{
	inline constexpr uint32_t PresentationVersion = 1;
	inline constexpr const char PresentationName[] = "HYP36R_ROAD_PRESENTATION_V1_SHADOW";

	enum class Candidate : uint8_t { Direct, Conditioned3 };

	struct ContinuousEnvelope
	{
		float normalized = 0.0f;
		float nativeChannelContrast = 0.0f;
		bool active = false;
	};

	struct SpatialImbalance
	{
		float group02Context = 0.0f;
		float group13Context = 0.0f;
		float neutralGroupAxis = 0.0f;
		float magnitude = 0.0f;
		bool categoricalDifference = false;
		bool asymmetric = false;
	};

	struct TransitionIntent
	{
		bool active = false;
		uint8_t changedMask = 0;
		uint8_t changedCount = 0;
		HYP36RRoad2::OccupancyTopology topology = HYP36RRoad2::OccupancyTopology::Unknown;
		HYP36RRoad2::Coverage coverage = HYP36RRoad2::Coverage::Unknown;
	};

	struct Frame
	{
		uint32_t presentationVersion = PresentationVersion;
		uint32_t roadPolicyVersion = HYP36RRoad2::PolicyVersion;
		uint64_t frameId = 0;
		Candidate candidate = Candidate::Direct;
		HYP36RRoad2::PolicyMeta meta{};
		ContinuousEnvelope continuous{};
		SpatialImbalance spatial{};
		TransitionIntent transition{};
	};

	class PassivePrototype
	{
	public:
		explicit PassivePrototype(Candidate candidate) noexcept : candidate_(candidate) {}
		const Frame& evaluate(const HYP36RRoad2::Frame& policy) noexcept;
		void reset() noexcept;
		const Frame& frame() const noexcept { return current_; }

	private:
		static constexpr size_t ConditioningSamples = 3;
		Candidate candidate_ = Candidate::Direct;
		Frame current_{};
		std::array<float, ConditioningSamples> envelopeHistory_{};
		std::array<float, ConditioningSamples> imbalanceHistory_{};
		size_t historyIndex_ = 0;
		size_t historyCount_ = 0;
	};
}
