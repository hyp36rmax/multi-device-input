#pragma once

#include <algorithm>
#include <chrono>
#include <string>
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
