#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "sound_request_trace.hpp"

#include <array>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <string_view>

#include <spdlog/spdlog.h>

#include "hook_mgr.hpp"
#include "plugin.hpp"
#include "product_identity.hpp"
#include "research_capture_path.hpp"
#include "telemetry_probe.hpp"

namespace SoundRequestTrace
{
	namespace
	{
		constexpr uintptr_t SetSndQueueRva = 0x24940;
		constexpr uintptr_t PrjSndRequestRva = 0x249F0;
		constexpr uintptr_t LowerPlayRouteRva = 0x2F1A0;

		struct Record
		{
			Boundary boundary = Boundary::State;
			uint32_t command = 0;
			uint32_t callerRva = 0;
			uint64_t frame = 0;
			double elapsed = 0.0;
			std::array<uint32_t, 4> surface{};
			std::array<uint32_t, 4> previousSurface{};
			uint8_t transitionMask = 0;
			float speed = 0.0f;
			uint32_t gear = 0;
			bool gearTransition = false;
			float impact = 0.0f;
			float steering = 0.0f;
		};

		std::array<Record, MaximumRecords> records{};
		size_t recordsUsed = 0;
		size_t dropped = 0;
		bool captureActive = false;
		bool captureComplete = false;
		bool traceWritten = false;
		double completedElapsed = 0.0;
		std::chrono::steady_clock::time_point captureStart{};
		struct Context
		{
			uint64_t frame = 0;
			std::array<uint32_t, 4> surface{};
			std::array<uint32_t, 4> previousSurface{};
			std::array<bool, 4> surfaceChanged{};
			float speed = 0.0f;
			uint32_t gear = 0;
			bool gearTransition = false;
			float impact = 0.0f;
			float steering = 0.0f;
		};
		Context latest{};
		std::filesystem::path outputPath;
		std::string scenarioName;
		std::string telemetryFile;
		std::string currentStatus;

		SafetyHookMid setSndQueueHook{};
		SafetyHookMid prjSndRequestHook{};
		SafetyHookMid lowerPlayRouteHook{};

		const char* boundary_name(Boundary boundary)
		{
			switch (boundary)
			{
			case Boundary::State: return "STATE";
			case Boundary::SetSndQueue: return "SET_SND_QUEUE";
			case Boundary::PrjSndRequest: return "PRJ_SND_REQUEST";
			case Boundary::LowerPlayRoute: return "LOWER_PLAY_ROUTE";
			}
			return "UNKNOWN";
		}

		std::string local_time_text(std::time_t value, const char* format)
		{
			std::tm local{};
			localtime_s(&local, &value);
			char text[64]{};
			std::strftime(text, sizeof(text), format, &local);
			return text;
		}

		void append(Boundary boundary, uint32_t command, uintptr_t caller)
		{
			if (!captureActive)
				return;
			if (recordsUsed >= records.size())
			{
				++dropped;
				captureActive = false;
				captureComplete = true;
				completedElapsed = std::chrono::duration<double>(
					std::chrono::steady_clock::now() - captureStart).count();
				currentStatus = "record limit reached; return to Debug to save";
				return;
			}

			Record& record = records[recordsUsed++];
			record.boundary = boundary;
			record.command = command;
			const uintptr_t base = reinterpret_cast<uintptr_t>(Module::ExeHandle);
			record.callerRva = caller >= base ? static_cast<uint32_t>(caller - base) : 0;
			record.frame = latest.frame;
			record.elapsed = std::chrono::duration<double>(
				std::chrono::steady_clock::now() - captureStart).count();
			record.surface = latest.surfaceRaw;
			record.previousSurface = latest.previousSurfaceRaw;
			record.transitionMask = 0;
			for (size_t i = 0; i < latest.surfaceChanged.size(); ++i)
				if (latest.surfaceChanged[i]) record.transitionMask |= static_cast<uint8_t>(1u << i);
			record.speed = latest.speed;
			record.gear = latest.gear;
			record.gearTransition = latest.gearTransition;
			record.impact = latest.impact;
			record.steering = latest.steering;
		}

		uintptr_t return_address(const safetyhook::Context& ctx)
		{
			return *reinterpret_cast<const uintptr_t*>(ctx.esp);
		}

		void set_snd_queue_destination(safetyhook::Context& ctx)
		{
			append(Boundary::SetSndQueue,
				*reinterpret_cast<const uint32_t*>(ctx.esp + sizeof(uint32_t)), return_address(ctx));
		}

		void prj_snd_request_destination(safetyhook::Context& ctx)
		{
			append(Boundary::PrjSndRequest,
				*reinterpret_cast<const uint32_t*>(ctx.esp + sizeof(uint32_t)), return_address(ctx));
		}

		void lower_play_route_destination(safetyhook::Context& ctx)
		{
			append(Boundary::LowerPlayRoute, ctx.esi, return_address(ctx));
		}

