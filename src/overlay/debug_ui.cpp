#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "hook_mgr.hpp"
#include "plugin.hpp"
#include "game_addrs.hpp"
#include "interpolation.hpp"
#include <array>
#include <cmath>
#include <imgui.h>
#include "overlay.hpp"
#include "research_scenario_runner.hpp"
#include "telemetry_probe.hpp"

namespace
{
	constexpr const char* ResearchCampaign = "R2-B — Surface / Road";
	constexpr const char* ResearchCampaignTitle = "R2-B — Surface / Road";
	HYP36RResearchRunner::Runner researchRunner{ HYP36RResearchRunner::R2BScenarios };
	std::string researchRunnerError;

	void update_research_runner(double now)
	{
		using namespace HYP36RResearchRunner;
		const Action action = researchRunner.update(now);
		if (action == Action::StartCapture)
		{
			const Scenario& scenario = researchRunner.scenario();
			Settings::TelemetryTestScenario = scenario.id;
			Settings::TelemetryNotes = scenario.notes;
			TelemetryProbe::set_research_context(ResearchCampaign, scenario.name,
				researchRunner.attempt(), scenario.durationSeconds);
			if (!TelemetryProbe::start_new_capture())
			{
				researchRunner.cancel(false);
				researchRunnerError = "Telemetry could not start. Check the log.";
			}
			else
				researchRunnerError.clear();
		}
		else if (action == Action::StopCapture)
		{
			TelemetryProbe::set_research_capture_status("pending_review",
				researchRunner.actual_duration());
			TelemetryProbe::stop_capture();
		}
	}
}

// Debug tab: game state readout, the switches for the free-floating tool
// windows, and whether each hook managed to apply.
class DebugWindow : public OverlayWindow
{
	static void draw_game_state()
	{
		uint8_t* frontEndData = *Module::exe_ptr<uint8_t*>(0x3B17E8);
		int frontEndStep = *(int*)frontEndData;
		int frontEndEvtStep = *(int*)(frontEndData + 0x218);
		char frontEndMenuLevel = *(char*)(frontEndData + 0x220);

		EVWORK_CAR* car = Game::pl_car();

		ImGui::Text("game_mode: %d", *Game::game_mode);
		ImGui::Text("current_mode: %d", *Game::current_mode);
		ImGui::Text("Frontend step indexes: %d / %d / %d", frontEndStep, frontEndEvtStep, int(frontEndMenuLevel));
		ImGui::Text("Lobby is active: %d", (*Game::SumoNet_CurNetDriver && (*Game::SumoNet_CurNetDriver)->is_in_lobby()));
		ImGui::Text("Lobby is host: %d", (*Game::SumoNet_CurNetDriver && (*Game::SumoNet_CurNetDriver)->is_hosting()));
		ImGui::Text("Is MP gamemode: %d", (*Game::game_mode == 3 || *Game::game_mode == 4));
		ImGui::Text("Car kind: %d", int(car->car_kind_11));
		ImGui::Text("Car position: %.3f %.3f %.3f", car->position_14.x, car->position_14.y, car->position_14.z);
		ImGui::Text("OnRoadPlace coli %d, stg %d, section %d",
			car->OnRoadPlace_5C.loadColiType_0,
			car->OnRoadPlace_5C.curStageIdx_C,
			car->OnRoadPlace_5C.roadSectionNum_8);

		GameStage cur_stage_num = *Game::stg_stage_num;
		ImGui::Text("Loaded Stage: %d (%s / %s)", cur_stage_num,
			Game::GetStageFriendlyName(cur_stage_num), Game::GetStageUniqueName(cur_stage_num));
	}

