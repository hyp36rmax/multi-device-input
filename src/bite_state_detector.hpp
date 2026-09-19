#pragma once

#include <array>
#include <cstddef>

#include "vehicle_state_interpreter.hpp"

namespace HYP36RBite
{
	enum class State { Inactive, Candidate, Bite, Returned };
	enum class ConvergenceSource { None, Driver, Vehicle, Both };

	struct Inputs
	{
		HYP36RVehicleState::Frame vehicleState{};
		float lateralSpeed = 0.0f;
		float slipRatio = 0.0f;
		float gripLoss = 0.0f;
		float deltaSeconds = 0.0f;
	};

	struct Frame
	{
		State state = State::Inactive;
		ConvergenceSource convergenceSource = ConvergenceSource::None;
		bool candidate = false;
		bool active = false;
		float confidence = 0.0f;
		float errorMagnitude = 0.0f;
		float errorClosingRate = 0.0f;
		float vehicleConvergence = 0.0f;
		float driverConvergence = 0.0f;
		float ageSeconds = 0.0f;
		float dynamicContext = 0.0f;
	};

	class Detector
	{
	public:
		const Frame& evaluate(const Inputs& inputs);
		void reset();
		const Frame& frame() const { return current_; }

	private:
		static constexpr size_t FilterSamples = 9;
		std::array<float, FilterSamples> errorHistory_{};
		std::array<float, FilterSamples> referenceRateHistory_{};
		std::array<float, FilterSamples> responseRateHistory_{};
		size_t filterIndex_ = 0;
		size_t filterCount_ = 0;
		float previousReference_ = 0.0f;
		float previousFilteredError_ = 0.0f;
		bool havePreviousReference_ = false;
		bool havePreviousFilteredError_ = false;
		unsigned candidateFrames_ = 0;
		unsigned returnedFrames_ = 0;
		unsigned returnedHoldFrames_ = 0;
		unsigned invalidFrames_ = 0;
		Frame current_{};
	};

	const Frame& evaluate(const Inputs& inputs);
	void reset();
	const Frame& frame();
	const char* state_name(State state);
	const char* convergence_source_name(ConvergenceSource source);
}
