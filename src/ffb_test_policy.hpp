#pragma once

#include <cstdint>

namespace FFBTestPolicy
{
	// DirectInput nominal force is 10,000. Direction tests use a fixed,
	// short-lived 20% request regardless of the live driving Strength setting.
	constexpr long DirectionTestMagnitude = 2000;
	constexpr long ShakeTestMagnitude = DirectionTestMagnitude;
	constexpr int ShakeHalfPeriodMilliseconds = 50;
	constexpr int ShakeDurationMilliseconds = 1200;
	constexpr int ShakeEffectWatchdogMilliseconds = 250;
	constexpr int ShakeMaximumRuntimeMilliseconds = 1500;

	enum class QuickSetupShakeState
	{
		Ready,
		Running,
		AwaitingFeedback,
		Failed,
		Verified,
		Skipped,
	};

	enum class PhysicalResponse
	{
		Untested,
		Yes,
		No,
		Unsure,
		Skipped,
	};

	constexpr float shake_direction(std::int64_t elapsedMilliseconds)
	{
		return ((elapsedMilliseconds / ShakeHalfPeriodMilliseconds) & 1) == 0 ? 1.0f : -1.0f;
	}

	constexpr bool shake_finished(std::int64_t elapsedMilliseconds)
	{
		return elapsedMilliseconds >= ShakeDurationMilliseconds;
	}

	constexpr bool shake_must_abort(bool focused, bool endpointReady, bool ffbEnabled,
		std::int64_t elapsedMilliseconds)
	{
		return !focused || !endpointReady || !ffbEnabled || elapsedMilliseconds >= ShakeMaximumRuntimeMilliseconds;
	}

	enum class ShakeRuntimeResult
	{
		Idle,
		Running,
		Completed,
		Failed,
		Interrupted,
	};

	struct ShakeRuntimeLifecycle
	{
		ShakeRuntimeResult result = ShakeRuntimeResult::Idle;
		std::int32_t lastApiResult = 0;

		void reset() { result = ShakeRuntimeResult::Idle; lastApiResult = 0; }
		void started() { result = ShakeRuntimeResult::Running; lastApiResult = 0; }
		void completed() { result = ShakeRuntimeResult::Completed; }
		void failed(std::int32_t apiResult) { result = ShakeRuntimeResult::Failed; lastApiResult = apiResult; }
		void interrupted() { if (result == ShakeRuntimeResult::Running) result = ShakeRuntimeResult::Interrupted; }
		bool active() const { return result == ShakeRuntimeResult::Running; }
	};

	struct QuickSetupShakeVerification
	{
		QuickSetupShakeState state = QuickSetupShakeState::Ready;
		PhysicalResponse response = PhysicalResponse::Untested;

		bool start(bool endpointReady)
		{
			if (!endpointReady || state == QuickSetupShakeState::Running)
				return false;
			state = QuickSetupShakeState::Running;
			response = PhysicalResponse::Untested;
			return true;
		}

		void test_stopped()
		{
			if (state == QuickSetupShakeState::Running)
				state = QuickSetupShakeState::AwaitingFeedback;
		}

		void fail()
		{
			if (state == QuickSetupShakeState::Running)
				state = QuickSetupShakeState::Failed;
		}

		void confirm(PhysicalResponse value)
		{
			if (state != QuickSetupShakeState::AwaitingFeedback)
				return;
			response = value;
			if (value == PhysicalResponse::Yes)
				state = QuickSetupShakeState::Verified;
		}

		void skip()
		{
			state = QuickSetupShakeState::Skipped;
			response = PhysicalResponse::Skipped;
		}

		void reset()
		{
			state = QuickSetupShakeState::Ready;
			response = PhysicalResponse::Untested;
		}

		bool may_complete() const
		{
			return state == QuickSetupShakeState::Verified || state == QuickSetupShakeState::Skipped;
		}
	};

	constexpr bool reverse_direction(float direction, bool invert)
	{
		return (direction < 0.0f) != invert;
	}
}
