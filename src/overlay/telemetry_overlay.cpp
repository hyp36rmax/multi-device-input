#include "hook_mgr.hpp"
#include "plugin.hpp"
#include "game_addrs.hpp"
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <format>
#include <string>
#include "guided_uat.hpp"
#include "overlay.hpp"
#include "road2_active.hpp"
#include "telemetry_probe.hpp"
#include "wheel_force_feedback.hpp"

namespace Settings {
extern Setting<int> WheelFFBStrength, WheelFFBSteeringLoad, WheelFFBRoadDetail, WheelFFBImpactLevel;
extern Setting<bool> Road2ArcadeAuthority;
extern Setting<int> Road2DebugAuthorityGain;
Setting<bool> TelemetryOverlayEnabled{ "Developer", "TelemetryOverlayEnabled", false,
	"Legacy compatibility setting; overlay visibility is automatic." };
namespace { struct HideLegacy { HideLegacy() { TelemetryOverlayEnabled.hidden(true); } } hideLegacy; }
}

namespace {
int road_gain() { return HYP36RRoad2Active::resolve_calibration_gain(Settings::Road2ArcadeAuthority.get(), Settings::Road2DebugAuthorityGain.get()); }
bool enhanced() { return HYP36RRoad2Active::frame().mode == HYP36RRoad2Active::Mode::Experimental; }
void row(const char* label, const std::string& value) { ImGui::TextUnformatted(label); ImGui::SameLine(190); ImGui::TextUnformatted(value.c_str()); }
std::string car() { return Game::is_in_game() && Game::pl_car() ? std::format("Car {}", unsigned(Game::pl_car()->car_kind_11)) : "Waiting for gameplay..."; }
std::string stage() { return Game::is_in_game() && Game::stg_stage_num ? Game::GetStageFriendlyName(*Game::stg_stage_num) : ""; }
}

class TelemetryOverlayWindow : public OverlayWindow {
	GuidedUat::RoadCalibrationSweep sweep_;
	bool oldEnabled_ = false;
	std::string oldScenario_, oldNotes_, error_;

	bool settings_ok() const { return enhanced() && Settings::WheelFFBStrength.get() == 100 && Settings::WheelFFBSteeringLoad.get() == 100 && Settings::WheelFFBRoadDetail.get() == 100; }
	void remember() { oldEnabled_ = Settings::TelemetryEnabled.get(); oldScenario_ = Settings::TelemetryTestScenario.get(); oldNotes_ = Settings::TelemetryNotes.get(); }
	void restore() {
		Settings::TelemetryEnabled = oldEnabled_; Settings::TelemetryTestScenario = oldScenario_; Settings::TelemetryNotes = oldNotes_;
		Settings::TelemetryEnabled.notify(); Settings::TelemetryTestScenario.notify(); Settings::TelemetryNotes.notify(); Settings::write(Module::UserIniPath);
	}
	void configuration() {
		row("FFB Strength", std::format("{}%", Settings::WheelFFBStrength.get()));
		row("Steering Load", std::format("{}%", Settings::WheelFFBSteeringLoad.get()));
		row("Impact", std::format("{}%", Settings::WheelFFBImpactLevel.get())); ImGui::Spacing();
		row("Road Mode", enhanced() ? "Enhanced" : "Classic");
		row("Road Detail", std::format("{}%", Settings::WheelFFBRoadDetail.get()));
		row("Road Calibration", enhanced() ? std::format("×{}", road_gain()) : "—");
	}
	void start_general() {
		TelemetryProbe::set_research_context("Regular Telemetry", Settings::TelemetryTestScenario.get().empty() ? "General Capture" : Settings::TelemetryTestScenario.get(), 1, 0);
		if (!TelemetryProbe::start_new_capture()) error_ = "Telemetry could not start. Check the log.";
	}
	bool start_stage_capture(double now) {
		if (road_gain() != sweep_.required_multiplier()) { error_ = "Restore the required Road Authority before capture."; return false; }
		Settings::TelemetryEnabled = true; Settings::TelemetryTestScenario = GuidedUat::Protocol;
		Settings::TelemetryNotes = std::format("Road Detail Calibration Sweep stage {} of 6; required x{}", sweep_.stage_index() + 1, sweep_.required_multiplier());
		Settings::TelemetryEnabled.notify(); Settings::TelemetryTestScenario.notify(); Settings::TelemetryNotes.notify();
		TelemetryProbe::set_research_context(GuidedUat::Protocol, std::format("Road calibration x{}", sweep_.required_multiplier()), unsigned(sweep_.stage_index() + 1), GuidedUat::CaptureSeconds);
		if (!TelemetryProbe::start_new_capture()) { error_ = "Guided capture could not start."; return false; }
		return sweep_.start_capture_now(road_gain(), now);
	}
	void stop_stage(double now) {
		sweep_.finish_capture(now); const auto& result = sweep_.results()[sweep_.stage_index()];
		TelemetryProbe::set_research_capture_status(result.configurationMismatch ? "configuration_mismatch" : "pending_assessment", result.measuredDuration);
		TelemetryProbe::stop_capture();
		TelemetryProbe::record_research_detail("UAT protocol", GuidedUat::Protocol);
		TelemetryProbe::record_research_detail("UAT stage", std::format("{} of 6", sweep_.stage_index() + 1));
		TelemetryProbe::record_research_detail("Required Road multiplier", std::format("x{}", result.requiredMultiplier));
		TelemetryProbe::record_research_detail("Actual Road multiplier", std::format("x{}", result.actualMultiplier));
		TelemetryProbe::record_research_detail("Configuration mismatch", result.configurationMismatch ? "true" : "false");
	}
	void cancel() { if (TelemetryProbe::snapshot().active) { TelemetryProbe::set_research_capture_status("cancelled", TelemetryProbe::capture_elapsed_seconds()); TelemetryProbe::stop_capture(); } TelemetryProbe::record_research_review("cancelled"); sweep_.cancel(); restore(); }

