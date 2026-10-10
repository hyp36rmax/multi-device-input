#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <string_view>
#include <unordered_map>

namespace InputDiscovery
{
	enum class Backend { Wgi, RawInput, DirectInput, XInput };
	enum class Override { Automatic, Wgi, DirectInput, RawInput, XInput, Invalid };
	enum class Readiness { NotEnumerated, Enumerated, OpenFailed, Opened, Registered, AvailableForBinding, Removed };

	inline Override parse_override(std::string_view value) noexcept
	{
		std::array<char, 32> normalized{};
		const auto length = (std::min)(value.size(), normalized.size() - 1);
		for (size_t index = 0; index < length; ++index)
		{
			const char c = value[index];
			normalized[index] = c >= 'a' && c <= 'z' ? char(c - ('a' - 'A')) : c;
		}
		const std::string_view result(normalized.data(), length);
		if (result.empty() || result == "AUTOMATIC" || result == "AUTO") return Override::Automatic;
		if (result == "DIRECTINPUT" || result == "DIRECT_INPUT") return Override::DirectInput;
		if (result == "WGI" || result == "WINDOWS.GAMING.INPUT") return Override::Wgi;
		if (result == "RAWINPUT" || result == "RAW_INPUT") return Override::RawInput;
		if (result == "XINPUT" || result == "X_INPUT") return Override::XInput;
		return Override::Invalid;
	}

	inline constexpr const char* backend_name(Backend backend) noexcept
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

	inline constexpr const char* override_value(Override value) noexcept
	{
		switch (value)
		{
		case Override::Automatic: return "AUTOMATIC";
		case Override::Wgi: return "WGI";
		case Override::DirectInput: return "DIRECTINPUT";
		case Override::RawInput: return "RAWINPUT";
		case Override::XInput: return "XINPUT";
		case Override::Invalid: return "AUTOMATIC";
		}
		return "AUTOMATIC";
	}

	inline constexpr const char* override_name(Override value) noexcept
	{
		switch (value)
		{
		case Override::Automatic: return "Automatic";
		case Override::Wgi: return "Windows.Gaming.Input";
		case Override::DirectInput: return "SDL DirectInput";
		case Override::RawInput: return "SDL RawInput";
		case Override::XInput: return "SDL XInput";
		case Override::Invalid: return "Automatic (invalid value ignored)";
		}
		return "Automatic";
	}

	inline constexpr Override normalized_override(Override value) noexcept
	{
		return value == Override::Invalid ? Override::Automatic : value;
	}

	inline constexpr bool restart_required(Override activeAtStartup, Override saved) noexcept
	{
		return normalized_override(activeAtStartup) != normalized_override(saved);
	}

	inline constexpr const char* discovery_summary(size_t registeredDevices, bool recoveryComplete, bool openFailed) noexcept
	{
		if (openFailed) return "Device Open Failed";
		if (!recoveryComplete) return "Discovery In Progress";
		if (!registeredDevices) return "No Devices Detected";
		return "Delayed Discovery Completed";
	}

	inline Backend resolve_backend(int requested, bool ffbAttached, Override developerOverride) noexcept
	{
		if (developerOverride == Override::DirectInput) return Backend::DirectInput;
		if (developerOverride == Override::Wgi) return Backend::Wgi;
		if (developerOverride == Override::RawInput) return Backend::RawInput;
		if (developerOverride == Override::XInput) return Backend::XInput;
		if (requested == 1) return Backend::RawInput;
		if (requested == 2) return Backend::DirectInput;
		if (requested == 3) return Backend::XInput;
		return ffbAttached ? Backend::DirectInput : Backend::Wgi;
	}

	inline constexpr const char* selection_reason(int requested, bool ffbAttached, Override requestedOverride) noexcept
	{
		const Override value = normalized_override(requestedOverride);
		if (value != Override::Automatic) return "Explicit developer override selected before SDL initialization; backend locked for this session";
		if (requested == 1) return "Saved Controls backend selected SDL RawInput; backend locked for this session";
		if (requested == 2) return "Saved Controls backend selected SDL DirectInput; backend locked for this session";
		if (requested == 3) return "Saved Controls backend selected SDL XInput; backend locked for this session";
		return ffbAttached
			? "Automatic policy detected an attached native FFB controller and preferred SDL DirectInput; backend locked for this session"
			: "Automatic policy found no attached native FFB controller and selected Windows.Gaming.Input; backend locked for this session";
	}

	inline constexpr std::array RecoveryPoints{
		std::chrono::seconds(1), std::chrono::seconds(3), std::chrono::seconds(5)
	};

	class RecoverySchedule
	{
		size_t next_ = 0;
	public:
		bool due(std::chrono::steady_clock::duration elapsed) const noexcept
		{
			return next_ < RecoveryPoints.size() && elapsed >= RecoveryPoints[next_];
		}
		int consume() noexcept
		{
			if (next_ >= RecoveryPoints.size()) return -1;
			return int(std::chrono::duration_cast<std::chrono::seconds>(RecoveryPoints[next_++]).count());
		}
		bool complete() const noexcept { return next_ >= RecoveryPoints.size(); }
		size_t observations() const noexcept { return next_; }
	};

	class RegistryModel
	{
		std::unordered_map<std::int64_t, Readiness> states_;
	public:
		bool enumerate(std::int64_t instanceId) { return states_.try_emplace(instanceId, Readiness::Enumerated).second; }
		void opened(std::int64_t instanceId, bool success) { states_[instanceId] = success ? Readiness::Opened : Readiness::OpenFailed; }
		void registered(std::int64_t instanceId) { states_[instanceId] = Readiness::Registered; }
		void available(std::int64_t instanceId) { states_[instanceId] = Readiness::AvailableForBinding; }
		void removed(std::int64_t instanceId) { states_[instanceId] = Readiness::Removed; }
		Readiness state(std::int64_t instanceId) const { const auto found = states_.find(instanceId); return found == states_.end() ? Readiness::NotEnumerated : found->second; }
		size_t size() const noexcept { return states_.size(); }
	};
}
