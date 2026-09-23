#include "wheel_force_feedback.hpp"

#include <dinput.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <format>
#include <spdlog/spdlog.h>

#include "Proxy.hpp"
#include "ffb_device_resolver.hpp"
#include "ffb_test_policy.hpp"
#include "output_exposure_observer.hpp"
#include "telemetry_probe.hpp"

// Keep game.hpp out of this translation unit: DirectInput's Windows headers
// define SND_* macros that collide with the game's SOUND_CMD enum.
namespace Module { extern std::filesystem::path UserIniPath; }

namespace Settings
{
	Setting<bool> WheelFFBEnabled{ "Controls", "WheelFFBEnabled", true,
		"Enable native force feedback for steering wheels." };
	Setting<int> WheelFFBStrength{ "Controls", "WheelFFBStrength", 100,
		"Master wheel force feedback strength.", Range<int>{ 0, 100 } };
	Setting<float> WheelFFBSpringStrength{ "Controls", "WheelFFBSpringStrength", 0.45f,
		"Speed-scaled steering centering strength.", Range<float>{ 0.0f, 1.0f } };
	Setting<float> WheelFFBDamperStrength{ "Controls", "WheelFFBDamperStrength", 0.10f,
		"Resistance to rapid steering movement.", Range<float>{ 0.0f, 1.0f } };
	Setting<float> WheelFFBImpactStrength{ "Controls", "WheelFFBImpactStrength", 0.65f,
		"Steering wheel kick from collisions and sharp vibration events.", Range<float>{ 0.0f, 1.0f } };
	Setting<float> WheelFFBRoadStrength{ "Controls", "WheelFFBRoadStrength", 0.50f,
		"Road and surface detail transmitted through the steering wheel.", Range<float>{ 0.0f, 1.0f } };
	Setting<float> WheelFFBGripLossStrength{ "Controls", "WheelFFBGripLossStrength", 0.55f,
		"How much steering weight lightens as the car slides.", Range<float>{ 0.0f, 1.0f } };
	Setting<bool> WheelFFBInvert{ "Controls", "WheelFFBInvert", false,
		"Reverse force feedback direction." };
	Setting<bool> WheelFFBDiagnosticLog{ "Controls", "WheelFFBDiagnosticLog", false,
		"Log detailed live force-feedback signals for troubleshooting." };
	Setting<std::string> WheelFFBDevice{ "Controls", "WheelFFBDevice", "",
		"Internal identifier for the wheel selected in the in-game controller screen." };
	namespace
	{
		struct HideLegacyFFBFields
		{
			HideLegacyFFBFields()
			{
				WheelFFBEnabled.hidden(true);
				WheelFFBDevice.hidden(true);
			}
		} hideLegacyFFBFields;
	}
}

namespace WheelForceFeedback
{
	namespace
	{
		struct EnumeratedDevice : DeviceInfo { GUID guid{}; };
		IDirectInput8W* directInput = nullptr;
		IDirectInputDevice8W* wheel = nullptr;
		IDirectInputEffect* testEffect = nullptr;
		IDirectInputEffect* driveEffect = nullptr;
		bool driveEffectTwoAxis = false;
		std::vector<EnumeratedDevice> foundDevices;
		std::vector<DeviceInfo> publicDevices;
		HWND gameWindow = nullptr;
		std::vector<DWORD> actuatorAxes;
		bool hasFocus = true;
		std::chrono::steady_clock::time_point stopAt{};
		std::chrono::steady_clock::time_point lastDriveUpdate{};
		std::chrono::steady_clock::time_point lastDriveRefresh{};
		std::chrono::steady_clock::time_point nextDriveCreateAttempt{};
		int driveRecoveryAttempt = 0;
		std::string statusText = "Not initialized";
		std::string activeDeviceId;

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
			spdlog::info("WheelFFB: enumerate [{}] CreateDevice begin", id);
			const HRESULT createResult = directInput->CreateDevice(instance->guidInstance, &device, nullptr);
			spdlog::info("WheelFFB: enumerate [{}] CreateDevice returned 0x{:08X}", id, static_cast<unsigned long>(createResult));
			if (FAILED(createResult))
			{
				spdlog::warn("WheelFFB: unable to inspect '{}' [{}], DirectInput 0x{:08X}", name, id, static_cast<unsigned long>(createResult));
				return DIENUM_CONTINUE;
			}
			DIDEVCAPS caps{ sizeof(caps) };
			spdlog::info("WheelFFB: enumerate [{}] GetCapabilities begin", id);
			const HRESULT capsResult = device->GetCapabilities(&caps);
			spdlog::info("WheelFFB: enumerate [{}] GetCapabilities returned 0x{:08X}", id, static_cast<unsigned long>(capsResult));
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
			spdlog::info("WheelFFB: enumerate [{}] Release begin", id);
			device->Release();
			spdlog::info("WheelFFB: enumerate [{}] Release complete", id);
			return DIENUM_CONTINUE;
		}

