#include "road2_presentation.hpp"

#include <algorithm>
#include <cmath>

namespace HYP36RRoad2Presentation
{
	namespace
	{
		float clamp_unit(float value)
		{
			return (std::clamp)(value, 0.0f, 1.0f);
		}

		float clamp_signed(float value)
		{
			return (std::clamp)(value, -1.0f, 1.0f);
		}

		template <size_t N>
		float mean(const std::array<float, N>& values, size_t count)
		{
			float sum = 0.0f;
			for (size_t n = 0; n < count; ++n)
				sum += values[n];
			return count ? sum / static_cast<float>(count) : 0.0f;
		}
	}

	const Frame& PassivePrototype::evaluate(const HYP36RRoad2::Frame& policy) noexcept
	{
		Frame next{};
		next.candidate = candidate_;
		next.roadPolicyVersion = policy.policyVersion;
		next.frameId = policy.frameId;
		next.meta = policy.meta;
		if (policy.policyVersion != HYP36RRoad2::PolicyVersion ||
			policy.meta.validity != HYP36RSignalState::Validity::Valid)
		{
			current_ = next;
			return current_;
		}

		float directEnvelope = 0.0f;
		float channelContrast = 0.0f;
		if (policy.activity.state == HYP36RRoad2::ActivityState::ContinuousCandidate)
		{
			const float left = clamp_unit(std::abs(policy.activity.leftEvidence));
			const float right = clamp_unit(std::abs(policy.activity.rightEvidence));
			directEnvelope = clamp_unit(std::sqrt((left * left + right * right) * 0.5f));
			const float activity = left + right;
			channelContrast = activity > 1.0e-6f ? clamp_signed((left - right) / activity) : 0.0f;
		}

		const float group02 = clamp_unit(
			static_cast<float>(policy.spatial.group02.differingFromReference) * 0.5f);
		const float group13 = clamp_unit(
			static_cast<float>(policy.spatial.group13.differingFromReference) * 0.5f);
		const float directImbalance = clamp_signed(group02 - group13);

		float presentedEnvelope = directEnvelope;
		float presentedImbalance = directImbalance;
		if (candidate_ == Candidate::Conditioned3)
		{
			envelopeHistory_[historyIndex_] = directEnvelope;
			imbalanceHistory_[historyIndex_] = directImbalance;
			historyIndex_ = (historyIndex_ + 1) % ConditioningSamples;
			if (historyCount_ < ConditioningSamples)
				++historyCount_;
			presentedEnvelope = mean(envelopeHistory_, historyCount_);
			presentedImbalance = mean(imbalanceHistory_, historyCount_);
		}

		next.continuous.normalized = clamp_unit(presentedEnvelope);
		next.continuous.nativeChannelContrast = channelContrast;
		next.continuous.active = next.continuous.normalized > 1.0e-6f;
		next.spatial.group02Context = group02;
		next.spatial.group13Context = group13;
		next.spatial.neutralGroupAxis = clamp_signed(presentedImbalance);
		next.spatial.magnitude = std::abs(next.spatial.neutralGroupAxis);
		next.spatial.categoricalDifference =
			policy.spatial.group02.raw != policy.spatial.group13.raw;
		next.spatial.asymmetric = next.spatial.magnitude > 1.0e-6f ||
			next.spatial.categoricalDifference;
		next.transition.active = policy.transition.active;
		next.transition.changedMask = policy.transition.changedMask;
		next.transition.changedCount = policy.transition.changedCount;
		next.transition.topology = policy.spatial.topology;
		next.transition.coverage = policy.spatial.coverage;
		current_ = next;
		return current_;
	}

	void PassivePrototype::reset() noexcept
	{
		current_ = {};
		current_.candidate = candidate_;
		envelopeHistory_ = {};
		imbalanceHistory_ = {};
		historyIndex_ = 0;
		historyCount_ = 0;
	}
}
