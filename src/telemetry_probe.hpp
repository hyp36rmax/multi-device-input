#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <string>

#include "settings.hpp"
#include "bite_state_detector.hpp"
#include "bite_shadow_restoration.hpp"
#include "force2_shadow_composer.hpp"
#include "vehicle_state_interpreter.hpp"

namespace Settings
{
	extern Setting<bool> TelemetryEnabled;
	extern Setting<std::string> TelemetryTestScenario;
	extern Setting<std::string> TelemetryNotes;
}

namespace TelemetryProbe
{
	struct SteeringResponseCandidates
	{
		float candidateD38 = 0.0f;
		float candidateD3C = 0.0f;
		float candidateD40 = 0.0f;
		int16_t candidateD44 = 0;
		int16_t candidateD46 = 0;
		int16_t candidateD48 = 0;
	};

	struct SyntheticVehicleState
	{
		float lateralSpeed = 0.0f;
		float slipRatio = 0.0f;
		float gripLoss = 0.0f;
	};

	struct HardwareSelection
	{
		float directional = 0.0f;
		float unloading = 0.0f;
	};

	struct Snapshot
	{
		uint64_t frameIndex = 0;
		double timestamp = 0.0;
		double elapsedTime = 0.0;
		float speed = 0.0f;
		float steeringInput = 0.0f;
		float xForce = std::numeric_limits<float>::quiet_NaN();
		bool xForceAvailable = false;
		std::array<uint32_t, 4> surfaceRaw{};
		std::array<float, 7> nativeCandidates{};
		SteeringResponseCandidates steeringResponse{};
		HYP36RVehicleState::Frame vehicleState{};
		SyntheticVehicleState syntheticVehicleState{};
		HYP36RForce2::Frame force2Shadow{};
		HYP36RBite::Frame biteState{};
		HYP36RBiteShadow::Frame biteShadow{};
		float ffbRaw = 0.0f;
		float ffbFinal = 0.0f;
		float ffbMasterStrength = 0.0f;
		bool ffbAvailable = false;
		bool active = false;
		std::string testScenario;
		std::string currentFilename;
	};

	// Called by the existing wheel output boundary. Values are observed only;
	// this component never changes the force sent to DirectInput.
	void observe_ffb(float rawForce, float finalRequestedForce, float masterStrength);

	// One call from the existing player-car update produces one CSV row while
	// the developer telemetry toggle is enabled.
	void sample(float speed, float steeringInput, const std::array<uint32_t, 4>& surfaceRaw,
		const std::array<float, 7>& nativeCandidates,
		const SteeringResponseCandidates& steeringResponse,
		const HYP36RVehicleState::Frame& vehicleState,
		const SyntheticVehicleState& syntheticVehicleState,
		const HYP36RForce2::Frame& force2Shadow,
		const HYP36RBite::Frame& biteState,
		const HYP36RBiteShadow::Frame& biteShadow,
		const HardwareSelection& hardwareSelection);

	void shutdown();
	bool start_new_capture();
	void stop_capture();
	const Snapshot& snapshot();
}