		BOOL CALLBACK detect_device(const DIDEVICEINSTANCEW*, void* context)
		{
			*static_cast<bool*>(context) = true;
			return DIENUM_STOP;
		}

		BOOL CALLBACK find_actuator_axis(const DIDEVICEOBJECTINSTANCEW* object, void*)
		{
			if ((object->dwType & DIDFT_FFACTUATOR) && actuatorAxes.size() < 2)
				actuatorAxes.push_back(object->dwOfs);
			return DIENUM_CONTINUE;
		}

		void close_wheel()
		{
			activeDeviceId.clear();
			if (testEffect)
			{
				spdlog::info("WheelFFB: close test effect begin");
				testEffect->Stop(); testEffect->Release(); testEffect = nullptr;
				spdlog::info("WheelFFB: close test effect complete");
			}
			if (driveEffect)
			{
				spdlog::info("WheelFFB: close driving effect begin");
				driveEffect->Stop(); driveEffect->Release(); driveEffect = nullptr;
				spdlog::info("WheelFFB: close driving effect complete");
			}
			if (wheel)
			{
				spdlog::info("WheelFFB: DISFFC_STOPALL begin");
				wheel->SendForceFeedbackCommand(DISFFC_STOPALL);
				spdlog::info("WheelFFB: Unacquire begin");
				wheel->Unacquire();
				spdlog::info("WheelFFB: wheel Release begin");
				wheel->Release(); wheel = nullptr;
				spdlog::info("WheelFFB: wheel Release complete");
			}
			nextDriveCreateAttempt = {};
		}

