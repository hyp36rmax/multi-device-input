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
	extern Setting<bool> WheelFFBInvert;
	extern Setting<std::string> WheelFFBDevice;
}

namespace WheelForceFeedback
{
	struct DeviceInfo
	{
		std::string id;
		std::string name;
	};

	void init(HWND hwnd);
	void shutdown();
	void update();
	void setFocused(bool focused);
	void refresh();
	void select(const std::string& id);
	void test(float direction);
	void drive(float force);
	void stop();
	bool ready();
	const std::vector<DeviceInfo>& devices();
	const std::string& status();
}
