#include "bite_shadow_restoration.hpp"

#include <algorithm>
#include <cmath>

namespace HYP36RBiteShadow
{
	namespace
	{
		// M4F passive research constants. The model has no hardware-output path.
		constexpr float MaximumM4CUnloading = 0.25f;
		constexpr float MaximumEarlyRestorationFraction = 0.60f;
		constexpr float MinimumBiteConfidence = 0.20f;
		constexpr float RenewedSeparationRatePerSecond = 0.08f;
		constexpr float RestorationBuildRatePerSecond = 0.08f;
		constexpr float AbortReturnRatePerSecond = 0.04f;
		constexpr float ReturnedConvergenceRatePerSecond = 0.08f;
		constexpr float MinimumDeltaSeconds = 1.0f / 240.0f;
		constexpr float MaximumDeltaSeconds = 0.1f;

		Model model{};

		constexpr float clamp_unit(float value)
		{
			return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
		}

		constexpr float move_toward(float value, float target, float maximumDelta)
		{
			return value < target ? (std::min)(target, value + maximumDelta)
				: (std::max)(target, value - maximumDelta);
		}

		float finite_or_zero(float value)
		{
			return std::isfinite(value) ? value : 0.0f;
		}

		constexpr bool valid_delta(float value)
		{
			return value >= MinimumDeltaSeconds && value <= MaximumDeltaSeconds;
		}

		static_assert(move_toward(0.0f, 1.0f, 0.1f) == 0.1f);
		static_assert(move_toward(1.0f, 0.0f, 0.1f) == 0.9f);
		static_assert(clamp_unit(-1.0f) == 0.0f);
		static_assert(clamp_unit(2.0f) == 1.0f);
	}

	const Frame& Model::evaluate(const Inputs& rawInputs)
	{
		const float dt = finite_or_zero(rawInputs.deltaSeconds);
		const float m4cUnloading = (std::clamp)(finite_or_zero(
			rawInputs.currentM4CUnloading), 0.0f, MaximumM4CUnloading);
		const float legacyDirectional = finite_or_zero(rawInputs.legacyDirectional);
		const bool nativeValid = rawInputs.nativeValidity == HYP36RVehicleState::Validity::Valid &&
			valid_delta(dt);
		const float confidence = clamp_unit(finite_or_zero(rawInputs.bite.confidence));
		const bool vehicleStillClosing = rawInputs.bite.errorClosingRate > 0.0f &&
			rawInputs.bite.vehicleConvergence >
				(std::max)(0.0f, rawInputs.bite.driverConvergence);
		const bool credibleBite = nativeValid && rawInputs.bite.active &&
			confidence >= MinimumBiteConfidence && vehicleStillClosing;
		const bool renewedSeparation = rawInputs.bite.errorClosingRate <
			-RenewedSeparationRatePerSecond;
		const bool holdBite = nativeValid && rawInputs.bite.active &&
			!credibleBite && !renewedSeparation;
		const bool returned = nativeValid && rawInputs.bite.state == HYP36RBite::State::Returned;

		const float previousAllowance = restorationAllowance_;
		Phase phase = Phase::Baseline;
		bool abortActive = false;
		if (credibleBite)
		{
			returnedConvergence_ = false;
			const float confidencePermission = clamp_unit((confidence - MinimumBiteConfidence) /
				(1.0f - MinimumBiteConfidence));
			const float maximumAllowance = m4cUnloading * MaximumEarlyRestorationFraction;
			restorationAllowance_ = move_toward(restorationAllowance_, maximumAllowance,
				RestorationBuildRatePerSecond * confidencePermission * dt);
			phase = restorationAllowance_ < maximumAllowance ? Phase::Restoring : Phase::Holding;
		}
		else if (holdBite)
		{
			returnedConvergence_ = false;
			phase = Phase::Holding;
		}
		else if (returned)
		{
			returnedConvergence_ = true;
			restorationAllowance_ = move_toward(restorationAllowance_, 0.0f,
				ReturnedConvergenceRatePerSecond * dt);
			phase = Phase::Returned;
		}
		else if (returnedConvergence_ && restorationAllowance_ > 0.0f)
		{
			restorationAllowance_ = move_toward(restorationAllowance_, 0.0f,
				(valid_delta(dt) ? ReturnedConvergenceRatePerSecond * dt : 0.0f));
			phase = Phase::Returned;
			if (restorationAllowance_ == 0.0f) returnedConvergence_ = false;
		}
		else if (restorationAllowance_ > 0.0f)
		{
			restorationAllowance_ = move_toward(restorationAllowance_, 0.0f,
				(valid_delta(dt) ? AbortReturnRatePerSecond * dt : 0.0f));
			phase = Phase::Aborting;
			abortActive = true;
		}

		restorationAllowance_ = (std::clamp)(finite_or_zero(restorationAllowance_),
			0.0f, MaximumM4CUnloading);
		const float allowedRestoration = (std::min)(restorationAllowance_,
			m4cUnloading * MaximumEarlyRestorationFraction);
		const float shadowUnloading = (std::clamp)(m4cUnloading - allowedRestoration,
			0.0f, MaximumM4CUnloading);
		const float shadowDirectional = legacyDirectional * (1.0f - shadowUnloading);
		const float restorationRate = valid_delta(dt)
			? (restorationAllowance_ - previousAllowance) / dt : 0.0f;

		current_ = {};
		current_.phase = phase;
		current_.active = allowedRestoration > 0.0f;
		current_.currentM4CUnloading = m4cUnloading;
		current_.shadowUnloading = shadowUnloading;
		current_.loadRestoration = allowedRestoration;
		current_.shadowDirectional = finite_or_zero(shadowDirectional);
		current_.restorationRate = finite_or_zero(restorationRate);
		current_.limiterActive = restorationAllowance_ > allowedRestoration ||
			(credibleBite && allowedRestoration >= m4cUnloading * MaximumEarlyRestorationFraction);
		current_.abortActive = abortActive;
		return current_;
	}

	void Model::reset() { *this = {}; }
	const Frame& evaluate(const Inputs& inputs) { return model.evaluate(inputs); }
	void reset() { model.reset(); }
	const Frame& frame() { return model.frame(); }

	const char* phase_name(Phase phase)
	{
		switch (phase)
		{
		case Phase::Restoring: return "restoring";
		case Phase::Holding: return "holding";
		case Phase::Aborting: return "aborting";
		case Phase::Returned: return "returned";
		default: return "baseline";
		}
	}
}