	void draw_regular() {
		const auto& t = TelemetryProbe::snapshot();
		ImGui::TextUnformatted("HYP36R TELEMETRY"); if (t.active) { ImGui::SameLine(); ImGui::TextColored({1,.25f,.2f,1}, "● REC"); }
		ImGui::Separator(); ImGui::TextUnformatted(car().c_str()); if (!stage().empty()) ImGui::TextUnformatted(stage().c_str()); ImGui::Spacing();
		if (t.active) { int s = int(TelemetryProbe::capture_elapsed_seconds()); row("Recording", std::format("{:02}:{:02}", s / 60, s % 60)); }
		else row("Ready", "");
		ImGui::Spacing(); configuration(); ImGui::Spacing();
		if (t.active) { if (ImGui::Button("Stop Capture")) { TelemetryProbe::set_research_capture_status("completed", TelemetryProbe::capture_elapsed_seconds()); TelemetryProbe::stop_capture(); } }
		else if (ImGui::Button("Start New Capture")) start_general();
	}
	void draw_assessment() {
		if (sweep_.current_mismatch()) ImGui::TextColored({1,.35f,.2f,1}, "Configuration mismatch recorded. Retry is recommended.");
		ImGui::TextUnformatted("How did Road Detail feel?");
		struct Choice { const char* name; GuidedUat::Assessment value; } choices[]{{"Too Shallow",GuidedUat::Assessment::TooShallow},{"Legible",GuidedUat::Assessment::Legible},{"Good",GuidedUat::Assessment::Good},{"Too Strong",GuidedUat::Assessment::TooStrong}};
		for (const auto& c : choices) { if (ImGui::Button(c.name)) { TelemetryProbe::record_research_detail("Subjective assessment", c.name); sweep_.assess(c.value); } ImGui::SameLine(); } ImGui::NewLine();
		if (ImGui::Button("Retry Stage")) { TelemetryProbe::record_research_review("retry"); sweep_.retry(); }
	}
	void draw_complete() {
		ImGui::TextUnformatted("ROAD DETAIL SWEEP COMPLETE");
		for (const auto& result : sweep_.results()) row(std::format("×{}", result.requiredMultiplier).c_str(), GuidedUat::assessment_name(result.assessment));
		ImGui::TextUnformatted("Which Road Detail did you prefer?");
		for (int gain : GuidedUat::RoadCalibrationStages) { if (ImGui::Button(std::format("×{}", gain).c_str())) sweep_.set_preference(gain); ImGui::SameLine(); } ImGui::NewLine();
		ImGui::BeginDisabled(sweep_.preferred_multiplier() == 0);
		if (ImGui::Button("Finish")) { TelemetryProbe::record_research_detail("Preferred Road multiplier", std::format("x{}", sweep_.preferred_multiplier())); sweep_.finish(); restore(); }
		ImGui::EndDisabled();
	}
	void draw_uat(double now) {
		sweep_.update(road_gain(), now); if (sweep_.capture_complete(now)) stop_stage(now);
		ImGui::TextUnformatted("HYP36R GUIDED UAT"); if (sweep_.phase() == GuidedUat::Phase::Recording) { ImGui::SameLine(); ImGui::TextColored({1,.25f,.2f,1}, "● REC"); }
		ImGui::TextUnformatted("Road Detail Calibration Sweep"); ImGui::Separator();
		if (sweep_.phase() == GuidedUat::Phase::Instructions) {
			ImGui::TextWrapped("Keep these settings fixed:"); configuration();
			if (!settings_ok()) ImGui::TextColored({1,.65f,.2f,1}, "Set Steering Load, Road Detail and FFB Strength to 100%%, and Road Mode to Enhanced.");
			ImGui::TextWrapped("Drive normally and include rough or textured road whenever possible. Change only Road Authority when asked.");
		}
		if (sweep_.phase() == GuidedUat::Phase::Instructions || sweep_.phase() == GuidedUat::Phase::WaitingForConfiguration) {
			ImGui::Text("STAGE %zu OF 6                 ×%d", sweep_.stage_index()+1, sweep_.required_multiplier());
			if (road_gain() != sweep_.required_multiplier() || !settings_ok()) { ImGui::Text("Set Road Detail Authority to ×%d", sweep_.required_multiplier()); row("Current", std::format("×{}", road_gain())); row("Required", std::format("×{}", sweep_.required_multiplier())); ImGui::TextDisabled("Waiting for the required configuration..."); }
			else if (ImGui::Button("Start Stage")) sweep_.begin_stage(road_gain(), true, now);
		} else if (sweep_.phase() == GuidedUat::Phase::Warmup) {
			ImGui::Text("STAGE %zu OF 6                 ×%d", sweep_.stage_index()+1, sweep_.required_multiplier()); ImGui::TextUnformatted("WARM UP"); ImGui::TextWrapped("Return to driving and get settled.");
			ImGui::Text("Measurement begins in: %.0f", std::ceil(sweep_.warmup_remaining(now))); row("Road Detail", std::format("×{} ✓", road_gain()));
			if (sweep_.warmup_remaining(now) <= 0.0) start_stage_capture(now); else if (ImGui::Button("Start Capture Now")) start_stage_capture(now);
		} else if (sweep_.phase() == GuidedUat::Phase::Recording) {
			double elapsed = sweep_.capture_elapsed(now); ImGui::Text("STAGE %zu OF 6                 ×%d", sweep_.stage_index()+1, sweep_.required_multiplier()); ImGui::TextWrapped("Drive normally. Include textured road where possible.");
			row("Road Detail", std::format("×{} {}", road_gain(), road_gain()==sweep_.required_multiplier()?"✓":"!")); ImGui::ProgressBar(float(elapsed/GuidedUat::CaptureSeconds), {-1,0}, std::format("{:.0f} / 36s", elapsed).c_str());
			if (sweep_.current_mismatch()) ImGui::TextColored({1,.35f,.2f,1}, "CONFIGURATION CHANGED — stage marked as a mismatch.");
		} else if (sweep_.phase() == GuidedUat::Phase::Assessment) draw_assessment(); else if (sweep_.phase() == GuidedUat::Phase::Complete) draw_complete();
		if (sweep_.active() && ImGui::Button("Cancel Test")) cancel();
	}
public:
	Kind kind() const override { return Kind::Hud; } const char* name() const override { return "HYP36Rforce Telemetry"; } int order() const override { return 15; } void init() override {}
	void render(bool) override {
		if (GuidedUat::consume_road_calibration_sweep_request()) { if (TelemetryProbe::snapshot().active) error_="Stop the current General Capture before starting Guided UAT."; else { remember(); sweep_.start(); error_.clear(); } }
		if (!Settings::TelemetryEnabled && !sweep_.active()) return;
		auto content=Overlay::content_rect(); ImGui::SetNextWindowPos({content.x+content.width-18,content.y+18},ImGuiCond_FirstUseEver,{1,0}); ImGui::SetNextWindowSizeConstraints({320,0},{440,620});
		if (!ImGui::Begin(sweep_.active()?"HYP36R Guided UAT":"HYP36R Telemetry",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoCollapse)) { ImGui::End(); return; }
		if (sweep_.active()) draw_uat(ImGui::GetTime()); else draw_regular(); if (!error_.empty()) ImGui::TextColored({1,.4f,.3f,1},"%s",error_.c_str()); ImGui::End();
	}
	static TelemetryOverlayWindow instance;
}; TelemetryOverlayWindow TelemetryOverlayWindow::instance;
