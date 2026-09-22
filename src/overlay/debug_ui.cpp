#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "hook_mgr.hpp"
#include "plugin.hpp"
#include "game_addrs.hpp"
#include "interpolation.hpp"
#include <cmath>
#include <imgui.h>
#include "overlay.hpp"
#include "telemetry_probe.hpp"
#include "native_unlock.hpp"
#include <filesystem>
#include <fstream>
#include <string>

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

	static void draw_e3b_clone_test()
	{
		if (!E2ManagedTestActive()) return;
		ImGui::SeparatorText("E3B Developer Test (MANAGED_TEST only)");
		ImGui::TextWrapped("Save to Profile first. Queue a clone, then exit OutRun normally. "
			"The copy runs only after the game process closes; relaunch to select the new native slot.");
		const int active = *Module::exe_ptr<int>(0x3B17F8);
		ImGui::Text("Current native licence: %s", active >= 0 && active < 4
			? std::to_string(active + 1).c_str() : "none");
		const auto root = Module::ExePath.parent_path() / "_E2ManagedTest" / "SaveGame";
		const auto statusPath = root.parent_path() / "MultiInput" / "ExperienceLicences" / "clone-run88-status.txt";
		static std::string message;
		static bool queuedThisRun = false;
		ImGui::BeginDisabled(active < 0 || active > 3 || queuedThisRun);
		if (ImGui::Button("Clone Active Licence to Free Slot (after exit)"))
		{
			const auto helper = Module::DllPath.parent_path() / "experience_clone_helper.exe";
			std::error_code fileError;
			if (!std::filesystem::is_regular_file(helper, fileError))
				message = "Clone helper is missing beside dinput8.dll.";
			else
			{
				std::wstring command = L"\"" + helper.wstring() + L"\" \"" +
					Module::ExePath.wstring() + L"\" \"" + root.wstring() + L"\" " +
					std::to_wstring(GetCurrentProcessId()) + L" " + std::to_wstring(active);
				STARTUPINFOW startup{ sizeof(startup) };
				PROCESS_INFORMATION process{};
				if (!CreateProcessW(helper.c_str(), command.data(), nullptr, nullptr, FALSE,
					CREATE_NO_WINDOW, nullptr, Module::ExePath.parent_path().c_str(), &startup, &process))
					message = "Could not start the E3B clone helper. No clone was attempted.";
				else
				{
					CloseHandle(process.hThread);
					CloseHandle(process.hProcess);
					queuedThisRun = true;
				message = "Queued. Exit the game normally, then relaunch and read the result below.";
					std::error_code folderError;
					std::filesystem::create_directories(statusPath.parent_path(), folderError);
					if (!folderError)
					{
						std::ofstream pending(statusPath, std::ios::binary | std::ios::trunc);
						pending << "E3B Run #88 pending: exit the game to run the disposable clone.\n";
					}
					spdlog::info("E3B Run #88 clone queued: sourceSlot={}, MANAGED_TEST", active + 1);
				}
			}
		}
		ImGui::EndDisabled();
		if (std::ifstream input(statusPath, std::ios::binary); input)
		{
			std::string line;
			std::getline(input, line);
			if (!line.empty()) message = line;
		}
		if (!message.empty()) ImGui::TextWrapped("%s", message.c_str());
	}

	static void draw_e3c_unlock_test()
	{
		if (!Settings::E3CNativeUnlockUAT) return;
		ImGui::SeparatorText("E3C Developer Test");
		ImGui::TextWrapped("Use a disposable selected profile. This changes only the current in-memory licence. "
			"OutRun's Save to Profile is a separate, optional player action.");
		static NativeUnlock::Result result;
		static bool attempted = false;
		if (ImGui::Button("Invoke Native Unlock All"))
		{
			result = NativeUnlock::UnlockAllContent();
			attempted = true;
			spdlog::info("E3C native unlock: activeLicence={}, supportedExe={}, nativeInvoked={}, "
				"verified={}, result={}", result.activeLicence,
				result.supportedExecutable, result.invoked, result.verified,
				NativeUnlock::CodeName(result.code));
		}
		if (attempted)
			ImGui::TextWrapped("Result: %s | Native invocation: %s | Transformation verified: %s",
				NativeUnlock::CodeName(result.code), result.invoked ? "yes" : "no",
				result.verified ? "yes" : "no");
	}

	static void draw_ffb_telemetry()
	{
		const auto& telemetry = TelemetryProbe::snapshot();
		if (ImGui::Button("Start New Capture"))
			TelemetryProbe::start_new_capture();
		ImGui::SameLine();
		if (ImGui::Button("Stop Capture"))
			TelemetryProbe::stop_capture();

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

		draw_e3b_clone_test();
		draw_e3c_unlock_test();

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
