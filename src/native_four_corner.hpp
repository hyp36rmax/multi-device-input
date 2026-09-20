#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace NativeFourCorner
{
	constexpr size_t CornerCount = 4;

	struct CornerBlock
	{
		uint8_t opaque_00[0x14];
		uint32_t surfaceFlags_14;
		uint8_t opaque_18[0x10];
		float displacementCandidate_28;
		uint8_t opaque_2C[0x80];
		float directionalCandidate_AC;
		float directionalCandidate_B0;
		uint8_t opaque_B4[0x34];
		float surfaceResponseCandidate_E8;
		int16_t orientationAngle_EC;
		int16_t orientationDelta_EE;
		uint8_t opaque_F0[0x4];
	};

	struct PhysicsContext
	{
		uint8_t opaque_000[0x248];
		std::array<uint32_t, CornerCount> cornerPointers_248;
		std::array<CornerBlock, CornerCount> corners_258;
		uint8_t opaque_628[0x2D8];
	};

	struct Frame
	{
		bool available = false;
		std::array<float, CornerCount> displacementCandidate{};
		std::array<float, CornerCount> directionalCandidateAC{};
		std::array<float, CornerCount> directionalCandidateB0{};
	};

	static_assert(sizeof(void*) == 4, "Native physics pointers are Win32 values");
	static_assert(sizeof(CornerBlock) == 0xF4);
	static_assert(offsetof(CornerBlock, surfaceFlags_14) == 0x14);
	static_assert(offsetof(CornerBlock, displacementCandidate_28) == 0x28);
	static_assert(offsetof(CornerBlock, directionalCandidate_AC) == 0xAC);
	static_assert(offsetof(CornerBlock, directionalCandidate_B0) == 0xB0);
	static_assert(offsetof(CornerBlock, surfaceResponseCandidate_E8) == 0xE8);
	static_assert(offsetof(CornerBlock, orientationAngle_EC) == 0xEC);
	static_assert(offsetof(CornerBlock, orientationDelta_EE) == 0xEE);
	static_assert(offsetof(PhysicsContext, cornerPointers_248) == 0x248);
	static_assert(offsetof(PhysicsContext, corners_258) == 0x258);
	static_assert(sizeof(PhysicsContext) == 0x900);

	// Reads the native context produced by OutRun's preceding player-car physics
	// update. The result is observation-only and is never consumed by FFB.
	Frame observe();
}
