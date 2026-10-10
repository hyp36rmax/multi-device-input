#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace DeviceDiagnostics
{
	inline constexpr const char* Version = "0.1.0-dev";
	inline constexpr int PhysicalOutputCeilingPercent = 20;
	inline constexpr auto MaximumRunTime = std::chrono::milliseconds(1500);
	inline constexpr auto WatchdogTimeout = std::chrono::milliseconds(250);
	inline constexpr auto BackendObservationTime = std::chrono::seconds(10);
	inline constexpr std::array<std::string_view, 3> VisibleFfbActions{ "Test Left", "Test Right", "Hold to Shake" };
	inline constexpr bool effective_right(bool requestedRight, bool inverted) { return requestedRight != inverted; }
	inline std::filesystem::path exports_path(const std::filesystem::path& documents)
	{
		return documents / "HYP36rforce Device Diagnostics" / "Exports";
	}

	enum class ResultState { Completed, Failed, Unavailable, Untested, Running };

	struct Device
	{
		std::string id;
		std::string name;
		std::string backend;
		uint16_t vendor = 0;
		uint16_t product = 0;
		int axes = 0;
		int buttons = 0;
		int hats = 0;
		bool inputAvailable = false;
		bool ffbAvailable = false;
	};

	struct BackendResult
	{
		std::string name;
		std::string effectiveBackend;
		ResultState state = ResultState::Untested;
		std::string startedUtc;
		std::string error;
		std::vector<Device> devices;
		std::vector<std::string> delayedEvents;
	};

	struct ShakeController
	{
		static constexpr int FrequencyHz = 10;
		static constexpr auto HalfPeriod = std::chrono::milliseconds(1000 / (FrequencyHz * 2));
		bool active = false;
		bool right = false;
		std::chrono::steady_clock::time_point nextTransition{};

		void begin(std::chrono::steady_clock::time_point now, bool initialRight = false)
		{
			active = true; right = initialRight; nextTransition = now + HalfPeriod;
		}
		std::optional<bool> update(std::chrono::steady_clock::time_point now)
		{
			if (!active || now < nextTransition) return std::nullopt;
			right = !right;
			do nextTransition += HalfPeriod; while (nextTransition <= now);
			return right;
		}
		void stop() { active = false; }
	};

	struct Assignment
	{
		std::string steering;
		std::string pedals;
		std::string shifter;
		std::string additional;
		std::string ffb;
	};

	struct CapturedInput
	{
		std::string deviceId;
		std::string deviceName;
		std::string control;
		bool ambiguous = false;
	};

	struct QuickSetupController
	{
		static constexpr auto CaptureTime = std::chrono::seconds(6);
		int step = 0;
		bool active = false;
		bool timedOut = false;
		std::optional<CapturedInput> candidate;
		std::chrono::steady_clock::time_point deadline{};
		std::vector<std::optional<CapturedInput>> saved = std::vector<std::optional<CapturedInput>>(8);
		std::vector<std::optional<CapturedInput>> backup;

		void start(std::chrono::steady_clock::time_point now)
		{
			backup = saved; step = 0; active = true; retry(now);
		}
		void retry(std::chrono::steady_clock::time_point now)
		{
			candidate.reset(); timedOut = false; deadline = now + CaptureTime;
		}
		void observe(CapturedInput input) { if (active && !timedOut && !candidate) candidate = std::move(input); }
		bool continue_step(std::chrono::steady_clock::time_point now)
		{
			if (!active || !candidate || candidate->ambiguous) return false;
			saved[step] = candidate;
			if (++step >= int(saved.size())) { active = false; return true; }
			retry(now); return true;
		}
		void skip(std::chrono::steady_clock::time_point now)
		{
			if (!active) return;
			if (++step >= int(saved.size())) { active = false; return; }
			retry(now);
		}
		void back(std::chrono::steady_clock::time_point now)
		{
			if (!active || step == 0) return;
			--step; retry(now);
		}
		void cancel() { if (!backup.empty()) saved = backup; active = false; candidate.reset(); timedOut = false; }
		void update(std::chrono::steady_clock::time_point now) { if (active && !candidate && now >= deadline) timedOut = true; }
	};

	enum class FfbResolution { NotFound, Restored, AutoSelected, SelectionRequired };

	struct FfbResolutionResult
	{
		FfbResolution state = FfbResolution::NotFound;
		int index = -1;
	};

	inline FfbResolutionResult resolve_ffb_device(const std::vector<std::string>& identities, std::string_view previous)
	{
		if (!previous.empty())
			for (size_t index = 0; index < identities.size(); ++index)
				if (identities[index] == previous) return { FfbResolution::Restored, int(index) };
		if (identities.empty()) return {};
		if (identities.size() == 1) return { FfbResolution::AutoSelected, 0 };
		return { FfbResolution::SelectionRequired, -1 };
	}

	struct RedetectLifecycle
	{
		bool effectStopped = false;
		bool deviceReleased = false;
		bool enumerationRefreshed = false;
		bool identityResolved = false;
		bool capabilitiesValidated = false;
		bool acquired = false;
		bool zeroForce = true;
	};

	inline std::string sanitize_filename_component(std::string_view value)
	{
		std::string out;
		bool separator = false;
		for (const unsigned char c : value)
		{
			const bool invalid = c < 32 || c == '<' || c == '>' || c == ':' || c == '"' || c == '/' || c == '\\' || c == '|' || c == '?' || c == '*';
			if (invalid || c == ' ' || c == '-' || c == '.') { separator = !out.empty(); continue; }
			if (separator) { out.push_back('_'); separator = false; }
			out.push_back(char(c));
		}
		while (!out.empty() && out.back() == '_') out.pop_back();
		return out.empty() ? "No_Wheel_Detected" : out;
	}

	struct SafetyController
	{
		bool authorized = false;
		bool running = false;
		std::chrono::steady_clock::time_point started{};
		std::chrono::steady_clock::time_point heartbeat{};

		int bounded_magnitude(int requestedPercent) const
		{
			return std::clamp(requestedPercent, 0, PhysicalOutputCeilingPercent) * 100;
		}

		bool begin(bool selected, bool focused, std::chrono::steady_clock::time_point now)
		{
			if (!authorized || !selected || !focused) return false;
			running = true;
			started = heartbeat = now;
			return true;
		}

		void beat(std::chrono::steady_clock::time_point now) { heartbeat = now; }
		bool must_stop(bool held, bool focused, bool connected, std::chrono::steady_clock::time_point now) const
		{
			return running && (!held || !focused || !connected || now - started >= MaximumRunTime || now - heartbeat >= WatchdogTimeout);
		}
		void stop() { running = false; }
	};
}
