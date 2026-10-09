#include "hook_mgr.hpp"
#include "plugin.hpp"
#include "game_addrs.hpp"
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <format>
#include <string>
#include "car_identity.hpp"
#include "guided_uat.hpp"
#include "force_character_presentation.hpp"
#include "overlay.hpp"
#include "product_identity.hpp"
#include "road2_active.hpp"
#include "surface_renderer.hpp"
#include "telemetry_probe.hpp"
#include "wheel_force_feedback.hpp"

namespace Settings {
extern Setting<int> WheelFFBStrength, WheelFFBSteeringLoad, WheelFFBRoadDetail, WheelFFBImpactLevel, WheelFFBSurface;
extern Setting<bool> Road2ArcadeAuthority;
extern Setting<bool> SurfaceTextureCeilingOverride;
extern Setting<int> Road2DebugAuthorityGain;
extern Setting<std::string> RoadPresentationMode;
extern Setting<std::string> RoadRenderer, SurfaceWaveform, SurfaceFrequencyProfile, SurfacePreferredWaveform, SurfacePreferredFrequencyProfile;
extern Setting<int> SurfaceRendererStrength, SurfaceAmplitudeCeiling, SurfacePreferredAmplitude;
Setting<bool> TelemetryOverlayEnabled{ "Developer", "TelemetryOverlayEnabled", false,
	"Legacy compatibility setting; overlay visibility is automatic." };
namespace { struct HideLegacy { HideLegacy() { TelemetryOverlayEnabled.hidden(true); } } hideLegacy; }
}

namespace {
int road_gain() { return HYP36RRoad2Active::resolve_calibration_gain(Settings::Road2ArcadeAuthority.get(), Settings::Road2DebugAuthorityGain.get()); }
bool enhanced() { return HYP36RRoad2Active::mode_from_string(Settings::RoadPresentationMode.get()) == HYP36RRoad2Active::Mode::Experimental; }
int surface_ceiling() { return HYP36RSurfaceRenderer::resolve_amplitude_ceiling_percent(
	Settings::SurfaceTextureCeilingOverride.get(), Settings::SurfaceAmplitudeCeiling.get()); }
void row(const char* label, const std::string& value) { ImGui::TextUnformatted(label); ImGui::SameLine(190); ImGui::TextUnformatted(value.c_str()); }
std::string car() { const bool available = Game::is_in_game() && Game::pl_car(); return CarIdentity::overlay_label(available, available ? unsigned(Game::pl_car()->car_kind_11) : 0); }
std::string stage() { return Game::is_in_game() && Game::stg_stage_num ? Game::GetStageFriendlyName(*Game::stg_stage_num) : ""; }
}

class TelemetryOverlayWindow : public OverlayWindow {
	GuidedUat::RoadCalibrationSweep sweep_;
	GuidedUat::SurfaceSweep surfaceSweep_;
	int inheritedAmplitude_ = 18;
	HYP36RSurfaceRenderer::Waveform inheritedWaveform_ = HYP36RSurfaceRenderer::DefaultWaveform;
	std::string amplitudeSource_ = "fallback";
	std::string waveformSource_ = "fallback";
	bool oldEnabled_ = false;
	std::string oldScenario_, oldNotes_, error_;

