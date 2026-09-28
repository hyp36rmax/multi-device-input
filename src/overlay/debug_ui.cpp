#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "hook_mgr.hpp"
#include "audio_sync.hpp"
#include "plugin.hpp"
#include "game_addrs.hpp"
#include "interpolation.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <imgui.h>
#include "overlay.hpp"
#include "product_identity.hpp"
#include "road2_active.hpp"
#include "road2_policy.hpp"
#include "signal_state.hpp"
#include "sound_request_trace.hpp"
#include "telemetry_probe.hpp"

namespace Settings
{
	extern Setting<int> WheelFFBSteeringLoad;
	extern Setting<int> WheelFFBRoadDetail;
	extern Setting<int> WheelFFBImpactLevel;
	extern Setting<bool> Road2ArcadeAuthority;
	extern Setting<bool> TelemetryOverlayEnabled;
}

namespace
{
	enum class AlphaSessionPhase { Ready, Recording, Review };
	AlphaSessionPhase alphaSessionPhase = AlphaSessionPhase::Ready;
	std::string alphaSessionError;
	double alphaSessionStarted = 0.0;
	unsigned alphaSessionAttempt = 1;
	enum class AudioSyncPhase { Ready, Recording, Review };
	AudioSyncPhase audioSyncPhase = AudioSyncPhase::Ready;
	double audioSyncStarted = 0.0;
	std::string audioSyncError;
	std::string soundTraceError;

	void persist_setting(Settings::SettingBase& setting)
	{
		setting.notify();
		Settings::write(Module::UserIniPath);
	}

	void set_force_character(Settings::Setting<int>& setting, int value, int maximum)
	{
		setting = (std::clamp)(value, 0, maximum);
		persist_setting(setting);
	}

