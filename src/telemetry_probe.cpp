#include "telemetry_probe.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>

#include <spdlog/spdlog.h>

#include "plugin.hpp"
#include "resource.h"

namespace Settings
{
	Setting<bool> TelemetryEnabled{ "Developer", "TelemetryEnabled", false,
		"Record developer vehicle and force-feedback telemetry." };
	Setting<std::string> TelemetryTestScenario{ "Developer", "TelemetryTestScenario", "",
		"Optional controlled-test scenario stored in telemetry CSV metadata." };
	Setting<std::string> TelemetryNotes{ "Developer", "TelemetryNotes", "",
		"Optional controlled-test notes stored in telemetry CSV metadata." };

	namespace
	{
		struct HideDeveloperSetting
		{
			HideDeveloperSetting()
			{
				TelemetryEnabled.hidden(true);
				TelemetryTestScenario.hidden(true);
				TelemetryNotes.hidden(true);
			}
		} hideDeveloperSetting;
	}
}

namespace TelemetryProbe
{
	namespace
	{
		constexpr const char* ProbeVersion = "TP-01C";
		constexpr size_t FlushEverySamples = 120;

		Snapshot current{};
		std::ofstream csv;
		std::string pendingRows;
		std::chrono::steady_clock::time_point sessionStart{};
		size_t samplesSinceFlush = 0;
		bool pendingFfbAvailable = false;
		bool captureRequested = true;
		float pendingFfbRaw = 0.0f;
		float pendingFfbFinal = 0.0f;
		float pendingFfbMaster = 0.0f;

		std::string local_time_text(std::time_t value, const char* format)
		{
			std::tm local{};
			localtime_s(&local, &value);
			char text[64]{};
			std::strftime(text, sizeof(text), format, &local);
			return text;
		}

		std::string metadata_text(std::string text)
		{
			for (char& c : text)
			{
				if (c == '\r' || c == '\n')
					c = ' ';
			}
			return text;
		}

		std::filesystem::path unique_csv_path(std::time_t started)
		{
			const auto folder = Module::DllPath.parent_path();
			const std::string stem = "telemetry_" + local_time_text(started, "%Y%m%d_%H%M%S");
			auto path = folder / (stem + ".csv");
			for (unsigned suffix = 1; std::filesystem::exists(path); ++suffix)
				path = folder / std::format("{}_{}.csv", stem, suffix);
			return path;
		}

		bool start_session()
		{
			const auto wallNow = std::chrono::system_clock::now();
			const std::time_t started = std::chrono::system_clock::to_time_t(wallNow);
			const auto path = unique_csv_path(started);
			const std::string scenario = metadata_text(TelemetryTestScenario.get());
			const std::string notes = metadata_text(TelemetryNotes.get());
			csv.clear();
			csv.open(path, std::ios::out | std::ios::trunc);
			if (!csv)
			{
				spdlog::error("TelemetryProbe: could not create {}", path.string());
				return false;
			}

			sessionStart = std::chrono::steady_clock::now();
			current = {};
			current.active = true;
			current.testScenario = scenario;
			current.currentFilename = path.filename().string();
			pendingRows.clear();
			samplesSinceFlush = 0;

			csv << "# telemetry_probe_version=" << ProbeVersion << '\n';
			csv << "# telemetry_probe=" << ProbeVersion << '\n';
			csv << "# tweaks_version=" << MODULE_VERSION_STR << '\n';
			csv << "# game_exe_timestamp=" << Util::GetModuleTimestamp(Module::ExeHandle) << '\n';
			csv << "# start_time_local=" << local_time_text(started, "%Y-%m-%d %H:%M:%S") << '\n';
			csv << "# test_scenario=" << scenario << '\n';
			csv << "# notes=" << notes << '\n';
			csv << "timestamp,frame,elapsed_time,speed,steering_input,xforce,surface_0,surface_1,surface_2,surface_3,ffb_raw,ffb_final,ffb_master,native_1D0,native_1D4,native_1DC,native_1E0,native_1E4,native_264,native_268\n";
			spdlog::info("TelemetryProbe: recording TP-01C samples to {}", path.string());
			return true;
		}

