#include "wheel_force_feedback.hpp"

#include <dinput.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <format>
#include <spdlog/spdlog.h>

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
		std::vector<DWORD> actuatorAxes;
		bool hasFocus = true;
		std::chrono::steady_clock::time_point stopAt{};
		std::string statusText = "Not initialized";

		std::string failed_status(const char* operation, HRESULT result)
		{
			const std::string message = std::format("{} failed (DirectInput 0x{:08X})", operation, static_cast<unsigned long>(result));
			spdlog::error("WheelFFB: {}", message);
			return message;
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
			int needed = WideCharToMultiByte(CP_UTF8, 0, instance->tszProductName, -1, nullptr, 0, nullptr, nullptr);
			std::string name(needed > 0 ? needed : 0, '\0');
			if (needed > 1)
			{
				WideCharToMultiByte(CP_UTF8, 0, instance->tszProductName, -1, name.data(), needed, nullptr, nullptr);
				name.pop_back();
			}
			const std::string id = guid_string(instance->guidInstance);
			IDirectInputDevice8W* device = nullptr;
			const HRESULT createResult = directInput->CreateDevice(instance->guidInstance, &device, nullptr);
			if (FAILED(createResult))
			{
				spdlog::warn("WheelFFB: unable to inspect '{}' [{}], DirectInput 0x{:08X}", name, id, static_cast<unsigned long>(createResult));
				return DIENUM_CONTINUE;
			}
			DIDEVCAPS caps{ sizeof(caps) };
			const HRESULT capsResult = device->GetCapabilities(&caps);
			if (SUCCEEDED(capsResult) && (caps.dwFlags & DIDC_FORCEFEEDBACK))
			{
				foundDevices.push_back({ { id, name }, instance->guidInstance });
				spdlog::info("WheelFFB: found '{}' [{}]: {} axes, {} buttons, {} POVs, force feedback supported",
					name, id, caps.dwAxes, caps.dwButtons, caps.dwPOVs);
			}
			else if (FAILED(capsResult))
				spdlog::warn("WheelFFB: capability query failed for '{}' [{}], DirectInput 0x{:08X}", name, id, static_cast<unsigned long>(capsResult));
			else
				spdlog::info("WheelFFB: skipped '{}' [{}]: driver reports no force-feedback capability", name, id);
			device->Release();
			return DIENUM_CONTINUE;
		}

		BOOL CALLBACK find_actuator_axis(const DIDEVICEOBJECTINSTANCEW* object, void*)
		{
			if ((object->dwType & DIDFT_FFACTUATOR) && actuatorAxes.size() < 2)
				actuatorAxes.push_back(object->dwOfs);
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
			{
				spdlog::info("WheelFFB: not opening a wheel (backend={}, enabled={}, compatible devices={})",
					directInput != nullptr, bool(Settings::WheelFFBEnabled), foundDevices.size());
				return false;
			}
			auto selected = std::find_if(foundDevices.begin(), foundDevices.end(), [](const auto& d) { return d.id == Settings::WheelFFBDevice.get(); });
			if (selected == foundDevices.end())
			{
				selected = foundDevices.begin();
				Settings::WheelFFBDevice = selected->id;
			}
			spdlog::info("WheelFFB: opening selected device '{}' [{}]", selected->name, selected->id);
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
			result = wheel->SetCooperativeLevel(gameWindow, DISCL_EXCLUSIVE | DISCL_BACKGROUND);
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
			const HRESULT autoCenterResult = wheel->SetProperty(DIPROP_AUTOCENTER, &autoCenter.diph);
			if (FAILED(autoCenterResult))
				spdlog::warn("WheelFFB: disabling driver auto-center failed, DirectInput 0x{:08X}", static_cast<unsigned long>(autoCenterResult));

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
			actuatorAxes.clear();
			wheel->EnumObjects(find_actuator_axis, nullptr, DIDFT_AXIS);
			if (actuatorAxes.empty()) actuatorAxes.push_back(DIJOFS_X);
			spdlog::info("WheelFFB: '{}' ready with {} force actuator axis/axes", selected->name, actuatorAxes.size());
			statusText = selected->name + " is ready";
			return true;
		}

		bool open_with_fallback(std::string requestedId)
		{
			Settings::WheelFFBDevice = requestedId;
			if (open_selected())
				return true;

			const std::string requestedName = [&]
			{
				auto requested = std::find_if(foundDevices.begin(), foundDevices.end(), [&](const auto& device) { return device.id == requestedId; });
				return requested == foundDevices.end() ? std::string("Selected interface") : requested->name;
			}();
			const std::string originalFailure = statusText;
			for (const auto& candidate : foundDevices)
			{
				if (candidate.id == requestedId)
					continue;
				Settings::WheelFFBDevice = candidate.id;
				spdlog::info("WheelFFB: selected interface was unusable; trying fallback '{}' [{}]", candidate.name, candidate.id);
				if (open_selected())
				{
					statusText = std::format("{} cannot output force; using its other interface for FFB", requestedName);
					spdlog::warn("WheelFFB: automatic fallback succeeded after: {}", originalFailure);
					return true;
				}
			}

			Settings::WheelFFBDevice = requestedId;
			statusText = originalFailure;
			return false;
		}
	}

	void init(HWND hwnd)
	{
		spdlog::info("WheelFFB: initializing native DirectInput backend (window={:p})", static_cast<void*>(hwnd));
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
		spdlog::info("WheelFFB: shutting down and stopping all effects");
		stop();
		close_wheel();
		if (directInput) { directInput->Release(); directInput = nullptr; }
	}

	void refresh()
	{
		spdlog::info("WheelFFB: refreshing attached force-feedback devices");
		stop();
		close_wheel();
		foundDevices.clear();
		publicDevices.clear();
		if (!directInput) return;
		directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, enumerate_device, nullptr, DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK);
		for (const auto& device : foundDevices) publicDevices.push_back(device);
		if (foundDevices.empty())
		{
			statusText = "No force-feedback wheel detected";
			spdlog::warn("WheelFFB: no attached device reported DirectInput force-feedback support");
		}
		else open_with_fallback(Settings::WheelFFBDevice.get());
	}

	void select(const std::string& id)
	{
		spdlog::info("WheelFFB: user selected device [{}]", id);
		open_with_fallback(id);
	}

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
		// Several wheel drivers (including Fanatec) expose one physical force
		// actuator but expect the legacy DirectInput X/Y polar descriptor. This
		// is also the layout used by Microsoft's own constant-force example.
		DWORD axes[] = { DIJOFS_X, DIJOFS_Y };
		const bool reverse = (direction < 0.f) != bool(Settings::WheelFFBInvert);
		LONG directions[] = { reverse ? 27000L : 9000L, 0L };
		const LONG magnitude = (std::clamp)(LONG(Settings::WheelFFBStrength) * 20L, 0L, 2000L);
		DICONSTANTFORCE force{ magnitude };
		DIEFFECT effect{};
		effect.dwSize = sizeof(effect);
		effect.dwFlags = DIEFF_POLAR | DIEFF_OBJECTOFFSETS;
		effect.dwDuration = 350000;
		effect.dwSamplePeriod = 0;
		effect.dwGain = DI_FFNOMINALMAX;
		effect.dwTriggerButton = DIEB_NOTRIGGER;
		effect.dwTriggerRepeatInterval = 0;
		effect.cAxes = 2;
		effect.rgdwAxes = axes;
		effect.rglDirection = directions;
		effect.lpEnvelope = nullptr;
		effect.cbTypeSpecificParams = sizeof(force);
		effect.lpvTypeSpecificParams = &force;
		result = wheel->CreateEffect(GUID_ConstantForce, &effect, &testEffect, nullptr);
		if (FAILED(result))
		{
			// Single-axis devices reverse a constant force by its signed
			// magnitude. Keep a unit Cartesian direction and use the canonical
			// X offset rather than a driver-specific enumerated offset.
			effect.cAxes = 1;
			effect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
			directions[0] = 1;
			force.lMagnitude = reverse ? -magnitude : magnitude;
			spdlog::warn("WheelFFB: two-axis constant effect failed (DirectInput 0x{:08X}); retrying with one axis",
				static_cast<unsigned long>(result));
			result = wheel->CreateEffect(GUID_ConstantForce, &effect, &testEffect, nullptr);
		}
		if (FAILED(result))
		{
			statusText = failed_status("Creating the constant-force test", result);
			return;
		}
		result = testEffect->Start(1, 0);
		if (SUCCEEDED(result))
		{
			statusText = direction < 0.f ? "Left test force sent" : "Right test force sent";
			spdlog::info("WheelFFB: {} test started at {}% user strength using {} axis/axes",
				direction < 0.f ? "left" : "right", int(Settings::WheelFFBStrength), effect.cAxes);
			stopAt = std::chrono::steady_clock::now() + std::chrono::milliseconds(350);
		}
		else
		{
			statusText = failed_status("Starting the constant-force test", result);
			stop();
		}
	}

	void update() { if (testEffect && std::chrono::steady_clock::now() >= stopAt) stop(); }
	void setFocused(bool focused)
	{
		hasFocus = focused;
		if (!focused)
		{
			spdlog::info("WheelFFB: game lost focus; stopping active effects");
			stop();
		}
	}
	void stop() { if (testEffect) { testEffect->Stop(); testEffect->Release(); testEffect = nullptr; } }
	bool ready() { return wheel != nullptr; }
	const std::vector<DeviceInfo>& devices() { return publicDevices; }
	const std::string& status() { return statusText; }
}