		HRESULT create_constant_effect(IDirectInputEffect** output, bool& twoAxis, DWORD duration, LONG signedMagnitude,
			bool tryTwoAxis = true)
		{
			DWORD axes[] = { DIJOFS_X, DIJOFS_Y };
			LONG directions[] = { signedMagnitude < 0 ? 27000L : 9000L, 0L };
			DICONSTANTFORCE force{ std::abs(signedMagnitude) };
			DIEFFECT effect{};
			effect.dwSize = sizeof(effect);
			effect.dwFlags = DIEFF_POLAR | DIEFF_OBJECTOFFSETS;
			effect.dwDuration = duration;
			effect.dwGain = DI_FFNOMINALMAX;
			effect.dwTriggerButton = DIEB_NOTRIGGER;
			effect.cAxes = 2;
			effect.rgdwAxes = axes;
			effect.rglDirection = directions;
			effect.cbTypeSpecificParams = sizeof(force);
			effect.lpvTypeSpecificParams = &force;
			HRESULT result = E_FAIL;
			twoAxis = false;
			if (tryTwoAxis)
			{
				result = wheel->CreateEffect(GUID_ConstantForce, &effect, output, nullptr);
				twoAxis = SUCCEEDED(result);
			}
			if (!twoAxis)
			{
				effect.cAxes = 1;
				effect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
				directions[0] = 1;
				force.lMagnitude = signedMagnitude;
				result = wheel->CreateEffect(GUID_ConstantForce, &effect, output, nullptr);
			}
			return result;
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
			spdlog::info("WheelFFB: selected CreateDevice begin");
			HRESULT result = directInput->CreateDevice(selected->guid, &wheel, nullptr);
			spdlog::info("WheelFFB: selected CreateDevice returned 0x{:08X}", static_cast<unsigned long>(result));
			if (FAILED(result))
			{
				statusText = failed_status("Opening the selected wheel", result);
				close_wheel();
				return false;
			}
			spdlog::info("WheelFFB: SetDataFormat begin");
			result = wheel->SetDataFormat(&c_dfDIJoystick2);
			spdlog::info("WheelFFB: SetDataFormat returned 0x{:08X}", static_cast<unsigned long>(result));
			if (FAILED(result))
			{
				statusText = failed_status("Setting the wheel data format", result);
				close_wheel();
				return false;
			}
			spdlog::info("WheelFFB: SetCooperativeLevel begin");
			result = wheel->SetCooperativeLevel(gameWindow, DISCL_EXCLUSIVE | DISCL_BACKGROUND);
			spdlog::info("WheelFFB: SetCooperativeLevel returned 0x{:08X}", static_cast<unsigned long>(result));
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

			spdlog::info("WheelFFB: Acquire begin");
			result = wheel->Acquire();
			spdlog::info("WheelFFB: Acquire returned 0x{:08X}", static_cast<unsigned long>(result));
			if (FAILED(result) && result != S_FALSE)
			{
				statusText = failed_status("Acquiring the selected wheel", result);
				close_wheel();
				return false;
			}
			spdlog::info("WheelFFB: DISFFC_RESET begin");
			wheel->SendForceFeedbackCommand(DISFFC_RESET);
			spdlog::info("WheelFFB: DISFFC_RESET complete");
			spdlog::info("WheelFFB: DISFFC_SETACTUATORSON begin");
			result = wheel->SendForceFeedbackCommand(DISFFC_SETACTUATORSON);
			spdlog::info("WheelFFB: DISFFC_SETACTUATORSON returned 0x{:08X}", static_cast<unsigned long>(result));
			if (FAILED(result))
			{
				statusText = failed_status("Enabling wheel actuators", result);
				close_wheel();
				return false;
			}
			actuatorAxes.clear();
			spdlog::info("WheelFFB: EnumObjects actuator axes begin");
			wheel->EnumObjects(find_actuator_axis, nullptr, DIDFT_AXIS);
			spdlog::info("WheelFFB: EnumObjects actuator axes complete");
			if (actuatorAxes.empty()) actuatorAxes.push_back(DIJOFS_X);

			// Some duplicate DirectInput interfaces claim FFB capability and accept
			// actuator commands, but cannot create a force effect. Validate the
			// actual output path before presenting an interface as ready. The
			// one-unit probe is 0.01% of nominal force and is stopped immediately.
			IDirectInputEffect* probeEffect = nullptr;
			bool probeTwoAxis = false;
			spdlog::info("WheelFFB: effect probe CreateEffect begin");
			result = create_constant_effect(&probeEffect, probeTwoAxis, 350000, 1);
			spdlog::info("WheelFFB: effect probe CreateEffect returned 0x{:08X} ({} axis/axes)",
				static_cast<unsigned long>(result), probeTwoAxis ? 2 : 1);
			if (FAILED(result) || !probeEffect)
			{
				statusText = failed_status("Creating a wheel force effect", FAILED(result) ? result : E_FAIL);
				close_wheel();
				return false;
			}
			spdlog::info("WheelFFB: effect probe Start begin");
			result = probeEffect->Start(1, 0);
			spdlog::info("WheelFFB: effect probe Start returned 0x{:08X}", static_cast<unsigned long>(result));
			const HRESULT stopResult = probeEffect->Stop();
			spdlog::info("WheelFFB: effect probe Stop returned 0x{:08X}", static_cast<unsigned long>(stopResult));
			probeEffect->Release();
			if (FAILED(result) || FAILED(stopResult))
			{
				statusText = FAILED(result)
					? failed_status("Starting a wheel force effect", result)
					: failed_status("Stopping the wheel force probe", stopResult);
				close_wheel();
				return false;
			}

			spdlog::info("WheelFFB: '{}' ready with {} force actuator axis/axes", selected->name, actuatorAxes.size());
			statusText = selected->name + " is ready";
			activeDeviceId = selected->id;
			return true;
		}

