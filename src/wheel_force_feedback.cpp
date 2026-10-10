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
#include "ffb_configuration.hpp"
#include "ffb_test_policy.hpp"
#include "output_exposure_observer.hpp"
#include "telemetry_probe.hpp"
#include "ffb_output_observer.hpp"
#include "simhub_live.hpp"

// Keep game.hpp out of this translation unit: DirectInput's Windows headers
// define SND_* macros that collide with the game's SOUND_CMD enum.
namespace Module { extern std::filesystem::path UserIniPath; }

namespace Settings
{
	Setting<bool> WheelFFBEnabled{ "Controls", "WheelFFBEnabled", HYP36RFFBConfiguration::DefaultEnabled,
		"Enable native force feedback for steering wheels." };
	Setting<int> WheelFFBStrength{ "Controls", "WheelFFBStrength", HYP36RFFBConfiguration::DefaultStrength,
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
	Setting<bool> WheelFFBInvert{ "Controls", "WheelFFBInvert", HYP36RFFBConfiguration::DefaultInvert,
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
				WheelFFBStrength.hidden(true);
				WheelFFBSpringStrength.hidden(true);
				WheelFFBDamperStrength.hidden(true);
				WheelFFBImpactStrength.hidden(true);
				WheelFFBRoadStrength.hidden(true);
				WheelFFBGripLossStrength.hidden(true);
				WheelFFBInvert.hidden(true);
				WheelFFBDiagnosticLog.hidden(true);
				WheelFFBDevice.hidden(true);
			}
		} hideLegacyFFBFields;
	}
}

