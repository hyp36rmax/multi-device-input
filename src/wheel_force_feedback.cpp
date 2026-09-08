#include "wheel_force_feedback.hpp"

#include <dinput.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <format>

#include "Proxy.hpp"

namespace Settings
{
	Setting<bool> WheelFFBEnabled{ "Controls", "WheelFFBEnabled", true,
		"Enable native force feedback for steering wheels." };
	Setting<int> WheelFFBStrength{ "Controls", "WheelFFBStrength", 50,
		"Master wheel force feedback strength.", Range<int>{ 0, 100 } };
	Setting<bool> WheelFFBInvert{ "Controls", "WheelFFBInvert", false,
		"Reverse force feedback direction." };
	Setting<std::string> WheelFFBDevice{ "Controls", "WheelFFBDevice", "",
		"Internal identifier for the wheel selected in the in-game controller screen." };
}

namespace WheelForceFeedback
{
	namespace
	{
		struct EnumeratedDevice : DeviceInfo { GUID guid{}; };
		IDirectInput8W* directInput = nullptr;
		IDirectInputDevice8W* wheel = nullptr;
		IDirectInputEffect* testEffect = nullptr;
		std::vector<EnumeratedDevice> foundDevices;
		std::vector<DeviceInfo> publicDevices;
		HWND gameWindow = nullptr;
		DWORD actuatorAxis = DIJOFS_X;
		bool hasFocus = true;
		std::chrono::steady_clock::time_point stopAt{};
		std::string statusText = "Not initialized";

		std::string failed_status(const char* operation, HRESULT result)
		{
			return std::format("{} failed (DirectInput 0x{:08X})", operation, static_cast<unsigned long>(result));
		}

		std::string guid_string(const GUID& guid)
		{
			char text[40]{};
			std::snprintf(text, sizeof(text), "%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
				guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
				guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
			return text;
		}

		BOOL CALLBACK enumerate_device(const DIDEVICEINSTANCEW* instance, void*)
		{
			IDirectInputDevice8W* device = nullptr;
			if (FAILED(directInput->CreateDevice(instance->guidInstance, &device, nullptr)))
				return DIENUM_CONTINUE;
			DIDEVCAPS caps{ sizeof(caps) };
			if (SUCCEEDED(device->GetCapabilities(&caps)) && (caps.dwFlags & DIDC_FORCEFEEDBACK))
			{
				int needed = WideCharToMultiByte(CP_UTF8, 0, instance->tszProductName, -1, nullptr, 0, nullptr, nullptr);
				std::string name(needed > 0 ? needed : 0, '\0');
				if (needed > 1)
				{
					WideCharToMultiByte(CP_UTF8, 0, instance->tszProductName, -1, name.data(), needed, nullptr, nullptr);
					name.pop_back();
				}
				foundDevices.push_back({ { guid_string(instance->guidInstance), name }, instance->guidInstance });
			}
			device->Release();
			return DIENUM_CONTINUE;
		}

		BOOL CALLBACK find_actuator_axis(const DIDEVICEOBJECTINSTANCEW* object, void*)
		{
			if (object->dwType & DIDFT_FFACTUATOR)
			{
				actuatorAxis = object->dwOfs;
				return DIENUM_STOP;
			}
			return DIENUM_CONTINUE;
		}

		void close_wheel()
		{
			if (testEffect) { testEffect->Stop(); testEffect->Release(); testEffect = nullptr; }
			if (wheel) { wheel->SendForceFeedbackCommand(DISFFC_STOPALL); wheel->Unacquire(); wheel->Release(); wheel = nullptr; }
		}

		bool open_selected()
		{
			close_wheel();
			if (!directInput || !Settings::WheelFFBEnabled || foundDevices.empty())
				return false;
			auto selected = std::find_if(foundDevices.begin(), foundDevices.end(), [](const auto& d) { return d.id == Settings::WheelFFBDevice.get(); });
			if (selected == foundDevices.end())
			{
				selected = foundDevices.begin();
				Settings::WheelFFBDevice = selected->id;
			}
			HRESULT result = directInput->CreateDevice(selected->guid, &wheel, nullptr);
			if (FAILED(result))
			{
				statusText = failed_status("Opening the selected wheel", result);
				close_wheel();
				return false;
			}
			result = wheel->SetDataFormat(&c_dfDIJoystick2);
			if (FAILED(result))
			{
				statusText = failed_status("Setting the wheel data format", result);
				close_wheel();
				return false;
			}
			result = wheel->SetCooperativeLevel(gameWindow, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
			if (FAILED(result))
			{
				statusText = failed_status("Requesting exclusive wheel access", result);
				close_wheel();
				return false;
			}

			DIPROPDWORD autoCenter{};
			autoCenter.diph.dwSize = sizeof(autoCenter);
			autoCenter.diph.dwHeaderSize = sizeof(autoCenter.diph);
			autoCenter.diph.dwHow = DIPH_DEVICE;
			autoCenter.dwData = DIPROPAUTOCENTER_OFF;
			wheel->SetProperty(DIPROP_AUTOCENTER, &autoCenter.diph);

			result = wheel->Acquire();
			if (FAILED(result) && result != S_FALSE)
			{
				statusText = failed_status("Acquiring the selected wheel", result);
				close_wheel();
				return false;
			}
			wheel->SendForceFeedbackCommand(DISFFC_RESET);
			result = wheel->SendForceFeedbackCommand(DISFFC_SETACTUATORSON);
			if (FAILED(result))
			{
				statusText = failed_status("Enabling wheel actuators", result);
				close_wheel();
				return false;
			}
			actuatorAxis = DIJOFS_X;
			wheel->EnumObjects(find_actuator_axis, nullptr, DIDFT_AXIS);
			statusText = selected->name + " is ready";
			return true;
		}
	}

	void init(HWND hwnd)
	{
		gameWindow = hwnd;
		using CreateFn = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
		auto create = reinterpret_cast<CreateFn>(GetProcAddress(proxy::origModule, "DirectInput8Create"));
		if (!create || FAILED(create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8W, reinterpret_cast<void**>(&directInput), nullptr)))
		{
			statusText = "DirectInput force feedback is unavailable";
			return;
		}
		refresh();
	}

