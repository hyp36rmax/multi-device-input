#include "hook_mgr.hpp"
#include "plugin.hpp"
#include "game_addrs.hpp"
#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <format>
#include <string>

#include "overlay.hpp"
#include "road2_active.hpp"
#include "telemetry_probe.hpp"

namespace Settings
{
	extern Setting<bool> Road2ArcadeAuthority;
	extern Setting<int> Road2DebugAuthorityGain;
	Setting<bool> TelemetryOverlayEnabled{ "Developer", "TelemetryOverlayEnabled", false,
		"Show the compact HYP36Rforce telemetry recorder." };
	namespace
	{
		struct HideTelemetryOverlaySetting
		{
			HideTelemetryOverlaySetting() { TelemetryOverlayEnabled.hidden(true); }
		} hideTelemetryOverlaySetting;
	}
}

class TelemetryOverlayWindow : public OverlayWindow
{
	enum class Phase { Ready, Countdown, Recording, Complete };
	Phase phase_ = Phase::Ready;
	double phaseStarted_ = 0.0;
	bool guided_ = false;
	unsigned attempt_ = 1;
	bool previousTelemetryEnabled_ = false;
	std::string previousScenario_;
	std::string previousNotes_;
	std::string captureName_;
	std::string error_;
	static constexpr double GuidedDuration = 10.0;

	static std::string context_text()
	{
		if (!Game::is_in_game() || !Game::pl_car() || !Game::stg_stage_num)
			return {};
		return std::format("Car {}  ·  {}", unsigned(Game::pl_car()->car_kind_11),
			Game::GetStageFriendlyName(*Game::stg_stage_num));
	}

	static int active_road_multiplier()
	{
		return HYP36RRoad2Active::resolve_calibration_gain(
			Settings::Road2ArcadeAuthority.get(), Settings::Road2DebugAuthorityGain.get());
	}

	void remember_settings()
	{
		previousTelemetryEnabled_ = Settings::TelemetryEnabled.get();
		previousScenario_ = Settings::TelemetryTestScenario.get();
		previousNotes_ = Settings::TelemetryNotes.get();
	}

	void restore_settings()
	{
		Settings::TelemetryEnabled = previousTelemetryEnabled_;
		Settings::TelemetryTestScenario = previousScenario_;
		Settings::TelemetryNotes = previousNotes_;
		Settings::TelemetryEnabled.notify();
		Settings::TelemetryTestScenario.notify();
		Settings::TelemetryNotes.notify();
		Settings::write(Module::UserIniPath);
	}

	bool begin_capture()
	{
		remember_settings();
		guided_ = !Settings::TelemetryTestScenario.get().empty();
		if (guided_)
		{
			captureName_ = Settings::TelemetryTestScenario.get();
		}
		else
		{
			captureName_ = "General Capture";
			Settings::TelemetryTestScenario = "GENERAL_CAPTURE";
			Settings::TelemetryNotes = "Player diagnostic capture";
		}
		Settings::TelemetryEnabled = true;
		Settings::TelemetryEnabled.notify();
		Settings::TelemetryTestScenario.notify();
		Settings::TelemetryNotes.notify();
		TelemetryProbe::set_research_context(guided_ ? "Guided UAT" : "Telemetry",
			captureName_, attempt_, guided_ ? GuidedDuration : 0.0);
		if (!TelemetryProbe::start_new_capture())
		{
			error_ = "Telemetry could not start. Check the log.";
			restore_settings();
			return false;
		}
		phaseStarted_ = ImGui::GetTime();
		phase_ = Phase::Recording;
		error_.clear();
		return true;
	}

	void stop_capture(const char* status)
	{
		const double elapsed = TelemetryProbe::capture_elapsed_seconds();
		TelemetryProbe::set_research_capture_status(status, elapsed);
		TelemetryProbe::stop_capture();
		restore_settings();
		phase_ = Phase::Complete;
	}

	void cancel_capture()
	{
		const double elapsed = TelemetryProbe::capture_elapsed_seconds();
		TelemetryProbe::set_research_capture_status("cancelled", elapsed);
		TelemetryProbe::stop_capture();
		TelemetryProbe::record_research_review("cancelled");
		restore_settings();
		phase_ = Phase::Ready;
	}

public:
	Kind kind() const override { return Kind::Hud; }
	const char* name() const override { return "HYP36Rforce Telemetry"; }
	int order() const override { return 15; }
	void init() override {}

