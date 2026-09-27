#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <mmsystem.h>
#ifdef SND_LOOP
#undef SND_LOOP
#endif

#include "audio_sync.hpp"
#include "plugin.hpp"
#include "product_identity.hpp"
#include "research_capture_path.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>

namespace AudioSync
{
	namespace
	{
		constexpr const char* Schema = "HYP36R_AUDIO_SYNC_V2";
		constexpr unsigned SampleRate = 48000;
		constexpr unsigned Channels = 2;
		constexpr double Pi = 3.14159265358979323846;

		struct Event
		{
			bool present = false;
			unsigned long long frame = 0;
			double relativeTimeSeconds = 0.0;
		};

		bool sessionActive = false;
		std::filesystem::path sidecarPath;
		std::string scenarioName;
		std::string sessionId;
		std::string completion = "not_started";
		std::string latestStatus;
		Event startEvent;
		Event endEvent;
		double telemetryTimeOrigin = 0.0;
		std::vector<std::uint8_t> markerWave;

		void append_u16(std::vector<std::uint8_t>& out, std::uint16_t value)
		{
			out.push_back(static_cast<std::uint8_t>(value));
			out.push_back(static_cast<std::uint8_t>(value >> 8));
		}

		void append_u32(std::vector<std::uint8_t>& out, std::uint32_t value)
		{
			for (unsigned shift = 0; shift < 32; shift += 8)
				out.push_back(static_cast<std::uint8_t>(value >> shift));
		}

		const std::vector<std::uint8_t>& marker_wave()
		{
			if (!markerWave.empty()) return markerWave;
			constexpr unsigned pulseFrames = SampleRate * 20 / 1000;
			constexpr unsigned gapFrames = SampleRate * 40 / 1000;
			constexpr unsigned tailFrames = SampleRate * 20 / 1000;
			constexpr unsigned totalFrames = pulseFrames * 2 + gapFrames + tailFrames;
			std::vector<std::int16_t> pcm(totalFrames * Channels, 0);
			auto pulse = [&](unsigned begin, double frequency)
			{
				for (unsigned i = 0; i < pulseFrames; ++i)
				{
					const double edge = (std::min)(i, pulseFrames - 1 - i) / 120.0;
					const double envelope = (std::min)(1.0, edge);
					const auto sample = static_cast<std::int16_t>(6500.0 * envelope *
						std::sin(2.0 * Pi * frequency * i / SampleRate));
					pcm[(begin + i) * 2] = sample;
					pcm[(begin + i) * 2 + 1] = static_cast<std::int16_t>(-sample);
				}
			};
			pulse(0, 3500.0);
			pulse(pulseFrames + gapFrames, 5500.0);

			const std::uint32_t dataBytes = static_cast<std::uint32_t>(pcm.size() * sizeof(std::int16_t));
			markerWave.insert(markerWave.end(), { 'R','I','F','F' }); append_u32(markerWave, 36 + dataBytes);
			markerWave.insert(markerWave.end(), { 'W','A','V','E','f','m','t',' ' }); append_u32(markerWave, 16);
			append_u16(markerWave, 1); append_u16(markerWave, Channels); append_u32(markerWave, SampleRate);
			append_u32(markerWave, SampleRate * Channels * 2); append_u16(markerWave, Channels * 2); append_u16(markerWave, 16);
			markerWave.insert(markerWave.end(), { 'd','a','t','a' }); append_u32(markerWave, dataBytes);
			const auto* bytes = reinterpret_cast<const std::uint8_t*>(pcm.data());
			markerWave.insert(markerWave.end(), bytes, bytes + dataBytes);
			return markerWave;
		}

		void write_sidecar()
		{
			std::ofstream out(sidecarPath, std::ios::out | std::ios::trunc);
			if (!out) { latestStatus = "Could not write audio sync sidecar"; return; }
			auto event = [&](const char* name, const Event& value)
			{
				out << "  \"" << name << "\": {\"present\": " << (value.present ? "true" : "false")
					<< ", \"marker_id\": \"" << name << "\", \"frame\": " << value.frame
					<< ", \"relative_time_seconds\": " << std::fixed << std::setprecision(9)
					<< value.relativeTimeSeconds << "}";
			};
			out << "{\n  \"schema\": \"" << Schema << "\",\n"
				<< "  \"product\": \"OutRun 2006 C2C Multi Input\",\n"
				<< "  \"build_identity\": \"" << ProductIdentity::Version << "\",\n"
				<< "  \"build_commit\": \"" << ProductIdentity::BuildCommit << "\",\n"
				<< "  \"session_id\": \"" << sessionId << "\",\n"
				<< "  \"scenario\": \"" << scenarioName << "\",\n"
				<< "  \"clock\": \"telemetry_session_steady_clock\",\n"
				<< "  \"time_units\": \"seconds\",\n"
				<< "  \"marker_format\": \"48000Hz_16bit_stereo_antiphase_two_pulse_3500_5500Hz\",\n"
				<< "  \"completion_state\": \"" << completion << "\",\n";
			event("START", startEvent); out << ",\n"; event("END", endEvent); out << "\n}\n";
		}
	}

	bool begin_session(const std::string& scenario, const std::string& telemetryFilename)
	{
		if (sessionActive || telemetryFilename.empty()) return false;
		scenarioName = HYP36RResearchPath::sanitize_scenario(scenario);
		sessionId = std::filesystem::path(telemetryFilename).stem().string();
		sidecarPath = Module::DllPath.parent_path() / "HYP36R" / "Research" / scenarioName /
			("audio_sync_" + sessionId + ".json");
		startEvent = {}; endEvent = {}; telemetryTimeOrigin = 0.0;
		completion = "in_progress"; sessionActive = true;
		latestStatus = "Audio sync session ready";
		write_sidecar();
		return true;
	}

	bool emit_marker(Marker marker, unsigned long long frame, double telemetryElapsedSeconds)
	{
		if (!sessionActive) return false;
		Event& value = marker == Marker::Start ? startEvent : endEvent;
		if (value.present) return false;
		if (marker == Marker::Start)
			telemetryTimeOrigin = telemetryElapsedSeconds;
		value = { true, frame, telemetryElapsedSeconds - telemetryTimeOrigin };
		const auto& wave = marker_wave();
		using PlaySoundFn = BOOL(WINAPI*)(LPCWSTR, HMODULE, DWORD);
		const HMODULE winmm = GetModuleHandleW(L"winmm.dll");
		const auto playSound = winmm ? reinterpret_cast<PlaySoundFn>(GetProcAddress(winmm, "PlaySoundW")) : nullptr;
		const bool played = playSound && playSound(reinterpret_cast<LPCWSTR>(wave.data()), nullptr,
			SND_MEMORY | SND_ASYNC | SND_NODEFAULT) != FALSE;
		latestStatus = played ? (marker == Marker::Start ? "START marker requested" : "END marker requested")
			: "Audio marker request failed";
		write_sidecar();
		return played;
	}

	void finish_session(const char* status)
	{
		if (!sessionActive) return;
		completion = status ? status : "unknown";
		write_sidecar();
		sessionActive = false;
	}

	bool active() { return sessionActive; }
	const std::string& status_text() { return latestStatus; }
}
