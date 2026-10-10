#pragma once

#include <array>
#include <string_view>

namespace DeviceDiagnosticsHelp
{
	struct Entry
	{
		std::string_view id;
		std::string_view label;
		std::string_view description;
	};

	inline constexpr Entry InitializeDevices{
		"initialize-devices", "Initialize Devices",
		"Starts device setup and checks which connected controls are available."
	};
	inline constexpr Entry DeviceDiscovery{
		"device-discovery", "Device Discovery",
		"Shows the controllers, wheels, pedals, and shifters currently seen by Multi-Input."
	};
	inline constexpr Entry InputTest{
		"input-test", "Input Test",
		"Move an axis or press a button to confirm that the game receives it."
	};
	inline constexpr Entry MultiInput{
		"multi-input", "Multi-Input",
		"Quick Setup can combine a wheel, pedals, shifter, and buttons from separate USB devices."
	};
	inline constexpr Entry QuickSetup{
		"quick-setup", "Quick Setup",
		"Detects each control for six seconds and can combine inputs from several USB devices."
	};
	inline constexpr Entry StartQuickSetup{ "start-quick-setup", "Start Quick Setup", "Begins the guided six-second capture for each driving control." };
	inline constexpr Entry NotNow{ "quick-setup-not-now", "Not Now", "Opens diagnostics without changing your saved diagnostic assignments." };
	inline constexpr Entry ShiftUp{ "shift-up", "Shift Up", "Assigns the button or paddle used to select the next gear." };
	inline constexpr Entry ShiftDown{ "shift-down", "Shift Down", "Assigns the button or paddle used to select the previous gear." };
	inline constexpr Entry StartMenu{ "start-menu", "Start / Menu", "Assigns the control used to start and confirm menu actions." };
	inline constexpr Entry BackButton{ "back-button", "Back Button", "Assigns the control used to return from a menu." };
	inline constexpr Entry FfbAutoSelection{ "ffb-auto-selection", "FFB Auto-selection", "Restores your previous wheel when available, or selects the only valid FFB interface." };
	inline constexpr Entry LeftFfb{ "ffb-left", "Test Left", "Hold for a short, limited request to verify the wheel's left direction." };
	inline constexpr Entry RightFfb{ "ffb-right", "Test Right", "Hold for a short, limited request to verify the wheel's right direction." };
	inline constexpr Entry EffectTest{ "effect-test", "Effect Test", "Runs the selected native or clearly labeled synthetic effect only while held." };
	inline constexpr Entry StopFfb{ "ffb-stop", "STOP", "Immediately stops and releases the active force-feedback effect." };
	inline constexpr Entry InvertFfbDiagnostic{ "ffb-invert-diagnostic", "Invert FFB", "Reverses Left, Right, and Shake directions without changing force strength." };
	inline constexpr Entry ShakeFfb{ "ffb-shake", "Hold to Shake", "Alternates a limited left/right force at 10 Hz while held, for no longer than 1.5 seconds." };
	inline constexpr Entry FfbReadiness{ "ffb-readiness", "FFB readiness", "Ready means the selected endpoint was acquired and its force actuator was enabled." };
	inline constexpr Entry SafetyLimitedOutput{ "ffb-safety-limit", "Safety-limited output", "The displayed request may be 20-100%, but physical output remains capped at 20% nominal." };
	inline constexpr Entry FfbTest{
		"ffb-test", "FFB Test",
		"Sends a short, limited force so you can check the wheel direction safely."
	};
	inline constexpr Entry BackendCompatibility{
		"backend-compatibility", "Backend Compatibility",
		"Chooses how Windows reports input devices. Restart the game after changing it."
	};
	inline constexpr Entry DelayedDiscovery{
		"delayed-discovery", "Delayed Discovery",
		"Some devices appear a few seconds after startup; this shows whether they arrived later."
	};
	inline constexpr Entry ExportReport{
		"export-report", "Export Report",
		"Export Report\n\nSaves your diagnostic results as TXT and JSON files.\n\nSave location:\nDocuments -> HYP36rforce Device Diagnostics -> Exports\n\nFiles are automatically named using your detected wheel, date, and time.\n\nClick Open Exports Folder to view your reports."
	};
	inline constexpr Entry OpenExportsFolder{ "open-exports-folder", "Open Exports Folder", "Opens the resolved Windows Documents report folder without uploading anything." };
	inline constexpr Entry ContinueSetup{ "quick-continue", "Continue", "Accepts the clear input candidate shown for this step and moves forward." };
	inline constexpr Entry RetrySetup{ "quick-retry", "Retry", "Clears only this temporary candidate and starts a fresh six-second detection window." };
	inline constexpr Entry SkipSetup{ "quick-skip", "Skip", "Moves forward without replacing an existing saved binding or creating a new one." };
	inline constexpr Entry BackSetup{ "quick-back", "Back", "Returns to the previous step while preserving completed selections." };
	inline constexpr Entry CancelSetup{ "quick-cancel", "Cancel", "Leaves Quick Setup and restores the diagnostic profile from before this run." };