	// Each interpolated subsystem can be switched off on its own so an artifact
	// can be narrowed to whichever one produces it.
	static void draw_interpolation()
	{
		auto& d = Interp::Debug;

		ImGui::Checkbox("Cars", &d.doCars);
		ImGui::SameLine(); ImGui::Checkbox("Camera", &d.doCamera);
		ImGui::SameLine(); ImGui::Checkbox("Particles", &d.doParticles);
		ImGui::Checkbox("Connections", &d.doConnections);
		ImGui::SameLine(); ImGui::Checkbox("Hearts", &d.doHearts);
		ImGui::SameLine(); ImGui::Checkbox("Stage scale", &d.doStageScale);

		ImGui::Checkbox("Shift particles along car display lag (off = particle velocity)",
			&d.particleUseDispLag);
		ImGui::SliderFloat("Particle shift x", &d.particleOffsetScale, 0.0f, 20.0f, "%.1f");

#ifdef _DEBUG
		ImGui::Text("alpha %.4f (effective %.4f)", d.alpha, d.effectiveAlpha);

		// An effective alpha of 1.0 makes every interpolator a no-op, which
		// otherwise looks the same as a hook that never ran.
		if (d.effectiveAlpha >= 1.0f)
			ImGui::TextColored(ImVec4(1, 0.7f, 0.2f, 1), "effective alpha 1.0, nothing to interpolate");

		ImGui::Text("Cars replayed: %d", d.carsReplayed);
		ImGui::Text("Particles: %d sources, %d shifted, last %.4f, car lag %.4f",
			d.particleSourcesSeen, d.particlesMoved, d.particleLastShift, d.carDispLag);

		// matrix_B0 holds the transform the car is drawn with, position_14 the
		// tick position the particles were emitted against, so the gap between
		// them is the shift particles need. Applied should match it.
		if (EVWORK_CAR* plc = Game::pl_car())
		{
			const float nx = plc->matrix_B0._41 - plc->position_14.x;
			const float ny = plc->matrix_B0._42 - plc->position_14.y;
			const float nz = plc->matrix_B0._43 - plc->position_14.z;
			ImGui::Text("Shift needed %.4f, applied %.4f",
				sqrtf(nx * nx + ny * ny + nz * nz), d.particleLastShift);
		}
#endif
	}

	// These write the game's own variables rather than any of our settings, so
	// they aren't part of the generated settings tab.
	static void draw_gameplay_toggles()
	{
		extern bool EnablePauseMenu;

		ImGui::Checkbox("Countdown timer enabled", Game::Sumo_CountdownTimerEnable);
		ImGui::Checkbox("Pause menu enabled", &EnablePauseMenu);
		ImGui::Checkbox("HUD enabled", (bool*)Game::navipub_disp_flg);
	}

