#include "vehicle_state_interpreter.hpp"

#include <bit>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numbers>

#include "game.hpp"

namespace HYP36RVehicleState
{
	namespace
	{
		constexpr float Pi = std::numbers::pi_v<float>;
		constexpr float NativeAngleScale = Pi / 32768.0f;
		constexpr float MinimumDeltaSeconds = 1.0f / 240.0f;
		constexpr float MaximumDeltaSeconds = 0.1f;

		Frame currentFrame{};
		std::chrono::steady_clock::time_point previousSample{};
		bool havePreviousSample = false;
		bool haveLastValidState = false;

		constexpr float native_angle_to_radians(int16_t value)
		{
			return static_cast<float>(value) * NativeAngleScale;
		}

		constexpr float wrap_signed_angle(float angle)
		{
			while (angle >= Pi)
				angle -= 2.0f * Pi;
			while (angle < -Pi)
				angle += 2.0f * Pi;
			return angle;
		}

		constexpr int16_t add_native_angles(int16_t left, int16_t right)
		{
			const uint16_t wrapped = static_cast<uint16_t>(left) + static_cast<uint16_t>(right);
			return std::bit_cast<int16_t>(wrapped);
		}

		constexpr bool valid_delta(float deltaSeconds)
		{
			return deltaSeconds >= MinimumDeltaSeconds && deltaSeconds <= MaximumDeltaSeconds;
		}

		constexpr Validity validity_for(bool semanticFloatsValid, uint8_t transitionCounter)
		{
			if (!semanticFloatsValid)
				return Validity::Unavailable;
			return transitionCounter > 0 ? Validity::TransitionSuppressed : Validity::Valid;
		}

		constexpr float response_rate(int16_t increment, float deltaSeconds)
		{
			return native_angle_to_radians(increment) / deltaSeconds;
		}

		constexpr float clamp_unit(float value)
		{
			return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
		}

		float clamp_finite_unit(float value, bool& valid)
		{
			if (!std::isfinite(value))
			{
				valid = false;
				return 0.0f;
			}
			return clamp_unit(value);
		}

		NativeSnapshot read_native_snapshot(const EVWORK_CAR* car)
		{
			NativeSnapshot snapshot{};
			snapshot.referenceRaw = car->field_32;
			snapshot.responseRaw = car->candidate_D46;
			snapshot.responseIncrementRaw = car->candidate_D48;
			snapshot.correctionRaw = car->candidate_D44;
			snapshot.responseAuthorityRaw = car->candidate_D38;
			snapshot.overshootAttenuationRaw = car->candidate_D3C;
			snapshot.physicsDirectionRaw = car->candidate_D40;
			snapshot.transitionCounterRaw = car->field_283;

			if (car->ptr_2B4 != 0)
			{
				const auto parameters = reinterpret_cast<const std::byte*>(
					static_cast<uintptr_t>(static_cast<uint32_t>(car->ptr_2B4)));
				snapshot.responseIncrementLimitRaw =
					*reinterpret_cast<const float*>(parameters + 0x205C);
			}
			return snapshot;
		}

		static_assert(native_angle_to_radians(0x0000) == 0.0f);
		static_assert(native_angle_to_radians(0x4000) > 1.5707f &&
			native_angle_to_radians(0x4000) < 1.5709f);
		static_assert(native_angle_to_radians(-0x4000) < -1.5707f &&
			native_angle_to_radians(-0x4000) > -1.5709f);
		static_assert(wrap_signed_angle(Pi) == -Pi);
		static_assert(add_native_angles(0x7FFF, 1) == std::numeric_limits<int16_t>::min());
		static_assert(valid_delta(1.0f / 60.0f));
		static_assert(!valid_delta(0.0f));
		static_assert(!valid_delta(1.0f));
		static_assert(validity_for(true, 0) == Validity::Valid);
		static_assert(validity_for(true, 1) == Validity::TransitionSuppressed);
		static_assert(validity_for(false, 0) == Validity::Unavailable);
		static_assert(response_rate(512, 1.0f / 60.0f) > 2.94f &&
			response_rate(512, 1.0f / 60.0f) < 2.95f);
		static_assert(clamp_unit(-1.0f) == 0.0f);
		static_assert(clamp_unit(0.5f) == 0.5f);
		static_assert(clamp_unit(2.0f) == 1.0f);

