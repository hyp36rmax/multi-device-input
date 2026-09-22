#include "native_unlock.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>

int main()
{
	using namespace NativeUnlock;
	const auto check = [](bool condition, const char* label) {
		if (!condition) std::fprintf(stderr, "native unlock test failed: %s\n", label);
		return condition;
	};
	if (!check(CheckContext(true, true, -1, true) == Code::NoActiveLicence, "negative slot") ||
		!check(CheckContext(true, true, 4, true) == Code::NoActiveLicence, "slot out of range") ||
		!check(CheckContext(false, true, 0, true) == Code::InvalidNativeContext, "outside menu") ||
		!check(CheckContext(true, false, 0, true) == Code::InvalidNativeContext, "save not loaded") ||
		!check(CheckContext(true, true, 0, false) == Code::InvalidNativeContext, "unoccupied slot") ||
		!check(CheckContext(true, true, 3, true) == Code::Success, "valid native context"))
		return 1;

	// Mirror the fixed byte writes from the supported EXE for a verification-only
	// fixture. This is not an offline invocation of the game routine.
	std::array<std::uint8_t, 0x40c> licence{};
	if (!check(!VerifyTransformation(licence), "unchanged licence")) return 1;
	const auto fill = [&](size_t offset, size_t count, std::uint8_t value) {
		std::fill_n(licence.begin() + offset, count, value);
	};
	fill(0x028, 150, 0xff);
	fill(0x125, 82, 0x06);
	fill(0x177, 82, 0x06);
	fill(0x1dd, 200, 0x06);
	fill(0x2a5, 200, 0x06);
	fill(0x36e, 132, 0x66);
	if (!check(VerifyTransformation(licence), "native write ranges")) return 1;
	licence[0x36e] = 0;
	if (!check(!VerifyTransformation(licence), "corrupted native range")) return 1;
	licence[0x36e] = 0x66;
	if (!check(VerifyTransformation(licence), "repeat verification") ||
		!check(!VerifyTransformation({ licence.data(), licence.size() - 1 }), "short payload"))
		return 1;

	// No loaded game module in this process: fail closed before any hard-coded
	// address is dereferenced. The live EXE call is deliberately not simulated.
	const auto result = UnlockAllContent();
	if (!check(result.code == Code::UnsupportedExecutable && !result.invoked, "unsupported process"))
		return 1;
	std::puts("native unlock offline gates and verifier passed");
}
