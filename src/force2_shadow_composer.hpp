#pragma once

#include "vehicle_state_interpreter.hpp"

namespace HYP36RForce2
{
	enum class ComposerMode
	{
		Legacy,
		Shadow,
	};

	enum class NativeAvailability
	{
		Valid,
		TransitionSuppressed,
		Unavailable,
	};

	enum class EventPhase
	{
		Normal,
		Emerging,
		Established,
		Recovering,
	};

	struct ForceIntent
	{
		float directionalLoad = 0.0f;
		float unloading = 0.0f;
		float motion = 0.0f;
		float roadTexture = 0.0f;
		float impact = 0.0f;
	};

	struct ComposerContext
	{
		NativeAvailability nativeAvailability = NativeAvailability::Unavailable;
		float nativeWeight = 0.0f;
		EventPhase eventPhase = EventPhase::Normal;
		bool recovering = false;
	};

	struct Inputs
	{
		float legacyDirectional = 0.0f;
		float roadTexture = 0.0f;
		float impact = 0.0f;
		float outputRamp = 0.0f;
		float syntheticLateralSpeed = 0.0f;
		float syntheticSlipRatio = 0.0f;
		float syntheticGripLoss = 0.0f;
		float masterStrength = 0.0f;
		bool invertOutput = false;
	};

	struct Frame
	{
		ComposerMode mode = ComposerMode::Shadow;
		ComposerContext context{};
		ForceIntent intent{};
		float responseAngleDiagnostic = 0.0f;
		float divergenceDiagnostic = 0.0f;
		float responseRateDiagnostic = 0.0f;
		float legacyDirectionalComponent = 0.0f;
		float shadowDirectional = 0.0f;
		float shadowTexture = 0.0f;
		float shadowImpact = 0.0f;
		float shadowPreBudget = 0.0f;
		float shadowPostBudget = 0.0f;
		float shadowPreMaster = 0.0f;
		float shadowOutput = 0.0f;
		bool rateLimitActive = false;
		bool headroomLimitActive = false;
	};

	// Passive research composer. The returned frame is telemetry-only and has no
	// connection to WheelForceFeedback::drive or DirectInput.
	const Frame& evaluate(const Inputs& inputs, const HYP36RVehicleState::Frame& vehicleState);
	void reset();
	const Frame& frame();

	const char* mode_name(ComposerMode mode);
	const char* availability_name(NativeAvailability availability);
	const char* event_phase_name(EventPhase phase);
}