		static_assert(offsetof(EVWORK_CAR, field_32) == 0x32);
		static_assert(offsetof(EVWORK_CAR, field_283) == 0x283);
		static_assert(offsetof(EVWORK_CAR, ptr_2B4) == 0x2B4);
	}

	void observe(const EVWORK_CAR* car)
	{
		const auto now = std::chrono::steady_clock::now();
		float deltaSeconds = 0.0f;
		if (havePreviousSample)
			deltaSeconds = std::chrono::duration<float>(now - previousSample).count();
		previousSample = now;
		havePreviousSample = true;

		if (!car)
		{
			currentFrame.current = {};
			currentFrame.current.validity = Validity::Unavailable;
			return;
		}

		const NativeSnapshot native = read_native_snapshot(car);
		State state{};
		bool semanticFloatsValid = true;
		state.steeringReferenceAngleRad = native_angle_to_radians(native.referenceRaw);
		state.responseAngleRad = native_angle_to_radians(native.responseRaw);
		state.referenceResponseErrorRad = wrap_signed_angle(
			state.steeringReferenceAngleRad - state.responseAngleRad);
		state.correctedReferenceAngleRad = wrap_signed_angle(native_angle_to_radians(
			add_native_angles(native.referenceRaw, native.correctionRaw)));
		state.responseAuthority = clamp_finite_unit(native.responseAuthorityRaw, semanticFloatsValid);
		state.overshootAttenuation = clamp_finite_unit(
			native.overshootAttenuationRaw, semanticFloatsValid);
		state.transitionFramesRemaining = native.transitionCounterRaw;

		state.responseRateValid = valid_delta(deltaSeconds);
		if (state.responseRateValid)
		{
			state.responseAngularRateRadPerSec = response_rate(
				native.responseIncrementRaw, deltaSeconds);
			state.responseRateValid = std::isfinite(state.responseAngularRateRadPerSec);
		}

		state.validity = validity_for(semanticFloatsValid, native.transitionCounterRaw);

		const bool dynamicStateValid = state.validity == Validity::Valid;
		state.responseAngleValid = dynamicStateValid;
		state.referenceResponseErrorValid = dynamicStateValid;
		state.correctedReferenceValid = dynamicStateValid;

		currentFrame.diagnostic = native;
		currentFrame.current = state;
		currentFrame.responseRateUtilization = 0.0f;
		currentFrame.responseRateUtilizationValid =
			std::isfinite(native.responseIncrementLimitRaw) &&
			native.responseIncrementLimitRaw > 0.0f;
		if (currentFrame.responseRateUtilizationValid)
		{
			currentFrame.responseRateUtilization = static_cast<float>(native.responseIncrementRaw) /
				native.responseIncrementLimitRaw;
		}

		if (dynamicStateValid)
		{
			currentFrame.lastValidDynamicState = state;
			currentFrame.lastValidDynamicState.lastValidStateAgeSeconds = 0.0f;
			haveLastValidState = true;
		}
		else if (haveLastValidState && valid_delta(deltaSeconds))
		{
			currentFrame.lastValidDynamicState.lastValidStateAgeSeconds += deltaSeconds;
		}
		currentFrame.current.lastValidStateAgeSeconds = haveLastValidState
			? currentFrame.lastValidDynamicState.lastValidStateAgeSeconds : 0.0f;
	}

	void reset()
	{
		currentFrame = {};
		previousSample = {};
		havePreviousSample = false;
		haveLastValidState = false;
	}

	const Frame& frame()
	{
		return currentFrame;
	}

	const char* validity_name(Validity validity)
	{
		switch (validity)
		{
		case Validity::Valid:
			return "valid";
		case Validity::TransitionSuppressed:
			return "transition_suppressed";
		default:
			return "unavailable";
		}
	}
}
