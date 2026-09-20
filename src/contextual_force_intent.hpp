#pragma once

#include "bite_shadow_restoration.hpp"
#include "force2_shadow_composer.hpp"
#include "four_corner_context.hpp"

namespace HYP36RContextualIntent
{
	enum class LateralState { Unknown, Low, FrontBiased, Balanced, RearBiased, Mixed };
	enum class LongitudinalState { Unknown, Low, FrontDominant, Balanced, RearDominant, Mixed };
	enum class ChassisState { Unknown, Settled, Transitional, Displaced };
	enum class RecoveryContext { NotApplicable, Low, Moderate, High };

	struct Frame
	{
		bool available = false;
		bool active = false;
		bool surfaceContaminated = false;
		HYP36RForce2::EventPhase gripPhase = HYP36RForce2::EventPhase::Normal;
		HYP36RBiteShadow::Phase recoveryPhase = HYP36RBiteShadow::Phase::Baseline;
		LateralState lateralState = LateralState::Unknown;
		LongitudinalState longitudinalState = LongitudinalState::Unknown;
		ChassisState chassisState = ChassisState::Unknown;
		RecoveryContext recoveryContext = RecoveryContext::NotApplicable;
		float lateralBalance = 0.0f;
		float lateralLevel = 0.0f;
		float longitudinalBalance = 0.0f;
		float longitudinalLevel = 0.0f;
		float chassisLevel = 0.0f;
		float confidence = 0.0f;
	};

	class Interpreter
	{
	public:
		const Frame& evaluate(const HYP36RFourCorner::Frame& context,
			HYP36RForce2::EventPhase gripPhase, HYP36RBiteShadow::Phase recoveryPhase);
		void reset();
		const Frame& frame() const { return current_; }

	private:
		bool baselineValid_ = false;
		float baselineFrontRear_ = 0.0f;
		float baselineLeftRight_ = 0.0f;
		LateralState lateralCandidate_ = LateralState::Unknown;
		LongitudinalState longitudinalCandidate_ = LongitudinalState::Unknown;
		ChassisState chassisCandidate_ = ChassisState::Unknown;
		unsigned lateralPersistence_ = 0;
		unsigned longitudinalPersistence_ = 0;
		unsigned chassisPersistence_ = 0;
		Frame current_{};
	};

	const Frame& evaluate(const HYP36RFourCorner::Frame& context,
		HYP36RForce2::EventPhase gripPhase, HYP36RBiteShadow::Phase recoveryPhase);
	void reset();
	const Frame& frame();
	const char* lateral_state_name(LateralState state);
	const char* longitudinal_state_name(LongitudinalState state);
	const char* chassis_state_name(ChassisState state);
	const char* recovery_context_name(RecoveryContext context);
}