	void shutdown()
	{
		stop();
		close_wheel();
		if (directInput) { directInput->Release(); directInput = nullptr; }
	}

	void refresh()
	{
		stop();
		close_wheel();
		foundDevices.clear();
		publicDevices.clear();
		if (!directInput) return;
		directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, enumerate_device, nullptr, DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK);
		for (const auto& device : foundDevices) publicDevices.push_back(device);
		if (foundDevices.empty()) statusText = "No force-feedback wheel detected";
		else open_selected();
	}

	void select(const std::string& id) { Settings::WheelFFBDevice = id; open_selected(); }

	void test(float direction)
	{
		stop();
		if (!wheel || !hasFocus || !Settings::WheelFFBEnabled) return;
		HRESULT result = wheel->Acquire();
		if (FAILED(result) && result != S_FALSE)
		{
			statusText = failed_status("Acquiring the wheel for the test", result);
			return;
		}
		wheel->SendForceFeedbackCommand(DISFFC_SETACTUATORSON);
		DWORD axes[] = { actuatorAxis };
		LONG directions[] = { (direction < 0.f ? -1L : 1L) * (Settings::WheelFFBInvert ? -1L : 1L) * DI_FFNOMINALMAX };
		DICONSTANTFORCE force{ std::clamp<LONG>(Settings::WheelFFBStrength * 20L, 0, 2000) };
		DIEFFECT effect{};
		effect.dwSize = sizeof(effect);
		effect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
		effect.dwDuration = 350000;
		effect.dwGain = DI_FFNOMINALMAX;
		effect.dwTriggerButton = DIEB_NOTRIGGER;
		effect.cAxes = 1;
		effect.rgdwAxes = axes;
		effect.rglDirection = directions;
		effect.cbTypeSpecificParams = sizeof(force);
		effect.lpvTypeSpecificParams = &force;
		result = wheel->CreateEffect(GUID_ConstantForce, &effect, &testEffect, nullptr);
		if (FAILED(result))
		{
			statusText = failed_status("Creating the constant-force test", result);
			return;
		}
		result = testEffect->Start(1, 0);
		if (SUCCEEDED(result))
		{
			statusText = direction < 0.f ? "Left test force sent" : "Right test force sent";
			stopAt = std::chrono::steady_clock::now() + std::chrono::milliseconds(350);
		}
		else
		{
			statusText = failed_status("Starting the constant-force test", result);
			stop();
		}
	}

	void update() { if (testEffect && std::chrono::steady_clock::now() >= stopAt) stop(); }
	void setFocused(bool focused) { hasFocus = focused; if (!focused) stop(); }
	void stop() { if (testEffect) { testEffect->Stop(); testEffect->Release(); testEffect = nullptr; } }
	bool ready() { return wheel != nullptr; }
	const std::vector<DeviceInfo>& devices() { return publicDevices; }
	const std::string& status() { return statusText; }
}
