#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "telemetry_probe.hpp"

#include <algorithm>
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
		constexpr const char* ProbeVersion = "HYP36R_RESEARCH_II_R1";
		constexpr size_t FlushEverySamples = 120;

		Snapshot current{};
		std::ofstream csv;
		std::string pendingRows;
		std::chrono::steady_clock::time_point sessionStart{};
		size_t samplesSinceFlush = 0;
		size_t writeFailures = 0;
		double maximumSampleGapSeconds = 0.0;
		double previousSampleElapsedSeconds = 0.0;
		bool pendingFfbAvailable = false;
		bool captureRequested = true;
		float pendingFfbRaw = 0.0f;
		float pendingFfbUnclamped = 0.0f;
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
			const std::string scenario = metadata_text(Settings::TelemetryTestScenario.get());
			const std::string notes = metadata_text(Settings::TelemetryNotes.get());
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
			writeFailures = 0;
			maximumSampleGapSeconds = 0.0;
			previousSampleElapsedSeconds = 0.0;

			csv << "# telemetry_probe_version=" << ProbeVersion << '\n';
			csv << "# telemetry_probe=" << ProbeVersion << '\n';
			csv << "# tweaks_version=" << MODULE_VERSION_STR << '\n';
			csv << "# game_exe_timestamp=" << Util::GetModuleTimestamp(Module::ExeHandle) << '\n';
			csv << "# start_time_local=" << local_time_text(started, "%Y-%m-%d %H:%M:%S") << '\n';
			csv << "# test_scenario=" << scenario << '\n';
			csv << "# notes=" << notes << '\n';
			csv << "timestamp,frame,elapsed_time,speed,steering_input,xforce,surface_0,surface_1,surface_2,surface_3,ffb_raw,ffb_final,ffb_master,native_1D0,native_1D4,native_1DC,native_1E0,native_1E4,native_264,native_268,candidate_D38,candidate_D3C,candidate_D40,candidate_D44,candidate_D46,candidate_D48,state_validity,steering_reference_rad,response_angle_rad,response_rate_rad_s,response_rate_valid,reference_response_error_rad,corrected_reference_rad,response_authority,overshoot_attenuation,transition_frames_remaining,last_valid_state_age_s,response_rate_utilization,response_rate_utilization_valid,synthetic_lateral_speed,synthetic_slip_ratio,synthetic_grip_loss,composer_mode,composer_native_availability,composer_native_weight,composer_event_phase,composer_recovering,intent_directional,intent_unloading,intent_motion,intent_road,intent_impact,legacy_directional_component,force2_shadow_directional,shadow_texture_component,shadow_impact_component,shadow_pre_budget,shadow_post_budget,shadow_rate_limit_active,shadow_headroom_limit_active,shadow_pre_master,force2_shadow_output,shadow_minus_legacy,legacy_force_output,active_directional_component,active_unloading_applied,bite_state,bite_candidate,bite_active,bite_confidence,bite_error_magnitude,bite_error_closing_rate,bite_vehicle_convergence,bite_driver_convergence,bite_age_s,bite_dynamic_context,bite_convergence_source,bite_shadow_phase,bite_shadow_active,bite_shadow_current_m4c_unloading,bite_shadow_unloading,bite_shadow_load_restoration,bite_shadow_directional,bite_shadow_restoration_rate,bite_shadow_limiter_active,bite_shadow_abort_active,m4c_unloaded_directional,hardware_selected_directional,hardware_selected_unloading,corner0_displacement_candidate,corner1_displacement_candidate,corner2_displacement_candidate,corner3_displacement_candidate,corner0_directional_ac,corner1_directional_ac,corner2_directional_ac,corner3_directional_ac,corner0_directional_b0,corner1_directional_b0,corner2_directional_b0,corner3_directional_b0,fc_front_displacement,fc_rear_displacement,fc_left_displacement,fc_right_displacement,fc_front_rear_displacement_bias,fc_left_right_displacement_bias,fc_front_lateral_response,fc_rear_lateral_response,fc_front_rear_lateral_bias,fc_front_longitudinal_response,fc_rear_longitudinal_response,fc_front_rear_longitudinal_bias,fc_fl_combined_response,fc_fr_combined_response,fc_rl_combined_response,fc_rr_combined_response,fc_front_combined_response,fc_rear_combined_response,fc_surface_asymmetry,m5_intent_lateral_state,m5_intent_lateral_balance,m5_intent_longitudinal_state,m5_intent_longitudinal_level,m5_intent_chassis_state,m5_intent_chassis_level,m5_intent_recovery_context,m5_intent_confidence,m5_intent_active,m5_intent_surface_contaminated,m5i_lateral_balance,m5i_lateral_activity,m5i_shadow_active,m5i_shadow_phase,m5i_shadow_modulation,m5i_shadow_directional,m5i_shadow_minus_m4,m5i_shadow_limiter_active,m5i_shadow_reason,m5j_mode,m5j_selected_directional,m5j_applied_modulation,s2_composer_input,s2_post_tanh,s2_instantaneous_magnitude,s2_normalized_headroom,s2_recent_peak_500ms,s2_sustained_rms_1s,s2_sustained_rms_3s,s2_occupancy_50_3s,s2_occupancy_75_3s,s2_occupancy_90_3s,s2_occupancy_98_3s,s2_output_slew_per_s,s2_pre_tanh_over_unity,s2_near_boundary,s2_directinput_clamp_active,presentation_profile,presentation_presence,presentation_contrast,presentation_directional_primary,presentation_secondary_raw,presentation_secondary_requested,presentation_secondary_permitted,presentation_directional_request,presentation_road_request,presentation_impact_request,presentation_vibration_request,presentation_secondary_budget_active,presentation_legacy_boundary_active,presentation_software_region,presentation_fallback_active,s9_mode,s9_directional_pre_presence,s9_directional_post_presence,s9_hardware_directional_selected,surface0_previous,surface1_previous,surface2_previous,surface3_previous,surface0_changed,surface1_changed,surface2_changed,surface3_changed,corner0_field14,corner1_field14,corner2_field14,corner3_field14,corner0_fieldE8,corner1_fieldE8,corner2_fieldE8,corner3_fieldE8,corner0_fieldEC,corner1_fieldEC,corner2_fieldEC,corner3_fieldEC,corner0_fieldEE,corner1_fieldEE,corner2_fieldEE,corner3_fieldEE,vibration_left_raw,vibration_right_raw,vibration_combined_raw,vibration_rise,gear_current,gear_previous_native,gear_transition,directional_pre_gain,road_pre_gain,impact_pre_gain,directional_post_gain,road_post_gain,impact_post_gain,steering_load_percent,road_detail_percent,impact_percent,output_ramp,composer_pre_tanh,composer_post_tanh,force_pre_drive,ffb_invert_enabled,directinput_unclamped_request\n";
			spdlog::info("TelemetryProbe: recording {} samples to {}", ProbeVersion, path.string());
			return true;
		}

		void flush_rows()
		{
			if (!csv || pendingRows.empty())
				return;
			csv << pendingRows;
			csv.flush();
			if (!csv)
			{
				++writeFailures;
				spdlog::error("TelemetryProbe: CSV write failed");
			}
			pendingRows.clear();
			samplesSinceFlush = 0;
		}
	}

	void observe_ffb(float rawForce, float unclampedRequestedForce,
		float finalRequestedForce, float masterStrength)
	{
		if (!Settings::TelemetryEnabled)
			return;
		pendingFfbRaw = rawForce;
		pendingFfbUnclamped = unclampedRequestedForce;
		pendingFfbFinal = finalRequestedForce;
		pendingFfbMaster = masterStrength;
		pendingFfbAvailable = true;
	}

	void sample(float speed, float steeringInput, const std::array<uint32_t, 4>& surfaceRaw,
		const std::array<float, 7>& nativeCandidates,
		const SteeringResponseCandidates& steeringResponse,
		const HYP36RVehicleState::Frame& vehicleState,
		const SyntheticVehicleState& syntheticVehicleState,
		const HYP36RForce2::Frame& force2Shadow,
		const HYP36RBite::Frame& biteState,
		const HYP36RBiteShadow::Frame& biteShadow,
		const NativeFourCorner::Frame& fourCorner,
		const HYP36RFourCorner::Frame& fourCornerContext,
		const HYP36RContextualIntent::Frame& contextualIntent,
		const HYP36RLateralContextShadow::Frame& lateralContextShadow,
		const HardwareSelection& hardwareSelection,
		const M5JSelection& m5jSelection,
		const HYP36RPresentation::Frame& presentationFrame,
		const ResearchIIObservation& researchII)
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
		if (current.frameIndex > 0)
			maximumSampleGapSeconds = (std::max)(maximumSampleGapSeconds,
				current.elapsedTime - previousSampleElapsedSeconds);
		previousSampleElapsedSeconds = current.elapsedTime;
		current.speed = speed;
		current.steeringInput = steeringInput;
		current.previousSurfaceRaw = current.surfaceRaw;
		for (size_t i = 0; i < surfaceRaw.size(); ++i)
			current.surfaceChanged[i] = current.frameIndex > 0 &&
				current.previousSurfaceRaw[i] != surfaceRaw[i];
		current.surfaceRaw = surfaceRaw;
		current.nativeCandidates = nativeCandidates;
		current.steeringResponse = steeringResponse;
		current.vehicleState = vehicleState;
		current.syntheticVehicleState = syntheticVehicleState;
		current.force2Shadow = force2Shadow;
		current.biteState = biteState;
		current.biteShadow = biteShadow;
		current.fourCorner = fourCorner;
		current.fourCornerContext = fourCornerContext;
		current.contextualIntent = contextualIntent;
		current.lateralContextShadow = lateralContextShadow;
		current.m5jSelection = m5jSelection;
		current.outputExposure = HYP36ROutputExposure::frame();
		current.presentation = presentationFrame;
		current.researchII = researchII;
		current.ffbAvailable = pendingFfbAvailable;
		if (pendingFfbAvailable)
		{
			current.ffbRaw = pendingFfbRaw;
			current.ffbUnclamped = pendingFfbUnclamped;
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

		const auto& semantic = current.vehicleState.current;
		const std::string responseAngleCell = semantic.responseAngleValid
			? std::format("{:.7f}", semantic.responseAngleRad) : std::string{};
		const std::string responseRateCell = semantic.responseRateValid
			? std::format("{:.7f}", semantic.responseAngularRateRadPerSec) : std::string{};
		const std::string responseErrorCell = semantic.referenceResponseErrorValid
			? std::format("{:.7f}", semantic.referenceResponseErrorRad) : std::string{};
		const std::string correctedReferenceCell = semantic.correctedReferenceValid
			? std::format("{:.7f}", semantic.correctedReferenceAngleRad) : std::string{};
		const std::string rateUtilizationCell = current.vehicleState.responseRateUtilizationValid
			? std::format("{:.7f}", current.vehicleState.responseRateUtilization) : std::string{};
		const std::string shadowMinusLegacyCell = current.ffbAvailable
			? std::format("{:.7f}", current.force2Shadow.shadowOutput -
				current.force2Shadow.legacyForceOutput)
			: std::string{};

		pendingRows += std::format(
			"{:.6f},{},{:.6f},{:.6f},{:.6f},{},{},{},{},{},{},{},{},{:.6f},{:.6f},{:.6f},{:.6f},{:.6f},{:.6f},{:.6f},{:.6f},{:.6f},{:.6f},{},{},{},{},{:.7f},{},{},{},{},{},{:.7f},{:.7f},{},{:.7f},{},{},{:.7f},{:.7f},{:.7f}",
			current.timestamp, current.frameIndex, current.elapsedTime,
			current.speed, current.steeringInput, xForceCell,
			current.surfaceRaw[0], current.surfaceRaw[1],
			current.surfaceRaw[2], current.surfaceRaw[3],
			ffbRawCell, ffbFinalCell, ffbMasterCell,
			current.nativeCandidates[0], current.nativeCandidates[1],
			current.nativeCandidates[2], current.nativeCandidates[3],
			current.nativeCandidates[4], current.nativeCandidates[5],
			current.nativeCandidates[6],
			current.steeringResponse.candidateD38,
			current.steeringResponse.candidateD3C,
			current.steeringResponse.candidateD40,
			current.steeringResponse.candidateD44,
			current.steeringResponse.candidateD46,
			current.steeringResponse.candidateD48,
			HYP36RVehicleState::validity_name(semantic.validity),
			semantic.steeringReferenceAngleRad,
			responseAngleCell,
			responseRateCell,
			semantic.responseRateValid ? 1 : 0,
			responseErrorCell,
			correctedReferenceCell,
			semantic.responseAuthority,
			semantic.overshootAttenuation,
			semantic.transitionFramesRemaining,
			semantic.lastValidStateAgeSeconds,
			rateUtilizationCell,
			current.vehicleState.responseRateUtilizationValid ? 1 : 0,
			current.syntheticVehicleState.lateralSpeed,
			current.syntheticVehicleState.slipRatio,
			current.syntheticVehicleState.gripLoss);
		pendingRows += std::format(
			",{},{},{:.7f},{},{},{:.7f},{:.7f},{:.7f},{:.7f},{:.7f},{:.7f},{:.7f},{:.7f},{:.7f},{:.7f},{:.7f},{},{},{:.7f},{:.7f},{},{:.7f},{:.7f},{:.7f},{},{},{},{:.7f},{:.7f},{:.7f},{:.7f},{:.7f},{:.7f},{:.7f},{}",
			HYP36RForce2::mode_name(current.force2Shadow.mode),
			HYP36RForce2::availability_name(current.force2Shadow.context.nativeAvailability),
			current.force2Shadow.context.nativeWeight,
			HYP36RForce2::event_phase_name(current.force2Shadow.context.eventPhase),
			current.force2Shadow.context.recovering ? 1 : 0,
			current.force2Shadow.intent.directionalLoad,
			current.force2Shadow.intent.unloading,
			current.force2Shadow.intent.motion,
			current.force2Shadow.intent.roadTexture,
			current.force2Shadow.intent.impact,
			current.force2Shadow.legacyDirectionalComponent,
			current.force2Shadow.shadowDirectional,
			current.force2Shadow.shadowTexture,
			current.force2Shadow.shadowImpact,
			current.force2Shadow.shadowPreBudget,
			current.force2Shadow.shadowPostBudget,
			current.force2Shadow.rateLimitActive ? 1 : 0,
			current.force2Shadow.headroomLimitActive ? 1 : 0,
			current.force2Shadow.shadowPreMaster,
			current.force2Shadow.shadowOutput,
			shadowMinusLegacyCell,
			current.force2Shadow.legacyForceOutput,
			current.force2Shadow.activeDirectional,
			current.force2Shadow.intent.unloading,
			HYP36RBite::state_name(current.biteState.state),
			current.biteState.candidate ? 1 : 0,
			current.biteState.active ? 1 : 0,
			current.biteState.confidence,
			current.biteState.errorMagnitude,
			current.biteState.errorClosingRate,
			current.biteState.vehicleConvergence,
			current.biteState.driverConvergence,
			current.biteState.ageSeconds,
			current.biteState.dynamicContext,
			HYP36RBite::convergence_source_name(current.biteState.convergenceSource));
		pendingRows += std::format(
			",{},{},{:.7f},{:.7f},{:.7f},{:.7f},{:.7f},{},{},{:.7f},{:.7f},{:.7f}",
			HYP36RBiteShadow::phase_name(current.biteShadow.phase),
			current.biteShadow.active ? 1 : 0,
			current.biteShadow.currentM4CUnloading,
			current.biteShadow.shadowUnloading,
			current.biteShadow.loadRestoration,
			current.biteShadow.shadowDirectional,
			current.biteShadow.restorationRate,
			current.biteShadow.limiterActive ? 1 : 0,
			current.biteShadow.abortActive ? 1 : 0,
			current.force2Shadow.activeDirectional,
			hardwareSelection.directional,
			hardwareSelection.unloading);
		if (current.fourCorner.available)
		{
			for (float value : current.fourCorner.displacementCandidate)
				pendingRows += std::format(",{:.7f}", value);
			for (float value : current.fourCorner.directionalCandidateAC)
				pendingRows += std::format(",{:.7f}", value);
			for (float value : current.fourCorner.directionalCandidateB0)
				pendingRows += std::format(",{:.7f}", value);
		}
		else
		{
			pendingRows += ",,,,,,,,,,,,";
		}
		if (current.fourCornerContext.available)
		{
			const auto& context = current.fourCornerContext;
			pendingRows += std::format(
				",{:.7f},{:.7f},{:.7f},{:.7f},{:.7f},{:.7f}"
				",{:.7f},{:.7f},{:.7f},{:.7f},{:.7f},{:.7f}"
				",{:.7f},{:.7f},{:.7f},{:.7f},{:.7f},{:.7f},{}",
				context.front.displacement, context.rear.displacement,
				context.left.displacement, context.right.displacement,
				context.frontRearDisplacementBias, context.leftRightDisplacementBias,
				context.front.lateralResponse, context.rear.lateralResponse,
				context.frontRearLateralBias,
				context.front.longitudinalResponse, context.rear.longitudinalResponse,
				context.frontRearLongitudinalBias,
				context.fl.combinedResponse, context.fr.combinedResponse,
				context.rl.combinedResponse, context.rr.combinedResponse,
				context.front.combinedResponse, context.rear.combinedResponse,
				context.surface.anyAsymmetry ? 1 : 0);
		}
		else
		{
			pendingRows += ",,,,,,,,,,,,,,,,,,,";
		}
		if (current.contextualIntent.available)
		{
			const auto& intent = current.contextualIntent;
			pendingRows += std::format(",{},{:.7f},{},{:.7f},{},{:.7f},{},{:.7f},{},{}",
				HYP36RContextualIntent::lateral_state_name(intent.lateralState),
				intent.lateralBalance,
				HYP36RContextualIntent::longitudinal_state_name(intent.longitudinalState),
				intent.longitudinalLevel,
				HYP36RContextualIntent::chassis_state_name(intent.chassisState),
				intent.chassisLevel,
				HYP36RContextualIntent::recovery_context_name(intent.recoveryContext),
				intent.confidence, intent.active ? 1 : 0,
				intent.surfaceContaminated ? 1 : 0);
		}
		else
		{
			pendingRows += ",,,,,,,,,,";
		}
		if (current.lateralContextShadow.available)
		{
			const auto& shadow = current.lateralContextShadow;
			pendingRows += std::format(",{:.7f},{:.7f},{},{},{:.7f},{:.7f},{:.7f},{},{}",
				shadow.lateralBalance, shadow.lateralActivity,
				shadow.active ? 1 : 0,
				HYP36RLateralContextShadow::phase_name(shadow.phase),
				shadow.modulation, shadow.shadowDirectional, shadow.shadowMinusM4,
				shadow.limiterActive ? 1 : 0,
				HYP36RLateralContextShadow::reason_name(shadow.reason));
		}
		else
		{
			pendingRows += ",,,,,,,,unavailable";
		}
		pendingRows += std::format(",{},{:.7f},{:.7f}",
			HYP36RLateralContextShadow::hardware_mode_name(current.m5jSelection.mode),
			current.m5jSelection.selectedDirectional,
			current.m5jSelection.appliedModulation);
		const auto& exposure = current.outputExposure;
		pendingRows += std::format(
			",{:.7f},{:.7f},{:.7f},{:.7f},{:.7f},{:.7f},{:.7f}"
			",{:.7f},{:.7f},{:.7f},{:.7f},{:.7f},{},{},{}",
			exposure.composerInput, exposure.postTanh,
			exposure.instantaneousMagnitude, exposure.normalizedHeadroom,
			exposure.recentPeak, exposure.sustainedOneSecond,
			exposure.sustainedThreeSeconds, exposure.occupancy50,
			exposure.occupancy75, exposure.occupancy90, exposure.occupancy98,
			exposure.outputSlewPerSecond, exposure.preTanhOverUnity ? 1 : 0,
			exposure.nearBoundary ? 1 : 0,
			exposure.directInputClampActive ? 1 : 0);
		const auto& presentation = current.presentation;
		pendingRows += std::format(
			",{},{:.2f},{:g},{:.7f},{:.7f},{:.7f},{:.7f},{:.7f},{:.7f},{:.7f},{:.7f},{},{},{},{}",
			HYP36RPresentation::profile_name(presentation.profile),
			presentation.presence, presentation.contrast,
			presentation.directionalPrimary, presentation.secondaryRaw,
			presentation.secondaryRequested, presentation.secondaryPermitted,
			presentation.directionalRequest, presentation.roadRequest,
			presentation.impactRequest, presentation.vibrationRequest,
			presentation.secondaryBudgetActive ? 1 : 0,
			presentation.legacyBoundaryActive ? 1 : 0,
			HYP36RPresentation::software_region_name(presentation.softwareRegion),
			presentation.fallbackActive ? 1 : 0);
		pendingRows += std::format(",{},{:.7f},{:.7f},{:.7f}",
			HYP36RPresentation::mode_name(presentation.mode),
			presentation.directionalPrePresence,
			presentation.directionalRequest,
			presentation.hardwareDirectionalSelected);
		if (current.frameIndex > 0)
		{
			for (uint32_t value : current.previousSurfaceRaw)
				pendingRows += std::format(",{}", value);
		}
		else
		{
			pendingRows += ",,,,";
		}
		for (bool value : current.surfaceChanged)
			pendingRows += std::format(",{}", value ? 1 : 0);
		if (current.fourCorner.available)
		{
			for (uint32_t value : current.fourCorner.field14)
				pendingRows += std::format(",{}", value);
			for (float value : current.fourCorner.fieldE8)
				pendingRows += std::format(",{:.7f}", value);
			for (int16_t value : current.fourCorner.fieldEC)
				pendingRows += std::format(",{}", value);
			for (int16_t value : current.fourCorner.fieldEE)
				pendingRows += std::format(",{}", value);
		}
		else
		{
			pendingRows += ",,,,,,,,,,,,,,,,";
		}
		const auto& r1 = current.researchII;
		pendingRows += std::format(
			",{:.7f},{:.7f},{:.7f},{:.7f},{},{},{},{:.7f},{:.7f},{:.7f}"
			",{:.7f},{:.7f},{:.7f},{},{},{},{:.7f},{:.7f},{:.7f},{:.7f},{},{}",
			r1.vibrationLeft, r1.vibrationRight, r1.vibrationCombined,
			r1.vibrationRise, r1.gearCurrent, r1.gearPreviousNative,
			r1.gearTransition ? 1 : 0,
			r1.directionalPreGain, r1.roadPreGain, r1.impactPreGain,
			r1.directionalPostGain, r1.roadPostGain, r1.impactPostGain,
			r1.steeringLoadPercent, r1.roadDetailPercent, r1.impactPercent,
			r1.outputRamp, r1.composerPreTanh, r1.composerPostTanh,
			r1.forcePreDrive, r1.invertEnabled ? 1 : 0,
			current.ffbAvailable ? std::format("{:.7f}", current.ffbUnclamped) : std::string{});
		pendingRows += '\n';
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
		const double sampleRate = current.elapsedTime > 0.0
			? static_cast<double>(current.frameIndex) / current.elapsedTime : 0.0;
		spdlog::info("TelemetryProbe: session closed after {} samples; rate={:.2f} Hz, maxGap={:.4f}s, writeFailures={}",
			current.frameIndex, sampleRate, maximumSampleGapSeconds, writeFailures);
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