		bool open_with_fallback(std::string requestedId)
		{
			const auto order = FFBDeviceResolver::candidate_order(foundDevices, requestedId);
			std::string originalFailure;
			for (size_t attempt = 0; attempt < order.size(); ++attempt)
			{
				const auto& candidate = foundDevices[order[attempt]];
				Settings::WheelFFBDevice = candidate.id;
				spdlog::info("WheelFFB: validating candidate {}/{} '{}' [{}]", attempt + 1,
					order.size(), candidate.name, candidate.id);
				if (open_selected())
				{
					if (attempt != 0)
					{
						statusText = candidate.name + " is ready (automatic FFB fallback)";
						spdlog::warn("WheelFFB: automatic fallback succeeded after: {}", originalFailure);
					}
					if (FFBDeviceResolver::should_persist_choice(foundDevices, requestedId, candidate.id))
						Settings::write(Module::UserIniPath);
					else
					{
						Settings::WheelFFBDevice = requestedId;
						if (candidate.id != requestedId)
							spdlog::info("WheelFFB: using a temporary endpoint while preserving the saved preference");
					}
					return true;
				}
				if (attempt == 0)
					originalFailure = statusText;
			}

			Settings::WheelFFBDevice = requestedId;
			if (!originalFailure.empty())
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

	bool has_attached_device()
	{
		using CreateFn = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
		auto create = reinterpret_cast<CreateFn>(GetProcAddress(proxy::origModule, "DirectInput8Create"));
		IDirectInput8W* probe = nullptr;
		if (!create || FAILED(create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8W,
			reinterpret_cast<void**>(&probe), nullptr)))
			return false;

		bool found = false;
		probe->EnumDevices(DI8DEVCLASS_GAMECTRL, detect_device, &found, DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK);
		probe->Release();
		return found;
	}

	void shutdown()
	{
		spdlog::info("WheelFFB: shutting down and stopping all effects");
		TelemetryProbe::shutdown();
		stop();
		close_wheel();
		if (directInput) { directInput->Release(); directInput = nullptr; }
	}

