#include "force2_shadow_composer.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace HYP36RForce2
{
	namespace
	{
		// M4B SHADOW RESEARCH constants. These values cannot affect hardware and
		// are intentionally provisional pending controlled shadow validation.
		constexpr float DivergenceFullScaleRad = 0.50f;
		constexpr float DivergenceActivation = 0.10f;
		constexpr float ResponseRateFullScaleRadPerSec = 3.0f;
		constexpr float EstablishedSlipRatio = 0.08f;
		constexpr float EstablishedGripLoss = 0.01f;
		constexpr float RecoveryAttenuation = 0.01f;
		constexpr unsigned EmergingPersistenceFrames = 3;
		constexpr unsigned RecoveryPersistenceFrames = 30;
		constexpr float NativeWeightStep = 1.0f / 30.0f;
		constexpr float MaximumNativeUnloading = 0.25f;
		constexpr float DirectionalBudget = 1.0f;
		constexpr float TextureBudget = 0.25f;
		constexpr float ImpactBudget = 0.55f;
		constexpr float TotalBudget = 1.0f;
		constexpr float ShadowSlewPerUpdate = 1.0f / 30.0f;

		Frame current{};
		float nativeWeight = 0.0f;
		float lastValidDivergence = 0.0f;
		float previousShadowPreMaster = 0.0f;
		bool havePreviousShadow = false;
		unsigned emergingFrames = 0;
		unsigned recoveryFrames = 0;
		EventPhase previousPhase = EventPhase::Normal;

		constexpr float clamp_unit(float value)
		{
			return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
		}

		constexpr float clamp_signed(float value)
		{
			return value < -1.0f ? -1.0f : (value > 1.0f ? 1.0f : value);
		}

		constexpr float normalize_divergence(float radians)
		{
			return clamp_signed(radians / DivergenceFullScaleRad);
		}

		constexpr float apply_unloading(float directional, float unloading)
		{
			return directional * (1.0f - clamp_unit(unloading));
		}

		constexpr float clamp_budget(float value, float budget)
		{
			return value < -budget ? -budget : (value > budget ? budget : value);
		}

		float finite_or_zero(float value)
		{
			return std::isfinite(value) ? value : 0.0f;
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

		NativeAvailability availability_for(HYP36RVehicleState::Validity validity)
		{
			switch (validity)
			{
			case HYP36RVehicleState::Validity::Valid:
				return NativeAvailability::Valid;
			case HYP36RVehicleState::Validity::TransitionSuppressed:
				return NativeAvailability::TransitionSuppressed;
			default:
				return NativeAvailability::Unavailable;
			}
		}

		static_assert(normalize_divergence(0.25f) == 0.5f);
		static_assert(normalize_divergence(-0.25f) == -0.5f);
		static_assert(normalize_divergence(1.0f) == 1.0f);
		static_assert(normalize_divergence(-1.0f) == -1.0f);
		static_assert(apply_unloading(0.8f, 0.25f) == 0.6f);
		static_assert(apply_unloading(-0.8f, 0.25f) == -0.6f);
		static_assert(apply_unloading(0.0f, 0.25f) == 0.0f);
		static_assert(apply_unloading(0.8f, 0.25f) <= 0.8f);
		static_assert(apply_unloading(-0.8f, 0.25f) >= -0.8f);
		static_assert(apply_unloading(0.8f, 1.5f) == 0.0f);
		static_assert(clamp_budget(2.0f, 1.0f) == 1.0f);
		static_assert(clamp_budget(-2.0f, 1.0f) == -1.0f);
	}

	const Frame& evaluate(const Inputs& rawInputs, const HYP36RVehicleState::Frame& vehicleState,
		ComposerMode mode)
	{
		const Inputs inputs{
			finite_or_zero(rawInputs.legacyDirectional),
			finite_or_zero(rawInputs.legacyForce),
			finite_or_zero(rawInputs.roadTexture),
			finite_or_zero(rawInputs.impact),
			clamp_unit(finite_or_zero(rawInputs.outputRamp)),
			finite_or_zero(rawInputs.syntheticLateralSpeed),
			clamp_unit(finite_or_zero(rawInputs.syntheticSlipRatio)),
			clamp_unit(finite_or_zero(rawInputs.syntheticGripLoss)),
			(std::max)(0.0f, finite_or_zero(rawInputs.masterStrength)),
			rawInputs.invertOutput,
		};

		const auto& semantic = vehicleState.current;
		const NativeAvailability availability = availability_for(semantic.validity);
		const bool nativeValid = availability == NativeAvailability::Valid;
		if (nativeValid)
			nativeWeight = (std::min)(1.0f, nativeWeight + NativeWeightStep);
		else
			nativeWeight = (std::max)(0.0f, nativeWeight - NativeWeightStep);

		float responseAngle = 0.0f;
		float responseRate = 0.0f;
		float divergence = lastValidDivergence;
		if (nativeValid)
		{
			if (semantic.responseAngleValid)
				responseAngle = finite_or_zero(semantic.responseAngleRad);
			if (semantic.responseRateValid)
				responseRate = clamp_signed(
					finite_or_zero(semantic.responseAngularRateRadPerSec) /
					ResponseRateFullScaleRadPerSec);
			if (semantic.referenceResponseErrorValid)
			{
				divergence = normalize_divergence(
					finite_or_zero(semantic.referenceResponseErrorRad));
				lastValidDivergence = divergence;
			}
		}

		const bool divergenceActive = nativeValid &&
			(std::abs(divergence) >= DivergenceActivation || semantic.responseAuthority < 0.99f);
		if (divergenceActive)
			emergingFrames = (std::min)(EmergingPersistenceFrames, emergingFrames + 1);
		else
			emergingFrames = 0;

		const bool established = inputs.syntheticSlipRatio >= EstablishedSlipRatio ||
			inputs.syntheticGripLoss >= EstablishedGripLoss;
		const bool overshootRecovery = nativeValid &&
			finite_or_zero(semantic.overshootAttenuation) >= RecoveryAttenuation;
		if (established)
			recoveryFrames = RecoveryPersistenceFrames;
		else if (recoveryFrames > 0)
			--recoveryFrames;

		EventPhase phase = EventPhase::Normal;
		if (established)
			phase = EventPhase::Established;
		else if (overshootRecovery || recoveryFrames > 0 ||
			previousPhase == EventPhase::Established)
			phase = EventPhase::Recovering;
		else if (emergingFrames >= EmergingPersistenceFrames)
			phase = EventPhase::Emerging;

		const float nativeUnloading = nativeWeight * std::abs(divergence) *
			MaximumNativeUnloading;
		const float directionalBeforeUnloading = clamp_budget(
			inputs.legacyDirectional, DirectionalBudget);
		const float directionalAfterUnloading = apply_unloading(
			directionalBeforeUnloading, nativeUnloading);
		const float activeDirectional = apply_unloading(
			inputs.legacyDirectional, nativeUnloading);

		const float shadowTexture = clamp_budget(inputs.roadTexture, TextureBudget);
		const float shadowImpact = clamp_budget(inputs.impact, ImpactBudget);
		const float preBudget = directionalAfterUnloading + inputs.roadTexture + inputs.impact;
		const float budgetedSum = directionalAfterUnloading + shadowTexture + shadowImpact;
		const float postBudget = clamp_budget(budgetedSum, TotalBudget);
		const bool headroomLimited = shadowTexture != inputs.roadTexture ||
			shadowImpact != inputs.impact || postBudget != budgetedSum ||
			directionalBeforeUnloading != inputs.legacyDirectional;

		const float prospectivePreMaster = std::tanh(postBudget) * inputs.outputRamp;
		float slewLimitedPreMaster = prospectivePreMaster;
		bool rateLimited = false;
		if (havePreviousShadow)
		{
			const float minimum = previousShadowPreMaster - ShadowSlewPerUpdate;
			const float maximum = previousShadowPreMaster + ShadowSlewPerUpdate;
			slewLimitedPreMaster = (std::clamp)(prospectivePreMaster, minimum, maximum);
			rateLimited = slewLimitedPreMaster != prospectivePreMaster;
		}
		previousShadowPreMaster = slewLimitedPreMaster;
		havePreviousShadow = true;

		float shadowOutput = slewLimitedPreMaster * inputs.masterStrength;
		if (inputs.invertOutput)
			shadowOutput = -shadowOutput;
		shadowOutput = clamp_signed(shadowOutput);
		float legacyForceOutput = inputs.legacyForce * inputs.masterStrength;
		if (inputs.invertOutput)
			legacyForceOutput = -legacyForceOutput;
		legacyForceOutput = clamp_signed(legacyForceOutput);

		current = {};
		current.mode = mode;
		current.context.nativeAvailability = availability;
		current.context.nativeWeight = clamp_unit(nativeWeight);
		current.context.eventPhase = phase;
		current.context.recovering = phase == EventPhase::Recovering;
		current.intent.directionalLoad = directionalBeforeUnloading;
		current.intent.unloading = clamp_unit(nativeUnloading);
		current.intent.motion = responseRate; // Diagnostic only in M4B.
		current.intent.roadTexture = inputs.roadTexture;
		current.intent.impact = inputs.impact;
		current.responseAngleDiagnostic = responseAngle;
		current.divergenceDiagnostic = divergence;
		current.responseRateDiagnostic = responseRate;
		current.legacyDirectionalComponent = inputs.legacyDirectional;
		current.legacyForceOutput = legacyForceOutput;
		current.activeDirectional = activeDirectional;
		current.shadowDirectional = directionalAfterUnloading;
		current.shadowTexture = shadowTexture;
		current.shadowImpact = shadowImpact;
		current.shadowPreBudget = preBudget;
		current.shadowPostBudget = postBudget;
		current.shadowPreMaster = slewLimitedPreMaster;
		current.shadowOutput = shadowOutput;
		current.rateLimitActive = rateLimited;
		current.headroomLimitActive = headroomLimited;
		previousPhase = phase;
		return current;
	}

	void reset()
	{
		current = {};
		nativeWeight = 0.0f;
		lastValidDivergence = 0.0f;
		previousShadowPreMaster = 0.0f;
		havePreviousShadow = false;
		emergingFrames = 0;
		recoveryFrames = 0;
		previousPhase = EventPhase::Normal;
	}

	const Frame& frame()
	{
		return current;
	}

	const char* mode_name(ComposerMode mode)
	{
		switch (mode)
		{
		case ComposerMode::Shadow:
			return "force2_shadow";
		case ComposerMode::Active:
			return "force2_active";
		default:
			return "legacy";
		}
	}

	ComposerMode mode_from_string(std::string_view value)
	{
		if (equals_ascii_case_insensitive(value, "Shadow"))
			return ComposerMode::Shadow;
		if (equals_ascii_case_insensitive(value, "Active"))
			return ComposerMode::Active;
		return ComposerMode::Legacy;
	}

	const char* availability_name(NativeAvailability availability)
	{
		switch (availability)
		{
		case NativeAvailability::Valid:
			return "native_valid";
		case NativeAvailability::TransitionSuppressed:
			return "transition_suppressed";
		default:
			return "native_unavailable";
		}
	}

	const char* event_phase_name(EventPhase phase)
	{
		switch (phase)
		{
		case EventPhase::Emerging:
			return "emerging";
		case EventPhase::Established:
			return "established";
		case EventPhase::Recovering:
			return "recovering";
		default:
			return "normal";
		}
	}
}
