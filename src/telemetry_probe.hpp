#pragma once

#include <array>
#include <cstdint>
#include <limits>

#include "settings.hpp"

namespace Settings
{
	extern Setting<bool> TelemetryEnabled;
}

namespace TelemetryProbe
{
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
		float ffbRaw = 0.0f;
		float ffbFinal = 0.0f;
		float ffbMasterStrength = 0.0f;
		bool ffbAvailable = false;
		bool active = false;
	};

	// Called by the existing wheel output boundary. Values are observed only;
	// this component never changes the force sent to DirectInput.
	void observe_ffb(float rawForce, float finalRequestedForce, float masterStrength);

	// One call from the existing player-car update produces one CSV row while
	// the developer telemetry toggle is enabled.
	void sample(float speed, float steeringInput, const std::array<uint32_t, 4>& surfaceRaw);

	void shutdown();
	const Snapshot& snapshot();
}
