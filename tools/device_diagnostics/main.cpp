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

		bool open(const char* backend, std::string& error)
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
				if (!joystick) continue;
				handles.push_back(joystick);
				const char* name = SDL_GetJoystickName(joystick);
				devices.push_back({ std::to_string(SDL_GetJoystickID(joystick)), name ? name : "Unnamed input device", backend,
					SDL_GetJoystickVendor(joystick), SDL_GetJoystickProduct(joystick), SDL_GetNumJoystickAxes(joystick),
					SDL_GetNumJoystickButtons(joystick), SDL_GetNumJoystickHats(joystick), true, false });
			}
			SDL_free(ids);
			return true;
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
		HRESULT lastAcquire = S_FALSE, lastActuators = S_FALSE, lastCreate = S_FALSE, lastDownload = S_FALSE, lastStart = S_FALSE, lastUpdate = S_FALSE, lastStop = S_FALSE;
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
			lastRequestedMagnitude = safety.bounded_magnitude(requestedPercent);
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

		bool run_catalog_effect(int catalog, int requestedPercent, SafetyController& safety)
		{
			if(!device)return false; stop(); lastRequestedMagnitude=safety.bounded_magnitude(requestedPercent);
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
		ShakeController shake;
		Page page = Page::Devices;
		bool initialized = false, showDetails = false, running = true;
		bool quickConfirmation = false;
		bool initializing = false;
		int initializationPhase = 0;
		std::string initializationError;
		std::string status = "Ready to check your setup?";
		std::array<BackendResult, 4> backends{{ {"Windows.Gaming.Input"}, {"SDL3 RawInput"}, {"SDL3 DirectInput"}, {"SDL3 XInput"} }};
		int backendIndex = -1;
		Clock::time_point backendStarted{};
		QuickSetupController quick;
		std::vector<std::vector<Sint16>> captureBaselines;
		std::vector<bool> deviceResponsive;
		int selectedInput = 0, strength = 20;
		bool invertFfb = false, exportSucceeded = false;
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
			out << "invert=" << (invertFfb ? 1 : 0) << '\n';
		}
		void load_profile()
		{
			std::ifstream in(profile_path());
			for (auto& binding : quick.saved)
			{
				std::string line; if (!std::getline(in, line) || line.empty()) continue;
				const auto first = line.find('|'); const auto second = first == std::string::npos ? first : line.find('|', first + 1);
				if (first != std::string::npos && second != std::string::npos)
					binding = CapturedInput{ line.substr(0, first), line.substr(first + 1, second - first - 1), line.substr(second + 1) };
			}
			std::string setting;if(std::getline(in,setting)&&setting.rfind("invert=",0)==0)invertFfb=setting.substr(7)=="1";
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
		std::string saved_ffb_identity() const { return quick.saved[7] ? quick.saved[7]->deviceId : std::string{}; }
		std::string ffb_output_classification() const
		{
			if(ffb.lastRequestedMagnitude==0)return safety.authorized?"A — output was never requested":"B — output remained blocked by safety authorization";
			if(ffb.selected<0)return "C — no validated FFB interface was selected";
			if(FAILED(ffb.lastAcquire)||FAILED(ffb.lastActuators)||FAILED(ffb.lastCreate)||FAILED(ffb.lastDownload)||FAILED(ffb.lastStart))return "D — a DirectInput operation was rejected";
			return "E — DirectInput accepted the request; physical response remains unverified";
		}
		void remember_selected_ffb()
		{
			if(ffb.selected<0||ffb.selected>=int(ffb.entries.size()))return;
			quick.saved[7]=CapturedInput{ffb.entries[ffb.selected].id,ffb_label(ffb.selected),"Native DirectInput FFB"}; save_profile();
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
			initialized=false; initializing=true; initializationPhase=0; initializationError.clear();
			status="Starting Windows input discovery...";
		}

		void update_initialization()
		{
			if(!initializing) return;
			if(initializationPhase==0)
			{
				status="Discovering SDL DirectInput devices...";
				if(!input.open("SDL3 DirectInput",initializationError)){initializing=false;status="Input initialization failed: "+initializationError;return;}
				initializationPhase=1; return;
			}
			if(initializationPhase==1)
			{
				status="Discovering native DirectInput FFB interfaces...";
				HWND hwnd=static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(window),SDL_PROP_WINDOW_WIN32_HWND_POINTER,nullptr));
				if(!ffb.initialize(GetModuleHandleW(nullptr),hwnd)&&!ffb.errors.empty()) initializationError=ffb.errors.back();
				initializationPhase=2; return;
			}
			initializing=false; initialized=true;
			status=std::format("Devices Discovered: {} input, {} FFB-capable",input.devices.size(),ffb.entries.size());
			deviceResponsive.assign(input.devices.size(),false);
			std::vector<std::string> ids;for(const auto& entry:ffb.entries)ids.push_back(entry.id);
			const auto resolution=resolve_ffb_device(ids,saved_ffb_identity());
			if(resolution.index>=0&&ffb.select(resolution.index)){remember_selected_ffb();ffbStatus="ready";}
			else ffbStatus=resolution.state==FfbResolution::SelectionRequired?"selection required":(ffb.entries.empty()?"not found":"initialization failed");
			page=Page::QuickSetup; quickConfirmation=true;
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

		void run_all_backends()
		{
			backendIndex = 0; backendStarted = Clock::now();
			for (auto& result : backends) result = BackendResult{ result.name };
			start_backend();
		}
		void start_backend()
		{
			auto& result = backends[backendIndex]; result.state = ResultState::Running; result.startedUtc = utc_now();
			std::string error;
			if (!input.open(result.name.c_str(), error)) { result.state = ResultState::Failed; result.error = error; }
			else { result.devices = input.devices; result.effectiveBackend=result.name+" isolated hint profile"; }
			backendStarted = Clock::now();
		}
		void update_backend()
		{
			if (backendIndex < 0) return;
			auto& result = backends[backendIndex]; input.pump(&result.delayedEvents);
			if (Clock::now() - backendStarted < BackendObservationTime) return;
			if (result.state == ResultState::Running) result.state = ResultState::Completed;
			if (++backendIndex < int(backends.size())) start_backend();
			else { backendIndex = -1; std::string error; input.open("SDL3 DirectInput", error); status = "Backend comparison complete"; }
		}

		DeviceDiagnosticsReport::Report report() const
		{
			using namespace DeviceDiagnosticsReport;
			Report value{ utc_now(), {} };
			Section discovery{ "Device discovery", initialized ? Status::Completed : Status::Untested,
				initialized ? std::format("{} input and {} FFB-capable device(s) discovered.", input.devices.size(), ffb.entries.size()) : "Initialization was not run.", {} };
			for (size_t i=0;i<input.devices.size();++i) { const auto& d=input.devices[i]; discovery.details.push_back(std::format("{} via {}: {} axes, {} buttons, {} hats; activity {}", device_label(i), d.backend, d.axes, d.buttons, d.hats, i<deviceResponsive.size()&&deviceResponsive[i]?"responsive":"not observed")); }
			for (size_t i=0;i<ffb.entries.size();++i) { const auto& d=ffb.entries[i]; discovery.details.push_back(std::format("{}: native DirectInput FFB, {} POVs", ffb_label(i), d.povs)); }
			value.sections.push_back(std::move(discovery));
			Section backend{ "Input backend comparison", Status::Untested, "Backend comparison was not completed.", {} };
			if (std::all_of(backends.begin(), backends.end(), [](const auto& r) { return r.state == ResultState::Completed; })) { backend.status = Status::Completed; backend.summary = "All isolated backend sessions completed."; }
			else if (std::any_of(backends.begin(), backends.end(), [](const auto& r) { return r.state == ResultState::Failed; })) { backend.status = Status::Failed; backend.summary = "One or more backend sessions failed."; }
			for (const auto& b : backends)
			{
				backend.details.push_back(std::format("{}: requested {}, effective {}, started {}, {} device(s), {} delayed event(s){}",state_name(b.state),b.name,b.effectiveBackend.empty()?"untested":b.effectiveBackend,b.startedUtc.empty()?"not started":b.startedUtc,b.devices.size(),b.delayedEvents.size(),b.error.empty()?"":", "+b.error));
				for(const auto& d:b.devices)backend.details.push_back(std::format("{} device: {} [VID {:04X}, PID {:04X}], {} axes, {} buttons, {} hats, input {}",b.name,d.name,d.vendor,d.product,d.axes,d.buttons,d.hats,d.inputAvailable?"available":"unavailable"));
			}
			value.sections.push_back(std::move(backend));
			Section delayed{ "Delayed discovery", Status::Untested, "No completed delayed-discovery observation.", {} };
			for (const auto& b : backends) for (const auto& event : b.delayedEvents) delayed.details.push_back(b.name + ": " + event);
			if (std::all_of(backends.begin(), backends.end(), [](const auto& r) { return r.state == ResultState::Completed; })) { delayed.status = Status::Completed; delayed.summary = std::format("{} delayed connection event(s) observed.", delayed.details.size()); }
			value.sections.push_back(std::move(delayed));
			value.sections.push_back({ "Input testing", inputStatus == "completed" ? Status::Completed : Status::Untested, inputStatus == "completed" ? "Live input activity was observed." : "No live input activity was recorded.", {} });
			static const std::array<const char*,8> names{{"Steering","Accelerator","Brake","Shift Up","Shift Down","Start / Menu","Back Button","FFB Device"}};
			Section assignments{ "Multi-Input assignments", Status::Untested, "No diagnostic assignments were saved.", {} };
			for(size_t i=0;i<quick.saved.size();++i) assignments.details.push_back(std::string(names[i])+": "+(quick.saved[i]?quick.saved[i]->deviceName+" — "+quick.saved[i]->control:"Unassigned"));
			if(std::any_of(quick.saved.begin(),quick.saved.end(),[](const auto& b){return b.has_value();})){assignments.status=Status::Completed;assignments.summary="Diagnostic-only assignments span independently selected physical interfaces.";}
			value.sections.push_back(std::move(assignments));
			value.sections.push_back({ "Quick Setup", quickStatus == "completed" ? Status::Completed : Status::Untested, quickStatus == "completed" ? "Quick Setup completed." : "Quick Setup was not completed.", {} });
			Section caps{ "FFB device capabilities", ffb.entries.empty() ? Status::Unavailable : Status::Completed, ffb.entries.empty() ? "No native DirectInput FFB endpoint found." : std::format("{} native FFB endpoint(s) found.", ffb.entries.size()), {} };
			for (size_t i=0;i<ffb.entries.size();++i) { const auto& d=ffb.entries[i]; caps.details.push_back(std::format("{}: {}", ffb_label(i), d.effects.empty() ? "no reported effects" : std::format("{} reported effects", d.effects.size()))); }
			value.sections.push_back(std::move(caps));
			Section ffbTest{ "FFB test results", ffbStatus == "completed" ? Status::Completed : (ffbStatus=="initialization failed"?Status::Failed:Status::Untested), ffbStatus == "completed" ? "A bounded DirectInput request completed; physical torque was not measured." : "No physical FFB request completed.", { std::format("Selected device: {}",ffb.selected>=0?ffb_label(ffb.selected):"None"),std::format("Readiness: {}",ffbStatus),std::format("Output classification: {}",ffb_output_classification()),std::format("Invert FFB: {}",invertFfb?"On":"Off"),std::format("Requested strength: {}%; effective safety-limited magnitude: {} / {}",strength,ffb.lastRequestedMagnitude,DI_FFNOMINALMAX),std::format("Left test: {}; Right test: {}; Shake test: {} at {} Hz",leftTestStatus,rightTestStatus,shakeTestStatus,ShakeController::FrequencyHz),std::format("Last descriptor: {}",ffb.lastDescriptor),std::format("Last safety shutdown: {}",lastSafetyShutdown),std::format("Acquire: 0x{:08X}; Actuators: 0x{:08X}; Create: 0x{:08X}; Download: 0x{:08X}; Start: 0x{:08X}; Update: 0x{:08X}; Stop: 0x{:08X}",unsigned(ffb.lastAcquire),unsigned(ffb.lastActuators),unsigned(ffb.lastCreate),unsigned(ffb.lastDownload),unsigned(ffb.lastStart),unsigned(ffb.lastUpdate),unsigned(ffb.lastStop)) } };
			for(const auto& line:ffb.recoveryLog)ffbTest.details.push_back("Re-detect: "+line);value.sections.push_back(std::move(ffbTest));
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

		void devices_page()
		{
			ImGui::Text("Device Discovery"); help_marker(DeviceDiagnosticsHelp::DeviceDiscovery);
			if (!initialized)
			{
				ImGui::Spacing(); ImGui::Text("Ready to check your setup?");
				if(!initializing){if (ImGui::Button(initializationError.empty()?"Initialize Devices":"Retry Initialization", {220,44})) begin_initialization(); help_marker(DeviceDiagnosticsHelp::InitializeDevices);}
				else { ImGui::ProgressBar(initializationPhase/2.0f,{300,0},status.c_str()); if(ImGui::Button("Cancel")){initializing=false;status="Initialization cancelled";} }
				if(!initializationError.empty())ImGui::TextColored({1,0.45f,0.35f,1},"%s",initializationError.c_str());
			}
			else
			{
				ImGui::Text("Devices Discovered");
				ImGui::TextDisabled("Input Devices"); for (size_t i=0;i<input.devices.size();++i) { const auto& d=input.devices[i]; ImGui::BulletText("%s — %d axes, %d buttons, %d hats (%s)", device_label(i).c_str(), d.axes, d.buttons, d.hats, d.backend.c_str()); }
				ImGui::TextDisabled("FFB-Capable Devices"); for (size_t i=0;i<ffb.entries.size();++i) ImGui::BulletText("%s — %zu effects", ffb_label(i).c_str(), ffb.entries[i].effects.size());
				if (ImGui::Button("View Discovery Details")) showDetails = !showDetails; ImGui::SameLine();
				if (ImGui::Button("Rescan")) begin_initialization();
			}
			if (showDetails)
			{
				ImGui::Separator(); ImGui::Text("Backend Compatibility"); help_marker(DeviceDiagnosticsHelp::BackendCompatibility);
				if (ImGui::Button("Run All Backend Tests") && backendIndex < 0) run_all_backends();
				ImGui::SameLine(); ImGui::TextDisabled("Each backend observes for at least 10 seconds.");
				if (ImGui::BeginTable("backends", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
				{
					for (const char* h : {"Requested","Effective","Status","Devices","Delayed events"}) { ImGui::TableNextColumn(); ImGui::TextUnformatted(h); }
					for (const auto& b : backends) { ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::TextUnformatted(b.name.c_str());ImGui::TableNextColumn();ImGui::TextUnformatted(b.effectiveBackend.empty()?"—":b.effectiveBackend.c_str()); ImGui::TableNextColumn(); ImGui::TextUnformatted(state_name(b.state)); ImGui::TableNextColumn(); ImGui::Text("%zu", b.devices.size()); ImGui::TableNextColumn(); ImGui::Text("%zu", b.delayedEvents.size()); }
					ImGui::EndTable();
				}
				ImGui::Text("Delayed Discovery"); help_marker(DeviceDiagnosticsHelp::DelayedDiscovery);
			}
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
			static const std::array<const char*,8> names{{"Steering","Accelerator","Brake","Shift Up","Shift Down","Start / Menu","Back Button","FFB Device"}};
			ImGui::Text("Quick Setup"); help_marker(DeviceDiagnosticsHelp::QuickSetup);
			if(quickConfirmation)
			{
				ImGui::Spacing();ImGui::TextWrapped("Your devices are ready.\nWould you like to configure your controls?");
				if(ImGui::Button("Start Quick Setup",{190,42})){quickConfirmation=false;quick.start(Clock::now());begin_capture();}help_marker(DeviceDiagnosticsHelp::StartQuickSetup);
				ImGui::SameLine();if(ImGui::Button("Not Now",{130,42})){quickConfirmation=false;page=Page::Devices;}help_marker(DeviceDiagnosticsHelp::NotNow);
				ImGui::TextDisabled("Not Now leaves every saved diagnostic binding unchanged.");return;
			}
			if (!quick.active && quick.step >= int(quick.saved.size()))
			{
				ImGui::Text("Quick Setup Complete");
				bool essentialMissing = false;
				for (size_t i=0;i<quick.saved.size();++i)
				{
					const auto& value=quick.saved[i]; ImGui::Text("%s: %s",names[i],value?value->deviceName.c_str():"Unassigned");
					if (i<3 && !value) essentialMissing=true;
				}
				if(essentialMissing) ImGui::TextColored({1,0.6f,0.25f,1},"Steering, accelerator, or brake remains unassigned.");
				if(!quick.saved[7]) ImGui::TextColored({1,0.6f,0.25f,1},"Physical FFB testing remains disabled until a device is explicitly selected.");
				if(ImGui::Button("Open Input Test")) page=Page::InputTest; ImGui::SameLine(); if(ImGui::Button("Open FFB Test")) page=Page::FfbTest; ImGui::SameLine();
				if(ImGui::Button("Run Quick Setup Again")){quick.start(Clock::now());begin_capture();} ImGui::SameLine(); if(ImGui::Button("Home")) page=Page::Devices;
				return;
			}
			if (!quick.active) { if(ImGui::Button("Start Quick Setup")){quick.start(Clock::now());begin_capture();} return; }
			quick.update(Clock::now());
			ImGui::ProgressBar(quick.step/float(quick.saved.size()),{400,0});
			ImGui::Text("STEP %d OF %d — %s",quick.step+1,int(quick.saved.size()),names[quick.step]);
			ImGui::TextWrapped("Each binding may come from a different physical device. Existing saved bindings are preserved when a step is skipped.");
			if (quick.step == 7)
			{
				const char* preview=quick.candidate?quick.candidate->deviceName.c_str():"Select an FFB device";
				if(ImGui::BeginCombo("Native DirectInput FFB",preview)){for(int i=0;i<int(ffb.entries.size());++i)if(ImGui::Selectable(ffb_label(i).c_str()))quick.candidate=CapturedInput{ffb.entries[i].id,ffb_label(i),"Native DirectInput FFB"};ImGui::EndCombo();}
			}
			else if (!quick.candidate && !quick.timedOut)
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
				const int acceptedStep=quick.step; const auto accepted=quick.candidate;
				if(quick.continue_step(Clock::now()))
				{
					if(acceptedStep==7 && accepted){for(int index=0;index<int(ffb.entries.size());++index)if(ffb.entries[index].id==accepted->deviceId){ffb.select(index);break;}}
					if(!quick.active){quickStatus="completed";save_profile();}else begin_capture();
				}
			}
			if(!canContinue)ImGui::EndDisabled(); help_marker(DeviceDiagnosticsHelp::ContinueSetup); ImGui::SameLine();
			if(ImGui::Button("Retry")){quick.retry(Clock::now());begin_capture();} help_marker(DeviceDiagnosticsHelp::RetrySetup); ImGui::SameLine();
			if(ImGui::Button("Skip")){quick.skip(Clock::now());if(!quick.active){quickStatus="completed";save_profile();}else begin_capture();} help_marker(DeviceDiagnosticsHelp::SkipSetup); ImGui::SameLine();
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
				const auto result=ffb.redetect(GetModuleHandleW(nullptr),hwnd,previous);safety.stop();shake.stop();safety.authorized=false;lastSafetyShutdown="Re-detect disarmed output";
				if(ffb.connected()){remember_selected_ffb();ffbStatus="ready";}else ffbStatus=result.state==FfbResolution::SelectionRequired?"selection required":(ffb.entries.empty()?"not found":"initialization failed");
			} help_marker(DeviceDiagnosticsHelp::RedetectWheel);help_marker(DeviceDiagnosticsHelp::FfbReadiness);
			if(ffbStatus=="selection required"||(!ffb.connected()&&ffb.entries.size()>1))
			{
				if(ImGui::BeginCombo("Confirm FFB interface","Select a device")){for(int i=0;i<int(ffb.entries.size());++i)if(ImGui::Selectable(ffb_label(i).c_str())){ffb.stop();safety.stop();shake.stop();safety.authorized=false;lastSafetyShutdown="Device selection disarmed output";if(ffb.select(i)){remember_selected_ffb();ffbStatus="ready";}}ImGui::EndCombo();}
			}
			ImGui::Separator();ImGui::Text("Directional Test");help_marker(DeviceDiagnosticsHelp::DirectionTest);
			if(ImGui::Checkbox("Invert FFB",&invertFfb)){ffb.stop();safety.stop();shake.stop();lastSafetyShutdown="Inversion change stopped output";save_profile();}help_marker(DeviceDiagnosticsHelp::InvertFfbDiagnostic);
			ImGui::Text("Strength");ImGui::SliderInt("##strength",&strength,20,100,"%d%%"); help_marker(DeviceDiagnosticsHelp::Strength);
			if(strength>PhysicalOutputCeilingPercent)ImGui::TextColored({1,0.72f,0.25f,1},"Requested: %d%%  |  Output limited to %d%%",strength,PhysicalOutputCeilingPercent);help_marker(DeviceDiagnosticsHelp::SafetyLimitedOutput);
			ImGui::Checkbox("I understand this will move the selected wheel",&safety.authorized);
			const bool canRun=safety.authorized&&ffb.connected();if(!canRun)ImGui::BeginDisabled();
			ImGui::Button("< Test Left",{170,42});const bool leftHeld=ImGui::IsItemActive();help_marker(DeviceDiagnosticsHelp::LeftFfb);ImGui::SameLine();
			ImGui::Button("Test Right >",{170,42});const bool rightHeld=ImGui::IsItemActive();help_marker(DeviceDiagnosticsHelp::RightFfb);
			ImGui::Separator();ImGui::Text("Shake Test");
			ImGui::Button("Hold to Shake",{190,42}); const bool shakeHeld=ImGui::IsItemActive();help_marker(DeviceDiagnosticsHelp::ShakeFfb);
			if(!canRun) ImGui::EndDisabled(); ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Button,{0.65f,0.08f,0.08f,1}); if(ImGui::Button("STOP",{110,42})){ffb.stop();safety.stop();shake.stop();lastSafetyShutdown="Emergency STOP";} ImGui::PopStyleColor(); help_marker(DeviceDiagnosticsHelp::StopFfb);
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
			ImGui::Text("Physical request: %s | requested %ld / %d | safety ceiling %d%%",safety.running?"ACTIVE":"stopped",ffb.lastRequestedMagnitude,DI_FFNOMINALMAX,PhysicalOutputCeilingPercent);
			ImGui::TextDisabled("Descriptor: %s | Create 0x%08X | Download 0x%08X | Start 0x%08X",ffb.lastDescriptor.c_str(),unsigned(ffb.lastCreate),unsigned(ffb.lastDownload),unsigned(ffb.lastStart));
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
				SDL_Event event{}; while(SDL_PollEvent(&event)){ImGui_ImplSDL3_ProcessEvent(&event);if(backendIndex>=0&&event.type==SDL_EVENT_JOYSTICK_ADDED)backends[backendIndex].delayedEvents.push_back(std::format("{}: SDL device {} connected",utc_now(),event.jdevice.which));if(event.type==SDL_EVENT_QUIT||event.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED)running=false;if(event.type==SDL_EVENT_WINDOW_FOCUS_LOST){ffb.stop();safety.stop();shake.stop();lastSafetyShutdown="Focus lost";}}
				update_initialization(); update_backend(); if(backendIndex<0) input.pump(); quick.update(Clock::now()); capture_quick_setup_input();
				ImGui_ImplSDLRenderer3_NewFrame();ImGui_ImplSDL3_NewFrame();ImGui::NewFrame();draw();ImGui::Render();
				SDL_SetRenderDrawColor(renderer,10,17,29,255);SDL_RenderClear(renderer);ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(),renderer);SDL_RenderPresent(renderer);
			}
			ffb.stop(); safety.stop(); return 0;
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