	static void draw_ffb_telemetry()
	{
		const auto& telemetry = TelemetryProbe::snapshot();
		using namespace HYP36RResearchRunner;
		const Scenario& scenario = researchRunner.scenario();
		ImGui::SeparatorText(ResearchCampaignTitle);
		ImGui::Text("Scenario: %s", scenario.name);
		ImGui::Text("Attempt: %u", researchRunner.attempt());
		const char* status = "READY";
		switch (researchRunner.phase())
		{
		case Phase::Countdown: status = "COUNTDOWN"; break;
		case Phase::Capturing: status = "RECORDING"; break;
		case Phase::Review: status = "CAPTURE COMPLETE"; break;
		case Phase::Finished: status = "CAMPAIGN COMPLETE"; break;
		default: break;
		}
		ImGui::Text("Status: %s", status);

		if (researchRunner.phase() == Phase::Ready)
		{
			if (ImGui::Button("Start Test"))
				researchRunner.start(ImGui::GetTime());
		}
		else if (researchRunner.phase() == Phase::Countdown ||
			researchRunner.phase() == Phase::Capturing)
		{
			if (ImGui::Button("Cancel Test"))
			{
				const bool wasCapturing = researchRunner.phase() == Phase::Capturing;
				const double actual = researchRunner.actual_duration();
				if (researchRunner.cancel(wasCapturing) && wasCapturing)
				{
					TelemetryProbe::set_research_capture_status("cancelled", actual);
					TelemetryProbe::stop_capture();
				}
			}
		}
		else if (researchRunner.phase() == Phase::Review)
		{
			if (ImGui::Button("Accept"))
			{
				TelemetryProbe::record_research_review("accepted");
				researchRunner.accept();
			}
			ImGui::SameLine();
			if (ImGui::Button("Retry"))
			{
				TelemetryProbe::record_research_review("retry");
				researchRunner.retry(ImGui::GetTime());
			}
		}

		for (std::size_t index = 0; index < researchRunner.scenarios().size(); ++index)
		{
			const char marker = researchRunner.phase() == Phase::Finished ||
				index < researchRunner.scenario_index() ? '+' :
				(index == researchRunner.scenario_index() ? '>' : ' ');
			ImGui::Text("%c %s", marker, researchRunner.scenarios()[index].name);
		}
		if (!researchRunnerError.empty())
			ImGui::TextColored(ImVec4(1.f, 0.4f, 0.3f, 1.f), "%s", researchRunnerError.c_str());

		ImGui::Text("Telemetry: %s", telemetry.active ? "Recording" : "Stopped");
		ImGui::Text("Scenario: %s", telemetry.testScenario.empty()
			? "(none)" : telemetry.testScenario.c_str());
		ImGui::Text("Samples: %llu", static_cast<unsigned long long>(telemetry.frameIndex));
		ImGui::Text("File: %s", telemetry.currentFilename.empty()
			? "(none)" : telemetry.currentFilename.c_str());
		ImGui::Text("Speed: %.5f", telemetry.speed);
		ImGui::Text("Steering: %.5f", telemetry.steeringInput);
		if (telemetry.xForceAvailable)
			ImGui::Text("XForce: %.5f", telemetry.xForce);
		else
			ImGui::TextDisabled("XForce: unavailable");

		ImGui::SeparatorText("Surface (raw index)");
		for (size_t i = 0; i < telemetry.surfaceRaw.size(); ++i)
			ImGui::Text("%zu  0x%08X", i, telemetry.surfaceRaw[i]);

		ImGui::SeparatorText("Native candidates");
		static constexpr const char* candidateLabels[]{ "1D0", "1D4", "1DC", "1E0", "1E4", "264", "268" };
		for (size_t i = 0; i < telemetry.nativeCandidates.size(); ++i)
			ImGui::Text("%s: %.6f", candidateLabels[i], telemetry.nativeCandidates[i]);

		ImGui::SeparatorText("Steering-response candidates");
		ImGui::Text("D38: %.6f", telemetry.steeringResponse.candidateD38);
		ImGui::Text("D3C: %.6f", telemetry.steeringResponse.candidateD3C);
		ImGui::Text("D40: %.6f", telemetry.steeringResponse.candidateD40);
		ImGui::Text("D44: %d", telemetry.steeringResponse.candidateD44);
		ImGui::Text("D46: %d", telemetry.steeringResponse.candidateD46);
		ImGui::Text("D48: %d", telemetry.steeringResponse.candidateD48);

		ImGui::SeparatorText("HYP36R vehicle state (passive)");
		const auto& vehicleState = telemetry.vehicleState.current;
		ImGui::Text("Validity: %s", HYP36RVehicleState::validity_name(vehicleState.validity));
		ImGui::Text("Reference: %.6f rad", vehicleState.steeringReferenceAngleRad);
		if (vehicleState.responseAngleValid)
			ImGui::Text("Response: %.6f rad", vehicleState.responseAngleRad);
		else
			ImGui::TextDisabled("Response: suppressed/unavailable");
		if (vehicleState.responseRateValid)
			ImGui::Text("Response rate: %.6f rad/s", vehicleState.responseAngularRateRadPerSec);
		else
			ImGui::TextDisabled("Response rate: timing unavailable");
		if (vehicleState.referenceResponseErrorValid)
			ImGui::Text("Reference/response error: %.6f rad", vehicleState.referenceResponseErrorRad);
		else
			ImGui::TextDisabled("Reference/response error: suppressed/unavailable");
		if (vehicleState.correctedReferenceValid)
			ImGui::Text("Corrected reference: %.6f rad", vehicleState.correctedReferenceAngleRad);
		else
			ImGui::TextDisabled("Corrected reference: suppressed/unavailable");
		ImGui::Text("Authority: %.6f", vehicleState.responseAuthority);
		ImGui::Text("Overshoot attenuation: %.6f", vehicleState.overshootAttenuation);
		ImGui::Text("Transition frames: %u", vehicleState.transitionFramesRemaining);
		ImGui::Text("Last valid age: %.3f s", vehicleState.lastValidStateAgeSeconds);

		ImGui::SeparatorText("FFB");
		if (telemetry.ffbAvailable)
		{
			ImGui::Text("Raw: %.5f", telemetry.ffbRaw);
			ImGui::Text("Final requested: %.5f", telemetry.ffbFinal);
			ImGui::Text("Master: %.3f", telemetry.ffbMasterStrength);
		}
		else
			ImGui::TextDisabled("Waiting for FFB output");
	}