	void reset_reference_plus()
	{
		Settings::WheelFFBSteeringLoad = 100;
		Settings::WheelFFBRoadDetail = 100;
		Settings::WheelFFBImpactLevel = 100;
		Settings::WheelFFBSteeringLoad.notify();
		Settings::WheelFFBRoadDetail.notify();
		Settings::WheelFFBImpactLevel.notify();
		Settings::write(Module::UserIniPath);
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
		ImGui::SeparatorText("HYP36R Force v1 — Development");
		ImGui::Text("HYP36R Force %.*s", int(ProductIdentity::Version.size()), ProductIdentity::Version.data());
		ImGui::TextWrapped("Long-form Force Character wheel UAT. Keep one setting stable per session.");
		auto draw_gain = [](const char* label, Settings::Setting<int>& setting,
			int maximum, int step)
		{
			ImGui::PushID(label);
			ImGui::TextUnformatted(label);
			ImGui::SameLine();
			if (setting.get() == 100)
				ImGui::Text("1.00x — Recommended");
			else
				ImGui::Text("%.2fx", setting.get() / 100.0f);
			ImGui::BeginDisabled(alphaSessionPhase == AlphaSessionPhase::Recording || setting.get() <= 0);
			if (ImGui::Button("-")) set_force_character(setting, setting.get() - step, maximum);
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::BeginDisabled(alphaSessionPhase == AlphaSessionPhase::Recording || setting.get() >= maximum);
			if (ImGui::Button("+")) set_force_character(setting, setting.get() + step, maximum);
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::TextDisabled("0.00x–%.2fx, %.2fx steps", maximum / 100.0f, step / 100.0f);
			ImGui::PopID();
		};
		draw_gain("Steering Load", Settings::WheelFFBSteeringLoad, 130, 5);
		draw_gain("Road Detail", Settings::WheelFFBRoadDetail, 200, 10);
		draw_gain("Impact", Settings::WheelFFBImpactLevel, 150, 5);
		ImGui::BeginDisabled(alphaSessionPhase == AlphaSessionPhase::Recording);
		if (ImGui::Button("RESET TO REFERENCE+")) reset_reference_plus();
		ImGui::EndDisabled();

		const char* status = alphaSessionPhase == AlphaSessionPhase::Ready ? "READY" :
			(alphaSessionPhase == AlphaSessionPhase::Recording ? "RECORDING" : "REVIEW");
		ImGui::Text("Session %u: %s", alphaSessionAttempt, status);
		if (alphaSessionPhase == AlphaSessionPhase::Ready)
		{
			if (ImGui::Button("Start Session"))
			{
				Settings::TelemetryEnabled = true;
				Settings::TelemetryTestScenario = "HYP36R_2_ALPHA1_FORCE_CHARACTER";
				Settings::TelemetryNotes = "Long-form physical wheel Force Character UAT";
				Settings::TelemetryEnabled.notify();
				Settings::TelemetryTestScenario.notify();
				Settings::TelemetryNotes.notify();
				Settings::write(Module::UserIniPath);
				TelemetryProbe::set_research_context("HYP36R Force 2.0 — Alpha 1",
					"Extended Force Character Wheel UAT", alphaSessionAttempt, 0.0);
				if (TelemetryProbe::start_new_capture())
				{
					alphaSessionStarted = ImGui::GetTime();
					alphaSessionPhase = AlphaSessionPhase::Recording;
					alphaSessionError.clear();
				}
				else alphaSessionError = "Telemetry could not start. Check the log.";
			}
		}
		else if (alphaSessionPhase == AlphaSessionPhase::Recording)
		{
			if (ImGui::Button("Finish Session"))
			{
				TelemetryProbe::set_research_capture_status("pending_review",
					ImGui::GetTime() - alphaSessionStarted);
				TelemetryProbe::stop_capture();
				alphaSessionPhase = AlphaSessionPhase::Review;
			}
		}
		else
		{
			if (ImGui::Button("Accept"))
			{
				TelemetryProbe::record_research_review("accepted");
				++alphaSessionAttempt;
				alphaSessionPhase = AlphaSessionPhase::Ready;
			}
			ImGui::SameLine();
			if (ImGui::Button("Retry"))
			{
				TelemetryProbe::record_research_review("retry");
				++alphaSessionAttempt;
				alphaSessionPhase = AlphaSessionPhase::Ready;
			}
		}
		if (!alphaSessionError.empty())
			ImGui::TextColored(ImVec4(1.f, 0.4f, 0.3f, 1.f), "%s", alphaSessionError.c_str());

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

	static void draw_audio_sync_validation()
	{
		constexpr double ValidationSeconds = 10.0;
		const auto& telemetry = TelemetryProbe::snapshot();
		ImGui::SeparatorText("R4.2F-T3S Audio Sync Validation");
		ImGui::TextWrapped("Stationary research instrumentation test. Record application audio digitally; no driving is required.");
		const char* phase = audioSyncPhase == AudioSyncPhase::Ready ? "READY" :
			(audioSyncPhase == AudioSyncPhase::Recording ? "RECORDING" : "COMPLETE");
		ImGui::Text("Status: %s", phase);

		if (audioSyncPhase == AudioSyncPhase::Ready)
		{
			if (ImGui::Button("Start 10-second sync validation"))
			{
				Settings::TelemetryEnabled = true;
				Settings::TelemetryTestScenario = "R4_2FT3S_SYNC_VALIDATION";
				Settings::TelemetryNotes = "Stationary audio and telemetry synchronization validation";
				TelemetryProbe::set_research_context("R4.2F-T3S — Audio Sync",
					"Stationary Sync Validation", 1, ValidationSeconds);
				if (!TelemetryProbe::start_new_capture())
					audioSyncError = "Telemetry could not start.";
				else if (!AudioSync::begin_session(Settings::TelemetryTestScenario.get(),
					TelemetryProbe::snapshot().currentFilename))
				{
					TelemetryProbe::set_research_capture_status("cancelled", 0.0);
					TelemetryProbe::stop_capture();
					audioSyncError = "Audio sync sidecar could not start.";
				}
				else
				{
					AudioSync::emit_marker(AudioSync::Marker::Start, telemetry.frameIndex,
						TelemetryProbe::capture_elapsed_seconds());
					audioSyncStarted = ImGui::GetTime();
					audioSyncPhase = AudioSyncPhase::Recording;
					audioSyncError.clear();
				}
			}
		}
		else if (audioSyncPhase == AudioSyncPhase::Recording)
		{
			const double elapsed = ImGui::GetTime() - audioSyncStarted;
			ImGui::Text("%.1f / %.0f seconds", elapsed, ValidationSeconds);
			if (elapsed >= ValidationSeconds)
			{
				AudioSync::emit_marker(AudioSync::Marker::End, telemetry.frameIndex,
					TelemetryProbe::capture_elapsed_seconds());
				AudioSync::finish_session("completed");
				TelemetryProbe::set_research_capture_status("pending_review", elapsed);
				TelemetryProbe::stop_capture();
				audioSyncPhase = AudioSyncPhase::Review;
			}
			else if (ImGui::Button("Cancel sync validation"))
			{
				AudioSync::finish_session("cancelled");
				TelemetryProbe::set_research_capture_status("cancelled", elapsed);
				TelemetryProbe::stop_capture();
				audioSyncPhase = AudioSyncPhase::Ready;
			}
		}
		else if (ImGui::Button("Reset sync validation"))
			audioSyncPhase = AudioSyncPhase::Ready;

		if (!AudioSync::status_text().empty()) ImGui::Text("Marker: %s", AudioSync::status_text().c_str());
		if (!audioSyncError.empty()) ImGui::TextColored(ImVec4(1.f, .4f, .3f, 1.f), "%s", audioSyncError.c_str());
	}

	static void draw_sound_request_ownership()
	{
		const auto& telemetry = TelemetryProbe::snapshot();
		ImGui::SeparatorText("R4.2F-T6 Sound Request Ownership");
		ImGui::TextWrapped("Passive 45-second research capture. Start before Tulip Garden, drive through the cobblestones, then finish after returning to normal road.");
		ImGui::Text("Status: %s", SoundRequestTrace::status_text().empty()
			? "ready" : SoundRequestTrace::status_text().c_str());
		ImGui::Text("Records: %zu / %zu  Dropped: %zu", SoundRequestTrace::record_count(),
			SoundRequestTrace::MaximumRecords, SoundRequestTrace::dropped_count());

		if (!SoundRequestTrace::recording() && !SoundRequestTrace::completed())
		{
			if (ImGui::Button("Start Sound Request Ownership"))
			{
				Settings::TelemetryEnabled = true;
				Settings::TelemetryTestScenario = "R4_2FT6_SOUND_REQUEST_OWNERSHIP";
				Settings::TelemetryNotes = "Passive native sound-request ownership probe at Tulip Garden cobblestones";
				TelemetryProbe::set_research_context("R4.2F-T6 — Native Sound Request Ownership",
					"Tulip Garden Cobblestone", 1, SoundRequestTrace::MaximumCaptureSeconds);
				if (!TelemetryProbe::start_new_capture())
					soundTraceError = "Telemetry could not start.";
				else if (!SoundRequestTrace::start_capture(Settings::TelemetryTestScenario.get(),
					TelemetryProbe::snapshot().currentFilename))
				{
					TelemetryProbe::set_research_capture_status("cancelled", 0.0);
					TelemetryProbe::stop_capture();
					soundTraceError = "Sound-request trace could not start.";
				}
				else soundTraceError.clear();
			}
		}
		else if (SoundRequestTrace::recording())
		{
			ImGui::Text("Elapsed: %.1f / %.0f seconds", SoundRequestTrace::elapsed_seconds(),
				SoundRequestTrace::MaximumCaptureSeconds);
			if (ImGui::Button("Finish Sound Request Ownership"))
			{
				const double elapsed = SoundRequestTrace::elapsed_seconds();
				SoundRequestTrace::finish_capture("completed");
				TelemetryProbe::set_research_capture_status("pending_review", elapsed);
				TelemetryProbe::stop_capture();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel Sound Request Ownership"))
			{
				const double elapsed = SoundRequestTrace::elapsed_seconds();
				SoundRequestTrace::cancel_capture();
				TelemetryProbe::set_research_capture_status("cancelled", elapsed);
				TelemetryProbe::stop_capture();
			}
		}
		else
		{
			if (SoundRequestTrace::needs_save())
			{
				ImGui::TextWrapped("Capture stopped at its safety limit. Save it now to finish telemetry and write the trace.");
				if (ImGui::Button("Save completed sound trace"))
				{
					const double elapsed = SoundRequestTrace::elapsed_seconds();
					SoundRequestTrace::finish_capture("completed_limit");
					TelemetryProbe::set_research_capture_status("pending_review", elapsed);
					TelemetryProbe::stop_capture();
				}
			}
			else if (ImGui::Button("Ready for another sound trace"))
				SoundRequestTrace::cancel_capture();
		}
		if (!SoundRequestTrace::trace_path().empty())
			ImGui::TextWrapped("Trace: %s", SoundRequestTrace::trace_path().string().c_str());
		if (!soundTraceError.empty())
			ImGui::TextColored(ImVec4(1.f, .4f, .3f, 1.f), "%s", soundTraceError.c_str());
		ImGui::TextDisabled("This records requests only. It does not alter game audio or wheel output.");
	}

	static void draw_road2_active_state()
	{
		const auto& active = HYP36RRoad2Active::frame();
		const auto& gain = HYP36RRoad2Active::gain_frame();
		const auto& policy = HYP36RRoad2::frame();
		const auto& signal = HYP36RSignalState::frame();
		ImGui::SeparatorText("Road 2.0 (Enhanced) C1 - Development");
		ImGui::Text("Mode: %s", HYP36RRoad2Active::mode_name(active.mode));
		ImGui::Text("Road Detail Mode: %s",
			active.mode == HYP36RRoad2Active::Mode::Experimental ? "Enhanced" : "Classic");
		ImGui::Text("Native authority: %.5f  presented: %.5f",
			active.nativeAuthority, active.presentationAuthority);
		ImGui::Text("Surface tuple: %08X %08X %08X %08X",
			signal.surfaces.current[0], signal.surfaces.current[1],
			signal.surfaces.current[2], signal.surfaces.current[3]);
		ImGui::Text("Occupancy: %u/4  target %.3f  envelope %.3f",
			unsigned(policy.spatial.differingFromReference), active.occupancyTarget,
			active.occupancyEnvelope);
		ImGui::Text("Transition: %s  changed mask: 0x%X",
			HYP36RRoad2Active::phase_name(active.phase), unsigned(policy.transition.changedMask));
		ImGui::Text("Surface character: %s",
			HYP36RRoad2Active::archetype_name(active.archetype));
		ImGui::Text("Generator raw: %.5f  conditioned: %.5f",
			active.rawGenerator, active.conditionedTexture);
		ImGui::Text("Aperiodic: %.5f  character: %.5f  resistance: %.5f",
			active.aperiodicBase, active.characterComponent, active.resistanceComponent);
		ImGui::Text("Pre-safety Road: %.5f", active.preSafetyContribution);
		ImGui::Text("Road2 contribution: %.5f / +/-%.3f",
			active.contribution, HYP36RRoad2Active::InternalCeiling);
		ImGui::Text("Safety: %s%s%s", HYP36RRoad2Active::safety_name(active.safety),
			active.clamped ? " | clamp" : "", active.slewLimited ? " | slew" : "");
		if (ImGui::Checkbox("Arcade Authority", Settings::Road2ArcadeAuthority.ptr()))
		{
			persist_setting(Settings::Road2ArcadeAuthority);
			HYP36RRoad2Active::reset_gain();
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Uses the stronger Enhanced Road calibration for a more pronounced arcade-style surface feel.");
		ImGui::TextDisabled("Shipping calibration: 8x  |  Arcade calibration: 10x");
		ImGui::Text("Road Detail scale: %.2fx", gain.roadDetailScale);
		ImGui::Text("Enhanced calibration: %dx", gain.developmentGain);
		ImGui::Text("Road2 pre-gain: %.5f  post-gain: %.5f  final Road: %.5f",
			gain.preGainRoad, gain.postGainRoad, gain.finalRoad);
		ImGui::Text("Road channel safety: %s%s", gain.clamped ? "clamped" : "clear",
			gain.slewLimited ? " | slew limited" : "");
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

		ImGui::SeparatorText("HYP36Rforce");
		if (ImGui::Checkbox("Telemetry Overlay", Settings::TelemetryOverlayEnabled.ptr()))
			persist_setting(Settings::TelemetryOverlayEnabled);

		if (Settings::TelemetryEnabled && ImGui::CollapsingHeader("FFB Telemetry", ImGuiTreeNodeFlags_DefaultOpen))
			draw_ffb_telemetry();

		if (ImGui::CollapsingHeader("Road Research Audio Sync"))
			draw_audio_sync_validation();

		if (ImGui::CollapsingHeader("Road Research"))
			draw_sound_request_ownership();

		if (ImGui::CollapsingHeader("Road 2.0 Experimental", ImGuiTreeNodeFlags_DefaultOpen))
			draw_road2_active_state();

		if (ImGui::CollapsingHeader("Tools", ImGuiTreeNodeFlags_DefaultOpen))
			draw_tools();

		if (ImGui::CollapsingHeader("Hooks"))
			draw_hook_status();
	}

	static DebugWindow instance;
};
DebugWindow DebugWindow::instance;