		void flush_rows()
		{
			if (!csv || pendingRows.empty())
				return;
			csv << pendingRows;
			csv.flush();
			pendingRows.clear();
			samplesSinceFlush = 0;
		}
	}

	void observe_ffb(float rawForce, float finalRequestedForce, float masterStrength)
	{
		if (!Settings::TelemetryEnabled)
			return;
		pendingFfbRaw = rawForce;
		pendingFfbFinal = finalRequestedForce;
		pendingFfbMaster = masterStrength;
		pendingFfbAvailable = true;
	}

	void sample(float speed, float steeringInput, const std::array<uint32_t, 4>& surfaceRaw,
		const std::array<float, 7>& nativeCandidates)
	{
		if (!Settings::TelemetryEnabled)
		{
			if (csv.is_open())
				shutdown();
			return;
		}
		if (!captureRequested)
			return;
		if (!csv.is_open() && !start_session())
			return;

		const auto steadyNow = std::chrono::steady_clock::now();
		const auto wallNow = std::chrono::system_clock::now();
		current.timestamp = std::chrono::duration<double>(wallNow.time_since_epoch()).count();
		current.elapsedTime = std::chrono::duration<double>(steadyNow - sessionStart).count();
		current.speed = speed;
		current.steeringInput = steeringInput;
		current.surfaceRaw = surfaceRaw;
		current.nativeCandidates = nativeCandidates;
		current.ffbAvailable = pendingFfbAvailable;
		if (pendingFfbAvailable)
		{
			current.ffbRaw = pendingFfbRaw;
			current.ffbFinal = pendingFfbFinal;
			current.ffbMasterStrength = pendingFfbMaster;
		}
		pendingFfbAvailable = false;
		current.active = true;

		// XForce is intentionally empty in TP-01. No current game field has been
		// confirmed to carry that meaning, and adjacent anonymous fields must not
		// be relabelled from assumption alone.
		const std::string xForceCell = current.xForceAvailable
			? std::format("{:.6f}", current.xForce) : std::string{};
		const std::string ffbRawCell = current.ffbAvailable
			? std::format("{:.6f}", current.ffbRaw) : std::string{};
		const std::string ffbFinalCell = current.ffbAvailable
			? std::format("{:.6f}", current.ffbFinal) : std::string{};
		const std::string ffbMasterCell = current.ffbAvailable
			? std::format("{:.3f}", current.ffbMasterStrength) : std::string{};

		pendingRows += std::format(
			"{:.6f},{},{:.6f},{:.6f},{:.6f},{},{},{},{},{},{},{},{},{:.6f},{:.6f},{:.6f},{:.6f},{:.6f},{:.6f},{:.6f}\n",
			current.timestamp, current.frameIndex, current.elapsedTime,
			current.speed, current.steeringInput, xForceCell,
			current.surfaceRaw[0], current.surfaceRaw[1],
			current.surfaceRaw[2], current.surfaceRaw[3],
			ffbRawCell, ffbFinalCell, ffbMasterCell,
			current.nativeCandidates[0], current.nativeCandidates[1],
			current.nativeCandidates[2], current.nativeCandidates[3],
			current.nativeCandidates[4], current.nativeCandidates[5],
			current.nativeCandidates[6]);
		++current.frameIndex;
		if (++samplesSinceFlush >= FlushEverySamples)
			flush_rows();
	}

	void shutdown()
	{
		if (!csv.is_open())
		{
			current.active = false;
			pendingFfbAvailable = false;
			return;
		}
		flush_rows();
		csv.close();
		current.active = false;
		pendingFfbAvailable = false;
		spdlog::info("TelemetryProbe: session closed after {} samples", current.frameIndex);
	}

	bool start_new_capture()
	{
		if (!Settings::TelemetryEnabled)
			return false;
		shutdown();
		captureRequested = true;
		return start_session();
	}

	void stop_capture()
	{
		captureRequested = false;
		shutdown();
	}

	const Snapshot& snapshot()
	{
		return current;
	}
}
