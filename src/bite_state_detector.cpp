#include "bite_state_detector.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace HYP36RBite
{
	namespace
	{
		// M4E passive research constants. None of these values feed force output.
		constexpr float MinimumDeltaSeconds = 1.0f / 240.0f;
		constexpr float MaximumDeltaSeconds = 0.1f;
		constexpr float MinimumErrorRad = 0.10f;
		constexpr float ReturnedErrorRad = 0.05f;
		constexpr float MinimumClosingRateRadPerSec = 0.08f;
		constexpr float MinimumVehicleConvergenceRadPerSec = 0.03f;
		constexpr float MinimumLateralSpeed = 0.15f;
		constexpr float MinimumSlipRatio = 0.08f;
		constexpr float MinimumGripLoss = 0.01f;
		constexpr unsigned CandidatePersistenceFrames = 8;
		constexpr unsigned ReturnedPersistenceFrames = 15;
		constexpr unsigned ReturnedHoldFrames = 30;
		constexpr unsigned InvalidDecayFrames = 8;

		Detector detector{};

		constexpr float clamp_unit(float value)
		{
			return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
		}

		constexpr float sign_of(float value)
		{
			return value > 0.0f ? 1.0f : (value < 0.0f ? -1.0f : 0.0f);
		}

		constexpr float vehicle_closing(float signedError, float responseRate)
		{
			return sign_of(signedError) * responseRate;
		}

		constexpr float driver_closing(float signedError, float referenceRate)
		{
			return -sign_of(signedError) * referenceRate;
		}

		constexpr float wrap_angle_delta(float delta)
		{
			constexpr float Pi = std::numbers::pi_v<float>;
			while (delta >= Pi) delta -= 2.0f * Pi;
			while (delta < -Pi) delta += 2.0f * Pi;
			return delta;
		}

		float finite_or_zero(float value)
		{
			return std::isfinite(value) ? value : 0.0f;
		}

		template <size_t Size>
		float average(const std::array<float, Size>& values, size_t count)
		{
			float total = 0.0f;
			for (size_t index = 0; index < count; ++index) total += values[index];
			return count > 0 ? total / static_cast<float>(count) : 0.0f;
		}

		constexpr bool valid_delta(float value)
		{
			return value >= MinimumDeltaSeconds && value <= MaximumDeltaSeconds;
		}

		static_assert(vehicle_closing(0.4f, 0.2f) == 0.2f);
		static_assert(vehicle_closing(-0.4f, -0.2f) == 0.2f);
		static_assert(vehicle_closing(0.4f, -0.2f) == -0.2f);
		static_assert(driver_closing(0.4f, -0.2f) == 0.2f);
		static_assert(driver_closing(-0.4f, 0.2f) == 0.2f);
		static_assert(clamp_unit(-1.0f) == 0.0f);
		static_assert(clamp_unit(2.0f) == 1.0f);
	}

	const Frame& Detector::evaluate(const Inputs& inputs)
	{
		const auto& semantic = inputs.vehicleState.current;
		const bool nativeValid = semantic.validity == HYP36RVehicleState::Validity::Valid &&
			semantic.referenceResponseErrorValid && semantic.responseRateValid &&
			std::isfinite(semantic.referenceResponseErrorRad) &&
			std::isfinite(semantic.steeringReferenceAngleRad) &&
			std::isfinite(semantic.responseAngularRateRadPerSec) &&
			valid_delta(inputs.deltaSeconds);
		if (!nativeValid)
		{
			candidateFrames_ = 0;
			returnedFrames_ = 0;
			invalidFrames_ = (std::min)(InvalidDecayFrames, invalidFrames_ + 1);
			if (current_.state == State::Candidate)
				current_ = {};
			else if (current_.state == State::Bite)
			{
				current_.confidence *= 1.0f - 1.0f / static_cast<float>(InvalidDecayFrames);
				current_.candidate = false;
			}
			else current_ = {};
			if (invalidFrames_ >= InvalidDecayFrames) *this = {};
			return current_;
		}

		invalidFrames_ = 0;
		const float dt = inputs.deltaSeconds;
		const float signedError = semantic.referenceResponseErrorRad;
		const float reference = semantic.steeringReferenceAngleRad;
		float referenceRate = 0.0f;
		if (havePreviousReference_)
			referenceRate = wrap_angle_delta(reference - previousReference_) / dt;
		previousReference_ = reference;
		havePreviousReference_ = true;

		errorHistory_[filterIndex_] = std::abs(signedError);
		referenceRateHistory_[filterIndex_] = finite_or_zero(referenceRate);
		responseRateHistory_[filterIndex_] = finite_or_zero(semantic.responseAngularRateRadPerSec);
		filterIndex_ = (filterIndex_ + 1) % FilterSamples;
		filterCount_ = (std::min)(FilterSamples, filterCount_ + 1);
		const float filteredError = average(errorHistory_, filterCount_);
		const float filteredReferenceRate = average(referenceRateHistory_, filterCount_);
		const float filteredResponseRate = average(responseRateHistory_, filterCount_);
		float errorClosingRate = 0.0f;
		if (havePreviousFilteredError_)
			errorClosingRate = (previousFilteredError_ - filteredError) / dt;
		previousFilteredError_ = filteredError;
		havePreviousFilteredError_ = true;

		const float vehicleConvergence = vehicle_closing(signedError, filteredResponseRate);
		const float driverConvergence = driver_closing(signedError, filteredReferenceRate);
		const float lateralSpeed = std::abs(finite_or_zero(inputs.lateralSpeed));
		const float slipRatio = clamp_unit(finite_or_zero(inputs.slipRatio));
		const float gripLoss = clamp_unit(finite_or_zero(inputs.gripLoss));
		const float dynamicContext = (std::max)({ clamp_unit(filteredError / 0.50f),
			clamp_unit(lateralSpeed / 0.50f), clamp_unit(slipRatio / 0.30f),
			clamp_unit(gripLoss / 0.50f) });
		const bool meaningfulDynamics = filteredError >= MinimumErrorRad &&
			(lateralSpeed >= MinimumLateralSpeed || slipRatio >= MinimumSlipRatio ||
				gripLoss >= MinimumGripLoss);
		const bool vehicleClosing = vehicleConvergence >= MinimumVehicleConvergenceRadPerSec;
		const bool qualifies = filterCount_ == FilterSamples && meaningfulDynamics &&
			errorClosingRate >= MinimumClosingRateRadPerSec && vehicleClosing &&
			vehicleConvergence > (std::max)(MinimumVehicleConvergenceRadPerSec, driverConvergence);

		const bool driverIsClosing = driverConvergence >= MinimumVehicleConvergenceRadPerSec;
		ConvergenceSource source = ConvergenceSource::None;
		if (vehicleClosing && driverIsClosing) source = ConvergenceSource::Both;
		else if (vehicleClosing) source = ConvergenceSource::Vehicle;
		else if (driverIsClosing) source = ConvergenceSource::Driver;

		if (current_.state == State::Inactive && qualifies)
		{
			current_.state = State::Candidate;
			candidateFrames_ = 1;
		}
		else if (current_.state == State::Candidate)
		{
			current_.ageSeconds += dt;
			if (!qualifies)
			{
				current_.state = State::Inactive;
				candidateFrames_ = 0;
			}
			else if (++candidateFrames_ >= CandidatePersistenceFrames)
			{
				current_.state = State::Bite;
				current_.ageSeconds = 0.0f;
			}
		}
		else if (current_.state == State::Bite)
		{
			current_.ageSeconds += dt;
			const bool returned = filteredError < ReturnedErrorRad &&
				lateralSpeed < MinimumLateralSpeed && slipRatio < MinimumSlipRatio &&
				gripLoss < MinimumGripLoss;
			returnedFrames_ = returned ? returnedFrames_ + 1 : 0;
			if (returnedFrames_ >= ReturnedPersistenceFrames)
			{
				current_.state = State::Returned;
				current_.ageSeconds = 0.0f;
				returnedHoldFrames_ = 0;
			}
		}
		else if (current_.state == State::Returned)
		{
			current_.ageSeconds += dt;
			if (++returnedHoldFrames_ >= ReturnedHoldFrames)
			{
				current_.state = State::Inactive;
				current_.ageSeconds = 0.0f;
			}
		}

		const float errorScore = clamp_unit((filteredError - ReturnedErrorRad) / 0.45f);
		const float closingScore = clamp_unit((errorClosingRate - MinimumClosingRateRadPerSec) / 0.42f);
		const float vehicleScore = clamp_unit((vehicleConvergence - MinimumVehicleConvergenceRadPerSec) / 0.47f);
		const float dominanceScore = clamp_unit((vehicleConvergence -
			(std::max)(0.0f, driverConvergence)) / 0.50f);
		const float persistenceScore = current_.state == State::Bite ? 1.0f :
			clamp_unit(static_cast<float>(candidateFrames_) / static_cast<float>(CandidatePersistenceFrames));
		float confidence = (errorScore + closingScore + vehicleScore + dominanceScore + dynamicContext) /
			5.0f * persistenceScore;
		if (current_.state == State::Inactive) confidence = 0.0f;
		else if (current_.state == State::Returned)
			confidence *= 1.0f - clamp_unit(static_cast<float>(returnedHoldFrames_) /
				static_cast<float>(ReturnedHoldFrames));

		current_.convergenceSource = source;
		current_.candidate = current_.state == State::Candidate;
		current_.active = current_.state == State::Bite;
		current_.confidence = clamp_unit(finite_or_zero(confidence));
		current_.errorMagnitude = finite_or_zero(filteredError);
		current_.errorClosingRate = finite_or_zero(errorClosingRate);
		current_.vehicleConvergence = finite_or_zero(vehicleConvergence);
		current_.driverConvergence = finite_or_zero(driverConvergence);
		current_.dynamicContext = clamp_unit(finite_or_zero(dynamicContext));
		return current_;
	}

	void Detector::reset() { *this = {}; }
	const Frame& evaluate(const Inputs& inputs) { return detector.evaluate(inputs); }
	void reset() { detector.reset(); }
	const Frame& frame() { return detector.frame(); }

	const char* state_name(State state)
	{
		switch (state)
		{
		case State::Candidate: return "candidate";
		case State::Bite: return "bite";
		case State::Returned: return "returned";
		default: return "none";
		}
	}

	const char* convergence_source_name(ConvergenceSource source)
	{
		switch (source)
		{
		case ConvergenceSource::Driver: return "driver";
		case ConvergenceSource::Vehicle: return "vehicle";
		case ConvergenceSource::Both: return "both";
		default: return "none";
		}
	}
}
