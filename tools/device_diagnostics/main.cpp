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
					SDL_GetNumJoystickAxes(joystick), SDL_GetNumJoystickButtons(joystick), SDL_GetNumJoystickHats(joystick), true, false });
			}
			SDL_free(ids);
			return true;
		}

		void pump(std::vector<std::string>* delayed = nullptr)
		{
			SDL_UpdateJoysticks();
			SDL_Event event{};
			while (SDL_PollEvent(&event))
			{
				ImGui_ImplSDL3_ProcessEvent(&event);
				if (delayed && event.type == SDL_EVENT_JOYSTICK_ADDED)
					delayed->push_back(std::format("{}: device {} connected", utc_now(), event.jdevice.which));
			}
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

	enum class Page { Devices, InputTest, MultiInput, QuickSetup, FfbTest };

	struct App
	{
		SDL_Window* window = nullptr;
		SDL_Renderer* renderer = nullptr;
		SDLDeviceSession input;
		NativeFFB ffb;
		SafetyController safety;
		Page page = Page::Devices;
		bool initialized = false, showDetails = false, running = true;
		std::string status = "Ready to check your setup?";
		std::array<BackendResult, 4> backends{{ {"Windows.Gaming.Input"}, {"SDL3 RawInput"}, {"SDL3 DirectInput"}, {"SDL3 XInput"} }};
		int backendIndex = -1;
		Clock::time_point backendStarted{};
		Assignment assignment;
		int selectedInput = 0, quickStep = 0, strength = 20, effectIndex = 0;
		std::string quickStatus = "untested", inputStatus = "untested", ffbStatus = "untested", reportStatus;
		std::filesystem::path appDirectory = documents_path() / "HYP36rforce Device Diagnostics";

		std::filesystem::path profile_path() const { return appDirectory / "diagnostic-profile.txt"; }
		void save_profile()
		{
			std::filesystem::create_directories(appDirectory);
			std::ofstream out(profile_path());
			out << assignment.steering << '\n' << assignment.pedals << '\n' << assignment.shifter << '\n' << assignment.additional << '\n' << assignment.ffb << '\n';
		}
		void load_profile()
		{
			std::ifstream in(profile_path());
			std::getline(in, assignment.steering); std::getline(in, assignment.pedals); std::getline(in, assignment.shifter);
			std::getline(in, assignment.additional); std::getline(in, assignment.ffb);
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

		void initialize_devices()
		{
			status = "Discovering Windows input devices...";
			std::string error;
			initialized = input.open("SDL3 DirectInput", error);
			HWND hwnd = static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
			ffb.initialize(GetModuleHandleW(nullptr), hwnd);
			status = initialized ? std::format("Devices Discovered: {} input, {} FFB-capable", input.devices.size(), ffb.entries.size()) : error;
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
			for (const auto& d : input.devices) discovery.details.push_back(std::format("{} via {}: {} axes, {} buttons, {} hats", d.name, d.backend, d.axes, d.buttons, d.hats));
			for (const auto& d : ffb.entries) discovery.details.push_back(std::format("{}: native DirectInput FFB, {} axes, {} buttons, {} POVs", d.name, d.axes, d.buttons, d.povs));
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
			value.sections.push_back({ "Multi-Input assignments", assignment.steering.empty() ? Status::Untested : Status::Completed, assignment.steering.empty() ? "No diagnostic assignments were saved." : "Diagnostic-only assignments were saved.", { "Steering: " + assignment.steering, "Pedals: " + assignment.pedals, "Shifter: " + assignment.shifter, "Additional: " + assignment.additional } });
			value.sections.push_back({ "Quick Setup", quickStatus == "completed" ? Status::Completed : Status::Untested, quickStatus == "completed" ? "Quick Setup completed." : "Quick Setup was not completed.", {} });
			Section caps{ "FFB device capabilities", ffb.entries.empty() ? Status::Unavailable : Status::Completed, ffb.entries.empty() ? "No native DirectInput FFB endpoint found." : std::format("{} native FFB endpoint(s) found.", ffb.entries.size()), {} };
			for (const auto& d : ffb.entries) caps.details.push_back(std::format("{}: {}", d.name, d.effects.empty() ? "no reported effects" : std::format("{} reported effects", d.effects.size())));
			value.sections.push_back(std::move(caps));
			value.sections.push_back({ "FFB test results", ffbStatus == "completed" ? Status::Completed : Status::Untested, ffbStatus == "completed" ? "A bounded DirectInput request completed; physical torque was not measured." : "No physical FFB request completed.", { std::format("Last requested magnitude: {} / {}", ffb.lastRequestedMagnitude, DI_FFNOMINALMAX), std::format("Create HRESULT: 0x{:08X}; Start: 0x{:08X}; Stop: 0x{:08X}", unsigned(ffb.lastCreate), unsigned(ffb.lastStart), unsigned(ffb.lastStop)) } });
		value.sections.push_back({ "API errors", ffb.errors.empty() ? Status::Completed : Status::Failed, ffb.errors.empty() ? "No retained DirectInput errors." : std::format("{} DirectInput error(s) retained.", ffb.errors.size()), ffb.errors });
		return value;
		}

		void export_report()
		{
			const auto result = DeviceDiagnosticsReport::write(appDirectory / "Reports", report());
			reportStatus = result.success ? "Saved " + result.textPath.filename().string() + " and " + result.jsonPath.filename().string() : result.error;
		}

		void header()
		{
			ImGui::TextColored({0.40f,0.68f,1.0f,1}, "HYP36rforce Device Diagnostics"); ImGui::SameLine(); ImGui::TextDisabled("v%s", Version);
			ImGui::TextDisabled("Input  |  Multi-Input  |  Force Feedback"); ImGui::Separator();
			const std::array<std::pair<const char*, Page>, 5> pages{{ {"Devices",Page::Devices},{"Input Test",Page::InputTest},{"Multi-Input",Page::MultiInput},{"Quick Setup",Page::QuickSetup},{"FFB Test",Page::FfbTest} }};
			for (const auto& [label, value] : pages) { if (page == value) ImGui::PushStyleColor(ImGuiCol_Button, {0.15f,0.38f,0.65f,1}); if (ImGui::Button(label)) page = value; if (page == value) ImGui::PopStyleColor(); ImGui::SameLine(); }
			ImGui::NewLine(); ImGui::Separator();
		}

		void combo_assignment(const char* label, std::string& target)
		{
			if (ImGui::BeginCombo(label, target.empty() ? "Not assigned" : target.c_str()))
			{
				if (ImGui::Selectable("Not assigned")) target.clear();
				for (const auto& d : input.devices) if (ImGui::Selectable(d.name.c_str())) target = d.name;
				ImGui::EndCombo();
			}
		}

		void devices_page()
		{
			ImGui::Text("Device Discovery"); help_marker(DeviceDiagnosticsHelp::DeviceDiscovery);
			if (!initialized)
			{
				ImGui::Spacing(); ImGui::Text("Ready to check your setup?");
				if (ImGui::Button("Initialize Devices", {220,44})) initialize_devices(); help_marker(DeviceDiagnosticsHelp::InitializeDevices);
			}
			else
			{
				ImGui::Text("Devices Discovered");
				ImGui::TextDisabled("Input Devices"); for (const auto& d : input.devices) ImGui::BulletText("%s — %d axes, %d buttons, %d hats (%s)", d.name.c_str(), d.axes, d.buttons, d.hats, d.backend.c_str());
				ImGui::TextDisabled("FFB-Capable Devices"); for (const auto& d : ffb.entries) ImGui::BulletText("%s — native DirectInput, %u axes, %zu effects", d.name.c_str(), d.axes, d.effects.size());
				if (ImGui::Button("Start Quick Setup")) page = Page::QuickSetup; ImGui::SameLine();
				if (ImGui::Button("View Discovery Details")) showDetails = !showDetails; ImGui::SameLine();
				if (ImGui::Button("Rescan")) initialize_devices();
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
			if (ImGui::BeginCombo("Device", input.devices[selectedInput].name.c_str())) { for (int i=0;i<int(input.devices.size());++i) if (ImGui::Selectable(input.devices[i].name.c_str(), i==selectedInput)) selectedInput=i; ImGui::EndCombo(); }
			auto* joystick = input.handles[selectedInput]; bool active = false;
			for (int i=0;i<SDL_GetNumJoystickAxes(joystick);++i) { const int raw=SDL_GetJoystickAxis(joystick,i); const float norm = raw < 0 ? raw/32768.0f : raw/32767.0f; ImGui::Text("Axis %d   raw %6d   normalized % .3f",i,raw,norm); ImGui::ProgressBar((norm+1)*0.5f,{350,0}); active |= std::abs(raw)>2500; }
			for (int i=0;i<SDL_GetNumJoystickButtons(joystick);++i) { if (i%12) ImGui::SameLine(); const bool down=SDL_GetJoystickButton(joystick,i); ImGui::TextColored(down?ImVec4{0.3f,1,0.5f,1}:ImVec4{0.6f,0.6f,0.6f,1},"B%d",i); active |= down; }
			for (int i=0;i<SDL_GetNumJoystickHats(joystick);++i) { const auto value=SDL_GetJoystickHat(joystick,i); ImGui::Text("POV %d: 0x%02X",i,value); active |= value != SDL_HAT_CENTERED; }
			if (active) inputStatus = "completed";
		}

		void multi_page()
		{
			ImGui::Text("Multi-Input"); help_marker(DeviceDiagnosticsHelp::MultiInput);
			ImGui::TextWrapped("Assign independent USB devices to one diagnostic control set. This profile never overwrites OutRun bindings.");
			combo_assignment("Steering", assignment.steering); combo_assignment("Pedals", assignment.pedals); combo_assignment("Shifter", assignment.shifter); combo_assignment("Additional controls", assignment.additional);
			if (ImGui::Button("Save Diagnostic Profile")) { save_profile(); status="Diagnostic profile saved"; }
			ImGui::SameLine(); if (ImGui::Button("Reload")) { load_profile(); status="Diagnostic profile reloaded"; }
		}

		void quick_page()
		{
			ImGui::Text("Quick Setup"); help_marker(DeviceDiagnosticsHelp::QuickSetup); ImGui::ProgressBar(quickStep/5.0f,{400,0});
			const char* titles[]{"Choose Your Steering Wheel","Calibrate Steering","Configure Pedals","Assign Shifting","Select FFB Device","Quick Setup Complete"}; ImGui::Text("STEP %d: %s",std::min(quickStep+1,5),titles[quickStep]);
			if (quickStep==0) combo_assignment("Steering device",assignment.steering);
			else if (quickStep==1) ImGui::TextWrapped("Turn the selected steering control fully left, fully right, then return to center. Use Input Test to inspect raw and normalized values.");
			else if (quickStep==2) combo_assignment("Pedal device",assignment.pedals);
			else if (quickStep==3) combo_assignment("Shifter",assignment.shifter);
			else if (quickStep==4) { if (ImGui::BeginCombo("FFB device",assignment.ffb.empty()?"Not selected":assignment.ffb.c_str())) { for (int i=0;i<int(ffb.entries.size());++i) if(ImGui::Selectable(ffb.entries[i].name.c_str())) { assignment.ffb=ffb.entries[i].name; ffb.select(i); } ImGui::EndCombo(); } }
			else { ImGui::Text("Steering: %s",assignment.steering.c_str()); ImGui::Text("Pedals: %s",assignment.pedals.c_str()); ImGui::Text("Shifter: %s",assignment.shifter.c_str()); ImGui::Text("FFB: %s",assignment.ffb.c_str()); if(ImGui::Button("Open FFB Test")) page=Page::FfbTest; ImGui::SameLine(); if(ImGui::Button("Run Quick Setup Again")) quickStep=0; ImGui::SameLine(); if(ImGui::Button("Home")) page=Page::Devices; }
			if (quickStep<5) { if (quickStep>0 && ImGui::Button("Back")) --quickStep; if (quickStep>0) ImGui::SameLine(); if(ImGui::Button("Continue")) { ++quickStep; if(quickStep==5){quickStatus="completed";save_profile();} } ImGui::SameLine(); if(ImGui::Button("Exit Setup")){quickStep=0;page=Page::Devices;} }
		}

		void ffb_page()
		{
			ImGui::Text("FFB Test"); help_marker(DeviceDiagnosticsHelp::FfbTest);
			ImGui::TextColored({1,0.75f,0.25f,1},"Motor output is disabled until you select a device and explicitly authorize it.");
			if (ImGui::BeginCombo("Native DirectInput FFB device", ffb.selected>=0?ffb.entries[ffb.selected].name.c_str():"Select a device")) { for(int i=0;i<int(ffb.entries.size());++i) if(ImGui::Selectable(ffb.entries[i].name.c_str(),i==ffb.selected)){ffb.select(i);assignment.ffb=ffb.entries[i].name;} ImGui::EndCombo(); }
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
			header(); ImGui::BeginChild("content",{0,-76},true); switch(page){case Page::Devices:devices_page();break;case Page::InputTest:input_page();break;case Page::MultiInput:multi_page();break;case Page::QuickSetup:quick_page();break;case Page::FfbTest:ffb_page();break;} ImGui::EndChild();
			ImGui::TextDisabled("%s",status.c_str());
			ImGui::SameLine(ImGui::GetWindowWidth()-260); if(ImGui::Button("Export Report")){export_report();} help_marker(DeviceDiagnosticsHelp::ExportReport);
			if(!reportStatus.empty()){ImGui::SameLine();ImGui::TextDisabled("%s",reportStatus.c_str());}
			ImGui::End();
		}

		int loop()
		{
			while(running)
			{
				SDL_Event event{}; while(SDL_PollEvent(&event)){ImGui_ImplSDL3_ProcessEvent(&event);if(event.type==SDL_EVENT_QUIT||event.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED)running=false;if(event.type==SDL_EVENT_WINDOW_FOCUS_LOST){ffb.stop();safety.stop();}}
				update_backend(); if(backendIndex<0) input.pump();
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
