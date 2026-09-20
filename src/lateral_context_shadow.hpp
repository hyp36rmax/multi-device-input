#pragma once

#include "contextual_force_intent.hpp"

namespace HYP36RLateralContextShadow
{
	enum class Phase { Observational, Release };
	enum class Reason
	{
		Unavailable,
		PhaseDisabled,
		BiteAuthoritative,
		SurfaceContaminated,
		LowActivity,
		MixedResponse,
		BalanceDeadband,
		Active,
	};

	struct Inputs
	{
		float m4Directional = 0.0f;
		float legacyDirectional = 0.0f;
		HYP36RContextualIntent::Frame context{};
	};

	struct Frame
	{
		bool available = false;
		bool active = false;
		bool limiterActive = false;
		Phase phase = Phase::Observational;
		Reason reason = Reason::Unavailable;
		float lateralBalance = 0.0f;
		float lateralActivity = 0.0f;
		float modulation = 0.0f;
		float shadowDirectional = 0.0f;
		float shadowMinusM4 = 0.0f;
	};

	const Frame& evaluate(const Inputs& inputs);
	void reset();
	const Frame& frame();
	const char* phase_name(Phase phase);
	const char* reason_name(Reason reason);
}