		bool write_trace(const char* status)
		{
			std::ofstream out(outputPath, std::ios::out | std::ios::trunc);
			if (!out)
			{
				currentStatus = "could not create trace file";
				spdlog::error("SoundRequestTrace: could not create {}", outputPath.string());
				return false;
			}
			out << "# trace_schema=" << SchemaVersion << '\n';
			out << "# product=" << ProductIdentity::Name << '\n';
			out << "# product_version=" << ProductIdentity::Version << '\n';
			out << "# build_commit=" << ProductIdentity::BuildCommit << '\n';
			out << "# scenario=" << scenarioName << '\n';
			out << "# telemetry_file=" << telemetryFile << '\n';
			out << "# status=" << status << '\n';
			out << "# record_count=" << recordsUsed << '\n';
			out << "# dropped_count=" << dropped << '\n';
			out << "boundary,command,sound_id,loop,pan_left,pan_right,pan_leftright,stop,caller_rva,frame,elapsed_time,surface_0,surface_1,surface_2,surface_3,previous_surface_0,previous_surface_1,previous_surface_2,previous_surface_3,surface_transition_mask,speed,gear,gear_transition,impact,steering\n";
			for (size_t i = 0; i < recordsUsed; ++i)
			{
				const Record& r = records[i];
				const DecodedCommand decoded = decode_command(r.command);
				out << boundary_name(r.boundary) << ',' << r.command << ',' << decoded.soundId << ','
					<< decoded.loop << ',' << decoded.panLeft << ',' << decoded.panRight << ','
					<< decoded.panLeftRight << ',' << decoded.stop << ",0x" << std::hex
					<< r.callerRva << std::dec << ',' << r.frame << ',' << r.elapsed;
				for (uint32_t value : r.surface) out << ',' << value;
				for (uint32_t value : r.previousSurface) out << ',' << value;
				out << ',' << static_cast<unsigned>(r.transitionMask) << ',' << r.speed << ','
					<< r.gear << ',' << r.gearTransition << ',' << r.impact << ',' << r.steering << '\n';
			}
			out.flush();
			currentStatus = out ? "trace saved" : "trace write failed";
			spdlog::info("SoundRequestTrace: {} records written to {}; dropped={}",
				recordsUsed, outputPath.string(), dropped);
			return !!out;
		}

		class SoundRequestTraceHooks : public Hook
		{
		public:
			std::string_view description() override { return "T6 sound-request ownership probe"; }
			bool apply() override
			{
				setSndQueueHook = safetyhook::create_mid(Module::exe_ptr(SetSndQueueRva), set_snd_queue_destination);
				prjSndRequestHook = safetyhook::create_mid(Module::exe_ptr(PrjSndRequestRva), prj_snd_request_destination);
				lowerPlayRouteHook = safetyhook::create_mid(Module::exe_ptr(LowerPlayRouteRva), lower_play_route_destination);
				return !!setSndQueueHook && !!prjSndRequestHook && !!lowerPlayRouteHook;
			}
			static SoundRequestTraceHooks instance;
		};
		SoundRequestTraceHooks SoundRequestTraceHooks::instance;
	}

	bool start_capture(const std::string& scenario, const std::string& telemetryFilename)
	{
		if (captureActive)
			return false;
		recordsUsed = 0;
		dropped = 0;
		captureComplete = false;
		traceWritten = false;
		completedElapsed = 0.0;
		scenarioName = HYP36RResearchPath::sanitize_scenario(scenario);
		telemetryFile = telemetryFilename;
		const auto folder = Module::DllPath.parent_path() / "HYP36R" / "Research" / scenarioName;
		std::error_code error;
		std::filesystem::create_directories(folder, error);
		if (error)
		{
			currentStatus = "could not create research folder";
			return false;
		}
		const auto now = std::chrono::system_clock::now();
		const auto started = std::chrono::system_clock::to_time_t(now);
		outputPath = folder / ("sound_requests_" + local_time_text(started, "%Y%m%d_%H%M%S") + ".csv");
		captureStart = std::chrono::steady_clock::now();
		observe_frame(TelemetryProbe::snapshot());
		captureActive = true;
		currentStatus = "recording";
		spdlog::info("SoundRequestTrace: recording {} to {}", SchemaVersion, outputPath.string());
		return true;
	}

	void finish_capture(const char* status)
	{
		if (captureActive)
			completedElapsed = std::chrono::duration<double>(
				std::chrono::steady_clock::now() - captureStart).count();
		captureActive = false;
		captureComplete = true;
		traceWritten = write_trace(status);
	}

	void cancel_capture()
	{
		captureActive = false;
		captureComplete = false;
		traceWritten = false;
		completedElapsed = 0.0;
		recordsUsed = 0;
		dropped = 0;
		currentStatus = "cancelled";
	}

	void observe_frame(const TelemetryProbe::Snapshot& snapshot)
	{
		latest.frame = snapshot.frameIndex;
		latest.surface = snapshot.surfaceRaw;
		latest.previousSurface = snapshot.previousSurfaceRaw;
		latest.surfaceChanged = snapshot.surfaceChanged;
		latest.speed = snapshot.speed;
		latest.gear = snapshot.researchII.gearCurrent;
		latest.gearTransition = snapshot.researchII.gearTransition;
		latest.impact = snapshot.researchII.impactPostGain;
		latest.steering = snapshot.steeringInput;
		if (!captureActive)
			return;
		append(Boundary::State, 0, 0);
		if (elapsed_seconds() >= MaximumCaptureSeconds)
		{
			captureActive = false;
			captureComplete = true;
			completedElapsed = MaximumCaptureSeconds;
			currentStatus = "45-second limit reached; return to Debug to save";
		}
	}

	bool recording() { return captureActive; }
	bool completed() { return captureComplete; }
	bool needs_save() { return captureComplete && !traceWritten; }
	double elapsed_seconds()
	{
		if (!captureActive && !captureComplete) return 0.0;
		if (captureComplete) return completedElapsed;
		return std::chrono::duration<double>(std::chrono::steady_clock::now() - captureStart).count();
	}
	size_t record_count() { return recordsUsed; }
	size_t dropped_count() { return dropped; }
	const std::filesystem::path& trace_path() { return outputPath; }
	const std::string& status_text() { return currentStatus; }
}
