#pragma once

#include <string>
#include <vector>

#include "settings.hpp"

struct HWND__;
using HWND = HWND__*;

namespace Settings
{
	extern Setting<bool> WheelFFBEnabled;
	extern Setting<int> WheelFFBStrength;
	extern Setting<float> WheelFFBSpringStrength;
	extern Setting<float> WheelFFBDamperStrength;
	extern Setting<float> WheelFFBImpactStrength;
	extern Setting<float> WheelFFBRoadStrength;
	extern Setting<float> WheelFFBGripLossStrength;
	extern Setting<bool> WheelFFBInvert;
	extern Setting<bool> WheelFFBDiagnosticLog;
	extern Setting<std::string> WheelFFBDevice;
}

namespace WheelForceFeedback
{
	struct DeviceInfo
	{
		std::string id;
		std::string name;
	};
	struct SurfaceStatus
	{
		bool periodicSupported = false;
		bool sineSupported = false;
		bool dynamicMagnitudeSupported = false;
		bool dynamicPeriodSupported = false;
		bool active = false;
		float requestedMagnitude = 0.0f;
		float frequencyHz = 0.0f;
	};

	void init(HWND hwnd);
	void shutdown();
	void update();
	void setFocused(bool focused);
	void refresh();
	void select(const std::string& id);
	void test(float direction);
	void drive(float force);
	void drive_surface(float magnitude, float frequencyHz, bool enabled);
	void stop_surface();
	void stop();
	bool ready();
	bool has_attached_device();
	const std::vector<DeviceInfo>& devices();
	const std::string& active_device_id();
	const std::string& status();
	const SurfaceStatus& surface_status();
}
