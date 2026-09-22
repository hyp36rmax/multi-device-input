#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace NativeUnlock
{
	enum class Code
	{
		Success,
		UnsupportedExecutable,
		NoActiveLicence,
		InvalidNativeContext,
		InvocationFailed,
		VerificationFailed
	};

	struct Result
	{
		Code code = Code::InvalidNativeContext;
		int activeLicence = -1; // zero-based native slot, never changed by this service
		bool supportedExecutable = false;
		bool invoked = false;
		bool verified = false;
	};

	// The native routine writes fixed ranges of the 0x40C-byte active payload.
	// This verifies those writes, not the game's derived unlock caches or a save.
	bool VerifyTransformation(std::span<const std::uint8_t> licence);
	Code CheckContext(bool inMenu, bool saveStateReady, int activeLicence, bool occupied);
	const char* CodeName(Code code);
	Result UnlockAllContent();
}
