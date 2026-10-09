#include "road2_periodic_shadow.hpp"

#include <algorithm>
#include <cmath>

namespace HYP36RRoad2Periodic
{
	namespace
	{
		constexpr float TwoPi = 6.2831853071795864769f;
		float clamp_unit(float value) { return (std::clamp)(value, 0.0f, 1.0f); }
		float wrap_unit(float value) { return value - std::floor(value); }
		float bounded(float value, bool& active)
		{
			const float result = (std::clamp)(value, -1.0f, 1.0f);
			active = result != value;
			return result;
		}
	}

	const Frame& PassivePeriodicRequest::evaluate(
		const HYP36RRoad2Presentation::Frame& road, const TimingInput& timing) noexcept
	{
		Frame next{};
		next.sessionId = timing.sessionId;
		next.frameId = timing.frameId;
		next.presentationVersion = road.presentationVersion;
		next.speedMagnitude = std::isfinite(timing.speedMagnitude)
			? std::max(timing.speedMagnitude, 0.0f) : 0.0f;

		if (!sessionEstablished_ || timing.sessionId != sessionId_)
		{
			reset(sessionEstablished_ ? ResetReason::SessionChanged : ResetReason::SessionStart);
			sessionId_ = timing.sessionId;
			sessionEstablished_ = true;
		}

		auto fail_closed = [&](ResetReason reason) -> const Frame& {
			reset(reason);
			next.available = false;
			next.valid = false;
			next.resetReason = reason;
			next.sessionId = timing.sessionId;
			next.frameId = timing.frameId;
			next.speedMagnitude = std::isfinite(timing.speedMagnitude)
				? std::max(timing.speedMagnitude, 0.0f) : 0.0f;
			current_ = next;
			return current_;
		};

		if (road.presentationVersion != HYP36RRoad2Presentation::PresentationVersion ||
			road.meta.validity != HYP36RSignalState::Validity::Valid)
			return fail_closed(ResetReason::SourceUnavailable);
		if (!std::isfinite(road.continuous.normalized))
			return fail_closed(ResetReason::InvalidAmplitude);
		if (!std::isfinite(timing.deltaTimeSeconds) || timing.deltaTimeSeconds <= 0.0f)
			return fail_closed(ResetReason::InvalidTiming);
		if (timing.deltaTimeSeconds > MaxValidDeltaSeconds)
			return fail_closed(ResetReason::TimingDiscontinuity);

		next.available = true;
		next.valid = true;
		next.resetReason = pendingReset_;
		pendingReset_ = ResetReason::None;
		next.provenance = Provenance::RoadPresentationCandidateA;
		next.confidence = road.meta.confidence;
		next.validDeltaTime = timing.deltaTimeSeconds;
		next.sourceAmplitude = clamp_unit(road.continuous.normalized);

		const bool requested = road.continuous.active && next.sourceAmplitude > 0.0f;
		if (!requested)
			return fail_closed(ResetReason::RoadInactive);
		const float target = requested ? next.sourceAmplitude : 0.0f;
		const float step = clamp_unit(timing.deltaTimeSeconds / ResearchEnvelopeSeconds);
		if (target > envelope_)
			envelope_ = std::min(target, envelope_ + step);
		else
			envelope_ = std::max(target, envelope_ - step);
		next.conditionedAmplitude = envelope_;
		next.proposedAmplitude = envelope_;
		next.envelopeState = envelope_ <= 0.0f ? EnvelopeState::Quiet
			: target > envelope_ ? EnvelopeState::Attack
			: target < envelope_ ? EnvelopeState::Release : EnvelopeState::Sustain;

		// Both rates are dimensionless research normalizations, not Hz or a
		// physical wavelength. Time advances one normalized cycle per second;
		// distance advances one normalized cycle per game speed-distance unit.
		timePhase_ = wrap_unit(timePhase_ + timing.deltaTimeSeconds);
		const float distanceIncrement = next.speedMagnitude * timing.deltaTimeSeconds;
		accumulatedDistance_ += distanceIncrement;
		distancePhase_ = wrap_unit(distancePhase_ + distanceIncrement);
		next.accumulatedDistance = accumulatedDistance_;
		next.timePhase = timePhase_;
		next.distancePhase = distancePhase_;
		next.timeRequest = std::sin(TwoPi * timePhase_) * envelope_;
		next.distanceRequest = std::sin(TwoPi * distancePhase_) * envelope_;
		next.timePreBound = next.timeRequest;
		next.distancePreBound = next.distanceRequest;
		next.timePostBound = bounded(next.timePreBound, next.timeBoundActive);
		next.distancePostBound = bounded(next.distancePreBound, next.distanceBoundActive);
		current_ = next;
		return current_;
	}

	void PassivePeriodicRequest::reset(ResetReason reason) noexcept
	{
		current_ = {};
		envelope_ = 0.0f;
		accumulatedDistance_ = 0.0f;
		timePhase_ = 0.0f;
		distancePhase_ = 0.0f;
		pendingReset_ = reason;
	}
}
