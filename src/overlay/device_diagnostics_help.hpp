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
		"Saves a support summary of device detection and test results without changing settings."
	};
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
		FfbTest, BackendCompatibility, DelayedDiscovery, ExportReport
	};
	inline constexpr std::array QuickSetupControls{ ContinueSetup, RetrySetup, SkipSetup, BackSetup, CancelSetup };

	inline constexpr std::array ForceControls{
		ForceProfile, Strength, SteeringLoad, RoadMode, RoadDetail, Surface,
		Impact, InvertWheel, DirectionTest, RedetectWheel, ArcadeStrength, ArcadeRoadDetail
	};
}
