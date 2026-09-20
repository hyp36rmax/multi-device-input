#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "native_four_corner.hpp"

namespace HYP36RFourCorner
{
	constexpr size_t FrontLeft = 0;
	constexpr size_t FrontRight = 1;
	constexpr size_t RearLeft = 2;
	constexpr size_t RearRight = 3;

	struct CornerContext
	{
		float displacement = 0.0f;
		float lateralResponse = 0.0f;
		float lateralResponseNormalized = 0.0f;
		float longitudinalResponse = 0.0f;
		float longitudinalResponseNormalized = 0.0f;
		float combinedResponse = 0.0f;
		float combinedResponseNormalized = 0.0f;
		uint32_t surfaceClassification = 0;
	};

	struct AggregateContext
	{
		float displacement = 0.0f;
		float lateralResponse = 0.0f;
		float longitudinalResponse = 0.0f;
		float combinedResponse = 0.0f;
	};

	struct SurfaceContext
	{
		bool allSame = true;
		bool frontAsymmetry = false;
		bool rearAsymmetry = false;
		bool leftAsymmetry = false;
		bool rightAsymmetry = false;
		bool anyAsymmetry = false;
	};

	struct Frame
	{
		bool available = false;
		CornerContext fl{};
		CornerContext fr{};
		CornerContext rl{};
		CornerContext rr{};
		AggregateContext front{};
		AggregateContext rear{};
		AggregateContext left{};
		AggregateContext right{};
		AggregateContext vehicle{};
		float frontRearDisplacementBias = 0.0f;
		float leftRightDisplacementBias = 0.0f;
		float frontRearLateralBias = 0.0f;
		float frontRearLongitudinalBias = 0.0f;
		SurfaceContext surface{};
	};

	class Interpreter
	{
	public:
		const Frame& evaluate(const NativeFourCorner::Frame& native,
			const std::array<uint32_t, NativeFourCorner::CornerCount>& surfaces);
		void reset();
		const Frame& frame() const { return current_; }

	private:
		std::array<float, NativeFourCorner::CornerCount> previousLateral_{};
		bool havePreviousLateral_ = false;
		Frame current_{};
	};

	const Frame& evaluate(const NativeFourCorner::Frame& native,
		const std::array<uint32_t, NativeFourCorner::CornerCount>& surfaces);
	void reset();
	const Frame& frame();
}
