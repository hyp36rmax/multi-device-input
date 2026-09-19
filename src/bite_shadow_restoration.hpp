#pragma once

#include "bite_state_detector.hpp"
#include "vehicle_state_interpreter.hpp"

namespace HYP36RBiteShadow
{
	enum class Phase { Baseline, Restoring, Holding, Aborting, Returned };

	struct Inputs
	{
		HYP36RBite::Frame bite{};
		HYP36RVehicleState::Validity nativeValidity = HYP36RVehicleState::Validity::Unavailable;
		float currentM4CUnloading = 0.0f;
		float legacyDirectional = 0.0f;
		float deltaSeconds = 0.0f;
	};

	struct Frame
	{
		Phase phase = Phase::Baseline;
		bool active = false;
		float currentM4CUnloading = 0.0f;
		float shadowUnloading = 0.0f;
		float loadRestoration = 0.0f;
		float shadowDirectional = 0.0f;
		float restorationRate = 0.0f;
		bool limiterActive = false;
		bool abortActive = false;
	};

	class Model
	{
	public:
		const Frame& evaluate(const Inputs& inputs);
		void reset();
		const Frame& frame() const { return current_; }

	private:
		float restorationAllowance_ = 0.0f;
		bool returnedConvergence_ = false;
		Frame current_{};
	};

	const Frame& evaluate(const Inputs& inputs);
	void reset();
	const Frame& frame();
	const char* phase_name(Phase phase);
}
