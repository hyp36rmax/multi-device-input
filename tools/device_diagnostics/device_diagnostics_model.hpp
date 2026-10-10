#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
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
		ResultState state = ResultState::Untested;
		std::string startedUtc;
		std::string error;
		std::vector<Device> devices;
		std::vector<std::string> delayedEvents;
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
		std::vector<std::optional<CapturedInput>> saved = std::vector<std::optional<CapturedInput>>(6);
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
