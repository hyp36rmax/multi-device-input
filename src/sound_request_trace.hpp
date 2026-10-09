#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

namespace TelemetryProbe { struct Snapshot; }

namespace SoundRequestTrace
{
	constexpr const char* SchemaVersion = "HYP36R_SOUND_REQUEST_TRACE_V1";
	constexpr double MaximumCaptureSeconds = 45.0;
	constexpr size_t MaximumRecords = 16384;

	enum class Boundary : uint8_t
	{
		State,
		SetSndQueue,
		PrjSndRequest,
		LowerPlayRoute
	};

	struct DecodedCommand
	{
		uint16_t soundId = 0;
		bool loop = false;
		bool panLeft = false;
		bool panRight = false;
		bool panLeftRight = false;
		bool stop = false;
	};

	constexpr DecodedCommand decode_command(uint32_t command)
	{
		return {
			static_cast<uint16_t>(command & 0x7ff),
			(command & 0x0800) != 0,
			(command & 0x1000) != 0,
			(command & 0x2000) != 0,
			(command & 0x4000) != 0,
			(command & 0x8000) != 0
		};
	}

	bool start_capture(const std::string& scenario, const std::string& telemetryFilename);
	void finish_capture(const char* status = "completed");
	void cancel_capture();
	void observe_frame(const TelemetryProbe::Snapshot& snapshot);
	bool recording();
	bool completed();
	bool needs_save();
	double elapsed_seconds();
	size_t record_count();
	size_t dropped_count();
	const std::filesystem::path& trace_path();
	const std::string& status_text();
}
