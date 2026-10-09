#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "signal_state.hpp"

namespace HYP36RRoad2
{
	inline constexpr uint32_t PolicyVersion = 1;
	inline constexpr const char PolicyName[] = "HYP36R_ROAD_POLICY_V1";

	enum class OccupancyTopology : uint8_t { Unknown, Uniform, PairedSides, GeneralMixed };
	enum class Coverage : uint8_t { Unknown, Reference, Partial, Broad };
	enum class ActivityState : uint8_t { Unavailable, Inactive, ContinuousCandidate, EventExcluded };
	enum class ExclusionReason : uint8_t {
		None, InvalidSurface, InvalidNativeEffect, ReferenceUnestablished,
		LowConfidence, NoSurfaceContext, GearEvent, CollisionCandidate, UnknownNativeEvent
	};

	struct PolicyMeta
	{
		HYP36RSignalState::Validity validity = HYP36RSignalState::Validity::Unavailable;
		HYP36RSignalState::Source source = HYP36RSignalState::Source::None;
		HYP36RSignalState::Confidence confidence = HYP36RSignalState::Confidence::Unknown;
		HYP36RSignalState::FallbackReason fallback = HYP36RSignalState::FallbackReason::Missing;
		uint64_t sampleFrame = 0;
	};

	struct SideContext
	{
		std::array<uint32_t, 2> raw{};
		std::array<bool, 2> changed{};
		uint8_t differingFromReference = 0;
		bool internallyUniform = true;
	};

	struct SpatialContext
	{
		PolicyMeta meta{};
		OccupancyTopology topology = OccupancyTopology::Unknown;
		Coverage coverage = Coverage::Unknown;
		SideContext group02{};
		SideContext group13{};
		uint8_t distinctRawCount = 0;
		uint8_t differingFromReference = 0;
		bool referenceEstablished = false;
		uint32_t referenceRaw = 0;
	};

	struct ContinuousActivity
	{
		PolicyMeta meta{};
		ActivityState state = ActivityState::Unavailable;
		ExclusionReason exclusion = ExclusionReason::None;
		float leftEvidence = 0.0f;
		float rightEvidence = 0.0f;
		float riseEvidence = 0.0f;
	};

	struct TransitionEvent
	{
		PolicyMeta meta{};
		bool active = false;
		uint8_t changedMask = 0;
		uint8_t changedCount = 0;
		std::array<uint32_t, HYP36RSignalState::CornerCount> from{};
		std::array<uint32_t, HYP36RSignalState::CornerCount> to{};
	};

	struct Frame
	{
		uint32_t policyVersion = PolicyVersion;
		uint32_t signalSchemaVersion = HYP36RSignalState::SchemaVersion;
		uint64_t frameId = 0;
		PolicyMeta meta{};
		SpatialContext spatial{};
		ContinuousActivity activity{};
		TransitionEvent transition{};
	};

	class PassivePolicy
	{
	public:
		const Frame& evaluate(const HYP36RSignalState::Frame& signalState) noexcept;
		void reset() noexcept;
		const Frame& frame() const noexcept { return current_; }

	private:
		static constexpr uint8_t ReferenceSettleFrames = 6;
		Frame current_{};
		uint32_t pendingReference_ = 0;
		uint32_t referenceRaw_ = 0;
		uint8_t stableUniformFrames_ = 0;
		bool referenceEstablished_ = false;
	};

	// Single-writer passive runtime observer. It is evaluated only after the
	// established v1 DirectInput request and has no scalar/output conversion.
	const Frame& evaluate(const HYP36RSignalState::Frame& signalState) noexcept;
	void reset() noexcept;
	const Frame& frame() noexcept;
}
