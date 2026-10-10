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
		struct Entry { GUID guid{}; std::string name; DWORD axes = 0, buttons = 0, povs = 0; std::vector<std::string> effects; };
		IDirectInput8W* api = nullptr;
		IDirectInputDevice8W* device = nullptr;
		IDirectInputEffect* effect = nullptr;
		HWND window = nullptr;
		std::vector<Entry> entries;
		std::vector<std::string> errors;
		int selected = -1;
		LONG lastRequestedMagnitude = 0;
		HRESULT lastCreate = S_FALSE, lastStart = S_FALSE, lastStop = S_FALSE;

		static std::string utf8(const wchar_t* value)
		{
			const int size = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
			std::string text(size > 0 ? size : 0, '\0');
			if (size > 1) { WideCharToMultiByte(CP_UTF8, 0, value, -1, text.data(), size, nullptr, nullptr); text.pop_back(); }
			return text;
		}

		static BOOL CALLBACK enum_effect(const DIEFFECTINFOW* info, void* context)
		{
			static_cast<Entry*>(context)->effects.push_back(utf8(info->tszName)); return DIENUM_CONTINUE;
		}
		static BOOL CALLBACK enum_device(const DIDEVICEINSTANCEW* instance, void* context)
		{
			auto& self = *static_cast<NativeFFB*>(context);
			IDirectInputDevice8W* candidate = nullptr;
			if (FAILED(self.api->CreateDevice(instance->guidInstance, &candidate, nullptr))) return DIENUM_CONTINUE;
			DIDEVCAPS caps{ sizeof(caps) };
			if (SUCCEEDED(candidate->GetCapabilities(&caps)) && (caps.dwFlags & DIDC_FORCEFEEDBACK))
			{
				Entry entry{ instance->guidInstance, utf8(instance->tszProductName), caps.dwAxes, caps.dwButtons, caps.dwPOVs };
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
			const HRESULT result = api->EnumDevices(DI8DEVCLASS_GAMECTRL, enum_device, this, DIEDFL_ATTACHEDONLY);
			if (FAILED(result)) errors.push_back(std::format("EnumDevices failed: 0x{:08X}", unsigned(result)));
			return SUCCEEDED(result);
		}

		void stop()
		{
			if (effect) { lastStop = effect->Stop(); effect->Release(); effect = nullptr; }
			if (device) device->SendForceFeedbackCommand(DISFFC_STOPALL);
		}

		void select(int index)
		{
			stop();
			if (device) { device->Unacquire(); device->Release(); device = nullptr; }
			selected = -1;
			if (index < 0 || index >= int(entries.size())) return;
			HRESULT result = api->CreateDevice(entries[index].guid, &device, nullptr);
			if (FAILED(result)) { errors.push_back(std::format("CreateDevice failed: 0x{:08X}", unsigned(result))); return; }
			device->SetDataFormat(&c_dfDIJoystick2);
			result = device->SetCooperativeLevel(window, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
			if (FAILED(result)) { errors.push_back(std::format("SetCooperativeLevel failed: 0x{:08X}", unsigned(result))); device->Release(); device = nullptr; return; }
			DIPROPDWORD gain{}; gain.diph.dwSize = sizeof(gain); gain.diph.dwHeaderSize = sizeof(gain.diph); gain.diph.dwHow = DIPH_DEVICE; gain.dwData = DI_FFNOMINALMAX;
			device->SetProperty(DIPROP_FFGAIN, &gain.diph);
			result = device->Acquire();
			if (FAILED(result)) { errors.push_back(std::format("Acquire failed: 0x{:08X}", unsigned(result))); return; }
			selected = index;
		}

		bool run_constant(int requestedPercent, SafetyController& safety)
		{
			if (!device) return false;
			stop();
			lastRequestedMagnitude = safety.bounded_magnitude(requestedPercent);
			DWORD axis = DIJOFS_X; LONG direction = 9000;
			DICONSTANTFORCE constant{ lastRequestedMagnitude };
			DIEFFECT desc{}; desc.dwSize = sizeof(desc); desc.dwFlags = DIEFF_POLAR | DIEFF_OBJECTOFFSETS;
			desc.dwDuration = DWORD(MaximumRunTime.count() * 1000); desc.dwGain = DI_FFNOMINALMAX; desc.dwTriggerButton = DIEB_NOTRIGGER;
			desc.cAxes = 1; desc.rgdwAxes = &axis; desc.rglDirection = &direction;
			desc.cbTypeSpecificParams = sizeof(constant); desc.lpvTypeSpecificParams = &constant;
			lastCreate = device->CreateEffect(GUID_ConstantForce, &desc, &effect, nullptr);
			if (FAILED(lastCreate)) { errors.push_back(std::format("CreateEffect failed: 0x{:08X}", unsigned(lastCreate))); return false; }
			lastStart = effect->Start(1, 0);
			if (FAILED(lastStart)) { errors.push_back(std::format("Effect Start failed: 0x{:08X}", unsigned(lastStart))); stop(); return false; }
			return true;
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
		Page page = Page::Devices;
		bool initialized = false, showDetails = false, running = true;
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
		int selectedInput = 0, strength = 20, effectIndex = 0;
		std::string quickStatus = "untested", inputStatus = "untested", ffbStatus = "untested", reportStatus;
		std::filesystem::path appDirectory = documents_path() / "HYP36rforce Device Diagnostics";
		std::filesystem::path lastExportDirectory;

		std::filesystem::path profile_path() const { return appDirectory / "diagnostic-profile.txt"; }
		void save_profile()
		{
			std::filesystem::create_directories(appDirectory);
			std::ofstream out(profile_path());
			for (const auto& binding : quick.saved)
				out << (binding ? binding->deviceId + "|" + binding->deviceName + "|" + binding->control : "") << '\n';
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
			page=Page::QuickSetup; quick.start(Clock::now()); begin_capture();
		}

		void capture_quick_setup_input()
		{
			if (!quick.active || quick.step == 5 || quick.candidate || quick.timedOut) return;
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
			else result.devices = input.devices;
			backendStarted = Clock::now();
		}
		void update_backend()
		{
			if (backendIndex < 0) return;
			auto& result = backends[backendIndex]; input.pump(&result.delayedEvents);
			if (Clock::now() - backendStarted < std::chrono::seconds(10)) return;
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
			for (const auto& b : backends) backend.details.push_back(std::format("{}: {}, {} device(s), {} delayed event(s){}", b.name, state_name(b.state), b.devices.size(), b.delayedEvents.size(), b.error.empty() ? "" : ", " + b.error));
			value.sections.push_back(std::move(backend));
			Section delayed{ "Delayed discovery", Status::Untested, "No completed delayed-discovery observation.", {} };
			for (const auto& b : backends) for (const auto& event : b.delayedEvents) delayed.details.push_back(b.name + ": " + event);
			if (std::all_of(backends.begin(), backends.end(), [](const auto& r) { return r.state == ResultState::Completed; })) { delayed.status = Status::Completed; delayed.summary = std::format("{} delayed connection event(s) observed.", delayed.details.size()); }
			value.sections.push_back(std::move(delayed));
			value.sections.push_back({ "Input testing", inputStatus == "completed" ? Status::Completed : Status::Untested, inputStatus == "completed" ? "Live input activity was observed." : "No live input activity was recorded.", {} });
			static const std::array<const char*,6> names{{"Steering","Accelerator","Brake","Shifting","Additional controls","FFB device"}};
			Section assignments{ "Multi-Input assignments", Status::Untested, "No diagnostic assignments were saved.", {} };
			for(size_t i=0;i<quick.saved.size();++i) assignments.details.push_back(std::string(names[i])+": "+(quick.saved[i]?quick.saved[i]->deviceName+" — "+quick.saved[i]->control:"Unassigned"));
			if(std::any_of(quick.saved.begin(),quick.saved.end(),[](const auto& b){return b.has_value();})){assignments.status=Status::Completed;assignments.summary="Diagnostic-only assignments span independently selected physical interfaces.";}
			value.sections.push_back(std::move(assignments));
			value.sections.push_back({ "Quick Setup", quickStatus == "completed" ? Status::Completed : Status::Untested, quickStatus == "completed" ? "Quick Setup completed." : "Quick Setup was not completed.", {} });
			Section caps{ "FFB device capabilities", ffb.entries.empty() ? Status::Unavailable : Status::Completed, ffb.entries.empty() ? "No native DirectInput FFB endpoint found." : std::format("{} native FFB endpoint(s) found.", ffb.entries.size()), {} };
			for (size_t i=0;i<ffb.entries.size();++i) { const auto& d=ffb.entries[i]; caps.details.push_back(std::format("{}: {}", ffb_label(i), d.effects.empty() ? "no reported effects" : std::format("{} reported effects", d.effects.size()))); }
			value.sections.push_back(std::move(caps));
			value.sections.push_back({ "FFB test results", ffbStatus == "completed" ? Status::Completed : Status::Untested, ffbStatus == "completed" ? "A bounded DirectInput request completed; physical torque was not measured." : "No physical FFB request completed.", { std::format("Last requested magnitude: {} / {}", ffb.lastRequestedMagnitude, DI_FFNOMINALMAX), std::format("Create HRESULT: 0x{:08X}; Start: 0x{:08X}; Stop: 0x{:08X}", unsigned(ffb.lastCreate), unsigned(ffb.lastStart), unsigned(ffb.lastStop)) } });
		value.sections.push_back({ "API errors", ffb.errors.empty() ? Status::Completed : Status::Failed, ffb.errors.empty() ? "No retained DirectInput errors." : std::format("{} DirectInput error(s) retained.", ffb.errors.size()), ffb.errors });
			value.sections.push_back({ "Application environment", Status::Completed, "Standalone diagnostic application metadata.", { std::string("Application version: ")+Version, std::format("SDL runtime version: {}",SDL_GetVersion()), "Windows platform: Win32", "Report timestamps include UTC evidence; filenames use local system time." } });
		return value;
		}

		void export_report()
		{
			lastExportDirectory=appDirectory/"Exports";
			std::string identity="No Wheel Detected";
			if(ffb.selected>=0&&ffb.selected<int(ffb.entries.size())) identity=ffb.entries[ffb.selected].name;
			else if(quick.saved[0]) identity=quick.saved[0]->deviceName;
			const std::string stem=sanitize_filename_component(identity)+"_"+local_filename_time();
			const auto result = DeviceDiagnosticsReport::write_named(lastExportDirectory, report(), stem);
			reportStatus = result.success ? "Saved " + result.textPath.filename().string() + " and " + result.jsonPath.filename().string() : result.error;
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
				if (ImGui::BeginTable("backends", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
				{
					for (const char* h : {"Backend","Status","Devices","Delayed events"}) { ImGui::TableNextColumn(); ImGui::TextUnformatted(h); }
					for (const auto& b : backends) { ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::TextUnformatted(b.name.c_str()); ImGui::TableNextColumn(); ImGui::TextUnformatted(state_name(b.state)); ImGui::TableNextColumn(); ImGui::Text("%zu", b.devices.size()); ImGui::TableNextColumn(); ImGui::Text("%zu", b.delayedEvents.size()); }
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
			static const std::array<const char*,6> names{{"Steering","Accelerator","Brake","Shifting","Additional controls","FFB device"}};
			ImGui::Text("Quick Setup"); help_marker(DeviceDiagnosticsHelp::QuickSetup);
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
				if(!quick.saved[5]) ImGui::TextColored({1,0.6f,0.25f,1},"Physical FFB testing remains disabled until a device is explicitly selected.");
				if(ImGui::Button("Open Input Test")) page=Page::InputTest; ImGui::SameLine(); if(ImGui::Button("Open FFB Test")) page=Page::FfbTest; ImGui::SameLine();
				if(ImGui::Button("Run Quick Setup Again")){quick.start(Clock::now());begin_capture();} ImGui::SameLine(); if(ImGui::Button("Home")) page=Page::Devices;
				return;
			}
			if (!quick.active) { if(ImGui::Button("Start Quick Setup")){quick.start(Clock::now());begin_capture();} return; }
			quick.update(Clock::now());
			ImGui::ProgressBar(quick.step/float(quick.saved.size()),{400,0});
			ImGui::Text("STEP %d OF %d — %s",quick.step+1,int(quick.saved.size()),names[quick.step]);
			ImGui::TextWrapped("Each binding may come from a different physical device. Existing saved bindings are preserved when a step is skipped.");
			if (quick.step == 5)
			{
				const char* preview=quick.candidate?quick.candidate->deviceName.c_str():"Select an FFB device";
				if(ImGui::BeginCombo("Native DirectInput FFB",preview)){for(int i=0;i<int(ffb.entries.size());++i)if(ImGui::Selectable(ffb_label(i).c_str()))quick.candidate=CapturedInput{std::format("native-ffb-{}",i),ffb_label(i),"Native DirectInput FFB"};ImGui::EndCombo();}
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
					if(acceptedStep==5 && accepted){const int index=std::atoi(accepted->deviceId.substr(11).c_str());ffb.select(index);}
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
			if (ImGui::BeginCombo("Native DirectInput FFB device", ffb.selected>=0?ffb_label(ffb.selected).c_str():"Select a device")) { for(int i=0;i<int(ffb.entries.size());++i) if(ImGui::Selectable(ffb_label(i).c_str(),i==ffb.selected)){ffb.select(i);quick.saved[5]=CapturedInput{std::format("native-ffb-{}",i),ffb_label(i),"Native DirectInput FFB"};} ImGui::EndCombo(); }
			static const std::array<const char*,15> effects{{"Left Force","Right Force","Centering Spring","Steering Load","Damper","Road Detail","Surface Sine","Surface Triangle","Surface Square","FFB Shake","Bump / Kerb","Impact","Grip Loss","Combined Effects","Capability-only check"}};
			ImGui::Combo("Effect Selector",&effectIndex,effects.data(),int(effects.size()));
			ImGui::SliderInt("Strength",&strength,20,100,"%d%%"); help_marker(DeviceDiagnosticsHelp::Strength);
			static int frequency=30,duration=500,direction=0; ImGui::SliderInt("Frequency",&frequency,1,60,"%d Hz"); ImGui::SliderInt("Duration",&duration,100,1500,"%d ms"); ImGui::SliderInt("Direction",&direction,-100,100);
			ImGui::TextDisabled("Selected effect: %s. Native support is reported by the device; game-derived effects are labeled synthetic approximations.", effects[effectIndex]);
			ImGui::Checkbox("I understand this will move the selected wheel",&safety.authorized);
			const bool canRun = safety.authorized && ffb.connected(); if(!canRun) ImGui::BeginDisabled();
			ImGui::Button("Hold to Run Test",{190,42}); const bool held=ImGui::IsItemActive();
			if(!canRun) ImGui::EndDisabled(); ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Button,{0.65f,0.08f,0.08f,1}); if(ImGui::Button("STOP",{110,42})){ffb.stop();safety.stop();} ImGui::PopStyleColor(); help_marker(DeviceDiagnosticsHelp::FfbTest);
			const bool focused=(SDL_GetWindowFlags(window)&SDL_WINDOW_INPUT_FOCUS)!=0; const auto now=Clock::now();
			if(held && !safety.running && safety.begin(ffb.connected(),focused,now)) { if(ffb.run_constant(strength,safety)) ffbStatus="completed"; else safety.stop(); }
			if(held && safety.running) safety.beat(now);
			if(safety.must_stop(held,focused,ffb.connected(),now)){ffb.stop();safety.stop();}
			ImGui::Text("Physical request: %s | requested %ld / %d | safety ceiling %d%%",safety.running?"ACTIVE":"stopped",ffb.lastRequestedMagnitude,DI_FFNOMINALMAX,PhysicalOutputCeilingPercent);
		}

		void draw()
		{
			ImGui::SetNextWindowPos({0,0}); ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
			ImGui::Begin("HYP36rforce Device Diagnostics",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoSavedSettings);
			header(); ImGui::BeginChild("content",{0,-76},true); switch(page){case Page::Devices:devices_page();break;case Page::InputTest:input_page();break;case Page::QuickSetup:quick_page();break;case Page::FfbTest:ffb_page();break;} ImGui::EndChild();
			ImGui::TextDisabled("%s",status.c_str());
			ImGui::SameLine(ImGui::GetWindowWidth()-260); if(ImGui::Button("Export Report")){export_report();} help_marker(DeviceDiagnosticsHelp::ExportReport);
			if(!reportStatus.empty()){ImGui::SameLine();ImGui::TextDisabled("%s",reportStatus.c_str());if(!lastExportDirectory.empty()){ImGui::SameLine();if(ImGui::Button("Open Exports Folder"))ShellExecuteW(nullptr,L"open",lastExportDirectory.c_str(),nullptr,nullptr,SW_SHOWNORMAL);}}
			ImGui::End();
		}

		int loop()
		{
			while(running)
			{
				SDL_Event event{}; while(SDL_PollEvent(&event)){ImGui_ImplSDL3_ProcessEvent(&event);if(backendIndex>=0&&event.type==SDL_EVENT_JOYSTICK_ADDED)backends[backendIndex].delayedEvents.push_back(std::format("{}: SDL device {} connected",utc_now(),event.jdevice.which));if(event.type==SDL_EVENT_QUIT||event.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED)running=false;if(event.type==SDL_EVENT_WINDOW_FOCUS_LOST){ffb.stop();safety.stop();}}
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
