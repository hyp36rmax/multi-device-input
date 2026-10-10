#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define DIRECTINPUT_VERSION 0x0800
#include <Windows.h>
#include <dinput.h>
#include <shellapi.h>
#include <ShlObj.h>

#include <SDL3/SDL.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>

#include "device_diagnostics/device_diagnostics_model.hpp"
#include "device_diagnostics_report.hpp"
#include "overlay/device_diagnostics_help.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <numeric>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace DeviceDiagnostics;
using Clock = std::chrono::steady_clock;

namespace
{
	std::string utc_now() { return DeviceDiagnosticsReport::utc_timestamp(); }
	std::string local_filename_time()
	{
		const auto now=std::chrono::system_clock::now(); const auto value=std::chrono::system_clock::to_time_t(now); std::tm local{};
		localtime_s(&local,&value); char buffer[32]{}; std::strftime(buffer,sizeof(buffer),"%Y-%m-%d_%H%M%S",&local); return buffer;
	}

	std::filesystem::path documents_path()
	{
		wchar_t buffer[MAX_PATH]{};
		if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, buffer)))
			return buffer;
		return std::filesystem::current_path();
	}

	const char* state_name(ResultState state)
	{
		switch (state)
		{
		case ResultState::Completed: return "completed";
		case ResultState::Failed: return "failed";
		case ResultState::Unavailable: return "unavailable";
		case ResultState::Running: return "running";
		case ResultState::Untested: return "untested";
		case ResultState::Cancelled: return "cancelled";
		}
		return "unavailable";
	}

	void help_marker(const DeviceDiagnosticsHelp::Entry& entry)
	{
		ImGui::SameLine();
		ImGui::PushID(entry.id.data());
		if (ImGui::SmallButton("?")) ImGui::OpenPopup("help");
		if (ImGui::IsItemHovered() || ImGui::IsItemFocused()) ImGui::SetTooltip("%s", entry.description.data());
		if (ImGui::BeginPopup("help")) { ImGui::TextWrapped("%s", entry.description.data()); ImGui::EndPopup(); }
		ImGui::PopID();
	}

	struct SDLDeviceSession
	{
		std::vector<SDL_Joystick*> handles;
		std::vector<Device> devices;

		void close()
		{
			for (auto* handle : handles) SDL_CloseJoystick(handle);
			handles.clear(); devices.clear();
			SDL_QuitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD);
		}

		bool open(const char* backend, std::string& error, std::vector<std::string>* openingResults = nullptr)
		{
			close();
			const bool wgi = std::string_view(backend) == "Windows.Gaming.Input";
			const bool raw = std::string_view(backend) == "SDL3 RawInput";
			const bool di = std::string_view(backend) == "SDL3 DirectInput";
			const bool xi = std::string_view(backend) == "SDL3 XInput";
			SDL_SetHint(SDL_HINT_JOYSTICK_WGI, wgi ? "1" : "0");
			SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT, raw ? "1" : "0");
			SDL_SetHint(SDL_HINT_JOYSTICK_DIRECTINPUT, di ? "1" : "0");
			SDL_SetHint(SDL_HINT_XINPUT_ENABLED, xi ? "1" : "0");
			if (!SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD)) { error = SDL_GetError(); return false; }
			int count = 0;
			SDL_JoystickID* ids = SDL_GetJoysticks(&count);
			if (!ids) { error = SDL_GetError(); return false; }
			for (int index = 0; index < count; ++index)
			{
				SDL_Joystick* joystick = SDL_OpenJoystick(ids[index]);
				if (!joystick) { if(openingResults)openingResults->push_back(std::format("SDL device {} open failed: {}",ids[index],SDL_GetError())); continue; }
				handles.push_back(joystick);
				const char* name = SDL_GetJoystickName(joystick);
				devices.push_back({ std::to_string(SDL_GetJoystickID(joystick)), name ? name : "Unnamed input device", backend,
					SDL_GetJoystickVendor(joystick), SDL_GetJoystickProduct(joystick), SDL_GetNumJoystickAxes(joystick),
					SDL_GetNumJoystickButtons(joystick), SDL_GetNumJoystickHats(joystick), true, false });
				if(openingResults)openingResults->push_back(std::format("SDL device {} opened successfully",ids[index]));
			}
			SDL_free(ids);
			return true;
		}

		void open_delayed(SDL_JoystickID id, std::string_view backend, std::vector<std::string>& events, std::vector<std::string>& openingResults)
		{
			if(std::any_of(handles.begin(),handles.end(),[id](SDL_Joystick* value){return SDL_GetJoystickID(value)==id;}))return;
			SDL_Joystick* joystick=SDL_OpenJoystick(id);
			if(!joystick){const auto message=std::format("SDL device {} arrived but open failed: {}",id,SDL_GetError());events.push_back(utc_now()+": "+message);openingResults.push_back(message);return;}
			handles.push_back(joystick);const char* name=SDL_GetJoystickName(joystick);
			devices.push_back({std::to_string(id),name?name:"Unnamed input device",std::string(backend),SDL_GetJoystickVendor(joystick),SDL_GetJoystickProduct(joystick),SDL_GetNumJoystickAxes(joystick),SDL_GetNumJoystickButtons(joystick),SDL_GetNumJoystickHats(joystick),true,false});
			events.push_back(std::format("{}: SDL device {} arrived and opened successfully",utc_now(),id));
			openingResults.push_back(std::format("Delayed SDL device {} opened successfully",id));
		}

		void pump(std::vector<std::string>* delayed = nullptr)
		{
			SDL_UpdateJoysticks();
			(void)delayed;
		}
	};

	struct NativeFFB
	{
		struct Entry { GUID guid{}; std::string id, name; DWORD axes = 0, buttons = 0, povs = 0; std::vector<DWORD> actuatorAxes; std::vector<std::string> effects; };
		IDirectInput8W* api = nullptr;
		IDirectInputDevice8W* device = nullptr;
		IDirectInputEffect* effect = nullptr;
		HWND window = nullptr;
		std::vector<Entry> entries;
		std::vector<std::string> errors;
		std::vector<std::string> recoveryLog;
		RedetectLifecycle lastRecovery{};
		int selected = -1;
		LONG lastRequestedMagnitude = 0;
		LONG lastUserRequestedMagnitude = 0;
		LONG currentOutputMagnitude = 0;
		LONG lastNonzeroRequestedMagnitude = 0;
		LONG lastNonzeroEffectiveMagnitude = 0;
		bool outputEverRequested = false;
		std::string lastNonzeroRequestUtc;
		std::string lastNonzeroRequestContext = "not recorded";
		HRESULT lastAcquire = S_FALSE, lastActuators = S_FALSE, lastCreate = S_FALSE, lastDownload = S_FALSE, lastStart = S_FALSE, lastUpdate = S_FALSE, lastStop = S_FALSE;
		HRESULT lastGainQuery = S_FALSE;
		DWORD deviceGain = 0;
		std::string lastDescriptor = "untested";

		static std::string utf8(const wchar_t* value)
		{
			const int size = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
			std::string text(size > 0 ? size : 0, '\0');
			if (size > 1) { WideCharToMultiByte(CP_UTF8, 0, value, -1, text.data(), size, nullptr, nullptr); text.pop_back(); }
			return text;
		}
		static std::string guid_text(const GUID& guid)
		{
			return std::format("{{{:08X}-{:04X}-{:04X}-{:02X}{:02X}-{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}}}",
				guid.Data1,guid.Data2,guid.Data3,guid.Data4[0],guid.Data4[1],guid.Data4[2],guid.Data4[3],guid.Data4[4],guid.Data4[5],guid.Data4[6],guid.Data4[7]);
		}

		static BOOL CALLBACK enum_effect(const DIEFFECTINFOW* info, void* context)
		{
			static_cast<Entry*>(context)->effects.push_back(utf8(info->tszName)); return DIENUM_CONTINUE;
		}
		static BOOL CALLBACK enum_axis(const DIDEVICEOBJECTINSTANCEW* object, void* context)
		{
			static_cast<Entry*>(context)->actuatorAxes.push_back(object->dwOfs); return DIENUM_CONTINUE;
		}
		static BOOL CALLBACK enum_device(const DIDEVICEINSTANCEW* instance, void* context)
		{
			auto& self = *static_cast<NativeFFB*>(context);
			IDirectInputDevice8W* candidate = nullptr;
			if (FAILED(self.api->CreateDevice(instance->guidInstance, &candidate, nullptr))) return DIENUM_CONTINUE;
			DIDEVCAPS caps{ sizeof(caps) };
			if (SUCCEEDED(candidate->GetCapabilities(&caps)) && (caps.dwFlags & DIDC_FORCEFEEDBACK))
			{
				Entry entry{}; entry.guid=instance->guidInstance; entry.id=guid_text(instance->guidInstance); entry.name=utf8(instance->tszProductName);
				entry.axes=caps.dwAxes; entry.buttons=caps.dwButtons; entry.povs=caps.dwPOVs;
				candidate->SetDataFormat(&c_dfDIJoystick2);
				candidate->EnumObjects(enum_axis, &entry, DIDFT_AXIS | DIDFT_FFACTUATOR);
				candidate->EnumEffects(enum_effect, &entry, DIEFT_ALL);
				self.entries.push_back(std::move(entry));
			}
			candidate->Release(); return DIENUM_CONTINUE;
		}

		bool initialize(HINSTANCE instance, HWND hwnd)
		{
			window = hwnd; entries.clear(); errors.clear();
			if (!api && FAILED(DirectInput8Create(instance, DIRECTINPUT_VERSION, IID_IDirectInput8W, reinterpret_cast<void**>(&api), nullptr)))
			{ errors.push_back("DirectInput8Create failed"); return false; }
			const HRESULT result = api->EnumDevices(DI8DEVCLASS_GAMECTRL, enum_device, this, DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK);
			if (FAILED(result)) errors.push_back(std::format("EnumDevices failed: 0x{:08X}", unsigned(result)));
			return SUCCEEDED(result);
		}

		void stop()
		{
			if (effect) { lastStop = effect->Stop(); effect->Release(); effect = nullptr; }
			if (device) device->SendForceFeedbackCommand(DISFFC_STOPALL);
			currentOutputMagnitude = 0;
		}

		void record_request(std::string_view context)
		{
			currentOutputMagnitude = lastRequestedMagnitude;
			if (lastRequestedMagnitude == 0) return;
			outputEverRequested = true;
			lastNonzeroRequestedMagnitude = lastUserRequestedMagnitude;
			lastNonzeroEffectiveMagnitude = lastRequestedMagnitude;
			lastNonzeroRequestUtc = utc_now();
			lastNonzeroRequestContext = std::string(context);
		}

		bool select(int index)
		{
			stop();
			if (device) { device->Unacquire(); device->Release(); device = nullptr; }
			selected = -1;
			if (index < 0 || index >= int(entries.size())) return false;
			HRESULT result = api->CreateDevice(entries[index].guid, &device, nullptr);
			if (FAILED(result)) { errors.push_back(std::format("CreateDevice failed: 0x{:08X}", unsigned(result))); return false; }
			result=device->SetDataFormat(&c_dfDIJoystick2);
			if(FAILED(result)){errors.push_back(std::format("SetDataFormat failed: 0x{:08X}",unsigned(result)));device->Release();device=nullptr;return false;}
			result = device->SetCooperativeLevel(window, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
			if (FAILED(result)) { errors.push_back(std::format("SetCooperativeLevel failed: 0x{:08X}", unsigned(result))); device->Release(); device = nullptr; return false; }
			if(entries[index].actuatorAxes.empty()){errors.push_back("Selected endpoint reports no force-feedback actuator axis");device->Release();device=nullptr;return false;}
			DIPROPDWORD gain{}; gain.diph.dwSize = sizeof(gain); gain.diph.dwHeaderSize = sizeof(gain.diph); gain.diph.dwHow = DIPH_DEVICE; gain.dwData = DI_FFNOMINALMAX;
			result=device->SetProperty(DIPROP_FFGAIN, &gain.diph); if(FAILED(result)) errors.push_back(std::format("Set device gain failed: 0x{:08X}",unsigned(result)));
			lastAcquire = device->Acquire();
			if (FAILED(lastAcquire)) { errors.push_back(std::format("Acquire failed: 0x{:08X}", unsigned(lastAcquire))); device->Release();device=nullptr;return false; }
			lastActuators=device->SendForceFeedbackCommand(DISFFC_SETACTUATORSON);
			if(FAILED(lastActuators)){errors.push_back(std::format("Enable actuators failed: 0x{:08X}",unsigned(lastActuators)));device->Unacquire();device->Release();device=nullptr;return false;}
			selected = index;
			DIPROPDWORD reportedGain{};reportedGain.diph.dwSize=sizeof(reportedGain);reportedGain.diph.dwHeaderSize=sizeof(reportedGain.diph);reportedGain.diph.dwHow=DIPH_DEVICE;
			lastGainQuery=device->GetProperty(DIPROP_FFGAIN,&reportedGain.diph);if(SUCCEEDED(lastGainQuery))deviceGain=reportedGain.dwData;
			return true;
		}

		bool start_effect(REFGUID type, DIEFFECT& desc, std::string_view descriptor, bool reportCreateFailure = true)
		{
			lastDescriptor=descriptor; lastCreate=device->CreateEffect(type,&desc,&effect,nullptr);
			if(FAILED(lastCreate)){if(reportCreateFailure)errors.push_back(std::format("CreateEffect ({}) failed: 0x{:08X}",descriptor,unsigned(lastCreate)));return false;}
			lastDownload=effect->Download();
			if(FAILED(lastDownload)){errors.push_back(std::format("Effect Download failed: 0x{:08X}",unsigned(lastDownload)));stop();return false;}
			lastStart=effect->Start(1,0);
			if(FAILED(lastStart)){errors.push_back(std::format("Effect Start failed: 0x{:08X}",unsigned(lastStart)));stop();return false;}
			return true;
		}

		bool run_direction(bool right, bool inverted, int requestedPercent, SafetyController& safety)
		{
			if (!device) return false;
			stop();
			lastUserRequestedMagnitude=requested_nominal_magnitude(requestedPercent);
			lastRequestedMagnitude = safety.bounded_magnitude(requestedPercent);
			record_request("directional test");
			std::array<DWORD,2> axis{{DIJOFS_X,DIJOFS_Y}};
			const bool effectiveRight=effective_right(right,inverted);
			std::array<LONG,2> direction{{effectiveRight?9000L:27000L,0}};
			DICONSTANTFORCE constant{ lastRequestedMagnitude };
			DIEFFECT desc{}; desc.dwSize = sizeof(desc); desc.dwFlags = DIEFF_POLAR | DIEFF_OBJECTOFFSETS;
			desc.dwDuration = DWORD(MaximumRunTime.count() * 1000); desc.dwGain = DI_FFNOMINALMAX; desc.dwTriggerButton = DIEB_NOTRIGGER;
			desc.cAxes = 2; desc.rgdwAxes = axis.data(); desc.rglDirection = direction.data();
			desc.cbTypeSpecificParams = sizeof(constant); desc.lpvTypeSpecificParams = &constant;
			if(start_effect(GUID_ConstantForce,desc,effectiveRight?"constant right (two-axis polar)":"constant left (two-axis polar)",false))return true;
			if(effect){effect->Release();effect=nullptr;} desc.cAxes=1;desc.dwFlags=DIEFF_CARTESIAN|DIEFF_OBJECTOFFSETS;direction[0]=1;constant.lMagnitude=effectiveRight?lastRequestedMagnitude:-lastRequestedMagnitude;
			return start_effect(GUID_ConstantForce,desc,effectiveRight?"constant right (one-axis fallback)":"constant left (one-axis fallback)");
		}

		bool update_direction(bool right, bool inverted)
		{
			if(!effect)return false;const bool effectiveRight=effective_right(right,inverted);
			std::array<LONG,2> direction{{effectiveRight?9000L:27000L,0}};DICONSTANTFORCE constant{lastRequestedMagnitude};
			DIEFFECT desc{};desc.dwSize=sizeof(desc);desc.dwFlags=DIEFF_POLAR|DIEFF_OBJECTOFFSETS;desc.cAxes=2;desc.rglDirection=direction.data();desc.cbTypeSpecificParams=sizeof(constant);desc.lpvTypeSpecificParams=&constant;
			if(lastDescriptor.find("one-axis")!=std::string::npos){desc.cAxes=1;desc.dwFlags=DIEFF_CARTESIAN|DIEFF_OBJECTOFFSETS;direction[0]=1;constant.lMagnitude=effectiveRight?lastRequestedMagnitude:-lastRequestedMagnitude;}
			lastUpdate=effect->SetParameters(&desc,DIEP_DIRECTION|DIEP_TYPESPECIFICPARAMS|DIEP_START);
			if(FAILED(lastUpdate))errors.push_back(std::format("Effect update failed: 0x{:08X}",unsigned(lastUpdate)));
			return SUCCEEDED(lastUpdate);
		}
		bool update_direction_strength(bool right,bool inverted,int requestedPercent,SafetyController& safety)
		{
			lastUserRequestedMagnitude=requested_nominal_magnitude(requestedPercent);
			lastRequestedMagnitude=safety.bounded_magnitude(requestedPercent);
			record_request("dynamic delivery test");
			return update_direction(right,inverted);
		}

		bool run_catalog_effect(int catalog, int requestedPercent, SafetyController& safety)
		{
			if(!device)return false; stop(); lastUserRequestedMagnitude=requested_nominal_magnitude(requestedPercent); lastRequestedMagnitude=safety.bounded_magnitude(requestedPercent); record_request("effect catalog test");
			DWORD axis=entries[selected].actuatorAxes[0]; LONG direction=0; DIEFFECT desc{}; desc.dwSize=sizeof(desc);desc.dwFlags=DIEFF_CARTESIAN|DIEFF_OBJECTOFFSETS;
			desc.dwDuration=DWORD(MaximumRunTime.count()*1000);desc.dwGain=DI_FFNOMINALMAX;desc.dwTriggerButton=DIEB_NOTRIGGER;desc.cAxes=1;desc.rgdwAxes=&axis;desc.rglDirection=&direction;
			if(catalog==0||catalog==2){DICONDITION condition{};condition.lPositiveCoefficient=condition.lNegativeCoefficient=LONG(lastRequestedMagnitude);condition.dwPositiveSaturation=condition.dwNegativeSaturation=DWORD(lastRequestedMagnitude);desc.cbTypeSpecificParams=sizeof(condition);desc.lpvTypeSpecificParams=&condition;return start_effect(catalog==0?GUID_Spring:GUID_Damper,desc,catalog==0?"native spring":"native damper");}
			if(catalog>=4&&catalog<=6){DIPERIODIC periodic{};periodic.dwMagnitude=DWORD(lastRequestedMagnitude);periodic.dwPeriod=1000000/30;desc.cbTypeSpecificParams=sizeof(periodic);desc.lpvTypeSpecificParams=&periodic;const GUID* type=catalog==4?&GUID_Sine:(catalog==5?&GUID_Triangle:&GUID_Square);return start_effect(*type,desc,catalog==4?"native sine 30 Hz":(catalog==5?"native triangle 30 Hz":"native square 30 Hz"));}
			DICONSTANTFORCE constant{lastRequestedMagnitude};desc.cbTypeSpecificParams=sizeof(constant);desc.lpvTypeSpecificParams=&constant;
			return start_effect(GUID_ConstantForce,desc,"bounded synthetic approximation (constant force)");
		}

		FfbResolutionResult redetect(HINSTANCE instance, HWND hwnd, std::string_view previous)
		{
			lastRecovery={}; recoveryLog.clear(); stop(); lastRecovery.effectStopped=true; recoveryLog.push_back("1. Active effects stopped");
			if(device){device->Unacquire();device->Release();device=nullptr;} selected=-1; lastRecovery.deviceReleased=true; recoveryLog.push_back("2. Previous device released");
			initialize(instance,hwnd); lastRecovery.enumerationRefreshed=true; recoveryLog.push_back(std::format("3. Enumeration refreshed: {} endpoint(s)",entries.size()));
			std::vector<std::string> ids;for(const auto& entry:entries)ids.push_back(entry.id);auto resolution=resolve_ffb_device(ids,previous);
			if(resolution.index>=0){lastRecovery.identityResolved=true;recoveryLog.push_back("4. Persistent endpoint resolved");if(select(resolution.index)){lastRecovery.capabilitiesValidated=true;lastRecovery.acquired=true;recoveryLog.push_back("5. Capabilities validated; device acquired; effect resources ready");}else resolution={FfbResolution::NotFound,-1};}
			else recoveryLog.push_back(resolution.state==FfbResolution::SelectionRequired?"4. Multiple endpoints require confirmation":"4. No endpoint found");
			lastRecovery.zeroForce=effect==nullptr; recoveryLog.push_back("6. Zero force retained"); return resolution;
		}

		bool connected() const { return selected >= 0 && device != nullptr; }
		void shutdown() { stop(); if (device) { device->Unacquire(); device->Release(); } device = nullptr; if (api) api->Release(); api = nullptr; }
	};

	enum class Page { Devices, InputTest, QuickSetup, FfbTest };

	struct App
	{
		SDL_Window* window = nullptr;
		SDL_Renderer* renderer = nullptr;
		SDLDeviceSession input;
		NativeFFB ffb;
		SafetyController safety;
		SafetyController compatibilitySafety;
		SafetyController quickFfbSafety;
		ShakeController shake;
		ShakeController quickFfbShake;
		Page page = Page::Devices;
		bool initialized = false, showDetails = false, running = true;
		bool quickConfirmation = false;
		bool initializing = false;
		int initializationPhase = 0;
		std::string initializationError;
		std::string status = "Ready to check your setup?";
		std::array<BackendResult, 4> backends{{ {"Windows.Gaming.Input"}, {"SDL3 RawInput"}, {"SDL3 DirectInput"}, {"SDL3 XInput"} }};
		std::array<BackendResult, 4> pendingBackends{{ {"Windows.Gaming.Input"}, {"SDL3 RawInput"}, {"SDL3 DirectInput"}, {"SDL3 XInput"} }};
		int backendIndex = -1;
		bool backendForInitialization = false, compatibilityComplete = false;
		Clock::time_point backendStarted{};
		QuickSetupController quick;
		QuickFfbCheck quickFfb;
		std::vector<std::vector<Sint16>> captureBaselines;
		std::vector<bool> deviceResponsive;
		int selectedInput = 0, strength = 20;
		bool invertFfb = false, exportSucceeded = false;
		DeliveryStage deliveryStage=DeliveryStage::Idle;
		CompatibilityResult simulatedLegacy,simulatedDynamic,physicalLegacy,physicalDynamic;
		std::vector<CompatibilityResult> legacyAttemptHistory,dynamicAttemptHistory;
		size_t compatibilitySignalIndex=0;Clock::time_point compatibilityNext{},compatibilityLast{},deliveryDeadline{},deliveryStarted{},safetyIntervalStarted{},compatibilityTestStarted{};
		CompatibilityStrategy pendingCompatibilityStrategy=CompatibilityStrategy::LegacyRecreation;
		std::chrono::milliseconds compatibilityTestElapsed{0};
		std::string deliveryPreference="Not provided",deliveryShutdownReason="none",persistedFfbIdentity;
		std::string quickStatus = "untested", inputStatus = "untested", ffbStatus = "untested", reportStatus;
		std::string leftTestStatus = "untested", rightTestStatus = "untested", shakeTestStatus = "untested", lastSafetyShutdown = "none";
		std::filesystem::path appDirectory = documents_path() / "HYP36rforce Device Diagnostics";
		std::filesystem::path lastExportDirectory = exports_path(documents_path());
		std::filesystem::path lastExportFile;

		std::filesystem::path profile_path() const { return appDirectory / "diagnostic-profile.txt"; }
		void save_profile()
		{
			std::filesystem::create_directories(appDirectory);
			std::ofstream out(profile_path());
			for (const auto& binding : quick.saved)
				out << (binding ? binding->deviceId + "|" + binding->deviceName + "|" + binding->control : "") << '\n';
			out << "ffb=" << persistedFfbIdentity << '\n';
			out << "invert=" << (invertFfb ? 1 : 0) << '\n';
		}
		void load_profile()
		{
			std::ifstream in(profile_path());std::vector<std::string> lines;for(std::string line;std::getline(in,line);)lines.push_back(line);
			for (size_t index=0;index<quick.saved.size()&&index<lines.size();++index)
			{
				const std::string& line=lines[index];if(line.empty())continue;
				const auto first = line.find('|'); const auto second = first == std::string::npos ? first : line.find('|', first + 1);
				if (first != std::string::npos && second != std::string::npos)
					quick.saved[index] = CapturedInput{ line.substr(0, first), line.substr(first + 1, second - first - 1), line.substr(second + 1) };
			}
			for(size_t index=quick.saved.size();index<lines.size();++index){const auto& setting=lines[index];if(setting.rfind("ffb=",0)==0)persistedFfbIdentity=setting.substr(4);else if(setting.rfind("invert=",0)==0)invertFfb=setting.substr(7)=="1";else if(persistedFfbIdentity.empty()){const auto separator=setting.find('|');if(separator!=std::string::npos)persistedFfbIdentity=setting.substr(0,separator);}}
		}

		std::string device_label(size_t index) const
		{
			if (index >= input.devices.size()) return "Unknown interface";
			const auto& d = input.devices[index];
			return std::format("{} [VID {:04X}, PID {:04X}, SDL {}]", d.name, d.vendor, d.product, d.id);
		}
		std::string ffb_label(size_t index) const
		{
			if(index>=ffb.entries.size()) return "Unknown FFB interface";
			const auto& d=ffb.entries[index]; return std::format("{} [DirectInput interface {}, {} axes, {} buttons]",d.name,index+1,d.axes,d.buttons);
		}
		std::string saved_ffb_identity() const { return persistedFfbIdentity; }
		std::string current_output_state() const
		{
			if (ffb.currentOutputMagnitude != 0 && ffb.effect) return "Active";
			if (ffb.selected < 0) return "Unavailable";
			return "Stopped";
		}
		std::string session_output_history() const
		{
			return ffb.outputEverRequested ? "Output requested during session" : "No output requested during session";
		}
		void run_software_compatibility()
		{
			const int axes=ffb.selected>=0?int(ffb.entries[ffb.selected].actuatorAxes.size()):1;
			simulatedLegacy=simulate_compatibility(CompatibilityStrategy::LegacyRecreation,axes);simulatedDynamic=simulate_compatibility(CompatibilityStrategy::PersistentDynamic,axes);const auto timestamp=utc_now();simulatedLegacy.startedUtc=simulatedLegacy.completedUtc=timestamp;simulatedDynamic.startedUtc=simulatedDynamic.completedUtc=timestamp;
			physicalLegacy={CompatibilityStrategy::LegacyRecreation};physicalDynamic={CompatibilityStrategy::PersistentDynamic};legacyAttemptHistory.clear();dynamicAttemptHistory.clear();deliveryStage=DeliveryStage::Ready;deliveryPreference="Not provided";deliveryShutdownReason="none";pendingCompatibilityStrategy=CompatibilityStrategy::LegacyRecreation;compatibilityTestElapsed=std::chrono::milliseconds(0);
		}
		bool begin_physical_compatibility(CompatibilityStrategy strategy,Clock::time_point now,bool focused)
		{
			compatibilitySafety.authorized=true;if(!compatibilitySafety.begin(ffb.connected(),focused,now))return false;
			auto& result=strategy==CompatibilityStrategy::LegacyRecreation?physicalLegacy:physicalDynamic;result={};result.strategy=strategy;result.state=ResultState::Running;result.simulated=false;result.actuatorAxes=ffb.selected>=0?int(ffb.entries[ffb.selected].actuatorAxes.size()):0;result.startedUtc=utc_now();
			compatibilitySignalIndex=0;compatibilityLast=now;compatibilityNext=now;compatibilityTestStarted=now;compatibilityTestElapsed=std::chrono::milliseconds(0);return true;
		}
		void cancel_delivery(std::string_view reason)
		{
			const bool hadEffect=ffb.effect!=nullptr;
			CompatibilityResult* active=physicalLegacy.state==ResultState::Running?&physicalLegacy:(physicalDynamic.state==ResultState::Running?&physicalDynamic:nullptr);
			ffb.stop();if(hadEffect&&active){++active->stopCount;++active->releaseCount;++active->finalCleanupCount;}
			compatibilitySafety.stop();if(physicalLegacy.state==ResultState::Running){physicalLegacy.state=ResultState::Cancelled;physicalLegacy.completedUtc=utc_now();physicalLegacy.note=std::string(reason);}if(physicalDynamic.state==ResultState::Running){physicalDynamic.state=ResultState::Cancelled;physicalDynamic.completedUtc=utc_now();physicalDynamic.note=std::string(reason);}deliveryShutdownReason=std::string(reason);lastSafetyShutdown=std::string(reason);deliveryStage=DeliveryStage::Cancelled;
		}
		bool update_physical_compatibility(CompatibilityStrategy strategy,Clock::time_point now,bool focused)
		{
			auto& result=strategy==CompatibilityStrategy::LegacyRecreation?physicalLegacy:physicalDynamic;
			if(!focused||!ffb.connected()){cancel_delivery(!focused?"Delivery test lost focus":"Delivery test device disconnected");return false;}
			compatibilitySafety.beat(now);if(compatibilitySafety.must_stop(true,focused,ffb.connected(),now)){cancel_delivery("Delivery test safety timeout");return false;}
			while(compatibilitySignalIndex<CompatibilitySignalPercent.size()&&now>=compatibilityNext)
			{
				if(compatibilitySignalIndex)result.intervalsMs.push_back(std::chrono::duration<double,std::milli>(now-compatibilityLast).count());compatibilityLast=now;
				const auto request=delivery_request(compatibilitySignalIndex);const int magnitudePercent=request.magnitudePercent;const bool right=request.right;bool ok=true;HRESULT apiResult=S_OK;
				if(magnitudePercent==0&&strategy==CompatibilityStrategy::LegacyRecreation)
				{
					const bool hadEffect=ffb.effect!=nullptr;ffb.stop();if(hadEffect){++result.stopCount;++result.releaseCount;}apiResult=ffb.lastStop;
				}
				else if(magnitudePercent==0&&!ffb.effect){apiResult=S_OK;}
				else if(magnitudePercent==0){ok=ffb.update_direction_strength(right,invertFfb,0,compatibilitySafety);++result.updateCount;apiResult=ffb.lastUpdate;}
				else if(strategy==CompatibilityStrategy::LegacyRecreation)
				{
					const bool replaced=ffb.effect!=nullptr;ok=ffb.run_direction(right,invertFfb,magnitudePercent,compatibilitySafety);
					if(replaced){++result.stopCount;++result.releaseCount;++result.replacementCount;}++result.createCount;++result.startCount;apiResult=ffb.lastCreate;
				}
				else if(!ffb.effect){ok=ffb.run_direction(right,invertFfb,magnitudePercent,compatibilitySafety);++result.createCount;++result.startCount;apiResult=ffb.lastCreate;}
				else {ok=ffb.update_direction_strength(right,invertFfb,magnitudePercent,compatibilitySafety);++result.updateCount;apiResult=ffb.lastUpdate;}
				result.apiResults.push_back(long(apiResult));if(!ok){++result.failureCount;result.state=ResultState::Failed;result.completedUtc=utc_now();result.note="DirectInput rejected the shared directional output path";const bool hadEffect=ffb.effect!=nullptr;ffb.stop();if(hadEffect){++result.stopCount;++result.releaseCount;++result.finalCleanupCount;}compatibilitySafety.stop();return true;}
				++compatibilitySignalIndex;compatibilityNext+=CompatibilityUpdatePeriod;
			}
			if(compatibilitySignalIndex>=CompatibilitySignalPercent.size())
			{
				std::vector<int> values;for(int value:CompatibilitySignalPercent)values.push_back(value*100);finalize_compatibility_statistics(result,values);
				const bool hadEffect=ffb.effect!=nullptr;ffb.stop();if(hadEffect){++result.stopCount;++result.releaseCount;++result.finalCleanupCount;}compatibilitySafety.stop();result.state=ResultState::Completed;result.completedUtc=utc_now();return true;
			}
			return false;
		}
		void update_delivery(Clock::time_point now,bool focused)
		{
			if(deliveryStage==DeliveryStage::Countdown&&now>=deliveryDeadline)
			{
				if(begin_physical_compatibility(pendingCompatibilityStrategy,now,focused))deliveryStage=pendingCompatibilityStrategy==CompatibilityStrategy::LegacyRecreation?DeliveryStage::Legacy:DeliveryStage::Dynamic;
				else cancel_delivery("Delivery test could not enter a safe output state");
			}
			else if(deliveryStage==DeliveryStage::Legacy&&update_physical_compatibility(CompatibilityStrategy::LegacyRecreation,now,focused)){ffb.stop();compatibilitySafety.stop();compatibilityTestElapsed=std::chrono::duration_cast<std::chrono::milliseconds>(now-compatibilityTestStarted);deliveryStage=DeliveryStage::LegacyFeedback;}
			else if(deliveryStage==DeliveryStage::SafetyInterval){if(!focused||!ffb.connected()){cancel_delivery(!focused?"Delivery test lost focus":"Delivery test device disconnected");}else if(now-safetyIntervalStarted>=std::chrono::seconds(1)){pendingCompatibilityStrategy=CompatibilityStrategy::PersistentDynamic;if(begin_physical_compatibility(pendingCompatibilityStrategy,now,focused))deliveryStage=DeliveryStage::Dynamic;else cancel_delivery("Method B could not enter a safe output state");}}
			else if(deliveryStage==DeliveryStage::Dynamic&&update_physical_compatibility(CompatibilityStrategy::PersistentDynamic,now,focused)){ffb.stop();compatibilitySafety.stop();compatibilityTestElapsed=std::chrono::duration_cast<std::chrono::milliseconds>(now-compatibilityTestStarted);deliveryShutdownReason="Normal bounded shutdown";deliveryStage=DeliveryStage::DynamicFeedback;}
		}
		void retry_current_compatibility(CompatibilityStrategy strategy,Clock::time_point now)
		{
			ffb.stop();compatibilitySafety.stop();auto& result=strategy==CompatibilityStrategy::LegacyRecreation?physicalLegacy:physicalDynamic;
			if(result.state!=ResultState::Untested)(strategy==CompatibilityStrategy::LegacyRecreation?legacyAttemptHistory:dynamicAttemptHistory).push_back(result);
			result={};result.strategy=strategy;pendingCompatibilityStrategy=strategy;compatibilityTestElapsed=std::chrono::milliseconds(0);deliveryDeadline=now+std::chrono::seconds(3);deliveryStage=DeliveryStage::Countdown;
		}
		void remember_selected_ffb()
		{
			if(ffb.selected<0||ffb.selected>=int(ffb.entries.size()))return;
			persistedFfbIdentity=ffb.entries[ffb.selected].id;save_profile();
		}

		void begin_capture()
		{
			captureBaselines.clear(); captureBaselines.resize(input.handles.size());
			for (size_t d = 0; d < input.handles.size(); ++d)
				for (int axis = 0; axis < SDL_GetNumJoystickAxes(input.handles[d]); ++axis)
					captureBaselines[d].push_back(SDL_GetJoystickAxis(input.handles[d], axis));
		}

		bool start()
		{
			if (!SDL_Init(SDL_INIT_VIDEO)) return false;
			window = SDL_CreateWindow("HYP36rforce Device Diagnostics", 1120, 720, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
			renderer = window ? SDL_CreateRenderer(window, nullptr) : nullptr;
			if (!window || !renderer) return false;
			SDL_SetRenderVSync(renderer, 1);
			IMGUI_CHECKVERSION(); ImGui::CreateContext();
			auto& io = ImGui::GetIO(); io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
			ImGui::StyleColorsDark();
			auto& style = ImGui::GetStyle(); style.WindowRounding = 8; style.ChildRounding = 7; style.FrameRounding = 5;
			ImGui_ImplSDL3_InitForSDLRenderer(window, renderer); ImGui_ImplSDLRenderer3_Init(renderer);
			load_profile(); return true;
		}

		void begin_initialization()
		{
			ffb.stop();safety.stop();compatibilitySafety.stop();quickFfbSafety.stop();shake.stop();quickFfbShake.stop();safety.authorized=false;deliveryStage=DeliveryStage::Idle;
			initialized=false; initializing=true; initializationPhase=0; initializationError.clear();quickConfirmation=false;compatibilityComplete=false;page=Page::Devices;
			status="Starting automatic compatibility checks...";
		}

		void update_initialization()
		{
			if(!initializing) return;
			if(initializationPhase==0)
			{
				start_backend_sequence(true);
				initializationPhase=1; return;
			}
			if(initializationPhase==1){if(backendIndex>=0)return;initializationPhase=2;return;}
			if(initializationPhase==2)
			{
				status="Discovering native DirectInput FFB interfaces...";
				HWND hwnd=static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(window),SDL_PROP_WINDOW_WIN32_HWND_POINTER,nullptr));
				if(!ffb.initialize(GetModuleHandleW(nullptr),hwnd)&&!ffb.errors.empty()) initializationError=ffb.errors.back();
				initializationPhase=3; return;
			}
			initializing=false; initialized=true;
			status=std::format("Devices Discovered: {} input, {} FFB-capable",input.devices.size(),ffb.entries.size());
			deviceResponsive.assign(input.devices.size(),false);
			std::vector<std::string> ids;for(const auto& entry:ffb.entries)ids.push_back(entry.id);
			const auto resolution=resolve_ffb_device(ids,saved_ffb_identity());
			if(resolution.index>=0&&ffb.select(resolution.index)){remember_selected_ffb();ffbStatus="ready";}
			else ffbStatus=resolution.state==FfbResolution::SelectionRequired?"selection required":(ffb.entries.empty()?"not found":"initialization failed");
			compatibilityComplete=true;page=Page::QuickSetup; quickConfirmation=true;
		}

		void capture_quick_setup_input()
		{
			if (!quick.active || quick.step == 7 || quick.candidate || quick.timedOut) return;
			std::vector<CapturedInput> observed;
			for (size_t d = 0; d < input.handles.size(); ++d)
			{
				auto* joystick = input.handles[d];
				for (int button = 0; button < SDL_GetNumJoystickButtons(joystick); ++button)
					if (SDL_GetJoystickButton(joystick, button)) observed.push_back({ input.devices[d].id, device_label(d), std::format("Button {}", button) });
				for (int hat = 0; hat < SDL_GetNumJoystickHats(joystick); ++hat)
				{
					const auto value = SDL_GetJoystickHat(joystick, hat);
					if (value != SDL_HAT_CENTERED) observed.push_back({ input.devices[d].id, device_label(d), std::format("POV {} value 0x{:02X}", hat, value) });
				}
				for (int axis = 0; axis < SDL_GetNumJoystickAxes(joystick) && axis < int(captureBaselines[d].size()); ++axis)
				{
					const int delta = int(SDL_GetJoystickAxis(joystick, axis)) - int(captureBaselines[d][axis]);
					if (std::abs(delta) > 16384) observed.push_back({ input.devices[d].id, device_label(d), std::format("Axis {} ({})", axis, delta > 0 ? "positive" : "negative") });
				}
			}
			if (!observed.empty())
			{
				observed.front().ambiguous = observed.size() > 1;
				quick.observe(std::move(observed.front()));
			}
		}

		void start_backend_sequence(bool startup)
		{
			backendForInitialization=startup;
			backendIndex = 0; backendStarted = Clock::now();
			for (size_t i=0;i<pendingBackends.size();++i) pendingBackends[i]=BackendResult{backends[i].name};
			start_backend();
		}
		void run_all_backends(){start_backend_sequence(false);}
		void start_backend()
		{
			auto& result = pendingBackends[backendIndex]; result.state = ResultState::Running; result.startedUtc = utc_now();
			std::string error;
			if (!input.open(result.name.c_str(), error,&result.openingResults)) { result.state = ResultState::Failed; result.error = error; }
			else { result.devices = input.devices; result.effectiveBackend=result.name+" isolated hint profile"; }
			backendStarted = Clock::now();
			if(result.state==ResultState::Failed)backendStarted-=BackendObservationTime;
		}
		void update_backend()
		{
			if (backendIndex < 0) return;
			auto& result = pendingBackends[backendIndex]; input.pump(&result.delayedEvents);
			if (Clock::now() - backendStarted < BackendObservationTime) return;
			if (result.state == ResultState::Running) { result.devices=input.devices; result.state = ResultState::Completed; }
			if (++backendIndex < int(backends.size())) start_backend();
			else
			{
				backends=pendingBackends;backendIndex=-1;std::string error;input.open("SDL3 DirectInput",error);deviceResponsive.assign(input.devices.size(),false);
				status=backendForInitialization?"Input backend compatibility checks complete":"Backend comparison complete";
			}
		}
		void cancel_backend_sequence()
		{
			if(backendIndex<0)return;
			for(auto& result:pendingBackends)if(result.state==ResultState::Running||result.state==ResultState::Untested)result.state=ResultState::Cancelled;
			input.close();ffb.stop();safety.stop();compatibilitySafety.stop();quickFfbSafety.stop();shake.stop();quickFfbShake.stop();safety.authorized=false;deliveryStage=DeliveryStage::Idle;
			if(backendForInitialization){backends=pendingBackends;initializing=false;initialized=true;status="Compatibility check cancelled; completed results were preserved";}
			else {std::string error;input.open("SDL3 DirectInput",error);status="Backend re-test cancelled; previous completed results were preserved";}
			backendIndex=-1;
		}

		DeviceDiagnosticsReport::Report report() const
		{
			using namespace DeviceDiagnosticsReport;
			Report value{ utc_now(), {} };
			const auto reportOutcome=[](const CompatibilityResult& result)
			{
				if(result.state==ResultState::Completed)return result.failureCount?"Method Unsuccessful":"Commands Accepted";
				if(result.state==ResultState::Failed)return "Method Unsuccessful";
				if(result.state==ResultState::Cancelled)return "Interrupted";
				return "Untested";
			};
			const auto physicalName=[](PhysicalConfirmation state)
			{
				return state==PhysicalConfirmation::Yes?"Yes":(state==PhysicalConfirmation::No?"No":(state==PhysicalConfirmation::Unsure?"Unsure":"Not Tested"));
			};
			std::string wheel="Not Detected";
			if(ffb.selected>=0&&ffb.selected<int(ffb.entries.size()))wheel=ffb.entries[ffb.selected].name;
			else if(quick.saved[0])wheel=quick.saved[0]->deviceName;
			const bool anyResponsive=std::any_of(deviceResponsive.begin(),deviceResponsive.end(),[](bool responsive){return responsive;});
			const size_t assignedCount=std::count_if(quick.saved.begin(),quick.saved.end(),[](const auto& binding){return binding.has_value();});
			std::string conclusion="No completed physical FFB comparison is available.";
			if(physicalLegacy.state==ResultState::Completed&&physicalDynamic.state==ResultState::Completed)
				conclusion=physicalLegacy.physical==PhysicalConfirmation::Yes&&physicalDynamic.physical==PhysicalConfirmation::Yes
					?"Both delivery methods were accepted by DirectInput and felt by the user."
					:"Both delivery methods completed; API acceptance and user observations are reported separately.";
			value.summary={
				{"Wheel",wheel},
				{"Input",anyResponsive?"Responsive":(input.devices.empty()?"Unavailable":"Not Observed")},
				{"Quick Setup",assignedCount==quick.saved.size()?"Completed":(assignedCount?"Partial":"Not Tested")},
				{"FFB Device",ffb.connected()?"Ready":(ffb.entries.empty()?"Not Detected":"Not Ready")},
				{"Quick Setup FFB Response",quickFfb.response==QuickFfbResponse::Confirmed?"Confirmed by User":(quickFfb.response==QuickFfbResponse::NotConfirmed?"Not Confirmed":"Not Tested")},
				{"Legacy Delivery",reportOutcome(physicalLegacy)},
				{"Legacy Physical Response",physicalName(physicalLegacy.physical)},
				{"Dynamic Delivery",reportOutcome(physicalDynamic)},
				{"Dynamic Physical Response",physicalName(physicalDynamic.physical)},
				{"Current Output",current_output_state()},
				{"Session Output History",session_output_history()},
				{"Compatibility Conclusion",conclusion},
				{"Limitations","Physical torque was not measured."}
			};
			Section discovery{ "Device discovery", initialized ? Status::Completed : Status::Untested,
				initialized ? std::format("{} input and {} FFB-capable device(s) discovered.", input.devices.size(), ffb.entries.size()) : "Initialization was not run.", {} };
			for (size_t i=0;i<input.devices.size();++i) { const auto& d=input.devices[i]; discovery.details.push_back(std::format("{} via {}: {} axes, {} buttons, {} hats; activity {}", device_label(i), d.backend, d.axes, d.buttons, d.hats, i<deviceResponsive.size()&&deviceResponsive[i]?"responsive":"not observed")); }
			for (size_t i=0;i<ffb.entries.size();++i) { const auto& d=ffb.entries[i]; discovery.details.push_back(std::format("{}: native DirectInput FFB, {} POVs", ffb_label(i), d.povs)); }
			value.sections.push_back(std::move(discovery));
			Section backend{ "Input backend comparison", Status::Untested, "Backend comparison was not completed.", {} };
			if (std::all_of(backends.begin(), backends.end(), [](const auto& r) { return r.state == ResultState::Completed; })) { backend.status = Status::Completed; backend.summary = "All isolated backend sessions completed."; }
			else if (std::any_of(backends.begin(), backends.end(), [](const auto& r) { return r.state == ResultState::Failed; })) { backend.status = Status::Failed; backend.summary = "One or more backend sessions failed."; }
			else if (std::any_of(backends.begin(), backends.end(), [](const auto& r) { return r.state == ResultState::Cancelled; })) { backend.summary = "Backend comparison was cancelled; completed results were preserved."; }
			for (const auto& b : backends)
			{
				backend.details.push_back(std::format("{}: requested {}, effective {}, started {}, {} device(s), {} delayed event(s){}",state_name(b.state),b.name,b.effectiveBackend.empty()?"untested":b.effectiveBackend,b.startedUtc.empty()?"not started":b.startedUtc,b.devices.size(),b.delayedEvents.size(),b.error.empty()?"":", "+b.error));
				for(const auto& d:b.devices)backend.details.push_back(std::format("{} device: {} [VID {:04X}, PID {:04X}], {} axes, {} buttons, {} hats, input {}",b.name,d.name,d.vendor,d.product,d.axes,d.buttons,d.hats,d.inputAvailable?"available":"unavailable"));
				for(const auto& opening:b.openingResults)backend.details.push_back(b.name+" opening: "+opening);
			}
			value.sections.push_back(std::move(backend));
			Section delayed{ "Delayed discovery", Status::Untested, "No completed delayed-discovery observation.", {} };
			for (const auto& b : backends) for (const auto& event : b.delayedEvents) delayed.details.push_back(b.name + ": " + event);
			if (std::all_of(backends.begin(), backends.end(), [](const auto& r) { return r.state == ResultState::Completed; })) { delayed.status = Status::Completed; delayed.summary = std::format("{} delayed connection event(s) observed.", delayed.details.size()); }
			value.sections.push_back(std::move(delayed));
			value.sections.push_back({ "Input testing", inputStatus == "completed" ? Status::Completed : Status::Untested, inputStatus == "completed" ? "Live input activity was observed." : "No live input activity was recorded.", {} });
			static const std::array<const char*,7> names{{"Steering","Accelerator","Brake","Shift Up","Shift Down","Start / Menu","Back Button"}};
			Section assignments{ "Multi-Input assignments", Status::Untested, "No diagnostic assignments were saved.", {} };
			for(size_t i=0;i<quick.saved.size();++i) assignments.details.push_back(std::string(names[i])+": "+(quick.saved[i]?quick.saved[i]->deviceName+" — "+quick.saved[i]->control:"Unassigned"));
			if(std::any_of(quick.saved.begin(),quick.saved.end(),[](const auto& b){return b.has_value();})){assignments.status=Status::Completed;assignments.summary="Diagnostic-only assignments span independently selected physical interfaces.";}
			value.sections.push_back(std::move(assignments));
			value.sections.push_back({ "Quick Setup", quickStatus == "completed" ? Status::Completed : Status::Untested, quickStatus == "completed" ? "Seven gameplay bindings completed; FFB is resolved separately." : "Quick Setup was not completed.", { std::format("Resolved FFB endpoint: {}",ffb.selected>=0?ffb_label(ffb.selected):"None"),std::format("Quick Setup FFB response: {}",quickFfb.response==QuickFfbResponse::Confirmed?"Confirmed by User":(quickFfb.response==QuickFfbResponse::NotConfirmed?"Not Confirmed":"Not Tested")) } });
			Section caps{ "FFB device capabilities", ffb.entries.empty() ? Status::Unavailable : Status::Completed, ffb.entries.empty() ? "No native DirectInput FFB endpoint found." : std::format("{} native FFB endpoint(s) found.", ffb.entries.size()), {} };
			for (size_t i=0;i<ffb.entries.size();++i) { const auto& d=ffb.entries[i]; caps.details.push_back(std::format("{}: {}", ffb_label(i), d.effects.empty() ? "no reported effects" : std::format("{} reported effects", d.effects.size()))); }
			value.sections.push_back(std::move(caps));
			Section ffbTest{ "FFB test results", ffb.outputEverRequested ? Status::Completed : (ffbStatus=="initialization failed"?Status::Failed:Status::Untested), ffb.outputEverRequested ? "A bounded output request occurred during this session; physical torque was not measured." : "No physical FFB output was requested in this session.", { std::format("Selected device: {}",ffb.selected>=0?ffb_label(ffb.selected):"None"),std::format("Readiness: {}",ffbStatus),std::format("Current output state: {}",current_output_state()),std::format("Session output history: {}",session_output_history()),std::format("Invert FFB: {}",invertFfb?"On":"Off"),std::format("Selected FFB Strength: {}%",strength),std::format("Last nonzero requested nominal magnitude: {} / {}",ffb.lastNonzeroRequestedMagnitude,DI_FFNOMINALMAX),std::format("Last nonzero effective safety-limited magnitude: {} / {}",ffb.lastNonzeroEffectiveMagnitude,DI_FFNOMINALMAX),std::format("Last nonzero request context: {}; timestamp: {}",ffb.lastNonzeroRequestContext,ffb.lastNonzeroRequestUtc.empty()?"not recorded":ffb.lastNonzeroRequestUtc),std::format("Current output magnitude: {} / {}",ffb.currentOutputMagnitude,DI_FFNOMINALMAX),std::format("Physical output ceiling: {}% ({} / {})",PhysicalOutputCeilingPercent,PhysicalOutputCeilingPercent*100,DI_FFNOMINALMAX),std::format("Left test: {}; Right test: {}; Shake test: {} at {} Hz",leftTestStatus,rightTestStatus,shakeTestStatus,ShakeController::FrequencyHz),std::format("Last descriptor: {}",ffb.lastDescriptor),std::format("Last safety shutdown: {}",lastSafetyShutdown),std::format("Acquire: 0x{:08X}; Actuators: 0x{:08X}; Create: 0x{:08X}; Download: 0x{:08X}; Start: 0x{:08X}; Update: 0x{:08X}; Stop: 0x{:08X}",unsigned(ffb.lastAcquire),unsigned(ffb.lastActuators),unsigned(ffb.lastCreate),unsigned(ffb.lastDownload),unsigned(ffb.lastStart),unsigned(ffb.lastUpdate),unsigned(ffb.lastStop)) } };
			for(const auto& line:ffb.recoveryLog)ffbTest.details.push_back("Re-detect: "+line);value.sections.push_back(std::move(ffbTest));
			const Status deliveryStatus=deliveryStage==DeliveryStage::Results?Status::Completed:(deliveryStage==DeliveryStage::Cancelled?Status::Cancelled:(physicalLegacy.state==ResultState::Failed||physicalDynamic.state==ResultState::Failed?Status::Failed:Status::Untested));
			Section compatibility{ "FFB delivery test", deliveryStatus, deliveryStage==DeliveryStage::Results?"Guided Legacy and Dynamic delivery comparison completed.":(deliveryStage==DeliveryStage::Cancelled?"Delivery test was cancelled; collected evidence was preserved.":"Delivery test was not completed."), {} };
			if(ffb.selected>=0){const auto& selected=ffb.entries[ffb.selected];compatibility.details.push_back(std::format("Selected endpoint: {}; actuator axes: {}; supported effects: {}",selected.name,selected.actuatorAxes.size(),selected.effects.empty()?"none reported":std::to_string(selected.effects.size())));compatibility.details.push_back(std::format("Device gain query: 0x{:08X}; reported gain: {}",unsigned(ffb.lastGainQuery),ffb.deviceGain));}
			const auto addCompatibility=[&compatibility](std::string_view label,const CompatibilityResult& result,std::string_view attempt)
			{
				compatibility.details.push_back(std::format("{} {} [{}]: started {}; completed {}; requested {} Hz; mean interval {:.2f} ms; jitter {:.2f} ms; create/start/update/stop/release/replacement/final-cleanup {}/{}/{}/{}/{}/{}/{}; failures {}; peak {}; average {:.2f}; RMS {:.2f}; zero time {:.0f} ms; physical {}; note {}",label,attempt,result.simulated?"simulated":"physical hardware",result.startedUtc.empty()?"not recorded":result.startedUtc,result.completedUtc.empty()?"not recorded":result.completedUtc,result.requestedRateHz,result.averageIntervalMs,result.jitterMs,result.createCount,result.startCount,result.updateCount,result.stopCount,result.releaseCount,result.replacementCount,result.finalCleanupCount,result.failureCount,result.peakMagnitude,result.averageMagnitude,result.rmsMagnitude,result.zeroTimeMs,result.physical==PhysicalConfirmation::Yes?"yes":(result.physical==PhysicalConfirmation::No?"no":(result.physical==PhysicalConfirmation::Unsure?"unsure":"untested")),result.note.empty()?"none":result.note));
				for(const long code:result.apiResults)compatibility.details.push_back(std::format("{} API result: 0x{:08X}",label,unsigned(code)));
			};
			compatibility.details.push_back("Requested magnitude sequence (% nominal): 0, 5, 10, 15, 20, 15, 10, 0, -10, -20, -10, 0");
			compatibility.details.push_back("Safety interval: minimum 1000 ms of requested zero force between methods.");compatibility.details.push_back("Shutdown: "+deliveryShutdownReason);
			if(simulatedLegacy.state!=ResultState::Untested)addCompatibility("Legacy recreation",simulatedLegacy,"software assessment");if(simulatedDynamic.state!=ResultState::Untested)addCompatibility("Persistent dynamic",simulatedDynamic,"software assessment");
			for(size_t index=0;index<legacyAttemptHistory.size();++index)addCompatibility("Legacy physical",legacyAttemptHistory[index],std::format("prior attempt {}",index+1));
			for(size_t index=0;index<dynamicAttemptHistory.size();++index)addCompatibility("Dynamic physical",dynamicAttemptHistory[index],std::format("prior attempt {}",index+1));
			if(physicalLegacy.state!=ResultState::Untested)addCompatibility("Legacy physical",physicalLegacy,"final accepted attempt");if(physicalDynamic.state!=ResultState::Untested)addCompatibility("Dynamic physical",physicalDynamic,"final accepted attempt");
			compatibility.details.push_back(std::string("Method A API outcome: ")+reportOutcome(physicalLegacy));compatibility.details.push_back(std::string("Method A user confirmation: ")+physicalName(physicalLegacy.physical));compatibility.details.push_back(std::string("Method B API outcome: ")+reportOutcome(physicalDynamic));compatibility.details.push_back(std::string("Method B user confirmation: ")+physicalName(physicalDynamic.physical));compatibility.details.push_back("API acceptance and physical response are independent evidence. An unsuccessful method does not establish wheel incompatibility.");value.sections.push_back(std::move(compatibility));
		value.sections.push_back({ "API errors", ffb.errors.empty() ? Status::Completed : Status::Failed, ffb.errors.empty() ? "No retained DirectInput errors." : std::format("{} DirectInput error(s) retained.", ffb.errors.size()), ffb.errors });
			value.sections.push_back({ "Application environment", Status::Completed, "Standalone diagnostic application metadata.", { std::string("Application version: ")+Version, std::format("SDL runtime version: {}",SDL_GetVersion()), "Windows platform: Win32", "Report timestamps include UTC evidence; filenames use local system time." } });
		return value;
		}

		void export_report()
		{
			lastExportDirectory=exports_path(documents_path());
			std::string identity="No Wheel Detected";
			if(ffb.selected>=0&&ffb.selected<int(ffb.entries.size())) identity=ffb.entries[ffb.selected].name;
			else if(quick.saved[0]) identity=quick.saved[0]->deviceName;
			const std::string stem=sanitize_filename_component(identity)+"_"+local_filename_time();
			const auto result = DeviceDiagnosticsReport::write_named(lastExportDirectory, report(), stem);
			exportSucceeded=result.success;lastExportFile=result.success?result.textPath:std::filesystem::path{};
			reportStatus = result.success ? "Report Exported Successfully" : result.error;
			ImGui::OpenPopup("Export result");
		}

		void header()
		{
			ImGui::TextColored({0.40f,0.68f,1.0f,1}, "HYP36rforce Device Diagnostics"); ImGui::SameLine(); ImGui::TextDisabled("v%s", Version);
			ImGui::TextDisabled("Input  |  Multi-Input  |  Force Feedback"); ImGui::Separator();
			if (!initialized) return;
			const std::array<std::pair<const char*, Page>, 4> pages{{ {"Devices",Page::Devices},{"Input Test",Page::InputTest},{"Quick Setup",Page::QuickSetup},{"FFB Test",Page::FfbTest} }};
			for (const auto& [label, value] : pages) { if (page == value) ImGui::PushStyleColor(ImGuiCol_Button, {0.15f,0.38f,0.65f,1}); if (ImGui::Button(label)) page = value; if (page == value) ImGui::PopStyleColor(); ImGui::SameLine(); }
			ImGui::NewLine(); ImGui::Separator();
		}

		void initialization_progress()
		{
			const bool startup=initializing&&backendForInitialization;
			const int totalStages=startup?5:4;
			float completed=0.0f;
			if(backendIndex>=0)
			{
				const float active=(std::clamp)(std::chrono::duration<float>(Clock::now()-backendStarted).count()/std::chrono::duration<float>(BackendObservationTime).count(),0.0f,1.0f);
				completed=float(backendIndex)+active;
			}
			else if(startup&&initializationPhase>=2)completed=4.0f;
			const float progress=(std::clamp)(completed/float(totalStages),0.0f,1.0f);
			ImGui::Text("Initializing Your Devices");help_marker(DeviceDiagnosticsHelp::InitializationProgress);
			ImGui::TextDisabled("Checking controller compatibility...");
			ImGui::Text("Overall Progress");ImGui::ProgressBar(progress,{500,0},std::format("{}%",int(progress*100.0f)).c_str());
			ImGui::Spacing();
			for(int i=0;i<int(pendingBackends.size());++i)
			{
				const auto state=pendingBackends[i].state;const char* marker=state==ResultState::Completed?"[OK]":(state==ResultState::Failed?"[!]":(state==ResultState::Running?"[>>]":(state==ResultState::Cancelled?"[--]":"[  ]")));
				ImGui::Text("%s %-24s %s",marker,pendingBackends[i].name.c_str(),state_name(state));
			}
			const char* nativeState=!startup?"not part of this re-test":(initializationPhase>=3?"complete":(initializationPhase==2&&backendIndex<0?"testing":"waiting"));
			ImGui::Text("%s %-24s %s",initializationPhase>=3?"[OK]":((initializationPhase==2&&backendIndex<0)?"[>>]":"[  ]"),"Native DirectInput FFB",nativeState);
			if(backendIndex>=0)
			{
				ImGui::Spacing();ImGui::Text("Currently Testing: %s",pendingBackends[backendIndex].name.c_str());
				ImGui::Text("Devices Found: %zu",input.devices.size());
				const float activeSeconds=std::chrono::duration<float>(Clock::now()-backendStarted).count();
				const int estimate=std::max(0,int(std::ceil((4-backendIndex)*10.0f-activeSeconds)));
				ImGui::Text("Estimated Remaining: ~%d seconds%s",estimate,startup?" plus native FFB discovery":"");
			}
			if(ImGui::Button("Cancel")){cancel_backend_sequence();}help_marker(DeviceDiagnosticsHelp::BackendProgress);
		}

		void devices_page()
		{
			ImGui::Text("Device Discovery"); help_marker(DeviceDiagnosticsHelp::DeviceDiscovery);
			if(initializing||backendIndex>=0){initialization_progress();return;}
			if (!initialized)
			{
				ImGui::Spacing(); ImGui::Text("Ready to check your setup?");
				if (ImGui::Button(initializationError.empty()?"Initialize Devices":"Retry Initialization", {220,44})) begin_initialization(); help_marker(DeviceDiagnosticsHelp::InitializeDevices);
				if(!initializationError.empty())ImGui::TextColored({1,0.45f,0.35f,1},"%s",initializationError.c_str());
				return;
			}
			ImGui::BeginChild("connected-devices",{0,220},true);
			ImGui::Text("CONNECTED DEVICES");
			if(input.devices.empty())ImGui::TextDisabled("No input devices detected on the active diagnostic interface.");
			for (size_t i=0;i<input.devices.size();++i)
			{
				const auto& d=input.devices[i];const bool responsive=i<deviceResponsive.size()&&deviceResponsive[i];
				ImGui::BulletText("%s",device_label(i).c_str());ImGui::SameLine();ImGui::TextDisabled("input detected | %d axes, %d buttons, %d hats | %s",d.axes,d.buttons,d.hats,responsive?"activity observed":"inactive/untested");
			}
			for (size_t i=0;i<ffb.entries.size();++i)ImGui::BulletText("%s — FFB detected%s",ffb_label(i).c_str(),int(i)==ffb.selected?" | selected":"");
			if (ImGui::Button("Re-detect Devices"))begin_initialization();
			ImGui::EndChild();
			ImGui::Spacing();ImGui::BeginChild("backend-compatibility",{0,260},true);
			ImGui::Text("INPUT BACKEND COMPATIBILITY");help_marker(DeviceDiagnosticsHelp::BackendCompatibility);
			if (ImGui::Button("Re-run Backend Tests")){start_backend_sequence(false);}help_marker(DeviceDiagnosticsHelp::RerunBackendTests);
			ImGui::SameLine(); ImGui::TextDisabled("Four isolated sessions; each observes for 10 seconds.");
			if (ImGui::BeginTable("backends", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
			{
				for (const char* h : {"Backend","Status","Devices","Input","Delayed","Error"}) { ImGui::TableNextColumn(); ImGui::TextUnformatted(h); }
				for (const auto& b : backends) { ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::TextUnformatted(b.name.c_str());ImGui::TableNextColumn();ImGui::TextUnformatted(state_name(b.state));ImGui::TableNextColumn();ImGui::Text("%zu",b.devices.size());ImGui::TableNextColumn();ImGui::TextUnformatted(std::any_of(b.devices.begin(),b.devices.end(),[](const auto& d){return d.inputAvailable;})?"available":(b.state==ResultState::Completed?"none":"—"));ImGui::TableNextColumn();ImGui::Text("%zu",b.delayedEvents.size());ImGui::TableNextColumn();ImGui::TextWrapped("%s",b.error.empty()?"—":b.error.c_str()); }
				ImGui::EndTable();
			}
			ImGui::Text("Delayed Discovery"); help_marker(DeviceDiagnosticsHelp::DelayedDiscovery);ImGui::EndChild();
		}

		void input_page()
		{
			ImGui::Text("Input Test"); help_marker(DeviceDiagnosticsHelp::InputTest);
			if (input.devices.empty()) { ImGui::TextDisabled("No input device is connected. Initialize or rescan devices first."); return; }
			selectedInput = std::clamp(selectedInput, 0, int(input.devices.size()) - 1);
			if (ImGui::BeginCombo("Device", device_label(selectedInput).c_str())) { for (int i=0;i<int(input.devices.size());++i) if (ImGui::Selectable(device_label(i).c_str(), i==selectedInput)) selectedInput=i; ImGui::EndCombo(); }
			auto* joystick = input.handles[selectedInput]; bool active = false;
			for (int i=0;i<SDL_GetNumJoystickAxes(joystick);++i) { const int raw=SDL_GetJoystickAxis(joystick,i); const float norm = raw < 0 ? raw/32768.0f : raw/32767.0f; ImGui::Text("Axis %d   raw %6d   normalized % .3f",i,raw,norm); ImGui::ProgressBar((norm+1)*0.5f,{350,0}); active |= std::abs(raw)>2500; }
			for (int i=0;i<SDL_GetNumJoystickButtons(joystick);++i) { if (i%12) ImGui::SameLine(); const bool down=SDL_GetJoystickButton(joystick,i); ImGui::TextColored(down?ImVec4{0.3f,1,0.5f,1}:ImVec4{0.6f,0.6f,0.6f,1},"B%d",i); active |= down; }
			for (int i=0;i<SDL_GetNumJoystickHats(joystick);++i) { const auto value=SDL_GetJoystickHat(joystick,i); ImGui::Text("POV %d: 0x%02X",i,value); active |= value != SDL_HAT_CENTERED; }
			if (active) { inputStatus = "completed"; if (selectedInput < int(deviceResponsive.size())) deviceResponsive[selectedInput] = true; }
			ImGui::TextColored(active ? ImVec4{0.3f,1,0.5f,1} : ImVec4{0.8f,0.7f,0.35f,1}, active ? "Responsive now" : (selectedInput < int(deviceResponsive.size()) && deviceResponsive[selectedInput] ? "Responsive (activity observed earlier)" : "Detected but inactive"));
		}

		void quick_page()
		{
			static const std::array<const char*,7> names{{"Steering","Accelerator","Brake","Shift Up","Shift Down","Start / Menu","Back Button"}};
			ImGui::Text("Quick Setup"); help_marker(DeviceDiagnosticsHelp::QuickSetup);
			if(quickConfirmation)
			{
				if(compatibilityComplete)
				{
					const int tested=int(std::count_if(backends.begin(),backends.end(),[](const auto& b){return b.state==ResultState::Completed||b.state==ResultState::Failed;}));
					const size_t discovered=std::accumulate(backends.begin(),backends.end(),size_t{0},[](size_t count,const auto& b){return count+b.devices.size();});
					const bool warnings=std::any_of(backends.begin(),backends.end(),[](const auto& b){return b.state==ResultState::Failed;});
					ImGui::Text("Device Compatibility Check Complete");
					ImGui::BulletText("%d input backends tested",tested);ImGui::BulletText("%zu input device observations",discovered);ImGui::BulletText("%zu FFB endpoints",ffb.entries.size());
					if(warnings)ImGui::TextColored({1,0.72f,0.25f,1},"Some backend checks reported warnings. Details are available under Devices.");
				}
				ImGui::Spacing();ImGui::TextWrapped("Your devices have been checked.\nWould you like to configure your controls?");
				if(ImGui::Button("Start Quick Setup",{190,42})){quickConfirmation=false;quick.start(Clock::now());begin_capture();}help_marker(DeviceDiagnosticsHelp::StartQuickSetup);
				ImGui::SameLine();if(ImGui::Button("Not Now",{130,42})){quickConfirmation=false;page=Page::Devices;}help_marker(DeviceDiagnosticsHelp::NotNow);
				ImGui::TextDisabled("Not Now leaves every saved diagnostic binding unchanged.");return;
			}
			if (!quick.active && quick.step >= int(quick.saved.size()))
			{
				const auto now=Clock::now();const bool focused=(SDL_GetWindowFlags(window)&SDL_WINDOW_INPUT_FOCUS)!=0;
				if(quickFfb.stage!=QuickFfbStage::Complete)
				{
					ImGui::Text("FORCE FEEDBACK CHECK");help_marker(DeviceDiagnosticsHelp::FfbTest);
					ImGui::Text("Selected wheel: %s",ffb.selected>=0?ffb.entries[ffb.selected].name.c_str():"Not detected");
					ImGui::TextWrapped("Your wheel has been detected. We'll gently shake it left and right to check that force feedback is responding. The wheel may move during this test. Keep your hands clear.");
					if(!ffb.connected()){ImGui::TextColored({1,0.6f,0.25f,1},"Force feedback was not detected.");if(ImGui::Button("Finish without FFB test"))quickFfb.skip();return;}
					if(quickFfb.stage==QuickFfbStage::Offer){if(ImGui::Button("Test Force Feedback")){quickFfb.begin(now);}ImGui::SameLine();if(ImGui::Button("Skip"))quickFfb.skip();}
					else if(quickFfb.stage==QuickFfbStage::Countdown){const int seconds=std::max(1,int(std::ceil(std::chrono::duration<float>(quickFfb.deadline-now).count())));ImGui::Text("Starting in %d...",seconds);if(quickFfb.countdown_complete(now)){quickFfbSafety.authorized=true;if(quickFfbSafety.begin(ffb.connected(),focused,now)){quickFfbShake.begin(now);if(ffb.run_direction(false,invertFfb,PhysicalOutputCeilingPercent,quickFfbSafety))quickFfb.start_output(now);else {quickFfbSafety.stop();quickFfb.answer(QuickFfbResponse::NotConfirmed);}}else quickFfb.answer(QuickFfbResponse::NotConfirmed);}}
					else if(quickFfb.stage==QuickFfbStage::Running){quickFfbSafety.beat(now);if(const auto direction=quickFfbShake.update(now);direction&&!ffb.update_direction(*direction,invertFfb)){ffb.stop();quickFfbSafety.stop();quickFfbShake.stop();quickFfb.answer(QuickFfbResponse::NotConfirmed);}else if(quickFfb.output_complete(now)){ffb.stop();quickFfbSafety.stop();quickFfbShake.stop();quickFfb.stage=QuickFfbStage::Confirm;}else if(quickFfbSafety.must_stop(true,focused,ffb.connected(),now)){ffb.stop();quickFfbSafety.stop();quickFfbShake.stop();quickFfb.answer(QuickFfbResponse::NotConfirmed);}ImGui::Text("Gently shaking wheel...");}
					else if(quickFfb.stage==QuickFfbStage::Confirm){ImGui::Text("Did you feel the wheel shake?");if(ImGui::Button("Yes"))quickFfb.answer(QuickFfbResponse::Confirmed);ImGui::SameLine();if(ImGui::Button("No"))quickFfb.answer(QuickFfbResponse::NotConfirmed);ImGui::SameLine();if(ImGui::Button("Skip"))quickFfb.skip();}
					else if(quickFfb.stage==QuickFfbStage::RetryChoice){ImGui::Text("Response was not confirmed.");if(ImGui::Button("Re-detect")){HWND hwnd=static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(window),SDL_PROP_WINDOW_WIN32_HWND_POINTER,nullptr));ffb.redetect(GetModuleHandleW(nullptr),hwnd,saved_ffb_identity());if(ffb.connected())remember_selected_ffb();quickFfb.retry();}ImGui::SameLine();if(ImGui::Button("Retry Test"))quickFfb.retry();ImGui::SameLine();if(ImGui::Button("Continue"))quickFfb.stage=QuickFfbStage::Complete;}
					ImGui::PushStyleColor(ImGuiCol_Button,{0.65f,0.08f,0.08f,1});if(ImGui::Button("STOP")){ffb.stop();quickFfbSafety.stop();quickFfbShake.stop();quickFfb.answer(QuickFfbResponse::NotConfirmed);}ImGui::PopStyleColor();return;
				}
				ImGui::Text("Quick Setup Complete");
				bool essentialMissing = false;
				for (size_t i=0;i<quick.saved.size();++i)
				{
					const auto& value=quick.saved[i]; ImGui::Text("%s: %s",names[i],value?value->deviceName.c_str():"Unassigned");
					if (i<3 && !value) essentialMissing=true;
				}
				if(essentialMissing) ImGui::TextColored({1,0.6f,0.25f,1},"Steering, accelerator, or brake remains unassigned.");
				ImGui::Text("Force Feedback: %s",ffb.connected()?"Detected":(ffb.entries.size()>1?"Selection Required":"Not Detected"));
				ImGui::Text("FFB Response: %s",quickFfb.response==QuickFfbResponse::Confirmed?"Confirmed by User":(quickFfb.response==QuickFfbResponse::NotConfirmed?"Not Confirmed":"Not Tested"));
				if(ImGui::Button("Open Input Test")) page=Page::InputTest; ImGui::SameLine(); if(ImGui::Button("Open FFB Test")) page=Page::FfbTest; ImGui::SameLine();
				if(ImGui::Button("Run Quick Setup Again")){quickFfb.retry();quick.start(Clock::now());begin_capture();} ImGui::SameLine(); if(ImGui::Button("Home")) page=Page::Devices;
				return;
			}
			if (!quick.active) { if(ImGui::Button("Start Quick Setup")){quick.start(Clock::now());begin_capture();} return; }
			quick.update(Clock::now());
			ImGui::ProgressBar(quick.step/float(quick.saved.size()),{400,0});
			ImGui::Text("STEP %d OF %d — %s",quick.step+1,int(quick.saved.size()),names[quick.step]);
			ImGui::TextWrapped("Each binding may come from a different physical device. Existing saved bindings are preserved when a step is skipped.");
			if (!quick.candidate && !quick.timedOut)
			{
				const float remaining=std::max(0.0f,std::chrono::duration<float>(quick.deadline-Clock::now()).count());
				ImGui::Text("Move or press the requested %s control",names[quick.step]);
				ImGui::Text("Detecting input..."); ImGui::ProgressBar(remaining/6.0f,{360,0},std::format("{:.1f} seconds remaining",remaining).c_str());
			}
			if(quick.candidate)
			{
				ImGui::Text("Detected device: %s",quick.candidate->deviceName.c_str()); ImGui::Text("Candidate: %s",quick.candidate->control.c_str());
				if(quick.candidate->ambiguous) ImGui::TextColored({1,0.55f,0.25f,1},"Several controls moved together. Retry and move only the requested control.");
			}
			else if(quick.timedOut) ImGui::TextColored({1,0.65f,0.3f,1},"No unambiguous input was detected. Retry, skip, or cancel.");

			if(quick.step>0){if(ImGui::Button("Back")){quick.back(Clock::now());begin_capture();}help_marker(DeviceDiagnosticsHelp::BackSetup);ImGui::SameLine();}
			const bool canContinue=quick.candidate && !quick.candidate->ambiguous; if(!canContinue)ImGui::BeginDisabled();
			if(ImGui::Button("Continue"))
			{
				if(quick.continue_step(Clock::now()))
				{
					if(!quick.active){quickStatus="completed";quickFfb.retry();save_profile();}else begin_capture();
				}
			}
			if(!canContinue)ImGui::EndDisabled(); help_marker(DeviceDiagnosticsHelp::ContinueSetup); ImGui::SameLine();
			if(ImGui::Button("Retry")){quick.retry(Clock::now());begin_capture();} help_marker(DeviceDiagnosticsHelp::RetrySetup); ImGui::SameLine();
			if(ImGui::Button("Skip")){quick.skip(Clock::now());if(!quick.active){quickStatus="completed";quickFfb.retry();save_profile();}else begin_capture();} help_marker(DeviceDiagnosticsHelp::SkipSetup); ImGui::SameLine();
			if(ImGui::Button("Cancel")){quick.cancel();quickStatus="untested";page=Page::Devices;} help_marker(DeviceDiagnosticsHelp::CancelSetup);
		}

		void ffb_page()
		{
			ImGui::Text("FFB Test"); help_marker(DeviceDiagnosticsHelp::FfbTest);
			ImGui::TextColored({1,0.75f,0.25f,1},"Motor output is disabled until you select a device and explicitly authorize it.");
			ImGui::Text("FFB Device: %s",ffb.selected>=0?ffb_label(ffb.selected).c_str():"No device selected");
			ImGui::Text("Status: %s",ffb.connected()?"Ready":(ffb.entries.empty()?"Not Found":(ffbStatus=="selection required"?"Selection Required":"Initialization Failed")));
			if(ImGui::Button("Re-detect"))
			{
				const std::string previous=ffb.selected>=0?ffb.entries[ffb.selected].id:saved_ffb_identity();
				HWND hwnd=static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(window),SDL_PROP_WINDOW_WIN32_HWND_POINTER,nullptr));
				const auto result=ffb.redetect(GetModuleHandleW(nullptr),hwnd,previous);safety.stop();compatibilitySafety.stop();shake.stop();safety.authorized=false;deliveryStage=DeliveryStage::Idle;lastSafetyShutdown="Re-detect disarmed output";
				if(ffb.connected()){remember_selected_ffb();ffbStatus="ready";}else ffbStatus=result.state==FfbResolution::SelectionRequired?"selection required":(ffb.entries.empty()?"not found":"initialization failed");
			} help_marker(DeviceDiagnosticsHelp::RedetectWheel);help_marker(DeviceDiagnosticsHelp::FfbReadiness);
			if(ffbStatus=="selection required"||(!ffb.connected()&&ffb.entries.size()>1))
			{
				if(ImGui::BeginCombo("Confirm FFB interface","Select a device")){for(int i=0;i<int(ffb.entries.size());++i)if(ImGui::Selectable(ffb_label(i).c_str())){ffb.stop();safety.stop();compatibilitySafety.stop();shake.stop();safety.authorized=false;deliveryStage=DeliveryStage::Idle;lastSafetyShutdown="Device selection disarmed output";if(ffb.select(i)){remember_selected_ffb();ffbStatus="ready";}}ImGui::EndCombo();}
			}
			ImGui::Separator();ImGui::Text("Directional Test");help_marker(DeviceDiagnosticsHelp::DirectionTest);
			if(ImGui::Checkbox("Invert FFB",&invertFfb)){ffb.stop();safety.stop();compatibilitySafety.stop();shake.stop();deliveryStage=DeliveryStage::Idle;lastSafetyShutdown="Inversion change stopped output";save_profile();}help_marker(DeviceDiagnosticsHelp::InvertFfbDiagnostic);
			ImGui::Text("FFB Strength");ImGui::SliderInt("##strength",&strength,20,100,"%d%%",ImGuiSliderFlags_AlwaysClamp); help_marker(DeviceDiagnosticsHelp::Strength);
			ImGui::Text("Requested nominal magnitude: %d / %d",requested_nominal_magnitude(strength),DI_FFNOMINALMAX);
			help_marker(DeviceDiagnosticsHelp::RequestedVsLimited);
			ImGui::TextColored({1,0.72f,0.25f,1},"Effective physical-test limit: %d%% (%d / %d)",PhysicalOutputCeilingPercent,safety_limited_magnitude(strength),DI_FFNOMINALMAX);help_marker(DeviceDiagnosticsHelp::SafetyLimitedOutput);
			ImGui::Checkbox("I understand this will move the selected wheel",&safety.authorized);
			const bool compatibilityRunning=deliveryStage==DeliveryStage::Countdown||deliveryStage==DeliveryStage::Legacy||deliveryStage==DeliveryStage::SafetyInterval||deliveryStage==DeliveryStage::Dynamic;
			const bool comparisonActive=deliveryStage!=DeliveryStage::Idle&&deliveryStage!=DeliveryStage::Results&&deliveryStage!=DeliveryStage::Cancelled;
			const bool canRun=safety.authorized&&ffb.connected()&&!comparisonActive;if(!canRun)ImGui::BeginDisabled();
			ImGui::Button("< Test Left",{170,42});const bool leftHeld=ImGui::IsItemActive();help_marker(DeviceDiagnosticsHelp::LeftFfb);ImGui::SameLine();
			ImGui::Button("Test Right >",{170,42});const bool rightHeld=ImGui::IsItemActive();help_marker(DeviceDiagnosticsHelp::RightFfb);
			ImGui::Separator();ImGui::Text("Shake Test");
			ImGui::Button("Hold to Shake",{190,42}); const bool shakeHeld=ImGui::IsItemActive();help_marker(DeviceDiagnosticsHelp::ShakeFfb);
			if(!canRun) ImGui::EndDisabled();
			const bool focused=(SDL_GetWindowFlags(window)&SDL_WINDOW_INPUT_FOCUS)!=0; const auto now=Clock::now();
			const bool held=leftHeld||rightHeld||shakeHeld;
			if(held&&!safety.running&&safety.begin(ffb.connected(),focused,now))
			{
				bool started=false;
				if(leftHeld){started=ffb.run_direction(false,invertFfb,strength,safety);leftTestStatus=started?"completed":"failed";}
				else if(rightHeld){started=ffb.run_direction(true,invertFfb,strength,safety);rightTestStatus=started?"completed":"failed";}
				else {shake.begin(now);started=ffb.run_direction(shake.right,invertFfb,strength,safety);shakeTestStatus=started?"completed":"failed";}
				if(started)ffbStatus="completed";else {safety.stop();shake.stop();}
			}
			if(shakeHeld&&safety.running)if(const auto direction=shake.update(now);direction&&!ffb.update_direction(*direction,invertFfb)){ffb.stop();safety.stop();shake.stop();shakeTestStatus="failed";lastSafetyShutdown="Shake update failed";}
			if(held&&safety.running)safety.beat(now);
			if(safety.must_stop(held,focused,ffb.connected(),now))
			{
				if(!held)lastSafetyShutdown="Control released";else if(!focused)lastSafetyShutdown="Focus lost";else if(!ffb.connected())lastSafetyShutdown="Device disconnected";else if(now-safety.started>=MaximumRunTime)lastSafetyShutdown="1.5-second timeout";else lastSafetyShutdown="Watchdog timeout";
				ffb.stop();safety.stop();shake.stop();
			}
			ImGui::Text("Physical request: %s | effective %ld / %d | safety ceiling %d%%",safety.running?"ACTIVE":"stopped",ffb.lastRequestedMagnitude,DI_FFNOMINALMAX,PhysicalOutputCeilingPercent);
			ImGui::TextDisabled("Descriptor: %s | Create 0x%08X | Download 0x%08X | Start 0x%08X",ffb.lastDescriptor.c_str(),unsigned(ffb.lastCreate),unsigned(ffb.lastDownload),unsigned(ffb.lastStart));
			ImGui::Separator();ImGui::Text("Compare FFB Response");help_marker(DeviceDiagnosticsHelp::FfbCompatibility);
			const size_t actuatorCount=ffb.selected>=0?ffb.entries[ffb.selected].actuatorAxes.size():0;
			ImGui::Text("Current Output Path: %s",actuatorCount==1?"single-axis layout; legacy fallback is possible":(actuatorCount>1?"multi-axis layout; persistent updates are normally used":"not detected"));
			const bool awaitingFeedback=deliveryStage==DeliveryStage::LegacyFeedback||deliveryStage==DeliveryStage::DynamicFeedback;
			const char* compatibilityStatus=deliveryStage==DeliveryStage::Idle?"Not Tested":(deliveryStage==DeliveryStage::Results?"Completed":(deliveryStage==DeliveryStage::Cancelled?"Cancelled":(awaitingFeedback?"Awaiting Feedback":"Testing")));
			ImGui::Text("Status: %s",compatibilityStatus);
			if(deliveryStage==DeliveryStage::Idle)
			{
				ImGui::TextWrapped("The selected wheel will move during two guided tests. Each method uses the same bounded request sequence, at no more than 20%% nominal output and 1.5 seconds. You will confirm what you felt after each test.");
				ImGui::Text("Selected device: %s",ffb.selected>=0?ffb.entries[ffb.selected].name.c_str():"None");
				if(!ffb.connected())ImGui::BeginDisabled();if(ImGui::Button("Compare FFB Response")){run_software_compatibility();deliveryStarted=now;deliveryDeadline=now+std::chrono::seconds(3);pendingCompatibilityStrategy=CompatibilityStrategy::LegacyRecreation;deliveryStage=DeliveryStage::Countdown;}if(!ffb.connected())ImGui::EndDisabled();
			}
			if(deliveryStage!=DeliveryStage::Idle)
			{
				ImGui::TextDisabled("Software simulation: legacy %d creations; dynamic %d creation + %d updates. No motor was activated.",simulatedLegacy.createCount,simulatedDynamic.createCount,simulatedDynamic.updateCount);
				ImGui::ProgressBar(delivery_progress(deliveryStage,compatibilitySignalIndex),{360,0});
				ImGui::Text("Overall elapsed: %.1f seconds",std::chrono::duration<float>(now-deliveryStarted).count());
				if(compatibilityRunning){ImGui::PushStyleColor(ImGuiCol_Button,{0.65f,0.08f,0.08f,1});if(ImGui::Button("STOP",{110,42}))cancel_delivery("Emergency STOP");ImGui::PopStyleColor();help_marker(DeviceDiagnosticsHelp::StopFfb);}
				if(deliveryStage==DeliveryStage::Countdown){ImGui::Text("%s safety countdown: %d",pendingCompatibilityStrategy==CompatibilityStrategy::LegacyRecreation?"Test 1 — Legacy":"Test 2 — Dynamic",std::max(1,int(std::ceil(std::chrono::duration<float>(deliveryDeadline-now).count()))));}
				else if(deliveryStage==DeliveryStage::Legacy){ImGui::Text("Test 1 — Legacy: recreating each bounded request.");ImGui::Text("Active test timer: %.2f seconds",std::chrono::duration<float>(now-compatibilityTestStarted).count());}
				else if(deliveryStage==DeliveryStage::SafetyInterval)ImGui::TextWrapped("Safety Interval: the first test has finished. Force output is stopped before the next method begins. %.1f seconds remaining.",std::max(0.0f,1.0f-std::chrono::duration<float>(now-safetyIntervalStarted).count()));
				else if(deliveryStage==DeliveryStage::Dynamic){ImGui::Text("Test 2 — Dynamic: updating one persistent effect.");ImGui::Text("Active test timer: %.2f seconds",std::chrono::duration<float>(now-compatibilityTestStarted).count());}
				update_delivery(now,focused);
				if(deliveryStage==DeliveryStage::LegacyFeedback||deliveryStage==DeliveryStage::DynamicFeedback)
				{
					auto& result=deliveryStage==DeliveryStage::LegacyFeedback?physicalLegacy:physicalDynamic;const bool first=deliveryStage==DeliveryStage::LegacyFeedback;
					ImGui::Text("%s — Awaiting Feedback",first?"Test 1 — Legacy":"Test 2 — Dynamic");ImGui::Text("Test timer stopped: %.2f seconds",compatibilityTestElapsed.count()/1000.0);ImGui::TextWrapped("Did you feel the wheel respond?");
					for(const auto [label,value]:{std::pair{"Yes",PhysicalConfirmation::Yes},std::pair{"No",PhysicalConfirmation::No},std::pair{"Unsure",PhysicalConfirmation::Unsure}}){if(ImGui::RadioButton(label,result.physical==value))result.physical=value;ImGui::SameLine();}ImGui::NewLine();
					if(ImGui::Button("Retry Test"))retry_current_compatibility(result.strategy,now);ImGui::SameLine();
					if(result.physical==PhysicalConfirmation::Untested)ImGui::BeginDisabled();
					if(ImGui::Button(first?"Confirm & Next":"Finish"))
					{
						ffb.stop();compatibilitySafety.stop();
						if(first){if(!focused||!ffb.connected())cancel_delivery(!focused?"Comparison lost focus before Test 2":"Comparison device disconnected before Test 2");else{safetyIntervalStarted=now;deliveryStage=DeliveryStage::SafetyInterval;}}
						else{deliveryStage=DeliveryStage::Shutdown;deliveryShutdownReason="Normal guided shutdown";deliveryStage=DeliveryStage::Results;}
					}
					if(result.physical==PhysicalConfirmation::Untested)ImGui::EndDisabled();
				}
				if(deliveryStage==DeliveryStage::Results){const auto outcome=[](const CompatibilityResult& value){if(value.state==ResultState::Completed)return value.failureCount?"Method Unsuccessful":"Commands Accepted";if(value.state==ResultState::Cancelled)return "Test Interrupted";if(value.state==ResultState::Failed)return "Method Unsuccessful";return "Inconclusive";};const auto felt=[](PhysicalConfirmation value){return value==PhysicalConfirmation::Yes?"Yes":(value==PhysicalConfirmation::No?"No":(value==PhysicalConfirmation::Unsure?"Unsure":"Not provided"));};ImGui::Text("FFB Response Comparison Complete");ImGui::Text("Device Readiness: %s",ffb.connected()?"Ready":"Unavailable");ImGui::Text("Test 1 — Legacy API: %s",outcome(physicalLegacy));help_marker(DeviceDiagnosticsHelp::CommandsAccepted);ImGui::Text("Test 1 — Physical response: %s",felt(physicalLegacy.physical));help_marker(DeviceDiagnosticsHelp::PhysicalResponseConfirmed);ImGui::Text("Test 2 — Dynamic API: %s",outcome(physicalDynamic));ImGui::Text("Test 2 — Physical response: %s",felt(physicalDynamic.physical));ImGui::TextWrapped("API acceptance and your physical observation are recorded independently. An unsuccessful method does not mean the wheel is defective.");help_marker(DeviceDiagnosticsHelp::TorqueLimitation);}
				if(deliveryStage==DeliveryStage::Cancelled)ImGui::TextColored({1,0.6f,0.25f,1},"Cancelled: %s",deliveryShutdownReason.c_str());
			}
		}

		void draw()
		{
			ImGui::SetNextWindowPos({0,0}); ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
			ImGui::Begin("HYP36rforce Device Diagnostics",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoSavedSettings);
			header(); ImGui::BeginChild("content",{0,-112},true); switch(page){case Page::Devices:devices_page();break;case Page::InputTest:input_page();break;case Page::QuickSetup:quick_page();break;case Page::FfbTest:ffb_page();break;} ImGui::EndChild();
			ImGui::TextDisabled("%s",status.c_str());
			ImGui::SameLine(ImGui::GetWindowWidth()-300); if(ImGui::Button("Export Report")){export_report();} help_marker(DeviceDiagnosticsHelp::ExportReport);
			ImGui::TextDisabled("Reports: %s",lastExportDirectory.string().c_str());ImGui::SameLine();if(ImGui::Button("Open Exports Folder")){std::filesystem::create_directories(lastExportDirectory);ShellExecuteW(nullptr,L"open",lastExportDirectory.c_str(),nullptr,nullptr,SW_SHOWNORMAL);}help_marker(DeviceDiagnosticsHelp::OpenExportsFolder);
			if(ImGui::BeginPopupModal("Export result",nullptr,ImGuiWindowFlags_AlwaysAutoResize))
			{
				if(exportSucceeded){ImGui::Text("Report Exported Successfully");ImGui::Text("%s",lastExportFile.filename().string().c_str());}
				else {ImGui::TextColored({1,0.4f,0.3f,1},"Export failed");ImGui::TextWrapped("%s",reportStatus.c_str());}
				if(exportSucceeded&&ImGui::Button("Open Exports Folder")){ShellExecuteW(nullptr,L"open",lastExportDirectory.c_str(),nullptr,nullptr,SW_SHOWNORMAL);}if(exportSucceeded)ImGui::SameLine();if(ImGui::Button("OK"))ImGui::CloseCurrentPopup();ImGui::EndPopup();
			}
			ImGui::End();
		}

		int loop()
		{
			while(running)
			{
				SDL_Event event{}; while(SDL_PollEvent(&event)){ImGui_ImplSDL3_ProcessEvent(&event);if(backendIndex>=0&&event.type==SDL_EVENT_JOYSTICK_ADDED)input.open_delayed(event.jdevice.which,pendingBackends[backendIndex].name,pendingBackends[backendIndex].delayedEvents,pendingBackends[backendIndex].openingResults);if(event.type==SDL_EVENT_QUIT||event.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED)running=false;if(event.type==SDL_EVENT_WINDOW_FOCUS_LOST){if(deliveryStage==DeliveryStage::Countdown||deliveryStage==DeliveryStage::Legacy||deliveryStage==DeliveryStage::SafetyInterval||deliveryStage==DeliveryStage::Dynamic)cancel_delivery("Focus lost");ffb.stop();safety.stop();quickFfbSafety.stop();shake.stop();quickFfbShake.stop();if(quickFfb.stage==QuickFfbStage::Countdown||quickFfb.stage==QuickFfbStage::Running)quickFfb.answer(QuickFfbResponse::NotConfirmed);lastSafetyShutdown="Focus lost";}}
				if((deliveryStage==DeliveryStage::Countdown||deliveryStage==DeliveryStage::Legacy||deliveryStage==DeliveryStage::SafetyInterval||deliveryStage==DeliveryStage::Dynamic)&&page!=Page::FfbTest)cancel_delivery("Delivery Test page was left");
				if((quickFfb.stage==QuickFfbStage::Countdown||quickFfb.stage==QuickFfbStage::Running)&&page!=Page::QuickSetup){ffb.stop();quickFfbSafety.stop();quickFfbShake.stop();quickFfb.answer(QuickFfbResponse::NotConfirmed);}
				update_initialization(); update_backend(); if(backendIndex<0) input.pump(); quick.update(Clock::now()); capture_quick_setup_input();
				ImGui_ImplSDLRenderer3_NewFrame();ImGui_ImplSDL3_NewFrame();ImGui::NewFrame();draw();ImGui::Render();
				SDL_SetRenderDrawColor(renderer,10,17,29,255);SDL_RenderClear(renderer);ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(),renderer);SDL_RenderPresent(renderer);
			}
			ffb.stop(); safety.stop();compatibilitySafety.stop();quickFfbSafety.stop(); return 0;
		}

		~App(){ffb.shutdown();input.close();if(ImGui::GetCurrentContext()){ImGui_ImplSDLRenderer3_Shutdown();ImGui_ImplSDL3_Shutdown();ImGui::DestroyContext();}if(renderer)SDL_DestroyRenderer(renderer);if(window)SDL_DestroyWindow(window);SDL_Quit();}
	};
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
	App app;
	if (!app.start()) { MessageBoxA(nullptr, SDL_GetError(), "HYP36rforce Device Diagnostics", MB_ICONERROR); return 1; }
	return app.loop();
}
