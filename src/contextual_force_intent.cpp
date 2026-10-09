#include "contextual_force_intent.hpp"

#include <algorithm>
#include <cmath>

namespace HYP36RContextualIntent
{
	namespace
	{
		// M5D/M5E research thresholds. These describe the validation vehicle's
		// normalized context and are not force gains or universal car calibration.
		constexpr float LateralNoiseLevel = 0.08f;
		constexpr float LateralBiasThreshold = 0.18f;
		constexpr float LongitudinalNoiseLevel = 0.04f;
		constexpr float LongitudinalBiasThreshold = 0.20f;
		constexpr float ChassisResearchScale = 0.025f;
		constexpr unsigned ClassificationPersistence = 4;

		float clamp_unit(float value)
		{
			return (std::clamp)(value, 0.0f, 1.0f);
		}

		float magnitude_balance(float front, float rear)
		{
			const float total = std::abs(front) + std::abs(rear);
			return total > 0.0001f ? (std::abs(front) - std::abs(rear)) / total : 0.0f;
		}

		template <typename State>
		void persist(State candidate, State& pending, unsigned& count, State& selected)
		{
			if (candidate != pending)
			{
				pending = candidate;
				count = 1;
			}
			else if (count < ClassificationPersistence)
			{
				++count;
			}
			if (count >= ClassificationPersistence)
				selected = candidate;
		}

		Interpreter sharedInterpreter{};
	}

	const Frame& Interpreter::evaluate(const HYP36RFourCorner::Frame& context,
		HYP36RForce2::EventPhase gripPhase, HYP36RBiteShadow::Phase recoveryPhase)
	{
		if (!context.available)
		{
			reset();
			return current_;
		}

		Frame next{};
		next.available = true;
		next.gripPhase = gripPhase;
		next.recoveryPhase = recoveryPhase;
		next.surfaceContaminated = context.surface.anyAsymmetry;

		const float frontLateral = context.front.lateralResponse;
		const float rearLateral = context.rear.lateralResponse;
		next.lateralLevel = clamp_unit((std::abs(frontLateral) + std::abs(rearLateral)) * 0.5f);
		next.lateralBalance = magnitude_balance(frontLateral, rearLateral);
		LateralState lateralCandidate = LateralState::Balanced;
		if (next.lateralLevel < LateralNoiseLevel)
			lateralCandidate = LateralState::Low;
		else if (frontLateral * rearLateral < 0.0f &&
			(std::min)(std::abs(frontLateral), std::abs(rearLateral)) >= LateralNoiseLevel)
			lateralCandidate = LateralState::Mixed;
		else if (next.lateralBalance > LateralBiasThreshold)
			lateralCandidate = LateralState::FrontBiased;
		else if (next.lateralBalance < -LateralBiasThreshold)
			lateralCandidate = LateralState::RearBiased;
		persist(lateralCandidate, lateralCandidate_, lateralPersistence_, next.lateralState);
		if (lateralPersistence_ < ClassificationPersistence)
			next.lateralState = current_.lateralState;

		const float frontLongitudinal = context.front.longitudinalResponse;
		const float rearLongitudinal = context.rear.longitudinalResponse;
		next.longitudinalLevel = clamp_unit(
			(std::abs(frontLongitudinal) + std::abs(rearLongitudinal)) * 0.5f);
		next.longitudinalBalance = magnitude_balance(frontLongitudinal, rearLongitudinal);
		LongitudinalState longitudinalCandidate = LongitudinalState::Balanced;
		if (next.longitudinalLevel < LongitudinalNoiseLevel)
			longitudinalCandidate = LongitudinalState::Low;
		else if (frontLongitudinal * rearLongitudinal < 0.0f &&
			(std::min)(std::abs(frontLongitudinal), std::abs(rearLongitudinal)) >= LongitudinalNoiseLevel)
			longitudinalCandidate = LongitudinalState::Mixed;
		else if (next.longitudinalBalance > LongitudinalBiasThreshold)
			longitudinalCandidate = LongitudinalState::FrontDominant;
		else if (next.longitudinalBalance < -LongitudinalBiasThreshold)
			longitudinalCandidate = LongitudinalState::RearDominant;
		persist(longitudinalCandidate, longitudinalCandidate_, longitudinalPersistence_,
			next.longitudinalState);
		if (longitudinalPersistence_ < ClassificationPersistence)
			next.longitudinalState = current_.longitudinalState;

		if (!baselineValid_)
		{
			baselineFrontRear_ = context.frontRearDisplacementBias;
			baselineLeftRight_ = context.leftRightDisplacementBias;
			baselineValid_ = true;
		}
		const bool quietReference = next.lateralLevel < 0.05f && next.longitudinalLevel < 0.03f &&
			gripPhase == HYP36RForce2::EventPhase::Normal && !next.surfaceContaminated;
		if (quietReference)
		{
			baselineFrontRear_ += (context.frontRearDisplacementBias - baselineFrontRear_) * 0.02f;
			baselineLeftRight_ += (context.leftRightDisplacementBias - baselineLeftRight_) * 0.02f;
		}
		const float frontRearOffset = context.frontRearDisplacementBias - baselineFrontRear_;
		const float leftRightOffset = context.leftRightDisplacementBias - baselineLeftRight_;
		next.chassisLevel = clamp_unit(std::hypot(frontRearOffset, leftRightOffset) /
			ChassisResearchScale);
		ChassisState chassisCandidate = next.chassisLevel < 0.30f ? ChassisState::Settled
			: next.chassisLevel < 0.70f ? ChassisState::Transitional : ChassisState::Displaced;
		persist(chassisCandidate, chassisCandidate_, chassisPersistence_, next.chassisState);
		if (chassisPersistence_ < ClassificationPersistence)
			next.chassisState = current_.chassisState;

		const float dynamicLevel = (std::max)({
			next.lateralLevel, next.longitudinalLevel, next.chassisLevel });
		if (recoveryPhase != HYP36RBiteShadow::Phase::Baseline)
			next.recoveryContext = dynamicLevel < 0.35f ? RecoveryContext::Low
				: dynamicLevel < 0.70f ? RecoveryContext::Moderate : RecoveryContext::High;

		const float persistence = clamp_unit(static_cast<float>((std::min)({
			lateralPersistence_, longitudinalPersistence_, chassisPersistence_ })) /
			static_cast<float>(ClassificationPersistence));
		const float dynamicRelevance = gripPhase == HYP36RForce2::EventPhase::Normal ? 0.75f : 1.0f;
		const float surfaceConfidence = next.surfaceContaminated ? 0.50f : 1.0f;
		next.confidence = clamp_unit(dynamicLevel * persistence * dynamicRelevance * surfaceConfidence);
		next.active = next.confidence >= 0.15f &&
			(next.lateralState != LateralState::Low ||
			 next.longitudinalState != LongitudinalState::Low ||
			 next.chassisState != ChassisState::Settled);
		current_ = next;
		return current_;
	}

