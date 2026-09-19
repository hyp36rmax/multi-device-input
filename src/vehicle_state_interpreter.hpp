#pragma once

#include <cstdint>

struct tagEVWORK_CAR;

namespace HYP36RVehicleState
{
	enum class Validity : uint8_t
	{
		Unavailable,
		Valid,
		TransitionSuppressed,
	};

	struct NativeSnapshot
	{
		int16_t referenceRaw = 0;
		int16_t responseRaw = 0;
		int16_t responseIncrementRaw = 0;
		int16_t correctionRaw = 0;
		float responseAuthorityRaw = 0.0f;
		float overshootAttenuationRaw = 0.0f;
		float physicsDirectionRaw = 0.0f;
		float responseIncrementLimitRaw = 0.0f;
		uint8_t transitionCounterRaw = 0;
	};

	struct State
	{
		Validity validity = Validity::Unavailable;
		float steeringReferenceAngleRad = 0.0f;
		float responseAngleRad = 0.0f;
		float responseAngularRateRadPerSec = 0.0f;
		float referenceResponseErrorRad = 0.0f;
		float correctedReferenceAngleRad = 0.0f;
		float responseAuthority = 0.0f;
		float overshootAttenuation = 0.0f;
		bool responseAngleValid = false;
		bool responseRateValid = false;
		bool referenceResponseErrorValid = false;
		bool correctedReferenceValid = false;
		uint8_t transitionFramesRemaining = 0;
		float lastValidStateAgeSeconds = 0.0f;
	};

	struct Frame
	{
		State current{};
		State lastValidDynamicState{};
		NativeSnapshot diagnostic{};
		float responseRateUtilization = 0.0f;
		bool responseRateUtilizationValid = false;
	};

	// Observes native game state only. Nothing in the force path consumes the
	// returned semantic state.
	void observe(const tagEVWORK_CAR* car);
	void reset();
	const Frame& frame();
	const char* validity_name(Validity validity);
}
