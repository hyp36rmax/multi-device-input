#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "native_four_corner.hpp"

#include <cmath>
#include <cstdint>

#include "plugin.hpp"

namespace NativeFourCorner
{
	namespace
	{
		constexpr uintptr_t PhysicsContextOffset = 0x42E7F0;
	}

	Frame observe()
	{
		Frame frame{};
		const auto* context = Module::exe_ptr<PhysicsContext>(PhysicsContextOffset);
		if (!context)
			return frame;

		// The game initializes these pointers to the four inline 0xF4 blocks.
		// Requiring exact matches prevents telemetry from following an invalid or
		// partially initialized arbitrary pointer.
		for (size_t i = 0; i < CornerCount; ++i)
		{
			const auto expected = static_cast<uint32_t>(
				reinterpret_cast<uintptr_t>(&context->corners_258[i]));
			if (context->cornerPointers_248[i] != expected)
				return frame;
		}

		for (size_t i = 0; i < CornerCount; ++i)
		{
			const auto& corner = context->corners_258[i];
			if (!std::isfinite(corner.displacementCandidate_28) ||
				!std::isfinite(corner.directionalCandidate_AC) ||
				!std::isfinite(corner.directionalCandidate_B0))
			{
				return frame;
			}

			frame.displacementCandidate[i] = corner.displacementCandidate_28;
			frame.directionalCandidateAC[i] = corner.directionalCandidate_AC;
			frame.directionalCandidateB0[i] = corner.directionalCandidate_B0;
		}

		frame.available = true;
		return frame;
	}
}