	void Interpreter::reset()
	{
		baselineValid_ = false;
		baselineFrontRear_ = baselineLeftRight_ = 0.0f;
		lateralCandidate_ = LateralState::Unknown;
		longitudinalCandidate_ = LongitudinalState::Unknown;
		chassisCandidate_ = ChassisState::Unknown;
		lateralPersistence_ = longitudinalPersistence_ = chassisPersistence_ = 0;
		current_ = {};
	}

	const Frame& evaluate(const HYP36RFourCorner::Frame& context,
		HYP36RForce2::EventPhase gripPhase, HYP36RBiteShadow::Phase recoveryPhase)
	{
		return sharedInterpreter.evaluate(context, gripPhase, recoveryPhase);
	}

	void reset() { sharedInterpreter.reset(); }
	const Frame& frame() { return sharedInterpreter.frame(); }

	const char* lateral_state_name(LateralState state)
	{
		switch (state) {
		case LateralState::Low: return "low";
		case LateralState::FrontBiased: return "front_biased";
		case LateralState::Balanced: return "balanced";
		case LateralState::RearBiased: return "rear_biased";
		case LateralState::Mixed: return "mixed";
		default: return "unknown";
		}
	}

	const char* longitudinal_state_name(LongitudinalState state)
	{
		switch (state) {
		case LongitudinalState::Low: return "low";
		case LongitudinalState::FrontDominant: return "front_dominant";
		case LongitudinalState::Balanced: return "balanced";
		case LongitudinalState::RearDominant: return "rear_dominant";
		case LongitudinalState::Mixed: return "mixed";
		default: return "unknown";
		}
	}

	const char* chassis_state_name(ChassisState state)
	{
		switch (state) {
		case ChassisState::Settled: return "settled";
		case ChassisState::Transitional: return "transitional";
		case ChassisState::Displaced: return "displaced";
		default: return "unknown";
		}
	}

	const char* recovery_context_name(RecoveryContext context)
	{
		switch (context) {
		case RecoveryContext::Low: return "low";
		case RecoveryContext::Moderate: return "moderate";
		case RecoveryContext::High: return "high";
		default: return "not_applicable";
		}
	}
}
