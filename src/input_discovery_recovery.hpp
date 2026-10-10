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
	enum class Override { Automatic, DirectInput, Wgi, Invalid };
	enum class Readiness { NotEnumerated, Enumerated, OpenFailed, Opened, Registered, AvailableForBinding, Removed };

	inline Override parse_override(std::string_view value) noexcept
	{
		std::array<char, 32> normalized{};
		const auto length = std::min(value.size(), normalized.size() - 1);
		for (size_t index = 0; index < length; ++index)
		{
			const char c = value[index];
			normalized[index] = c >= 'a' && c <= 'z' ? char(c - ('a' - 'A')) : c;
		}
		const std::string_view result(normalized.data(), length);
		if (result.empty() || result == "AUTOMATIC" || result == "AUTO") return Override::Automatic;
		if (result == "DIRECTINPUT" || result == "DIRECT_INPUT") return Override::DirectInput;
		if (result == "WGI" || result == "WINDOWS.GAMING.INPUT") return Override::Wgi;
		return Override::Invalid;
	}

	inline Backend resolve_backend(int requested, bool ffbAttached, Override developerOverride) noexcept
	{
		if (developerOverride == Override::DirectInput) return Backend::DirectInput;
		if (developerOverride == Override::Wgi) return Backend::Wgi;
		if (requested == 1) return Backend::RawInput;
		if (requested == 2) return Backend::DirectInput;
		if (requested == 3) return Backend::XInput;
		return ffbAttached ? Backend::DirectInput : Backend::Wgi;
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
