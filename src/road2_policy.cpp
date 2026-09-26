#include "road2_policy.hpp"

#include <cmath>

namespace HYP36RRoad2
{
	namespace
	{
		PassivePolicy RuntimePolicy{};

		PolicyMeta valid_meta(HYP36RSignalState::Confidence confidence, uint64_t frame)
		{
			return { HYP36RSignalState::Validity::Valid,
				HYP36RSignalState::Source::DerivedEvidenceSupported, confidence,
				HYP36RSignalState::FallbackReason::None, frame };
		}

		PolicyMeta inherited_invalid(const HYP36RSignalState::SignalMeta& source)
		{
			return { source.validity, source.source, source.confidence, source.fallback,
				source.sampleFrame };
		}

		PolicyMeta unavailable_meta(HYP36RSignalState::FallbackReason fallback, uint64_t frame)
		{
			return { HYP36RSignalState::Validity::Unavailable,
				HYP36RSignalState::Source::None, HYP36RSignalState::Confidence::Unknown,
				fallback, frame };
		}

		ExclusionReason event_exclusion(HYP36RSignalState::EventClass event)
		{
			switch (event)
			{
			case HYP36RSignalState::EventClass::GearShift:
				return ExclusionReason::GearEvent;
			case HYP36RSignalState::EventClass::CollisionCandidate:
				return ExclusionReason::CollisionCandidate;
			case HYP36RSignalState::EventClass::UnknownNativeEvent:
				return ExclusionReason::UnknownNativeEvent;
			default:
				return ExclusionReason::None;
			}
		}
	}