	void refresh()
	{
		spdlog::info("WheelFFB: refreshing attached force-feedback devices");
		spdlog::info("WheelFFB: refresh stop effects begin");
		stop();
		spdlog::info("WheelFFB: refresh stop effects complete; close wheel begin");
		close_wheel();
		spdlog::info("WheelFFB: refresh close wheel complete");
		foundDevices.clear();
		publicDevices.clear();
		driveRecoveryAttempt = 0;
		if (!directInput) return;
		spdlog::info("WheelFFB: refresh EnumDevices begin");
		const HRESULT enumResult = directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, enumerate_device, nullptr, DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK);
		spdlog::info("WheelFFB: refresh EnumDevices returned 0x{:08X}; {} candidates", static_cast<unsigned long>(enumResult), foundDevices.size());
		for (const auto& device : foundDevices) publicDevices.push_back(device);
		if (foundDevices.empty())
		{
			statusText = "No force-feedback wheel detected";
			spdlog::warn("WheelFFB: no attached device reported DirectInput force-feedback support");
		}
		else
		{
			spdlog::info("WheelFFB: refresh open candidate begin");
			open_with_fallback(Settings::WheelFFBDevice.get());
			spdlog::info("WheelFFB: refresh open candidate complete; ready={}", wheel != nullptr);
		}
	}

	void select(const std::string& id)
	{
		spdlog::info("WheelFFB: user selected device [{}]", id);
		driveRecoveryAttempt = 0;
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
		const bool reverse = FFBTestPolicy::reverse_direction(direction, Settings::WheelFFBInvert);
		LONG directions[] = { reverse ? 27000L : 9000L, 0L };
		// Direction tests remain capped at 20% nominal output, independently of
		// Reference+ presentation and Force Character.
		const LONG magnitude = FFBTestPolicy::DirectionTestMagnitude;
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
			spdlog::info("WheelFFB: {} test started at 20% nominal force using {} axis/axes",
				direction < 0.f ? "left" : "right", effect.cAxes);
			stopAt = std::chrono::steady_clock::now() + std::chrono::milliseconds(350);
		}
		else
		{
			statusText = failed_status("Starting the constant-force test", result);
			stop();
		}
	}

	void drive(float normalizedForce)
	{
		if (!wheel || !hasFocus || !Settings::WheelFFBEnabled || testEffect)
			return;
		const auto now = std::chrono::steady_clock::now();
		lastDriveUpdate = now;
		const float rawForce = normalizedForce;
		if (Settings::WheelFFBInvert) normalizedForce = -normalizedForce;
		const float scaledRequest = normalizedForce * Settings::WheelFFBStrength / 100.0f;
		HYP36ROutputExposure::observe_directinput_clamp(
			std::isfinite(scaledRequest) && std::abs(scaledRequest) > 1.0f);
		const LONG magnitude = (std::clamp)(LONG(normalizedForce * Settings::WheelFFBStrength * 100.0f),
			LONG(-DI_FFNOMINALMAX), LONG(DI_FFNOMINALMAX));
		if (Settings::TelemetryEnabled)
		{
			TelemetryProbe::observe_ffb(rawForce, scaledRequest,
				static_cast<float>(magnitude) / static_cast<float>(DI_FFNOMINALMAX),
				static_cast<float>(Settings::WheelFFBStrength) / 100.0f);
		}
		if (!driveEffect)
		{
			if (now < nextDriveCreateAttempt)
				return;
			HRESULT result = create_constant_effect(&driveEffect, driveEffectTwoAxis, 250000, magnitude);
			if (FAILED(result))
			{
				if (driveRecoveryAttempt == 0)
				{
					const std::string preferredId = Settings::WheelFFBDevice.get();
					const std::string currentId = activeDeviceId.empty() ? Settings::WheelFFBDevice.get() : activeDeviceId;
					spdlog::warn("WheelFFB: live force was unavailable (DirectInput 0x{:08X}); reacquiring current interface",
						static_cast<unsigned long>(result));
					Settings::WheelFFBDevice = currentId;
					const bool reopened = open_selected();
					Settings::WheelFFBDevice = preferredId;
					if (!reopened)
					{
						open_with_fallback(preferredId);
						if (!wheel)
							return;
					}
					driveRecoveryAttempt = 1;
					statusText = "Wheel force control was lost; reconnecting...";
					nextDriveCreateAttempt = now + std::chrono::milliseconds(100);
				}
				else if (driveRecoveryAttempt == 1 && foundDevices.size() > 1)
				{
					const std::string preferredId = Settings::WheelFFBDevice.get();
					const std::string currentId = activeDeviceId.empty() ? preferredId : activeDeviceId;
					const auto order = FFBDeviceResolver::candidate_order(foundDevices, currentId);
					if (order.size() > 1)
					{
						const auto& candidate = foundDevices[order[1]];
						spdlog::warn("WheelFFB: reacquiring current interface did not restore force; trying next candidate '{}' [{}]",
							candidate.name, candidate.id);
						Settings::WheelFFBDevice = candidate.id;
						const bool reopened = open_selected();
						Settings::WheelFFBDevice = preferredId;
						if (!reopened)
						{
							open_with_fallback(preferredId);
							if (!wheel)
								return;
						}
						else if (FFBDeviceResolver::should_persist_choice(foundDevices, preferredId, candidate.id))
						{
							Settings::WheelFFBDevice = candidate.id;
							Settings::write(Module::UserIniPath);
						}
						driveRecoveryAttempt = 2;
						statusText = "Trying another wheel force interface...";
						nextDriveCreateAttempt = now + std::chrono::milliseconds(100);
					}
				}
				else
				{
					statusText = failed_status("Restoring live wheel force", result);
					nextDriveCreateAttempt = now + std::chrono::seconds(5);
				}
				return;
			}
			result = driveEffect->Start(1, 0);
			if (FAILED(result))
			{
				statusText = failed_status("Starting the live driving effect", result);
				driveEffect->Release();
				driveEffect = nullptr;
				nextDriveCreateAttempt = now + std::chrono::seconds(1);
			}
			else
			{
				if (driveRecoveryAttempt != 0)
					statusText = "Wheel force reconnected";
				driveRecoveryAttempt = 0;
				nextDriveCreateAttempt = {};
				lastDriveRefresh = now;
				spdlog::info("WheelFFB: live driving effect started using {} axis/axes{}", driveEffectTwoAxis ? 2 : 1,
					driveEffectTwoAxis ? "" : " (15 Hz compatibility mode)");
			}
			return;
		}

		// Some single-axis drivers accept SetParameters but silently retain the
		// magnitude used at CreateEffect time. The direction tests prove that
		// creating and starting a new effect works on those devices, so refresh
		// that exact descriptor at a conservative rate. Two-axis wheels keep the
		// normal smooth SetParameters path below.
		if (!driveEffectTwoAxis)
		{
			if (now - lastDriveRefresh < std::chrono::milliseconds(66))
				return;
			lastDriveRefresh = now;
			IDirectInputEffect* replacement = nullptr;
			bool replacementTwoAxis = false;
			HRESULT result = create_constant_effect(&replacement, replacementTwoAxis, 250000, magnitude, false);
			if (FAILED(result))
			{
				statusText = failed_status("Refreshing the live driving effect", result);
				return;
			}
			result = replacement->Start(1, 0);
			if (FAILED(result))
			{
				statusText = failed_status("Restarting the live driving effect", result);
				replacement->Release();
				return;
			}
			// Keep the previous force running until its replacement has started.
			// This avoids the brief zero-torque gap that made compatibility mode
			// feel weaker than the requested strength on direct-drive wheels.
			driveEffect->Stop();
			driveEffect->Release();
			driveEffect = replacement;
			driveEffectTwoAxis = replacementTwoAxis;
			return;
		}

		DWORD axes[] = { DIJOFS_X, DIJOFS_Y };
		LONG directions[] = { magnitude < 0 ? 27000L : 9000L, 0L };
		if (!driveEffectTwoAxis) directions[0] = 1;
		DICONSTANTFORCE force{ driveEffectTwoAxis ? std::abs(magnitude) : magnitude };
		DIEFFECT effect{};
		effect.dwSize = sizeof(effect);
		effect.dwFlags = (driveEffectTwoAxis ? DIEFF_POLAR : DIEFF_CARTESIAN) | DIEFF_OBJECTOFFSETS;
		effect.cAxes = driveEffectTwoAxis ? 2 : 1;
		effect.rgdwAxes = axes;
		effect.rglDirection = directions;
		effect.cbTypeSpecificParams = sizeof(force);
		effect.lpvTypeSpecificParams = &force;
		const HRESULT result = driveEffect->SetParameters(&effect, DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_START);
		if (FAILED(result))
		{
			statusText = failed_status("Updating the live driving effect", result);
			driveEffect->Release();
			driveEffect = nullptr;
		}
	}

	void update()
	{
		const auto now = std::chrono::steady_clock::now();
		if (testEffect && now >= stopAt)
		{
			testEffect->Stop();
			testEffect->Release();
			testEffect = nullptr;
		}
		if (driveEffect && now - lastDriveUpdate > std::chrono::milliseconds(250))
		{
			spdlog::info("WheelFFB: driving update watchdog stopped stale force");
			driveEffect->Stop();
			driveEffect->Release();
			driveEffect = nullptr;
		}
	}
	void setFocused(bool focused)
	{
		hasFocus = focused;
		if (!focused)
		{
			spdlog::info("WheelFFB: game lost focus; stopping active effects");
			stop();
		}
	}
	void stop()
	{
		if (testEffect)
		{
			spdlog::info("WheelFFB: stop test effect begin");
			testEffect->Stop(); testEffect->Release(); testEffect = nullptr;
			spdlog::info("WheelFFB: stop test effect complete");
		}
		if (driveEffect)
		{
			spdlog::info("WheelFFB: stop driving effect begin");
			driveEffect->Stop(); driveEffect->Release(); driveEffect = nullptr;
			spdlog::info("WheelFFB: stop driving effect complete");
		}
	}
	bool ready() { return wheel != nullptr; }
	const std::vector<DeviceInfo>& devices() { return publicDevices; }
	const std::string& active_device_id() { return activeDeviceId; }
	const std::string& status() { return statusText; }
}
