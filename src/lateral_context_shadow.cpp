#include "lateral_context_shadow.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace HYP36RLateralContextShadow
{
	namespace
	{
		// PROVISIONAL DINO-BASELINE RESEARCH VALUES. These are passive-study
		// bounds, not universal normalization or production Force tuning.
		constexpr float LateralActivityFloor = 0.08f;
		constexpr float LateralActivityFull = 0.25f;
		constexpr float BalanceDeadband = 0.02f;
		constexpr float ModulationCeiling = 0.04f;
		constexpr float ActiveEpsilon = 0.000001f;

		Frame current{};

		float clamp_unit(float value)
		{
			return (std::clamp)(value, 0.0f, 1.0f);
		}

		float signed_balance_weight(float balance)
		{
			const float magnitude = std::abs(balance);
			if (magnitude <= BalanceDeadband)
				return 0.0f;
			const float weight = clamp_unit(
				(magnitude - BalanceDeadband) / (1.0f - BalanceDeadband));
			return std::copysign(weight, balance);
		}

		bool equals_ascii_case_insensitive(std::string_view left, std::string_view right)
		{
			if (left.size() != right.size())
				return false;
			for (size_t index = 0; index < left.size(); ++index)
			{
				const auto leftChar = static_cast<unsigned char>(left[index]);
				const auto rightChar = static_cast<unsigned char>(right[index]);
				if (std::tolower(leftChar) != std::tolower(rightChar))
					return false;
			}
			return true;
		}
	}

	const Frame& evaluate(const Inputs& inputs)
	{
		Frame next{};
		next.shadowDirectional = inputs.m4Directional;

		if (!inputs.context.available)
		{
			current = next;
			return current;
		}

		next.available = true;
		next.lateralBalance = (std::clamp)(inputs.context.lateralBalance, -1.0f, 1.0f);
		next.lateralActivity = clamp_unit(inputs.context.lateralLevel);

		if (inputs.biteActive ||
			inputs.context.recoveryPhase != HYP36RBiteShadow::Phase::Baseline)
			next.reason = Reason::BiteAuthoritative;
		else if (inputs.context.gripPhase != HYP36RForce2::EventPhase::Emerging)
			next.reason = Reason::PhaseDisabled;
		else
		{
			next.phase = Phase::Release;
			if (inputs.context.surfaceContaminated)
				next.reason = Reason::SurfaceContaminated;
			else if (!inputs.context.active || next.lateralActivity <= LateralActivityFloor)
				next.reason = Reason::LowActivity;
			else if (inputs.context.lateralState == HYP36RContextualIntent::LateralState::Mixed)
				next.reason = Reason::MixedResponse;
			else
			{
				const float balanceWeight = signed_balance_weight(next.lateralBalance);
				if (balanceWeight == 0.0f)
					next.reason = Reason::BalanceDeadband;
				else
				{
					const float activityWeight = clamp_unit(
						(next.lateralActivity - LateralActivityFloor) /
						(LateralActivityFull - LateralActivityFloor));
					const float confidence = clamp_unit(inputs.context.confidence);
					next.modulation = ModulationCeiling * balanceWeight *
						activityWeight * confidence;
					const float candidate = inputs.m4Directional * (1.0f + next.modulation);
					const float legacyLimit = std::abs(inputs.legacyDirectional);
					next.shadowDirectional = (std::clamp)(candidate, -legacyLimit, legacyLimit);
					next.limiterActive = next.shadowDirectional != candidate;
					next.shadowMinusM4 = next.shadowDirectional - inputs.m4Directional;
					next.active = std::abs(next.shadowMinusM4) > ActiveEpsilon;
					next.reason = next.active ? Reason::Active : Reason::BalanceDeadband;
				}
			}
		}

		current = next;
		return current;
	}

	void reset() { current = {}; }
	const Frame& frame() { return current; }

	const char* phase_name(Phase phase)
	{
		return phase == Phase::Release ? "release" : "observational";
	}

	const char* reason_name(Reason reason)
	{
		switch (reason) {
		case Reason::PhaseDisabled: return "phase_disabled";
		case Reason::BiteAuthoritative: return "bite_authoritative";
		case Reason::SurfaceContaminated: return "surface_contaminated";
		case Reason::LowActivity: return "low_activity";
		case Reason::MixedResponse: return "mixed_response";
		case Reason::BalanceDeadband: return "balance_deadband";
		case Reason::Active: return "active";
		default: return "unavailable";
		}
	}

	HardwareMode hardware_mode_from_string(std::string_view value)
	{
		if (equals_ascii_case_insensitive(value, "M5_LATERAL_ACTIVE"))
			return HardwareMode::M5LateralActive;
		return HardwareMode::M4Only;
	}

	const char* hardware_mode_name(HardwareMode mode)
	{
		return mode == HardwareMode::M5LateralActive
			? "m5_lateral_active" : "m4_only";
	}
}