	const Frame& PassivePolicy::evaluate(const HYP36RSignalState::Frame& signal) noexcept
	{
		Frame next{};
		next.signalSchemaVersion = signal.schemaVersion;
		next.frameId = signal.frameId;
		if (signal.schemaVersion != HYP36RSignalState::SchemaVersion ||
			signal.surfaces.meta.validity != HYP36RSignalState::Validity::Valid ||
			signal.surfaces.meta.confidence < HYP36RSignalState::Confidence::Moderate)
		{
			if (signal.schemaVersion != HYP36RSignalState::SchemaVersion)
				next.meta = unavailable_meta(HYP36RSignalState::FallbackReason::UnsupportedSource,
					signal.frameId);
			else if (signal.surfaces.meta.validity == HYP36RSignalState::Validity::Valid)
				next.meta = unavailable_meta(HYP36RSignalState::FallbackReason::Missing,
					signal.frameId);
			else
				next.meta = inherited_invalid(signal.surfaces.meta);
			next.spatial.meta = next.meta;
			next.activity.meta = next.meta;
			next.activity.exclusion = signal.surfaces.meta.validity == HYP36RSignalState::Validity::Valid
				? ExclusionReason::LowConfidence : ExclusionReason::InvalidSurface;
			next.transition.meta = next.meta;
			current_ = next;
			return current_;
		}

		next.meta = valid_meta(HYP36RSignalState::Confidence::High, signal.frameId);
		next.spatial.meta = next.meta;
		next.spatial.distinctRawCount = signal.occupancy.distinctCount;
		if (signal.occupancy.allSame)
			next.spatial.topology = OccupancyTopology::Uniform;
		else if (signal.occupancy.pair02Same && signal.occupancy.pair13Same)
			next.spatial.topology = OccupancyTopology::PairedSides;
		else
			next.spatial.topology = OccupancyTopology::GeneralMixed;

		next.spatial.group02.raw = { signal.surfaces.current[0], signal.surfaces.current[2] };
		next.spatial.group13.raw = { signal.surfaces.current[1], signal.surfaces.current[3] };
		next.spatial.group02.changed = { signal.surfaces.changed[0], signal.surfaces.changed[2] };
		next.spatial.group13.changed = { signal.surfaces.changed[1], signal.surfaces.changed[3] };
		next.spatial.group02.internallyUniform = signal.surfaces.current[0] == signal.surfaces.current[2];
		next.spatial.group13.internallyUniform = signal.surfaces.current[1] == signal.surfaces.current[3];

		if (!referenceEstablished_ && signal.occupancy.allSame)
		{
			const auto candidate = signal.surfaces.current[0];
			if (stableUniformFrames_ == 0 || pendingReference_ != candidate)
			{
				pendingReference_ = candidate;
				stableUniformFrames_ = 1;
			}
			else if (stableUniformFrames_ < ReferenceSettleFrames)
				++stableUniformFrames_;
			if (stableUniformFrames_ >= ReferenceSettleFrames)
			{
				referenceRaw_ = pendingReference_;
				referenceEstablished_ = true;
			}
		}

		next.spatial.referenceEstablished = referenceEstablished_;
		next.spatial.referenceRaw = referenceRaw_;
		if (referenceEstablished_)
		{
			for (size_t n = 0; n < HYP36RSignalState::CornerCount; ++n)
			{
				if (signal.surfaces.current[n] != referenceRaw_)
				{
					++next.spatial.differingFromReference;
					if (n == 0 || n == 2)
						++next.spatial.group02.differingFromReference;
					else
						++next.spatial.group13.differingFromReference;
				}
			}
			next.spatial.coverage = next.spatial.differingFromReference == 0
				? Coverage::Reference : next.spatial.differingFromReference == 4
				? Coverage::Broad : Coverage::Partial;
		}

		next.transition.meta = valid_meta(HYP36RSignalState::Confidence::High, signal.frameId);
		next.transition.from = signal.surfaces.previous;
		next.transition.to = signal.surfaces.current;
		for (size_t n = 0; n < HYP36RSignalState::CornerCount; ++n)
		{
			if (signal.surfaces.changed[n])
			{
				next.transition.active = true;
				next.transition.changedMask |= static_cast<uint8_t>(1u << n);
				++next.transition.changedCount;
			}
		}

		if (signal.nativeEffect.meta.validity != HYP36RSignalState::Validity::Valid ||
			!std::isfinite(signal.nativeEffect.left) || !std::isfinite(signal.nativeEffect.right) ||
			!std::isfinite(signal.nativeEffect.rise))
		{
			next.activity.meta = inherited_invalid(signal.nativeEffect.meta);
			next.activity.exclusion = ExclusionReason::InvalidNativeEffect;
		}
		else
		{
			next.activity.meta = valid_meta(HYP36RSignalState::Confidence::Possible, signal.frameId);
			next.activity.leftEvidence = signal.nativeEffect.left;
			next.activity.rightEvidence = signal.nativeEffect.right;
			next.activity.riseEvidence = signal.nativeEffect.rise;
			const auto eventReason = event_exclusion(signal.eventIntent.classification);
			if (eventReason != ExclusionReason::None)
			{
				next.activity.state = ActivityState::EventExcluded;
				next.activity.exclusion = eventReason;
			}
			else if (!referenceEstablished_)
			{
				next.activity.state = ActivityState::Inactive;
				next.activity.exclusion = ExclusionReason::ReferenceUnestablished;
			}
			else if (next.spatial.coverage == Coverage::Reference)
			{
				next.activity.state = ActivityState::Inactive;
				next.activity.exclusion = ExclusionReason::NoSurfaceContext;
			}
			else if (std::abs(signal.nativeEffect.left) > 1.0e-6f ||
				std::abs(signal.nativeEffect.right) > 1.0e-6f)
			{
				next.activity.state = ActivityState::ContinuousCandidate;
				next.activity.meta.confidence = HYP36RSignalState::Confidence::Moderate;
			}
			else
				next.activity.state = ActivityState::Inactive;
		}

		current_ = next;
		return current_;
	}

	void PassivePolicy::reset() noexcept
	{
		current_ = {};
		pendingReference_ = 0;
		referenceRaw_ = 0;
		stableUniformFrames_ = 0;
		referenceEstablished_ = false;
	}

	const Frame& evaluate(const HYP36RSignalState::Frame& signalState) noexcept
	{
		return RuntimePolicy.evaluate(signalState);
	}
	void reset() noexcept { RuntimePolicy.reset(); }
	const Frame& frame() noexcept { return RuntimePolicy.frame(); }
}
