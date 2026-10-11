#pragma once

#include <algorithm>
#include <array>
#include <string_view>

namespace HYP36RInputBackend
{
	enum class Backend { Wgi = 0, RawInput = 1, DirectInput = 2, XInput = 3 };
	enum class Override { Automatic, Wgi, DirectInput, RawInput, XInput, Invalid };

	inline Override parse(std::string_view value) noexcept
	{
		std::array<char, 32> normalized{};
		const size_t length = (std::min)(value.size(), normalized.size() - 1);
		for (size_t i = 0; i < length; ++i)
		{
			const char c = value[i];
			normalized[i] = c >= 'a' && c <= 'z' ? char(c - ('a' - 'A')) : c;
		}
		const std::string_view result(normalized.data(), length);
		if (result.empty() || result == "AUTOMATIC" || result == "AUTO") return Override::Automatic;
		if (result == "WGI" || result == "WINDOWS.GAMING.INPUT") return Override::Wgi;
		if (result == "DIRECTINPUT" || result == "DIRECT_INPUT") return Override::DirectInput;
		if (result == "RAWINPUT" || result == "RAW_INPUT") return Override::RawInput;
		if (result == "XINPUT" || result == "X_INPUT") return Override::XInput;
		return Override::Invalid;
	}

	inline constexpr Override normalized(Override value) noexcept
	{
		return value == Override::Invalid ? Override::Automatic : value;
	}

	inline constexpr Backend resolve(int productionPreference, bool ffbAttached, Override overrideValue) noexcept
	{
		switch (normalized(overrideValue))
		{
		case Override::Wgi: return Backend::Wgi;
		case Override::DirectInput: return Backend::DirectInput;
		case Override::RawInput: return Backend::RawInput;
		case Override::XInput: return Backend::XInput;
		default: break;
		}
		if (productionPreference == 1) return Backend::RawInput;
		if (productionPreference == 2) return Backend::DirectInput;
		if (productionPreference == 3) return Backend::XInput;
		return ffbAttached ? Backend::DirectInput : Backend::Wgi;
	}

	inline constexpr const char* value(Override selection) noexcept
	{
		switch (selection)
		{
		case Override::Wgi: return "WGI";
		case Override::DirectInput: return "DIRECTINPUT";
		case Override::RawInput: return "RAWINPUT";
		case Override::XInput: return "XINPUT";
		default: return "AUTOMATIC";
		}
	}

	inline constexpr const char* name(Override selection) noexcept
	{
		switch (selection)
		{
		case Override::Wgi: return "Windows.Gaming.Input";
		case Override::DirectInput: return "SDL DirectInput";
		case Override::RawInput: return "SDL RawInput";
		case Override::XInput: return "SDL XInput";
		case Override::Invalid: return "Automatic (invalid value ignored)";
		default: return "Automatic";
		}
	}

	inline constexpr const char* name(Backend backend) noexcept
	{
		switch (backend)
		{
		case Backend::Wgi: return "Windows.Gaming.Input";
		case Backend::RawInput: return "SDL RawInput";
		case Backend::DirectInput: return "SDL DirectInput";
		case Backend::XInput: return "SDL XInput";
		}
		return "Unknown";
	}

	inline constexpr bool restart_required(Override activeAtStartup, Override saved) noexcept
	{
		return normalized(activeAtStartup) != normalized(saved);
	}
}