	inline constexpr Entry ForceProfile{
		"force-profile", "Force Profile",
		"Selects the overall force-feedback character while keeping the same wheel device."
	};
	inline constexpr Entry Strength{
		"strength", "Strength",
		"Adjusts overall wheel force without changing the balance between effects."
	};
	inline constexpr Entry ArcadeStrength{
		"arcade-strength", "Arcade Strength",
		"Adjusts the overall strength of the Arcade Experience profile only."
	};
	inline constexpr Entry ArcadeRoadDetail{
		"arcade-road-detail", "Arcade Road Detail",
		"Adjusts short road events and surface changes in the Arcade Experience profile."
	};
	inline constexpr Entry SteeringLoad{
		"steering-load", "Steering Load",
		"Adjusts the weight felt while steering and cornering."
	};
	inline constexpr Entry RoadMode{
		"road-mode", "Road Mode",
		"Chooses the original road feel or the more detailed Enhanced presentation."
	};
	inline constexpr Entry RoadDetail{
		"road-detail", "Road Detail",
		"Adjusts feedback from road textures and changes in driving surface."
	};
	inline constexpr Entry Surface{
		"surface", "Surface",
		"Adjusts the texture felt from road surfaces, bumps, and surface changes."
	};
	inline constexpr Entry Impact{
		"impact", "Impact",
		"Adjusts the short force felt from collisions and impacts."
	};
	inline constexpr Entry InvertWheel{
		"invert-wheel", "Invert Wheel",
		"Reverses force direction if the wheel pulls the wrong way."
	};
	inline constexpr Entry DirectionTest{
		"direction-test", "Left / Right Test",
		"Briefly moves force left or right so you can verify direction before driving."
	};
	inline constexpr Entry RedetectWheel{
		"redetect-wheel", "Re-detect Wheel",
		"Checks again for supported force-feedback wheel interfaces."
	};

	inline constexpr std::array RequiredSections{
		InitializeDevices, DeviceDiscovery, InputTest, QuickSetup,
		FfbTest, BackendCompatibility, DelayedDiscovery, ExportReport, OpenExportsFolder
	};
	inline constexpr std::array QuickSetupControls{ StartQuickSetup, NotNow, ContinueSetup, RetrySetup, SkipSetup, BackSetup, CancelSetup, ShiftUp, ShiftDown, StartMenu, BackButton, FfbAutoSelection };

	inline constexpr std::array ForceControls{
		ForceProfile, Strength, SteeringLoad, RoadMode, RoadDetail, Surface,
		Impact, InvertWheel, DirectionTest, RedetectWheel, ArcadeStrength, ArcadeRoadDetail,
		InvertFfbDiagnostic, ShakeFfb, FfbReadiness, SafetyLimitedOutput
	};
}