	static void draw_tools()
	{
		for (OverlayWindow* window : Overlay::windows())
		{
			if (window->kind() != Kind::Tool)
				continue;
#ifndef _DEBUG
			if (window->debug_only())
				continue;
#endif
			ImGui::Checkbox(window->name(), &window->visible);
		}

		if (ImGui::Button("Open Binding Dialog"))
			Overlay::IsBindingDialogActive = true;
	}

	// A hook with no description is one that never logs either, so there is
	// nothing useful to show for it.
	static void draw_hook_status()
	{
		if (!ImGui::BeginTable("##hooks", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
			return;

		ImGui::TableSetupColumn("Hook");
		ImGui::TableSetupColumn("State");
		ImGui::TableHeadersRow();

		for (Hook* hook : HookManager::hooks())
		{
			if (!hook || hook->description().empty())
				continue;

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(hook->description().data(),
				hook->description().data() + hook->description().size());

			ImGui::TableNextColumn();
			if (hook->active())
				ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.f), "applied");
			else if (hook->error())
				ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.4f, 1.f), "failed");
			else
				ImGui::TextDisabled("off");
		}

		ImGui::EndTable();
	}

public:
	Kind kind() const override { return Kind::Tab; }
	const char* name() const override { return "Debug"; }
	int order() const override { return 90; }

	void init() override {}

	void render(bool overlayEnabled) override
	{
		if (ImGui::CollapsingHeader("Game state", ImGuiTreeNodeFlags_DefaultOpen))
			draw_game_state();

#ifdef _DEBUG
		if (ImGui::CollapsingHeader("Interpolation", ImGuiTreeNodeFlags_DefaultOpen))
			draw_interpolation();
#endif

		if (ImGui::CollapsingHeader("Gameplay", ImGuiTreeNodeFlags_DefaultOpen))
			draw_gameplay_toggles();

		if (Settings::TelemetryEnabled && ImGui::CollapsingHeader("FFB Telemetry", ImGuiTreeNodeFlags_DefaultOpen))
			draw_ffb_telemetry();

		if (ImGui::CollapsingHeader("Tools", ImGuiTreeNodeFlags_DefaultOpen))
			draw_tools();

		if (ImGui::CollapsingHeader("Hooks"))
			draw_hook_status();
	}

	static DebugWindow instance;
};
DebugWindow DebugWindow::instance;

class ResearchRunnerHud : public OverlayWindow
{
public:
	Kind kind() const override { return Kind::Hud; }
	const char* name() const override { return "R2-B Research Runner"; }
	int order() const override { return 95; }
	bool debug_only() const override { return true; }
	void init() override {}

	void render(bool) override
	{
		using namespace HYP36RResearchRunner;
		const double now = ImGui::GetTime();
		update_research_runner(now);
		if (researchRunner.phase() != Phase::Countdown &&
			researchRunner.phase() != Phase::Capturing)
			return;

		ImGui::SetNextWindowBgAlpha(0.82f);
		ImGui::SetNextWindowPos(ImVec2(20.f, 20.f), ImGuiCond_Always);
		ImGui::Begin("R2-B Research Capture", nullptr,
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav |
			ImGuiWindowFlags_NoSavedSettings);
		const Scenario& scenario = researchRunner.scenario();
		ImGui::Text("%s", scenario.id);
		ImGui::Text("%s", scenario.name);
		if (researchRunner.phase() == Phase::Countdown)
		{
			const double remaining = 3.0 - researchRunner.phase_elapsed(now);
			const int count = remaining > 2.0 ? 3 : (remaining > 1.0 ? 2 : 1);
			ImGui::Text("STARTING IN %d", count);
		}
		else
		{
			if (researchRunner.actual_duration() < 0.75)
				ImGui::Text("CAPTURE");
			ImGui::Text("RECORDING  %.1f / %.0f sec",
				researchRunner.actual_duration(), scenario.durationSeconds);
			ImGui::Text("%s", scenario.instruction);
			ImGui::TextDisabled("%s", scenario.avoidance);
		}
		ImGui::End();
	}

	static ResearchRunnerHud instance;
};
ResearchRunnerHud ResearchRunnerHud::instance;