	void render(bool) override
	{
		if (!Settings::TelemetryOverlayEnabled)
			return;

		const auto content = Overlay::content_rect();
		ImGui::SetNextWindowPos(ImVec2(content.x + content.width - 18.0f, content.y + 18.0f),
			ImGuiCond_FirstUseEver, ImVec2(1.0f, 0.0f));
		ImGui::SetNextWindowSizeConstraints(ImVec2(270.0f, 0.0f), ImVec2(390.0f, 500.0f));
		if (!ImGui::Begin("HYP36Rforce Telemetry", nullptr,
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse))
		{
			ImGui::End();
			return;
		}

		const std::string context = context_text();
		if (context.empty() && phase_ == Phase::Ready)
		{
			ImGui::TextDisabled("Waiting for gameplay...");
			ImGui::End();
			return;
		}

		const auto& telemetry = TelemetryProbe::snapshot();
		if (phase_ == Phase::Ready)
			guided_ = !Settings::TelemetryTestScenario.get().empty();
		const char* stateLabel = phase_ == Phase::Ready ? (guided_ ? "TEST" : "READY")
			: phase_ == Phase::Countdown ? "TEST"
			: phase_ == Phase::Recording ? "REC" : "DONE";
		ImGui::TextDisabled("%s", stateLabel);
		if (!context.empty())
			ImGui::TextUnformatted(context.c_str());
		if (guided_ && HYP36RRoad2Active::frame().mode == HYP36RRoad2Active::Mode::Experimental)
			ImGui::Text("Road Detail: ×%d", active_road_multiplier());

		if (phase_ == Phase::Ready)
		{
			ImGui::TextUnformatted(guided_ ? Settings::TelemetryTestScenario.get().c_str() : "General Capture");
			if (guided_ && !Settings::TelemetryNotes.get().empty())
				ImGui::TextWrapped("%s", Settings::TelemetryNotes.get().c_str());
			if (ImGui::Button(guided_ ? "Start Test" : "Start Recording"))
			{
				if (guided_)
				{
					phase_ = Phase::Countdown;
					phaseStarted_ = ImGui::GetTime();
				}
				else begin_capture();
			}
		}
		else if (phase_ == Phase::Countdown)
		{
			const double remaining = 3.0 - (ImGui::GetTime() - phaseStarted_);
			ImGui::Text("Starting in %d", (std::max)(1, int(std::ceil(remaining))));
			if (remaining <= 0.0)
				begin_capture();
			else
			{
				ImGui::SameLine();
				if (ImGui::Button("Cancel")) phase_ = Phase::Ready;
			}
		}
		else if (phase_ == Phase::Recording)
		{
			const double elapsed = TelemetryProbe::capture_elapsed_seconds();
			const double rate = telemetry.elapsedTime > 0.0
				? telemetry.frameIndex / telemetry.elapsedTime : 0.0;
			if (guided_)
			{
				ImGui::TextUnformatted(captureName_.c_str());
				ImGui::Text("%05.1f / %04.1f  ·  %.1f Hz", elapsed, GuidedDuration, rate);
			}
			else
				ImGui::Text("%05.1f  ·  %llu samples  ·  %.1f Hz", elapsed,
					static_cast<unsigned long long>(telemetry.frameIndex), rate);
			if (guided_ && elapsed >= GuidedDuration)
				stop_capture("pending_review");
			else
			{
				if (!guided_ && ImGui::Button("Stop"))
					stop_capture("completed");
				if (!guided_) ImGui::SameLine();
				if (ImGui::Button("Cancel")) cancel_capture();
			}
		}
		else
		{
			if (guided_)
				ImGui::TextUnformatted(captureName_.c_str());
			const double rate = telemetry.elapsedTime > 0.0
				? telemetry.frameIndex / telemetry.elapsedTime : 0.0;
			ImGui::Text("%.1f s  ·  %llu samples  ·  %.1f Hz", telemetry.elapsedTime,
				static_cast<unsigned long long>(telemetry.frameIndex), rate);
			if (telemetry.writeFailures == 0)
				ImGui::TextDisabled("Capture complete");
			else
				ImGui::TextColored(ImVec4(1.f, .4f, .3f, 1.f), "Capture write error");
			if (guided_)
			{
				if (ImGui::Button("Accept"))
				{
					TelemetryProbe::record_research_review("accepted");
					phase_ = Phase::Ready;
					attempt_ = 1;
				}
				ImGui::SameLine();
				if (ImGui::Button("Retry"))
				{
					TelemetryProbe::record_research_review("retry");
					++attempt_;
					Settings::TelemetryTestScenario = previousScenario_;
					Settings::TelemetryNotes = previousNotes_;
					phase_ = Phase::Countdown;
					phaseStarted_ = ImGui::GetTime();
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel"))
				{
					TelemetryProbe::record_research_review("cancelled");
					phase_ = Phase::Ready;
				}
			}
			else if (ImGui::Button("Done")) phase_ = Phase::Ready;
		}

		if (!error_.empty())
			ImGui::TextColored(ImVec4(1.f, .4f, .3f, 1.f), "%s", error_.c_str());
		ImGui::End();
	}

	static TelemetryOverlayWindow instance;
};
TelemetryOverlayWindow TelemetryOverlayWindow::instance;
