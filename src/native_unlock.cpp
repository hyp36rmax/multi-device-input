#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "native_unlock.hpp"
#include "plugin.hpp"
#include "game_addrs.hpp"
#include "save_recovery.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <string_view>

namespace NativeUnlock
{
	namespace
	{
		constexpr std::string_view SupportedExeSha256 =
			"68ceb386829066f8455b9d027320af962584321f3e2e8a79c72841495a6134c3";
		constexpr size_t PayloadSize = 0x40c;
		constexpr uintptr_t NativeRoutine = 0x447360 - 0x400000;
		constexpr uintptr_t NativeCaller = 0x4de544 - 0x400000;
		constexpr uintptr_t NativeSelectedSlot = 0x7b17f8 - 0x400000;
		constexpr uintptr_t NativeActiveLicence = 0x7c23e0 - 0x400000;
		constexpr uintptr_t NativeSaveReady = 0x7457a9 - 0x400000;
		constexpr size_t OccupiedFlag = 0x3f4;

		// Also validate the loaded image at the two relevant call sites. A matching
		// on-disk file alone is insufficient if an injected patch changed its code.
		constexpr std::array<std::uint8_t, 11> RoutineEntry{
			0x8b, 0xd1, 0x57, 0x83, 0xc8, 0xff, 0xb9, 0x25, 0x00, 0x00, 0x00
		};
		constexpr std::array<std::uint8_t, 10> CallerEntry{
			0xb9, 0xe0, 0x23, 0x7c, 0x00, 0xe8, 0x12, 0x8e, 0xf6, 0xff
		};

		bool allBytes(std::span<const std::uint8_t> licence, size_t start, size_t count, std::uint8_t value)
		{
			return std::all_of(licence.begin() + start, licence.begin() + start + count,
				[value](std::uint8_t byte) { return byte == value; });
		}
	}

	bool VerifyTransformation(std::span<const std::uint8_t> licence)
	{
		if (licence.size() < PayloadSize) return false;
		return allBytes(licence, 0x028, 150, 0xff) &&
			allBytes(licence, 0x125, 82, 0x06) &&
			allBytes(licence, 0x177, 82, 0x06) &&
			allBytes(licence, 0x1c9, 10, 0) &&
			allBytes(licence, 0x1d3, 10, 0) &&
			allBytes(licence, 0x1dd, 200, 0x06) &&
			allBytes(licence, 0x2a5, 200, 0x06) &&
			allBytes(licence, 0x36e, 132, 0x66);
	}

	Code CheckContext(bool inMenu, bool saveStateReady, int activeLicence, bool occupied)
	{
		if (activeLicence < 0 || activeLicence > 3) return Code::NoActiveLicence;
		if (!inMenu || !saveStateReady || !occupied) return Code::InvalidNativeContext;
		return Code::Success;
	}

	const char* CodeName(Code code)
	{
		switch (code)
		{
			case Code::Success: return "SUCCESS";
			case Code::UnsupportedExecutable: return "UNSUPPORTED_EXECUTABLE";
			case Code::NoActiveLicence: return "NO_ACTIVE_LICENCE";
			case Code::InvalidNativeContext: return "INVALID_NATIVE_CONTEXT";
			case Code::InvocationFailed: return "INVOCATION_FAILED";
			case Code::VerificationFailed: return "VERIFICATION_FAILED";
		}
		return "INVALID_NATIVE_CONTEXT";
	}

	Result UnlockAllContent()
	{
		Result result;
		if (sizeof(void*) != 4 || !Module::ExeHandle || !Game::current_mode ||
			GetModuleHandleW(nullptr) != Module::ExeHandle)
		{
			result.code = Code::UnsupportedExecutable;
			return result;
		}
		try
		{
			if (SaveRecovery::FileSha256(Module::ExePath) != SupportedExeSha256)
			{
				result.code = Code::UnsupportedExecutable;
				return result;
			}
		}
		catch (...)
		{
			result.code = Code::UnsupportedExecutable;
			return result;
		}
		if (std::memcmp(Module::exe_ptr(NativeRoutine), RoutineEntry.data(), RoutineEntry.size()) ||
			std::memcmp(Module::exe_ptr(NativeCaller), CallerEntry.data(), CallerEntry.size()))
		{
			result.code = Code::UnsupportedExecutable;
			return result;
		}
		result.supportedExecutable = true;

		result.activeLicence = *Module::exe_ptr<int>(NativeSelectedSlot);
		auto* licence = Module::exe_ptr(NativeActiveLicence);
		result.code = CheckContext(*Game::current_mode == GameState::STATE_MENU,
			*Module::exe_ptr<std::uint8_t>(NativeSaveReady) != 0,
			result.activeLicence, (licence[OccupiedFlag] & 1) != 0);
		if (result.code != Code::Success) return result;

		// 0x4DE544 loads ECX=0x7C23E0 immediately before the native CALL. The
		// callee uses only ECX as its payload pointer, saves/restores EDI, has no
		// stack arguments and returns with RET. Keep this on the game render thread.
		using Transform = void(__thiscall*)(void*);
		const auto transform = Module::fn_ptr<Transform>(NativeRoutine);
		if (!transform)
		{
			result.code = Code::InvocationFailed;
			return result;
		}
		transform(licence);
		result.invoked = true;
		result.verified = VerifyTransformation({ licence, PayloadSize });
		result.code = result.verified ? Code::Success : Code::VerificationFailed;
		return result;
	}
}
