#include "four_corner_context.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace HYP36RFourCorner
{
	namespace
	{
		// M5B research normalization. The front/rear scales are separate because
		// the native rear channels have a larger normal operating range. These
		// bounds are context scales, not physical units or grip percentages.
		constexpr float FrontLateralScale = 1400.0f;
		constexpr float RearLateralScale = 2000.0f;
		constexpr float FrontLongitudinalScale = 2500.0f;
		constexpr float RearLongitudinalScale = 2200.0f;

		float normalize_signed(float value, float scale)
		{
			return (std::clamp)(value / scale, -1.0f, 1.0f);
		}

		float average(float left, float right)
		{
			return (left + right) * 0.5f;
		}

		AggregateContext aggregate(const CornerContext& first, const CornerContext& second)
		{
			return {
				average(first.displacement, second.displacement),
				average(first.lateralResponseNormalized, second.lateralResponseNormalized),
				average(first.longitudinalResponseNormalized, second.longitudinalResponseNormalized),
				average(first.combinedResponse, second.combinedResponse)
			};
		}

		Interpreter sharedInterpreter{};
	}

	const Frame& Interpreter::evaluate(const NativeFourCorner::Frame& native,
		const std::array<uint32_t, NativeFourCorner::CornerCount>& surfaces)
	{
		if (!native.available)
		{
			reset();
			return current_;
		}

		Frame next{};
		std::array<CornerContext*, NativeFourCorner::CornerCount> corners{
			&next.fl, &next.fr, &next.rl, &next.rr
		};

		for (size_t i = 0; i < NativeFourCorner::CornerCount; ++i)
		{
			const float rawLateral = native.directionalCandidateAC[i];
			const float conditionedLateral = havePreviousLateral_
				? average(rawLateral, previousLateral_[i])
				: 0.0f;
			const float lateralScale = i < RearLeft ? FrontLateralScale : RearLateralScale;
			const float longitudinalScale = i < RearLeft
				? FrontLongitudinalScale : RearLongitudinalScale;

			auto& corner = *corners[i];
			corner.displacement = native.displacementCandidate[i];
			corner.lateralResponse = conditionedLateral;
			corner.lateralResponseNormalized = normalize_signed(conditionedLateral, lateralScale);
			corner.longitudinalResponse = native.directionalCandidateB0[i];
			corner.longitudinalResponseNormalized = normalize_signed(
				corner.longitudinalResponse, longitudinalScale);
			// This preserves the exact native relationship identified in M5A/M5C.
			corner.combinedResponse = std::hypot(rawLateral, corner.longitudinalResponse);
			corner.combinedResponseNormalized = (std::min)(1.0f, std::hypot(
				corner.lateralResponseNormalized, corner.longitudinalResponseNormalized));
			corner.surfaceClassification = surfaces[i];
		}

		previousLateral_ = native.directionalCandidateAC;
		havePreviousLateral_ = true;

		next.front = aggregate(next.fl, next.fr);
		next.rear = aggregate(next.rl, next.rr);
		next.left = aggregate(next.fl, next.rl);
		next.right = aggregate(next.fr, next.rr);
		next.vehicle = {
			average(next.front.displacement, next.rear.displacement),
			average(next.front.lateralResponse, next.rear.lateralResponse),
			average(next.front.longitudinalResponse, next.rear.longitudinalResponse),
			average(next.front.combinedResponse, next.rear.combinedResponse)
		};
		next.frontRearDisplacementBias = next.front.displacement - next.rear.displacement;
		next.leftRightDisplacementBias = next.left.displacement - next.right.displacement;
		next.frontRearLateralBias = (next.front.lateralResponse - next.rear.lateralResponse) * 0.5f;
		next.frontRearLongitudinalBias =
			(next.front.longitudinalResponse - next.rear.longitudinalResponse) * 0.5f;

		next.surface.frontAsymmetry = surfaces[FrontLeft] != surfaces[FrontRight];
		next.surface.rearAsymmetry = surfaces[RearLeft] != surfaces[RearRight];
		next.surface.leftAsymmetry = surfaces[FrontLeft] != surfaces[RearLeft];
		next.surface.rightAsymmetry = surfaces[FrontRight] != surfaces[RearRight];
		next.surface.allSame = surfaces[FrontLeft] == surfaces[FrontRight] &&
			surfaces[FrontLeft] == surfaces[RearLeft] &&
			surfaces[FrontLeft] == surfaces[RearRight];
		next.surface.anyAsymmetry = !next.surface.allSame;
		next.available = true;
		current_ = next;
		return current_;
	}

	void Interpreter::reset()
	{
		previousLateral_ = {};
		havePreviousLateral_ = false;
		current_ = {};
	}

	const Frame& evaluate(const NativeFourCorner::Frame& native,
		const std::array<uint32_t, NativeFourCorner::CornerCount>& surfaces)
	{
		return sharedInterpreter.evaluate(native, surfaces);
	}

	void reset()
	{
		sharedInterpreter.reset();
	}

	const Frame& frame()
	{
		return sharedInterpreter.frame();
	}

	static_assert(FrontLeft == 0 && FrontRight == 1 && RearLeft == 2 && RearRight == 3);
}