	HYP36RForceCharacter::PlayerConfiguration user_configuration() const {
		return HYP36RForceCharacter::to_player_configuration(Settings::WheelFFBStrength.get(),
			Settings::WheelFFBSteeringLoad.get(), Settings::WheelFFBRoadDetail.get(),
			Settings::WheelFFBImpactLevel.get(), Settings::WheelFFBSurface.get(), enhanced());
	}
	bool settings_ok() const { const auto user = user_configuration(); return user.enhancedRoad && user.ffbStrengthPercent == 100 && user.steeringLoadPercent == 100 && user.roadDetailPercent == 100; }
	int surface_value() const {
		if (surfaceSweep_.protocol() == GuidedUat::SurfaceProtocol::Amplitude) return surface_ceiling();
		if (surfaceSweep_.protocol() == GuidedUat::SurfaceProtocol::Waveform) return int(HYP36RSurfaceRenderer::waveform_from_string(Settings::SurfaceWaveform.get()));
		return int(HYP36RSurfaceRenderer::frequency_profile_from_string(Settings::SurfaceFrequencyProfile.get()));
	}
	bool surface_constants_ok() const {
		const auto user = user_configuration();
		if (!user.enhancedRoad || user.ffbStrengthPercent != 100 || user.steeringLoadPercent != 100 || user.roadDetailPercent != 100 || user.impactPercent != 100 || user.surfacePercent != 100 || road_gain() != 30) return false;
		if (surfaceSweep_.protocol() == GuidedUat::SurfaceProtocol::Amplitude)
			return HYP36RSurfaceRenderer::waveform_from_string(Settings::SurfaceWaveform.get()) == HYP36RSurfaceRenderer::DefaultWaveform && HYP36RSurfaceRenderer::frequency_profile_from_string(Settings::SurfaceFrequencyProfile.get()) == HYP36RSurfaceRenderer::FrequencyProfile::Reference;
		if (surfaceSweep_.protocol() == GuidedUat::SurfaceProtocol::Waveform)
			return surface_ceiling() == inheritedAmplitude_ && HYP36RSurfaceRenderer::frequency_profile_from_string(Settings::SurfaceFrequencyProfile.get()) == HYP36RSurfaceRenderer::FrequencyProfile::Reference;
		return surface_ceiling() == inheritedAmplitude_ && HYP36RSurfaceRenderer::waveform_from_string(Settings::SurfaceWaveform.get()) == inheritedWaveform_;
	}
	void remember() { oldEnabled_ = Settings::TelemetryEnabled.get(); oldScenario_ = Settings::TelemetryTestScenario.get(); oldNotes_ = Settings::TelemetryNotes.get(); }
	void restore() {
		Settings::TelemetryEnabled = oldEnabled_; Settings::TelemetryTestScenario = oldScenario_; Settings::TelemetryNotes = oldNotes_;
		Settings::TelemetryEnabled.notify(); Settings::TelemetryTestScenario.notify(); Settings::TelemetryNotes.notify(); Settings::write(Module::UserIniPath);
	}
	void configuration() {
		const auto user = user_configuration();
		row("FFB Strength", std::format("{}%", user.ffbStrengthPercent));
		row("Steering Load", std::format("{}%", user.steeringLoadPercent));
		row("Impact", std::format("{}%", user.impactPercent)); ImGui::Spacing();
		row("Road Mode", user.enhancedRoad ? "Enhanced" : "Classic");
		row("Road Detail", std::format("{}%", user.roadDetailPercent));
		row("Road Calibration", user.enhancedRoad ? std::format("×{}", road_gain()) : "—");
		row("Surface", std::format("{}%", user.surfacePercent));
	}
	void start_general() {
		TelemetryProbe::clear_research_context();
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
	void cancel() { if (TelemetryProbe::snapshot().active) { TelemetryProbe::set_research_capture_status("cancelled", TelemetryProbe::capture_elapsed_seconds()); TelemetryProbe::stop_capture(); } TelemetryProbe::record_research_review("cancelled"); sweep_.cancel(); surfaceSweep_.cancel(); restore(); }

	void draw_regular() {
		const auto& t = TelemetryProbe::snapshot();
		ImGui::Text("HYP36R TELEMETRY • v%.*s", int(ProductIdentity::ReleaseVersion.size()), ProductIdentity::ReleaseVersion.data()); if (t.active) { ImGui::SameLine(); ImGui::TextColored({1,.25f,.2f,1}, "● REC"); }
		ImGui::Separator(); ImGui::TextWrapped("%s", car().c_str()); if (!stage().empty()) ImGui::TextUnformatted(stage().c_str()); ImGui::Spacing();
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
		ImGui::TextUnformatted("HYP36rforce FFB GUIDED UAT"); if (sweep_.phase() == GuidedUat::Phase::Recording) { ImGui::SameLine(); ImGui::TextColored({1,.25f,.2f,1}, "● REC"); }
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
	const char* surface_protocol_name() const {
		return surfaceSweep_.protocol() == GuidedUat::SurfaceProtocol::Amplitude ? "Surface — Amplitude"
			: surfaceSweep_.protocol() == GuidedUat::SurfaceProtocol::Waveform ? "Surface — Waveform" : "Surface — Frequency";
	}
	const char* surface_protocol_id() const {
		return surfaceSweep_.protocol() == GuidedUat::SurfaceProtocol::Amplitude ? "UAT_SURFACE_AMPLITUDE_V1"
			: surfaceSweep_.protocol() == GuidedUat::SurfaceProtocol::Waveform ? "UAT_SURFACE_WAVEFORM_V1" : "UAT_SURFACE_FREQUENCY_V1";
	}
	std::string surface_stage_label(int value) const {
		if (surfaceSweep_.protocol() == GuidedUat::SurfaceProtocol::Amplitude) return std::format("{}%", value);
		if (surfaceSweep_.protocol() == GuidedUat::SurfaceProtocol::Waveform) return HYP36RSurfaceRenderer::waveform_name(HYP36RSurfaceRenderer::Waveform(value));
		return HYP36RSurfaceRenderer::frequency_profile_name(HYP36RSurfaceRenderer::FrequencyProfile(value));
	}
	void begin_surface_uat(GuidedUat::SurfaceProtocol protocol) {
		remember();
		const auto amplitude = GuidedUat::resolve_amplitude_preference(Settings::SurfacePreferredAmplitude.get());
		inheritedAmplitude_ = amplitude.value; amplitudeSource_ = amplitude.inherited ? "preferred amplitude UAT" : "fallback";
		const bool waveformSet = Settings::SurfacePreferredWaveform.get() != "UNSET";
		const auto waveform = GuidedUat::resolve_waveform_preference(int(HYP36RSurfaceRenderer::waveform_from_string(Settings::SurfacePreferredWaveform.get())), waveformSet);
		inheritedWaveform_ = HYP36RSurfaceRenderer::Waveform(waveform.value); waveformSource_ = waveform.inherited ? "preferred waveform UAT" : "fallback";
		surfaceSweep_.start(protocol); error_.clear();
	}
	bool start_surface_capture(double now) {
		if (!surface_constants_ok() || surface_value() != surfaceSweep_.required_value()) { error_ = "Restore the required Surface configuration before capture."; return false; }
		Settings::TelemetryEnabled = true; Settings::TelemetryTestScenario = surface_protocol_id();
		Settings::TelemetryNotes = std::format("{} stage {} of {}; {}", surface_protocol_name(), surfaceSweep_.stage_index()+1, surfaceSweep_.stage_count(), surface_stage_label(surfaceSweep_.required_value()));
		Settings::TelemetryEnabled.notify(); Settings::TelemetryTestScenario.notify(); Settings::TelemetryNotes.notify();
		TelemetryProbe::set_research_context(surface_protocol_id(), surface_stage_label(surfaceSweep_.required_value()), unsigned(surfaceSweep_.stage_index()+1), GuidedUat::CaptureSeconds);
		if (!TelemetryProbe::start_new_capture()) { error_ = "Guided capture could not start."; return false; }
		TelemetryProbe::record_research_detail("Inherited amplitude", std::format("{}% ({})", inheritedAmplitude_, amplitudeSource_));
		TelemetryProbe::record_research_detail("Inherited waveform", std::format("{} ({})", HYP36RSurfaceRenderer::waveform_name(inheritedWaveform_), waveformSource_));
		return surfaceSweep_.start_capture_now(surface_value(), now);
	}
	void stop_surface_stage(double now) {
		surfaceSweep_.finish_capture(now);
		TelemetryProbe::set_research_capture_status(surfaceSweep_.current_mismatch()?"configuration_mismatch":"pending_assessment", GuidedUat::CaptureSeconds);
		TelemetryProbe::stop_capture();
		TelemetryProbe::record_research_detail("UAT protocol", surface_protocol_id());
		TelemetryProbe::record_research_detail("Required selection", surface_stage_label(surfaceSweep_.required_value()));
		TelemetryProbe::record_research_detail("Configuration mismatch", surfaceSweep_.current_mismatch()?"true":"false");
	}
	void persist_surface_preference() {
		const int value = surfaceSweep_.preferred_value();
		if (surfaceSweep_.protocol() == GuidedUat::SurfaceProtocol::Amplitude) Settings::SurfacePreferredAmplitude = value;
		else if (surfaceSweep_.protocol() == GuidedUat::SurfaceProtocol::Waveform) Settings::SurfacePreferredWaveform = value == 1 ? "TRIANGLE" : value == 2 ? "SQUARE" : "SINE";
		else Settings::SurfacePreferredFrequencyProfile = value == 0 ? "LOW" : value == 2 ? "MEDIUM" : value == 3 ? "HIGH" : "REFERENCE";
		Settings::write(Module::UserIniPath);
	}
	void draw_surface_uat(double now) {
		surfaceSweep_.update(surface_value(), surface_constants_ok(), now); if (surfaceSweep_.capture_complete(now)) stop_surface_stage(now);
		ImGui::TextUnformatted("HYP36rforce FFB GUIDED UAT"); if (surfaceSweep_.phase()==GuidedUat::Phase::Recording) { ImGui::SameLine(); ImGui::TextColored({1,.25f,.2f,1}, "● REC"); }
		ImGui::TextUnformatted(surface_protocol_name()); ImGui::Separator();
		if (surfaceSweep_.phase()==GuidedUat::Phase::Instructions) {
			ImGui::TextWrapped("Testing one Surface variable. Keep FFB, Steering, Impact, Road Detail and player Surface at 100%%; Enhanced x30.");
			row("Amplitude", std::format("{}% ({})", inheritedAmplitude_, amplitudeSource_)); row("Waveform", std::format("{} ({})", HYP36RSurfaceRenderer::waveform_name(inheritedWaveform_), waveformSource_));
		}
		if (surfaceSweep_.phase()==GuidedUat::Phase::Instructions || surfaceSweep_.phase()==GuidedUat::Phase::WaitingForConfiguration) {
			ImGui::Text("STAGE %zu OF %zu", surfaceSweep_.stage_index()+1, surfaceSweep_.stage_count()); row("Required", surface_stage_label(surfaceSweep_.required_value())); row("Current", surface_stage_label(surface_value()));
			if (!surface_constants_ok() || surface_value()!=surfaceSweep_.required_value()) ImGui::TextDisabled("Waiting for the required configuration...");
			else if (ImGui::Button("Start Stage")) surfaceSweep_.begin_stage(surface_value(), true, now);
		} else if (surfaceSweep_.phase()==GuidedUat::Phase::Warmup) {
			ImGui::TextUnformatted("WARM UP"); ImGui::Text("Measurement begins in: %.0f", std::ceil(surfaceSweep_.warmup_remaining(now))); row("Selection", surface_stage_label(surface_value()));
			if (surfaceSweep_.warmup_remaining(now)<=0.0) start_surface_capture(now); else if (ImGui::Button("Start Capture Now")) start_surface_capture(now);
		} else if (surfaceSweep_.phase()==GuidedUat::Phase::Recording) {
			const double elapsed=surfaceSweep_.capture_elapsed(now); row("Selection", surface_stage_label(surface_value())); ImGui::ProgressBar(float(elapsed/GuidedUat::CaptureSeconds),{-1,0},std::format("{:.0f} / 36s",elapsed).c_str());
			if (surfaceSweep_.current_mismatch()) ImGui::TextColored({1,.35f,.2f,1},"CONFIGURATION CHANGED — stage marked as a mismatch.");
		} else if (surfaceSweep_.phase()==GuidedUat::Phase::Assessment) {
			const char* const* labels; static const char* amplitude[]{"Too Shallow","Legible","Good","Too Strong"}; static const char* waveform[]{"Too Soft","Natural","Pronounced","Too Harsh"}; static const char* frequency[]{"Too Slow","Natural","Pronounced","Too Fine"};
			labels=surfaceSweep_.protocol()==GuidedUat::SurfaceProtocol::Amplitude?amplitude:surfaceSweep_.protocol()==GuidedUat::SurfaceProtocol::Waveform?waveform:frequency;
			for(int i=0;i<4;++i){ if(ImGui::Button(labels[i])){TelemetryProbe::record_research_detail("Subjective assessment",labels[i]);surfaceSweep_.assess(i+1);} ImGui::SameLine(); } ImGui::NewLine(); if(ImGui::Button("Retry Stage")) surfaceSweep_.retry();
		} else if (surfaceSweep_.phase()==GuidedUat::Phase::Complete) {
			ImGui::TextUnformatted("SURFACE SWEEP COMPLETE"); ImGui::TextUnformatted("Choose the preferred result:");
			for(std::size_t i=0;i<surfaceSweep_.stage_count();++i){ const int v=surfaceSweep_.protocol()==GuidedUat::SurfaceProtocol::Amplitude?GuidedUat::SurfaceAmplitudeStages[i]:surfaceSweep_.protocol()==GuidedUat::SurfaceProtocol::Waveform?GuidedUat::SurfaceWaveformStages[i]:GuidedUat::SurfaceFrequencyStages[i]; if(ImGui::Button(surface_stage_label(v).c_str()))surfaceSweep_.set_preference(v); ImGui::SameLine(); } ImGui::NewLine(); ImGui::BeginDisabled(surfaceSweep_.preferred_value()<0); if(ImGui::Button("Finish")){persist_surface_preference();surfaceSweep_.finish();restore();} ImGui::EndDisabled();
		}
		if(surfaceSweep_.active()&&ImGui::Button("Cancel Test"))cancel();
	}
public:
	Kind kind() const override { return Kind::Hud; } const char* name() const override { return "HYP36rforce FFB Telemetry"; } int order() const override { return 15; } void init() override {}
	void render(bool) override {
		if (GuidedUat::consume_road_calibration_sweep_request()) { if (TelemetryProbe::snapshot().active) error_="Stop the current General Capture before starting Guided UAT."; else { remember(); sweep_.start(); error_.clear(); } }
		const auto surfaceRequest=GuidedUat::consume_surface_sweep_request(); if(surfaceRequest!=GuidedUat::SurfaceProtocol::None){if(TelemetryProbe::snapshot().active)error_="Stop the current General Capture before starting Guided UAT.";else begin_surface_uat(surfaceRequest);}
		if (!Settings::TelemetryEnabled && !sweep_.active() && !surfaceSweep_.active()) return;
		auto content=Overlay::content_rect(); ImGui::SetNextWindowPos({content.x+content.width-18,content.y+18},ImGuiCond_FirstUseEver,{1,0}); ImGui::SetNextWindowSizeConstraints({320,0},{440,620});
		if (!ImGui::Begin((sweep_.active()||surfaceSweep_.active())?"HYP36rforce FFB Guided UAT":"HYP36rforce FFB Telemetry",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoCollapse)) { ImGui::End(); return; }
		if (sweep_.active()) draw_uat(ImGui::GetTime()); else if(surfaceSweep_.active()) draw_surface_uat(ImGui::GetTime()); else draw_regular(); if (!error_.empty()) ImGui::TextColored({1,.4f,.3f,1},"%s",error_.c_str()); ImGui::End();
	}
	static TelemetryOverlayWindow instance;
}; TelemetryOverlayWindow TelemetryOverlayWindow::instance;