namespace WheelForceFeedback
{
	namespace
	{
		uint64_t observation_time_us()
		{
			return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
				std::chrono::steady_clock::now().time_since_epoch()).count());
		}
		struct EnumeratedDevice : DeviceInfo { GUID guid{}; };
		IDirectInput8W* directInput = nullptr;
		IDirectInputDevice8W* wheel = nullptr;
		IDirectInputEffect* testEffect = nullptr;
		IDirectInputEffect* driveEffect = nullptr;
		IDirectInputEffect* surfaceEffect = nullptr;
		IDirectInputEffect* surfaceBumpEffect = nullptr;
		bool driveEffectTwoAxis = false;
		std::vector<EnumeratedDevice> foundDevices;
		std::vector<DeviceInfo> publicDevices;
		HWND gameWindow = nullptr;
		std::vector<DWORD> actuatorAxes;
		bool hasFocus = true;
		std::chrono::steady_clock::time_point stopAt{};
		std::chrono::steady_clock::time_point surfaceBumpStopAt{};
		std::chrono::steady_clock::time_point lastDriveUpdate{};
		std::chrono::steady_clock::time_point lastDriveRefresh{};
		std::chrono::steady_clock::time_point nextDriveCreateAttempt{};
		int driveRecoveryAttempt = 0;
		std::string statusText = "Not initialized";
		std::string activeDeviceId;
		SurfaceStatus surfaceStatus{};
		HYP36RSurfaceRenderer::Waveform activeSurfaceWaveform = HYP36RSurfaceRenderer::DefaultWaveform;

		BOOL CALLBACK enumerate_periodic_effect(const DIEFFECTINFOW* info, void*)
		{
			surfaceStatus.periodicSupported = true;
			const bool dynamic = (info->dwDynamicParams & DIEP_TYPESPECIFICPARAMS) != 0;
			if (IsEqualGUID(info->guid, GUID_Sine))
			{
				surfaceStatus.sineSupported = true;
				surfaceStatus.sineDynamicSupported = dynamic;
				surfaceStatus.dynamicMagnitudeSupported |= dynamic;
				surfaceStatus.dynamicPeriodSupported = surfaceStatus.dynamicMagnitudeSupported;
			}
			else if (IsEqualGUID(info->guid, GUID_Triangle)) { surfaceStatus.triangleSupported = true; surfaceStatus.triangleDynamicSupported = dynamic; surfaceStatus.dynamicMagnitudeSupported |= dynamic; }
			else if (IsEqualGUID(info->guid, GUID_Square)) { surfaceStatus.squareSupported = true; surfaceStatus.squareDynamicSupported = dynamic; surfaceStatus.dynamicMagnitudeSupported |= dynamic; }
			return DIENUM_CONTINUE;
		}

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
				spdlog::warn("WheelFFB: unable to inspect '{}', DirectInput 0x{:08X}", name, static_cast<unsigned long>(createResult));
				return DIENUM_CONTINUE;
			}
			DIDEVCAPS caps{ sizeof(caps) };
			const HRESULT capsResult = device->GetCapabilities(&caps);
			if (SUCCEEDED(capsResult) && (caps.dwFlags & DIDC_FORCEFEEDBACK))
			{
				foundDevices.push_back({ { id, name }, instance->guidInstance });
				spdlog::info("WheelFFB: found '{}': {} axes, {} buttons, {} POVs, force feedback supported",
					name, caps.dwAxes, caps.dwButtons, caps.dwPOVs);
			}
			else if (FAILED(capsResult))
				spdlog::warn("WheelFFB: capability query failed for '{}', DirectInput 0x{:08X}", name, static_cast<unsigned long>(capsResult));
			else
				spdlog::info("WheelFFB: skipped '{}': driver reports no force-feedback capability", name);
			device->Release();
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
				testEffect->Stop(); testEffect->Release(); testEffect = nullptr;
			}
			if (driveEffect)
			{
				driveEffect->Stop(); driveEffect->Release(); driveEffect = nullptr;
			}
			if (surfaceEffect)
			{
				surfaceEffect->Stop(); surfaceEffect->Release(); surfaceEffect = nullptr;
			}
			if (surfaceBumpEffect) { surfaceBumpEffect->Stop(); surfaceBumpEffect->Release(); surfaceBumpEffect = nullptr; }
			surfaceStatus = {};
			if (wheel)
			{
				wheel->SendForceFeedbackCommand(DISFFC_STOPALL);
				wheel->Unacquire();
				wheel->Release(); wheel = nullptr;
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
			HYP36RFFBOutput::record_api(HYP36RFFBOutput::Operation::CreateEffect,
				static_cast<int32_t>(result), observation_time_us(), HYP36RFFBOutput::EffectType::Constant,
				twoAxis ? HYP36RFFBOutput::Strategy::TwoAxisPolar : HYP36RFFBOutput::Strategy::OneAxisCartesianRecreation,
				static_cast<uint32_t>(actuatorAxes.size()), wheel != nullptr);
			return result;
		}

		bool open_selected()
		{
			HYP36RFFBOutput::set_enabled(Settings::TelemetryEnabled);
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
			spdlog::info("WheelFFB: opening selected device '{}'", selected->name);
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
			HYP36RFFBOutput::record_api(HYP36RFFBOutput::Operation::Acquire, static_cast<int32_t>(result),
				observation_time_us(), HYP36RFFBOutput::EffectType::Unavailable,
				HYP36RFFBOutput::Strategy::Unavailable, static_cast<uint32_t>(actuatorAxes.size()), wheel != nullptr);
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
			surfaceStatus = {};
			wheel->EnumEffects(enumerate_periodic_effect, nullptr, DIEFT_PERIODIC);
			spdlog::info("Surface FFB: periodic effects {}; Sine {}; dynamic magnitude {}; dynamic period {}",
				surfaceStatus.periodicSupported ? "supported" : "unsupported",
				surfaceStatus.sineSupported ? "supported" : "unsupported",
				surfaceStatus.dynamicMagnitudeSupported ? "supported" : "unsupported",
				surfaceStatus.dynamicPeriodSupported ? "supported" : "unsupported");

			// Some duplicate DirectInput interfaces claim FFB capability and accept
			// actuator commands, but cannot create a force effect. Validate the
			// actual output path before presenting an interface as ready. The
			// one-unit probe is 0.01% of nominal force and is stopped immediately.
			IDirectInputEffect* probeEffect = nullptr;
			bool probeTwoAxis = false;
			result = create_constant_effect(&probeEffect, probeTwoAxis, 350000, 1);
			if (FAILED(result) || !probeEffect)
			{
				statusText = failed_status("Creating a wheel force effect", FAILED(result) ? result : E_FAIL);
				close_wheel();
				return false;
			}
			result = probeEffect->Start(1, 0);
			const HRESULT stopResult = probeEffect->Stop();
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
				spdlog::info("WheelFFB: validating candidate {}/{} '{}'", attempt + 1,
					order.size(), candidate.name);
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
		spdlog::info("WheelFFB: initializing native DirectInput backend");
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
		SimHubLive::shutdown();
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
		driveRecoveryAttempt = 0;
		if (!directInput) return;
		const HRESULT enumResult = directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, enumerate_device, nullptr, DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK);
		if (FAILED(enumResult))
			spdlog::warn("WheelFFB: device enumeration failed, DirectInput 0x{:08X}", static_cast<unsigned long>(enumResult));
		for (const auto& device : foundDevices) publicDevices.push_back(device);
		if (foundDevices.empty())
		{
			statusText = "No force-feedback wheel detected";
			spdlog::warn("WheelFFB: no attached device reported DirectInput force-feedback support");
		}
		else
		{
			open_with_fallback(Settings::WheelFFBDevice.get());
		}
	}

	void select(const std::string& id)
	{
		spdlog::info("WheelFFB: user selected a force-feedback endpoint");
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
		HYP36RFFBOutput::set_enabled(Settings::TelemetryEnabled);
		if (!wheel || !hasFocus || !Settings::WheelFFBEnabled || testEffect)
		{
			if (!hasFocus) HYP36RFFBOutput::safety(HYP36RFFBOutput::SafetyState::FocusLost);
			else if (!Settings::WheelFFBEnabled) HYP36RFFBOutput::safety(HYP36RFFBOutput::SafetyState::Disabled);
			return;
		}
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
		const auto strategy = driveEffectTwoAxis ? HYP36RFFBOutput::Strategy::TwoAxisPolar
			: HYP36RFFBOutput::Strategy::OneAxisCartesianRecreation;
		HYP36RFFBOutput::record({ static_cast<float>(magnitude) / static_cast<float>(DI_FFNOMINALMAX),
			static_cast<int32_t>(magnitude), magnitude < 0 ? 27000 : 9000,
			HYP36RFFBOutput::EffectType::Constant, strategy, static_cast<uint32_t>(actuatorAxes.size()),
			wheel != nullptr, HYP36RFFBOutput::Operation::None, 0, observation_time_us(),
			driveEffectTwoAxis ? 0u : 66000u, false, false });
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
			HYP36RFFBOutput::record_api(HYP36RFFBOutput::Operation::Start, static_cast<int32_t>(result),
				observation_time_us(), HYP36RFFBOutput::EffectType::Constant,
				driveEffectTwoAxis ? HYP36RFFBOutput::Strategy::TwoAxisPolar : HYP36RFFBOutput::Strategy::OneAxisCartesianRecreation,
				static_cast<uint32_t>(actuatorAxes.size()), wheel != nullptr);
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
			HYP36RFFBOutput::record({ static_cast<float>(magnitude) / static_cast<float>(DI_FFNOMINALMAX),
				static_cast<int32_t>(magnitude), 1, HYP36RFFBOutput::EffectType::Constant,
				HYP36RFFBOutput::Strategy::OneAxisCartesianRecreation, static_cast<uint32_t>(actuatorAxes.size()),
				wheel != nullptr, HYP36RFFBOutput::Operation::Start, static_cast<int32_t>(result),
				observation_time_us(), 66000, true, false });
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
		HYP36RFFBOutput::record({ static_cast<float>(magnitude) / static_cast<float>(DI_FFNOMINALMAX),
			static_cast<int32_t>(magnitude), static_cast<int32_t>(directions[0]), HYP36RFFBOutput::EffectType::Constant,
			HYP36RFFBOutput::Strategy::TwoAxisPolar, static_cast<uint32_t>(actuatorAxes.size()), wheel != nullptr,
			HYP36RFFBOutput::Operation::SetParameters, static_cast<int32_t>(result), observation_time_us(), 0, false, true });
		if (FAILED(result))
		{
			statusText = failed_status("Updating the live driving effect", result);
			driveEffect->Release();
			driveEffect = nullptr;
		}
	}

	void stop_surface()
	{
		if (surfaceEffect)
		{
			const HRESULT stopResult = surfaceEffect->Stop();
			HYP36RFFBOutput::record_api(HYP36RFFBOutput::Operation::Stop, static_cast<int32_t>(stopResult),
				observation_time_us(), activeSurfaceWaveform == HYP36RSurfaceRenderer::Waveform::Triangle
					? HYP36RFFBOutput::EffectType::Triangle : activeSurfaceWaveform == HYP36RSurfaceRenderer::Waveform::Square
					? HYP36RFFBOutput::EffectType::Square : HYP36RFFBOutput::EffectType::Sine,
				HYP36RFFBOutput::Strategy::PeriodicPersistent, static_cast<uint32_t>(actuatorAxes.size()), wheel != nullptr);
			surfaceEffect->Release();
			surfaceEffect = nullptr;
		}
		surfaceStatus.active = false;
		surfaceStatus.requestedMagnitude = 0.0f;
		if (surfaceBumpEffect) { surfaceBumpEffect->Stop(); surfaceBumpEffect->Release(); surfaceBumpEffect = nullptr; }
		surfaceStatus.bumpActive = false;
		surfaceStatus.bumpRequestedMagnitude = 0.0f;
	}

	void trigger_surface_bump(float signedMagnitude, int durationMilliseconds, bool enabled)
	{
		if (!enabled || !wheel || !hasFocus || !Settings::WheelFFBEnabled || testEffect) return;
		const float bounded = HYP36RSurfaceRenderer::bound_bump_transport_magnitude(signedMagnitude);
		if (std::abs(bounded) <= 0.0001f) return;
		if (surfaceBumpEffect) { surfaceBumpEffect->Stop(); surfaceBumpEffect->Release(); surfaceBumpEffect = nullptr; }
		DWORD axis = DIJOFS_X; LONG direction = 1L;
		DICONSTANTFORCE force{ LONG(bounded * DI_FFNOMINALMAX) };
		DIEFFECT effect{}; effect.dwSize = sizeof(effect); effect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
		effect.dwDuration = DWORD((std::clamp)(durationMilliseconds, 20, 200)) * 1000; effect.dwGain = DI_FFNOMINALMAX;
		effect.dwTriggerButton = DIEB_NOTRIGGER; effect.cAxes = 1; effect.rgdwAxes = &axis;
		effect.rglDirection = &direction; effect.cbTypeSpecificParams = sizeof(force); effect.lpvTypeSpecificParams = &force;
		const HRESULT result = wheel->CreateEffect(GUID_ConstantForce, &effect, &surfaceBumpEffect, nullptr);
		HRESULT startResult = FAILED(result) || !surfaceBumpEffect ? result : surfaceBumpEffect->Start(1, 0);
		HYP36RFFBOutput::record({ bounded, static_cast<int32_t>(force.lMagnitude), 1,
			HYP36RFFBOutput::EffectType::Bump, HYP36RFFBOutput::Strategy::OneShotBump,
			static_cast<uint32_t>(actuatorAxes.size()), wheel != nullptr, FAILED(result)
				? HYP36RFFBOutput::Operation::CreateEffect : HYP36RFFBOutput::Operation::Start,
			static_cast<int32_t>(startResult), observation_time_us(), 0, false, false });
		if (FAILED(result) || !surfaceBumpEffect || FAILED(startResult)) {
			if (surfaceBumpEffect) { surfaceBumpEffect->Release(); surfaceBumpEffect = nullptr; }
			spdlog::warn("Surface FFB: bump pulse failed (DirectInput 0x{:08X})", static_cast<unsigned long>(result)); return;
		}
		surfaceStatus.bumpActive = true; surfaceStatus.bumpRequestedMagnitude = std::abs(bounded);
		surfaceBumpStopAt = std::chrono::steady_clock::now() + std::chrono::milliseconds((std::clamp)(durationMilliseconds, 20, 200));
	}

	void drive_surface(float magnitude, float frequencyHz, int amplitudeCeilingPercent,
		HYP36RSurfaceRenderer::Waveform waveform, bool enabled)
	{
		if (surfaceEffect && activeSurfaceWaveform != waveform)
		{
			surfaceEffect->Stop(); surfaceEffect->Release(); surfaceEffect = nullptr;
			surfaceStatus.active = false; surfaceStatus.requestedMagnitude = 0.0f;
		}
		const float safetyCeiling = (std::clamp)(float(amplitudeCeilingPercent) / 100.0f, 0.12f, 1.00f);
		surfaceStatus.requestedMagnitude = (std::clamp)(
			std::isfinite(magnitude) ? magnitude : 0.0f, 0.0f, safetyCeiling);
		surfaceStatus.frequencyHz = (std::clamp)(
			std::isfinite(frequencyHz) ? frequencyHz : 18.0f, 12.0f, 60.0f);
		const bool waveformSupported = waveform == HYP36RSurfaceRenderer::Waveform::Sine
			? surfaceStatus.sineSupported && surfaceStatus.sineDynamicSupported
			: waveform == HYP36RSurfaceRenderer::Waveform::Triangle
			? surfaceStatus.triangleSupported && surfaceStatus.triangleDynamicSupported
			: surfaceStatus.squareSupported && surfaceStatus.squareDynamicSupported;
		if (!enabled || !wheel || !hasFocus || !Settings::WheelFFBEnabled || testEffect ||
			!waveformSupported || !surfaceStatus.dynamicMagnitudeSupported)
		{
			stop_surface();
			return;
		}
		if (surfaceStatus.requestedMagnitude <= 0.0001f)
		{
			if (surfaceEffect) { surfaceEffect->Stop(); surfaceEffect->Release(); surfaceEffect = nullptr; }
			surfaceStatus.active = false;
			return;
		}

		DWORD axis = DIJOFS_X;
		LONG direction = 1;
		DIPERIODIC periodic{};
		periodic.dwMagnitude = DWORD((std::clamp)(LONG(surfaceStatus.requestedMagnitude * DI_FFNOMINALMAX), 0L, LONG(DI_FFNOMINALMAX)));
		periodic.lOffset = 0;
		periodic.dwPhase = 0;
		periodic.dwPeriod = DWORD(1000000.0f / surfaceStatus.frequencyHz);
		DIEFFECT effect{};
		effect.dwSize = sizeof(effect);
		effect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
		effect.dwDuration = INFINITE;
		effect.dwGain = DI_FFNOMINALMAX;
		effect.dwTriggerButton = DIEB_NOTRIGGER;
		effect.cAxes = 1;
		effect.rgdwAxes = &axis;
		effect.rglDirection = &direction;
		effect.cbTypeSpecificParams = sizeof(periodic);
		effect.lpvTypeSpecificParams = &periodic;

		if (!surfaceEffect)
		{
			const GUID& effectGuid = waveform == HYP36RSurfaceRenderer::Waveform::Triangle
				? GUID_Triangle : waveform == HYP36RSurfaceRenderer::Waveform::Square ? GUID_Square : GUID_Sine;
			const HRESULT createResult = wheel->CreateEffect(effectGuid, &effect, &surfaceEffect, nullptr);
			HYP36RFFBOutput::record_api(HYP36RFFBOutput::Operation::CreateEffect, static_cast<int32_t>(createResult),
				observation_time_us(), waveform == HYP36RSurfaceRenderer::Waveform::Triangle ? HYP36RFFBOutput::EffectType::Triangle
				: waveform == HYP36RSurfaceRenderer::Waveform::Square ? HYP36RFFBOutput::EffectType::Square : HYP36RFFBOutput::EffectType::Sine,
				HYP36RFFBOutput::Strategy::PeriodicPersistent, static_cast<uint32_t>(actuatorAxes.size()), wheel != nullptr);
			if (FAILED(createResult) || !surfaceEffect)
			{
				spdlog::warn("Surface FFB: {} creation failed (DirectInput 0x{:08X})",
					HYP36RSurfaceRenderer::waveform_name(waveform),
					static_cast<unsigned long>(createResult));
				if (waveform == HYP36RSurfaceRenderer::Waveform::Sine) surfaceStatus.sineSupported = false;
				else if (waveform == HYP36RSurfaceRenderer::Waveform::Triangle) surfaceStatus.triangleSupported = false;
				else surfaceStatus.squareSupported = false;
				stop_surface();
				return;
			}
			const HRESULT startResult = surfaceEffect->Start(1, 0);
			HYP36RFFBOutput::record({ surfaceStatus.requestedMagnitude, static_cast<int32_t>(periodic.dwMagnitude), 1,
				waveform == HYP36RSurfaceRenderer::Waveform::Triangle ? HYP36RFFBOutput::EffectType::Triangle
				: waveform == HYP36RSurfaceRenderer::Waveform::Square ? HYP36RFFBOutput::EffectType::Square : HYP36RFFBOutput::EffectType::Sine,
				HYP36RFFBOutput::Strategy::PeriodicPersistent, static_cast<uint32_t>(actuatorAxes.size()), wheel != nullptr,
				HYP36RFFBOutput::Operation::Start, static_cast<int32_t>(startResult), observation_time_us(), 0, false, false });
			if (FAILED(startResult))
			{
				spdlog::warn("Surface FFB: {} start failed (DirectInput 0x{:08X})",
					HYP36RSurfaceRenderer::waveform_name(waveform),
					static_cast<unsigned long>(startResult));
				stop_surface();
				return;
			}
			surfaceStatus.active = true;
			activeSurfaceWaveform = waveform;
			spdlog::info("Surface FFB: {} effect started", HYP36RSurfaceRenderer::waveform_name(waveform));
			return;
		}

		DWORD flags = DIEP_TYPESPECIFICPARAMS | DIEP_START;
		const HRESULT updateResult = surfaceEffect->SetParameters(&effect, flags);
		HYP36RFFBOutput::record({ surfaceStatus.requestedMagnitude, static_cast<int32_t>(periodic.dwMagnitude), 1,
			waveform == HYP36RSurfaceRenderer::Waveform::Triangle ? HYP36RFFBOutput::EffectType::Triangle
			: waveform == HYP36RSurfaceRenderer::Waveform::Square ? HYP36RFFBOutput::EffectType::Square : HYP36RFFBOutput::EffectType::Sine,
			HYP36RFFBOutput::Strategy::PeriodicPersistent, static_cast<uint32_t>(actuatorAxes.size()), wheel != nullptr,
			HYP36RFFBOutput::Operation::SetParameters, static_cast<int32_t>(updateResult), observation_time_us(), 0, false, true });
		if (FAILED(updateResult))
		{
			spdlog::warn("Surface FFB: {} update failed (DirectInput 0x{:08X})",
				HYP36RSurfaceRenderer::waveform_name(waveform),
				static_cast<unsigned long>(updateResult));
			stop_surface();
		}
		else surfaceStatus.active = true;
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
			HYP36RFFBOutput::safety(HYP36RFFBOutput::SafetyState::Watchdog);
			spdlog::info("WheelFFB: driving update watchdog stopped stale force");
			const HRESULT stopResult = driveEffect->Stop();
			HYP36RFFBOutput::record_api(HYP36RFFBOutput::Operation::Stop, static_cast<int32_t>(stopResult),
				observation_time_us(), HYP36RFFBOutput::EffectType::Constant,
				driveEffectTwoAxis ? HYP36RFFBOutput::Strategy::TwoAxisPolar : HYP36RFFBOutput::Strategy::OneAxisCartesianRecreation,
				static_cast<uint32_t>(actuatorAxes.size()), wheel != nullptr);
			driveEffect->Release();
			driveEffect = nullptr;
		}
		if (surfaceEffect && (now - lastDriveUpdate > std::chrono::milliseconds(250) ||
			!Settings::WheelFFBEnabled || !hasFocus))
		{
			spdlog::info("Surface FFB: watchdog stopped stale effect");
			stop_surface();
		}
		if (surfaceBumpEffect && now >= surfaceBumpStopAt) {
			surfaceBumpEffect->Stop(); surfaceBumpEffect->Release(); surfaceBumpEffect = nullptr;
			surfaceStatus.bumpActive = false; surfaceStatus.bumpRequestedMagnitude = 0.0f;
		}
	}
	void setFocused(bool focused)
	{
		hasFocus = focused;
		if (!focused)
		{
			HYP36RFFBOutput::safety(HYP36RFFBOutput::SafetyState::FocusLost);
			spdlog::info("WheelFFB: game lost focus; stopping active effects");
			stop();
		}
	}
	void stop()
	{
		HYP36RFFBOutput::safety(HYP36RFFBOutput::SafetyState::Stopped);
		if (testEffect)
		{
			testEffect->Stop(); testEffect->Release(); testEffect = nullptr;
		}
		if (driveEffect)
		{
			const HRESULT stopResult = driveEffect->Stop();
			HYP36RFFBOutput::record_api(HYP36RFFBOutput::Operation::Stop, static_cast<int32_t>(stopResult),
				observation_time_us(), HYP36RFFBOutput::EffectType::Constant,
				driveEffectTwoAxis ? HYP36RFFBOutput::Strategy::TwoAxisPolar : HYP36RFFBOutput::Strategy::OneAxisCartesianRecreation,
				static_cast<uint32_t>(actuatorAxes.size()), wheel != nullptr);
			driveEffect->Release(); driveEffect = nullptr;
		}
		stop_surface();
	}
	bool ready() { return wheel != nullptr; }
	const std::vector<DeviceInfo>& devices() { return publicDevices; }
	const std::string& active_device_id() { return activeDeviceId; }
	const std::string& status() { return statusText; }
	size_t actuator_axis_count() { return actuatorAxes.size(); }
	const char* output_path()
	{
		if (!wheel) return "Inactive";
		if (!driveEffect) return "Stopped";
		return driveEffectTwoAxis ? "DirectInput constant force (two-axis)" : "DirectInput constant force (one-axis fallback)";
	}
	const SurfaceStatus& surface_status() { return surfaceStatus; }
}
