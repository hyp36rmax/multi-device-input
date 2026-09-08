#include "input_manager.hpp"
#include "wheel_force_feedback.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>

//
// Binding editor.
//
// An action holds a list of bindings and fires on whichever of them reads
// highest, so several inputs per action can be assigned.
// This screen displays each action on the left, and selecting one will
// display and allow changing the bindings for it on the right.
//
class InputBindingsUI : public OverlayWindow
{
public:
	// A modal that in-game code can raise on its own, so it has to be drawn
	// whether or not the overlay is open.
	Kind kind() const override { return Kind::Hud; }
	const char* name() const override { return "Input Bindings"; }

private:
	// One action, either an analog channel or a digital switch. Held by value
	// rather than by pointer because the prompt outlives a frame, and
	// setupDefaultBindings()/readBindingIni() replace the binding vectors.
	struct Selection
	{
		using Kind = InputManager::ActionKind;

		Kind kind = Kind::Volume;
		int index = 0;

		bool operator==(const Selection& other) const
		{
			return kind == other.kind && index == other.index;
		}

		bool isVolume() const { return kind == Kind::Volume; }
	};

	// The order actions are listed in, grouped by when they get used.
	struct ActionListEntry
	{
		const char* group;
		Selection::Kind kind;
		int index;
	};

	static constexpr auto Vol = Selection::Kind::Volume;
	static constexpr auto Sw = Selection::Kind::Switch;
	static constexpr auto Mod = Selection::Kind::Mod;

	static inline const ActionListEntry ActionList[] = {
		{ "Driving", Vol, int(ADChannel::Steering)        },
		{ "Driving", Vol, int(ADChannel::Acceleration)    },
		{ "Driving", Vol, int(ADChannel::Brake)           },
		{ "Driving", Sw,  int(SwitchId::GearUp)           },
		{ "Driving", Sw,  int(SwitchId::GearDown)         },
		{ "Driving", Sw,  int(SwitchId::ChangeView)       },

		{ "Menus",   Sw,  int(SwitchId::Start)            },
		{ "Menus",   Sw,  int(SwitchId::Back)             },
		{ "Menus",   Sw,  int(SwitchId::A)                },
		{ "Menus",   Sw,  int(SwitchId::B)                },
		{ "Menus",   Sw,  int(SwitchId::X)                },
		{ "Menus",   Sw,  int(SwitchId::Y)                },
		{ "Menus",   Sw,  int(SwitchId::SelectionUp)      },
		{ "Menus",   Sw,  int(SwitchId::SelectionDown)    },
		{ "Menus",   Sw,  int(SwitchId::SelectionLeft)    },
		{ "Menus",   Sw,  int(SwitchId::SelectionRight)   },

		{ "Online",  Sw,  int(SwitchId::License)          },
		{ "Online",  Sw,  int(SwitchId::SignIn)           },

		{ "Tweaks",  Mod, int(ModAction::OverlayToggle)   },
		{ "Tweaks",  Mod, int(ModAction::HudToggle)       },
		{ "Tweaks",  Mod, int(ModAction::OpenChat)        },
		{ "Tweaks",  Mod, int(ModAction::MusicNext)       },
		{ "Tweaks",  Mod, int(ModAction::MusicPrevious)   },
	};

	struct QuickSetupEntry
	{
		const char* title;
		const char* prompt;
		Selection::Kind kind;
		int index;
	};

	static inline const QuickSetupEntry QuickSetupSteps[] = {
		{ "Steering",    "Turn the wheel or move the stick you want to steer with.", Vol, int(ADChannel::Steering) },
		{ "Accelerator", "Press the accelerator fully.",                              Vol, int(ADChannel::Acceleration) },
		{ "Brake",       "Press the brake fully.",                                    Vol, int(ADChannel::Brake) },
		{ "Shift Up",    "Press the upshift paddle or button.",                       Sw,  int(SwitchId::GearUp) },
		{ "Shift Down",  "Press the downshift paddle or button.",                     Sw,  int(SwitchId::GearDown) },
		{ "Start",       "Press the button you want to use for Start and Pause.",      Sw,  int(SwitchId::Start) },
		{ "Confirm",     "Press the button you want to use to confirm menu choices.",  Sw,  int(SwitchId::A) },
		{ "Back",        "Press the button you want to use to go back.",               Sw,  int(SwitchId::B) },
	};

	// How far an analog action has to move from rest before its name lights up.
	// Digital actions use the game's own threshold instead, via
	// InputState::isPressed.
	static constexpr float ActiveThreshold = 0.25f;

	Selection selected{ Selection::Kind::Volume, int(ADChannel::Steering) };

	// What the listening prompt will write into. -1 appends a binding instead of
	// replacing one.
	Selection bindTarget;
	int bindIndex = -1;
	std::string bindingName;
	std::unordered_map<SDL_JoystickID, std::vector<Sint16>> axisBaseline;

	// Track binding changes (options tab are handled differently)
	bool unsavedChanges = false;
	bool confirmingReset = false;
	bool calibrationOpen = false;
	Selection calibrationTarget;
	int calibrationIndex = -1;
	int calibrationRest = 0;
	int calibrationMinimum = 0;
	int calibrationMaximum = 0;
	bool quickSetupActive = false;
	bool quickSetupComplete = false;
	int quickSetupStep = 0;
	bool quickSetupPreviousUnsaved = false;
	std::vector<std::pair<Selection, std::vector<InputBinding>>> quickSetupBackup;
	std::optional<InputBinding> quickSetupCandidate;
	bool quickSetupTimedOut = false;
	std::chrono::steady_clock::time_point quickSetupCaptureDeadline{};
	static constexpr auto QuickSetupCaptureTime = std::chrono::seconds(6);

	std::vector<Settings::SettingBase*> pendingSettings;

	static InputAction& action_for(const Selection& selection)
	{
		return InputManager::instance.actionFor(selection.kind, selection.index);
	}

	static const std::string& name_for(const Selection& selection)
	{
		return InputManager::actionName(selection.kind, selection.index);
	}

	// Steering reads as a signed axis, and its bindings are named differently
	// because of it (left/right rather than negated/not).
	static bool is_steering(const Selection& selection)
	{
		return selection.isVolume() && selection.index == int(ADChannel::Steering);
	}

	void begin_listening(const Selection& target, int index)
	{
		// Waits for the click that got here to be let go of first, otherwise it
		// binds the mouse button or whatever key triggered it.
		isListeningForInput = ListenState::WaitForButtonRelease;
		bindTarget = target;
		bindIndex = index;
		bindingName = name_for(target);
		axisBaseline.clear();
		if (quickSetupActive)
		{
			quickSetupCandidate.reset();
			quickSetupTimedOut = false;
		}
		for (const auto& device : InputManager::instance.devices)
		{
			auto& baseline = axisBaseline[device.instanceId];
			const int axisCount = SDL_GetNumJoystickAxes(device.joystick);
			baseline.reserve(axisCount);
			for (int axis = 0; axis < axisCount; ++axis)
				baseline.push_back(SDL_GetJoystickAxis(device.joystick, axis));
		}
	}

	static Selection quick_setup_selection(int step)
	{
		const auto& entry = QuickSetupSteps[step];
		return Selection{ entry.kind, entry.index };
	}

	void restore_quick_setup_backup()
	{
		for (auto& [selection, bindings] : quickSetupBackup)
			action_for(selection).bindings() = bindings;
		quickSetupBackup.clear();
		quickSetupActive = false;
		quickSetupComplete = false;
		quickSetupStep = 0;
		quickSetupCandidate.reset();
		quickSetupTimedOut = false;
		unsavedChanges = quickSetupPreviousUnsaved;
	}

	void start_quick_setup()
	{
		quickSetupBackup.clear();
		quickSetupPreviousUnsaved = unsavedChanges;
		for (int i = 0; i < int(std::size(QuickSetupSteps)); ++i)
		{
			const Selection selection = quick_setup_selection(i);
			if (std::find_if(quickSetupBackup.begin(), quickSetupBackup.end(), [&selection](const auto& saved)
				{
					return saved.first == selection;
				}) == quickSetupBackup.end())
				quickSetupBackup.emplace_back(selection, action_for(selection).bindings());
		}
		quickSetupStep = 0;
		quickSetupComplete = false;
		quickSetupActive = true;
		begin_listening(quick_setup_selection(quickSetupStep), -1);
	}

public:
	void init() override {}
	static InputBinding with_device_identity(InputBinding binding, const InputManager::InputDevice& device)
	{
		binding.deviceVendor = device.vendor;
		binding.deviceProduct = device.product;
		binding.deviceSerial = device.serial;
		binding.devicePath = device.path;
		return binding;
	}

	//
	// Listens for any input at all rather than being told up front whether to
	// expect a key or a pad: the binding records which it was. Escape cancels
	// and Delete clears, in every case, so there is no separate keyboard timeout
	// and the two halves behave the same.
	//
	bool HandleNewBinding()
	{
		InputAction& action = action_for(bindTarget);
		auto& bindings = action.bindings();

		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			if (quickSetupActive)
				restore_quick_setup_backup();
			isListeningForInput = ListenState::False;
			ImGui::CloseCurrentPopup();
			return false;
		}

		if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace))
		{
			if (quickSetupActive)
			{
				restore_quick_setup_backup();
				isListeningForInput = ListenState::False;
				ImGui::CloseCurrentPopup();
				return false;
			}
			const bool removed = bindIndex >= 0 && bindIndex < int(bindings.size());
			if (removed)
				bindings.erase(bindings.begin() + bindIndex);

			isListeningForInput = ListenState::False;
			ImGui::CloseCurrentPopup();
			return removed;
		}

		// Replaces the binding that was clicked, or appends when the add button
		// was. Appending is the whole point of the list, so nothing here clears
		// what is already bound.
		const auto commit = [&](const InputBinding& binding)
		{
			if (quickSetupActive)
			{
				quickSetupCandidate = binding;
				return;
			}
			else if (bindIndex >= 0 && bindIndex < int(bindings.size()))
				bindings[bindIndex] = binding;
			else
				action.add(binding);

			isListeningForInput = ListenState::WaitForBindButtonRelease;
			ImGui::CloseCurrentPopup();
		};

		if (quickSetupCandidate || quickSetupTimedOut)
			return false;

		// Keyboard
		{
			int keyCount = 0;
			const bool* keyState = SDL_GetKeyboardState(&keyCount);
			for (int i = 0; i < keyCount; i++)
			{
				// Reserved above, so they can't be bound to anything.
				if (i == SDL_SCANCODE_ESCAPE || i == SDL_SCANCODE_DELETE || i == SDL_SCANCODE_BACKSPACE)
					continue;

				if (keyState[i])
				{
					commit(InputBinding(static_cast<SDL_Scancode>(i)));
					return true;
				}
			}
		}

		// Controller
		for (const auto& device : InputManager::instance.devices)
		{
			for (int button = 0; button < SDL_GetNumJoystickButtons(device.joystick); ++button)
				if (SDL_GetJoystickButton(device.joystick, button))
				{
					commit(with_device_identity(
						InputBinding::joystickButton(device.guid, device.occurrence, button), device));
					return true;
				}

			for (int hat = 0; hat < SDL_GetNumJoystickHats(device.joystick); ++hat)
			{
				const Uint8 value = SDL_GetJoystickHat(device.joystick, hat);
				if (value != SDL_HAT_CENTERED)
				{
					commit(with_device_identity(
						InputBinding::joystickHat(device.guid, device.occurrence, hat, value), device));
					return true;
				}
			}

			const auto baselineIt = axisBaseline.find(device.instanceId);
			if (baselineIt == axisBaseline.end())
				continue;
			for (int axis = 0; axis < SDL_GetNumJoystickAxes(device.joystick) && axis < int(baselineIt->second.size()); ++axis)
			{
				const Sint16 current = SDL_GetJoystickAxis(device.joystick, axis);
				const int delta = int(current) - int(baselineIt->second[axis]);
				if (std::abs(delta) > 16384)
				{
					const auto mode = is_steering(bindTarget)
						? InputBinding::AxisMode::Signed : InputBinding::AxisMode::FromRest;
					commit(with_device_identity(
						InputBinding::joystickAxis(device.guid, device.occurrence, axis, false,
							mode, baselineIt->second[axis], delta > 0), device));
					return true;
				}
			}
		}

		// Legacy gamepad capture remains as a fallback when SDL exposes no raw
		// joystick handle for the selected gamepad.
		if (auto* controller = InputManager::instance.getPrimaryGamepad())
		{
			for (int i = SDL_GAMEPAD_BUTTON_SOUTH; i < SDL_GAMEPAD_BUTTON_COUNT; i++)
			{
				if (SDL_GetGamepadButton(controller, static_cast<SDL_GamepadButton>(i)))
				{
					commit(InputBinding(static_cast<SDL_GamepadButton>(i)));
					return true;
				}
			}

			for (int i = SDL_GAMEPAD_AXIS_LEFTX; i < SDL_GAMEPAD_AXIS_COUNT; i++)
			{
				const float value = SDL_GetGamepadAxis(controller, static_cast<SDL_GamepadAxis>(i)) / 32768.0f;
				if (std::abs(value) > 0.5f)
				{
					// The direction it was pushed becomes the binding's, which
					// the invert toggle can flip afterwards.
					commit(InputBinding(static_cast<SDL_GamepadAxis>(i), value < 0));
					return true;
				}
			}
		}

		return false;
	}

private:
	void begin_calibration(const Selection& target, int index)
	{
		auto& bindings = action_for(target).bindings();
		if (index < 0 || index >= int(bindings.size()) || bindings[index].kind != InputBinding::Kind::JoyAxis)
			return;
		auto* joystick = InputManager::instance.joystickForBinding(bindings[index]);
		if (!joystick)
			return;

		calibrationTarget = target;
		calibrationIndex = index;
		calibrationRest = SDL_GetJoystickAxis(joystick, bindings[index].controlIndex);
		calibrationMinimum = calibrationRest;
		calibrationMaximum = calibrationRest;
		calibrationOpen = true;
	}

	void draw_calibration_popup()
	{
		if (!calibrationOpen)
			return;
		ImGui::OpenPopup("Calibrate axis");
		if (!ImGui::BeginPopupModal("Calibrate axis", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			return;

		auto& bindings = action_for(calibrationTarget).bindings();
		if (calibrationIndex < 0 || calibrationIndex >= int(bindings.size()))
		{
			calibrationOpen = false;
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
			return;
		}

		auto& binding = bindings[calibrationIndex];
		auto* joystick = InputManager::instance.joystickForBinding(binding);
		if (!joystick)
		{
			ImGui::TextWrapped("Reconnect this device to continue calibration.");
		}
		else
		{
			const int current = SDL_GetJoystickAxis(joystick, binding.controlIndex);
			calibrationMinimum = (std::min)(calibrationMinimum, current);
			calibrationMaximum = (std::max)(calibrationMaximum, current);

			ImGui::TextWrapped(binding.axisMode == InputBinding::AxisMode::Signed
				? "Leave the wheel centered and set its center. Then turn fully left and fully right."
				: "Release the pedal and set its resting position. Then press it fully and release it.");
			ImGui::Spacing();
			ImGui::ProgressBar((current + 32768.0f) / 65535.0f, ImVec2(320.0f, 0),
				std::format("Current: {}", current).c_str());
			ImGui::TextDisabled("Detected range: %d to %d", calibrationMinimum, calibrationMaximum);

			if (ImGui::Button(binding.axisMode == InputBinding::AxisMode::Signed ? "Set center" : "Set resting position"))
			{
				calibrationRest = current;
				calibrationMinimum = current;
				calibrationMaximum = current;
			}

			const int negativeTravel = calibrationRest - calibrationMinimum;
			const int positiveTravel = calibrationMaximum - calibrationRest;
			const bool enoughTravel = binding.axisMode == InputBinding::AxisMode::Signed
				? negativeTravel > 4096 && positiveTravel > 4096
				: (std::max)(negativeTravel, positiveTravel) > 4096;

			ImGui::SameLine();
			if (!enoughTravel)
				ImGui::BeginDisabled();
			if (ImGui::Button("Save calibration"))
			{
				binding.axisMinimum = calibrationMinimum;
				binding.axisRest = calibrationRest;
				binding.axisMaximum = calibrationMaximum;
				binding.axisPositive = positiveTravel >= negativeTravel;
				unsavedChanges = true;
				calibrationOpen = false;
				ImGui::CloseCurrentPopup();
			}
			if (!enoughTravel)
				ImGui::EndDisabled();
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			calibrationOpen = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	// Left pane: every action, grouped, with the ones currently reading input
	// picked out. Watching a name light up is how a binding gets verified
	// without leaving the screen.
	void draw_action_list()
	{
		const ImVec4 activeColour = ImGui::GetStyle().Colors[ImGuiCol_CheckMark];

		const char* group = nullptr;
		for (const ActionListEntry& entry : ActionList)
		{
			if (!group || std::strcmp(group, entry.group) != 0)
			{
				group = entry.group;
				ImGui::SeparatorText(group);
			}

			const Selection action{ entry.kind, entry.index };
			const InputState& state = action_for(action).getState();

			// A binding's value keeps its sign, and negating a binding flips it,
			// so an axis bound to two opposing digital actions gives one of them
			// a positive value and the other a negative one. Only the positive
			// side fires, which is what isPressed tests for - taking the
			// magnitude instead would light up both ends of the same stick.
			// An analog action uses the sign for direction rather than for on
			// and off, so there either end of the range counts as movement.
			const bool active = action.isVolume()
				? std::abs(state.currentValue) >= ActiveThreshold
				: state.isPressed();

			if (active)
				ImGui::PushStyleColor(ImGuiCol_Text, activeColour);

			if (ImGui::Selectable(name_for(action).c_str(), selected == action))
				selected = action;

			if (active)
				ImGui::PopStyleColor();
		}
	}

	// Right pane: the selected action's bindings, one row each.
	void draw_binding_editor(SDL_GamepadType padType)
	{
		InputAction& action = action_for(selected);
		auto& bindings = action.bindings();
		const bool steering = is_steering(selected);

		ImGui::SeparatorText(name_for(selected).c_str());

		int removeIndex = -1;

		if (ImGui::BeginTable("##bindings", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
		{
			ImGui::TableSetupColumn("##input", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("##calibrate", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("##invert", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("##remove", ImGuiTableColumnFlags_WidthFixed);

			for (int i = 0; i < int(bindings.size()); i++)
			{
				InputBinding& binding = bindings[i];

				ImGui::PushID(i);
				ImGui::TableNextRow();

				// The binding's own name is the rebind button, so there is no
				// separate control for the most common thing to want.
				ImGui::TableNextColumn();
				std::string sourceName = binding.isKeyboard() ? "Keyboard" : "Gamepad";
				if (binding.isRawDevice())
				{
					if (const auto* device = InputManager::instance.deviceForBinding(binding))
					{
						const char* name = SDL_GetJoystickName(device->joystick);
						sourceName = name && name[0] ? name : "USB device";
						if (InputManager::instance.deviceMatchCount(binding) > 1)
							sourceName += " - choose device";
					}
					else
						sourceName = "Reconnect device";
				}
				const std::string label = std::format("{}  ({})",
					binding.displayName(padType, steering), sourceName);

				if (ImGui::Button(label.c_str(), ImVec2(-FLT_MIN, 0)))
					begin_listening(selected, i);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Rebind this input");

				ImGui::TableNextColumn();
				if (binding.kind == InputBinding::Kind::JoyAxis)
				{
					if (ImGui::Button("Calibrate"))
						begin_calibration(selected, i);
				}

				// Inverting is the only way to reach the '-' suffix the INI
				// format has always had: it sends an analog action the opposite
				// direction, and makes a digital action fire on negative input.
				ImGui::TableNextColumn();
				if (binding.kind == InputBinding::Kind::JoyAxis && binding.axisMode == InputBinding::AxisMode::FromRest)
				{
					ImGui::TextDisabled("Auto");
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip("Pedal direction is detected automatically during calibration");
				}
				else
				{
					if (ImGui::Checkbox("##invert", &binding.negate))
						unsavedChanges = true;
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip(steering
							? "Steer the other way with this input"
							: "Invert this input");
				}

				ImGui::TableNextColumn();
				if (ImGui::Button("X"))
					removeIndex = i;
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Remove this binding");

				ImGui::PopID();
			}

			ImGui::EndTable();
		}

		if (removeIndex >= 0)
		{
			bindings.erase(bindings.begin() + removeIndex);
			unsavedChanges = true;
		}

		if (bindings.empty())
			ImGui::TextDisabled("Nothing bound.");

		if (ImGui::Button("+ Add binding"))
			begin_listening(selected, -1);

		// Live value of the selected action.
		const float value = action.getState().currentValue;
		const float filled = selected.isVolume() ? std::abs(value) : value;

		ImGui::Spacing();
		ImGui::TextDisabled("Reading");
		ImGui::ProgressBar(std::clamp(filled, 0.0f, 1.0f), ImVec2(-FLT_MIN, 0),
			std::format("{:.2f}", value).c_str());

		draw_calibration_popup();
	}

	void draw_controllers()
	{
		auto& manager = InputManager::instance;

		if (manager.devices.empty())
		{
			ImGui::TextDisabled("No input devices detected.");
			ImGui::Spacing();
			ImGui::TextWrapped("Connect a controller, wheel, pedal set or shifter. Devices appear here automatically.");
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::TextDisabled("Multi-device input by hyp36rmax");
			return;
		}

		ImGui::Text("%d input device%s detected", int(manager.devices.size()), manager.devices.size() == 1 ? "" : "s");
		ImGui::TextDisabled("Move a control to verify that OutRun can see it.");
		ImGui::Spacing();

		for (const auto& device : manager.devices)
		{
			SDL_Joystick* joystick = device.joystick;
			const char* deviceName = SDL_GetJoystickName(joystick);
			if (!deviceName || !deviceName[0])
				deviceName = "Unknown input device";

			ImGui::PushID(int(device.instanceId));
			if (ImGui::TreeNodeEx("##device", ImGuiTreeNodeFlags_DefaultOpen,
				"%s  [%s]", deviceName, device.isGamepad ? "Gamepad" : "USB device"))
			{
				const int axisCount = SDL_GetNumJoystickAxes(joystick);
				const int buttonCount = SDL_GetNumJoystickButtons(joystick);
				const int hatCount = SDL_GetNumJoystickHats(joystick);
				ImGui::TextDisabled("%d axes  |  %d buttons  |  %d hats", axisCount, buttonCount, hatCount);

				for (int axis = 0; axis < axisCount; ++axis)
				{
					const float value = SDL_GetJoystickAxis(joystick, axis) / 32768.0f;
					ImGui::Text("Axis %d", axis + 1);
					ImGui::SameLine();
					ImGui::ProgressBar((value + 1.0f) * 0.5f, ImVec2(-FLT_MIN, 0),
						std::format("{:.2f}", value).c_str());
				}

				bool anyButton = false;
				for (int button = 0; button < buttonCount; ++button)
					if (SDL_GetJoystickButton(joystick, button))
					{
						if (!anyButton)
							ImGui::Text("Pressed:");
						ImGui::SameLine();
						ImGui::Text("%d", button + 1);
						anyButton = true;
					}
				if (!anyButton && buttonCount > 0)
					ImGui::TextDisabled("Press a button to test it");

				for (int hat = 0; hat < hatCount; ++hat)
					ImGui::Text("Hat %d: 0x%02X", hat + 1, SDL_GetJoystickHat(joystick, hat));

				ImGui::TreePop();
			}
			ImGui::PopID();
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::TextDisabled("Multi-device input by hyp36rmax");
	}

	// These are tweaks settings rather than bindings, so they go to the tweaks INI
	// the moment they are changed, the way the settings window writes them. Save
	// and its unsaved marker stay about bindings alone.
	void setting_changed(Settings::SettingBase& setting)
	{
		if (std::find(pendingSettings.begin(), pendingSettings.end(), &setting) == pendingSettings.end())
			pendingSettings.emplace_back(&setting);
	}

	// Held back until the control being dragged is let go of, so a slider doesn't
	// write the INI on every frame of the drag.
	void flush_settings()
	{
		if (pendingSettings.empty() || ImGui::IsAnyItemActive())
			return;

		for (Settings::SettingBase* setting : pendingSettings)
			setting->notify();
		pendingSettings.clear();

		Settings::write(Module::UserIniPath);
	}

	void draw_options()
	{
		auto& manager = InputManager::instance;

		ImGui::TextUnformatted("Controller compatibility");
		const char* inputBackends[] = { "Automatic (recommended)", "Raw Input", "DirectInput (wheels)", "XInput" };
		if (ImGui::Combo("Input backend", Settings::InputBackend.ptr(), inputBackends, IM_ARRAYSIZE(inputBackends)))
			setting_changed(Settings::InputBackend);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Use DirectInput when a wheel is listed but its axes or buttons do not respond.");
		ImGui::TextDisabled("Restart the game after changing the input backend.");
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		const char* vibrationModes[] = { "Disabled", "Enabled", "Swap L/R", "Merge L/R" };
		if (ImGui::Combo("Vibration Mode", Settings::VibrationMode.ptr(), vibrationModes, IM_ARRAYSIZE(vibrationModes)))
			setting_changed(Settings::VibrationMode);
		if (ImGui::SliderInt("Vibration Strength", Settings::VibrationStrength.ptr(), 0, 10))
			setting_changed(Settings::VibrationStrength);
		if (ImGui::Combo("Impulse Vibration", Settings::ImpulseVibrationMode.ptr(), vibrationModes, IM_ARRAYSIZE(vibrationModes)))
			setting_changed(Settings::ImpulseVibrationMode);

		int deadzonePercent = int(Settings::SteeringDeadZone * 100.f);
		if (ImGui::SliderInt("Steering Deadzone", &deadzonePercent, 5, 20, "%d%%"))
		{
			Settings::SteeringDeadZone = float(deadzonePercent) / 100.f;
			setting_changed(Settings::SteeringDeadZone);
		}

		if (ImGui::Checkbox("Bypass Sensitivity", Settings::BypassGameSensitivity.ptr()))
			setting_changed(Settings::BypassGameSensitivity);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Passes steering input to the game directly, allows for more sensitive controls.\n"
				"Only used when UseNewInput is enabled.");
	}

	void draw_force_feedback()
	{
		ImGui::TextWrapped("Choose your wheel, set the strength, then use the two direction tests. That's all most players need.");
		ImGui::Spacing();
		if (ImGui::Checkbox("Enable force feedback", Settings::WheelFFBEnabled.ptr()))
		{
			setting_changed(Settings::WheelFFBEnabled);
			WheelForceFeedback::refresh();
		}

		const auto& devices = WheelForceFeedback::devices();
		auto device_label = [&devices](size_t index)
		{
			const auto& device = devices[index];
			const int duplicateCount = int(std::count_if(devices.begin(), devices.end(), [&](const auto& other) { return other.name == device.name; }));
			if (duplicateCount < 2)
				return device.name;
			int occurrence = 1;
			for (size_t previous = 0; previous < index; ++previous)
				if (devices[previous].name == device.name) ++occurrence;
			return std::format("{} (Device {})", device.name, occurrence);
		};

		std::string preview = devices.empty() ? "No compatible wheel" : "Select wheel";
		for (size_t i = 0; i < devices.size(); ++i)
			if (devices[i].id == Settings::WheelFFBDevice.get()) preview = device_label(i);
		ImGui::BeginDisabled(!Settings::WheelFFBEnabled);
		if (ImGui::BeginCombo("Wheel", preview.c_str()))
		{
			for (size_t i = 0; i < devices.size(); ++i)
			{
				const auto& device = devices[i];
				const std::string label = device_label(i);
				bool selected = device.id == Settings::WheelFFBDevice.get();
				ImGui::PushID(device.id.c_str());
				if (ImGui::Selectable(label.c_str(), selected))
				{
					WheelForceFeedback::select(device.id);
					setting_changed(Settings::WheelFFBDevice);
				}
				ImGui::PopID();
			}
			ImGui::EndCombo();
		}
		ImGui::TextDisabled("Fanatec and some other bases use separate input and FFB interfaces; this is normal.");
		if (ImGui::SliderInt("Strength", Settings::WheelFFBStrength.ptr(), 0, 100, "%d%%"))
			setting_changed(Settings::WheelFFBStrength);

		ImGui::SeparatorText("Test your wheel");
		ImGui::BeginDisabled(!WheelForceFeedback::ready());
		if (ImGui::Button("Test left")) WheelForceFeedback::test(-1.f);
		ImGui::SameLine();
		if (ImGui::Button("Test right")) WheelForceFeedback::test(1.f);
		ImGui::EndDisabled();
		ImGui::TextDisabled("Tests use a gentle force and stop automatically.");
		ImGui::EndDisabled();

		ImGui::Spacing();
		ImGui::SeparatorText("Status");
		ImGui::TextWrapped("%s", WheelForceFeedback::status().c_str());

		ImGui::Spacing();
		if (ImGui::CollapsingHeader("Advanced", ImGuiTreeNodeFlags_None))
		{
			ImGui::TextWrapped("These options are only needed when a wheel behaves incorrectly or was connected after the game started.");
			if (ImGui::SliderFloat("Centering", Settings::WheelFFBSpringStrength.ptr(), 0.0f, 1.0f, "%.2f"))
				setting_changed(Settings::WheelFFBSpringStrength);
			if (ImGui::SliderFloat("Damping", Settings::WheelFFBDamperStrength.ptr(), 0.0f, 1.0f, "%.2f"))
				setting_changed(Settings::WheelFFBDamperStrength);
			if (ImGui::SliderFloat("Impacts", Settings::WheelFFBImpactStrength.ptr(), 0.0f, 1.0f, "%.2f"))
				setting_changed(Settings::WheelFFBImpactStrength);
			if (ImGui::SliderFloat("Road detail", Settings::WheelFFBRoadStrength.ptr(), 0.0f, 1.0f, "%.2f"))
				setting_changed(Settings::WheelFFBRoadStrength);
			if (ImGui::SliderFloat("Grip loss", Settings::WheelFFBGripLossStrength.ptr(), 0.0f, 1.0f, "%.2f"))
				setting_changed(Settings::WheelFFBGripLossStrength);
			if (ImGui::Checkbox("Invert force direction", Settings::WheelFFBInvert.ptr()))
				setting_changed(Settings::WheelFFBInvert);
			if (ImGui::Button("Refresh connected wheels")) WheelForceFeedback::refresh();
			ImGui::TextDisabled("Compatibility details are recorded automatically in OutRun2006Tweaks.log.");
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::TextDisabled("Force feedback by hyp36rmax");
	}

	// The prompt shown while an input is being waited on.
	void draw_listening_popup()
	{
		if (isListeningForInput == ListenState::False)
			return;

		auto& manager = InputManager::instance;

		if (isListeningForInput == ListenState::WaitForButtonRelease)
		{
			if (!manager.anyInputPressed())
			{
				isListeningForInput = ListenState::Listening;
				if (quickSetupActive)
					quickSetupCaptureDeadline = std::chrono::steady_clock::now() + QuickSetupCaptureTime;
			}
			return;
		}

		if (isListeningForInput == ListenState::WaitForBindButtonRelease)
		{
			if (!manager.anyInputPressed())
			{
				if (quickSetupActive && quickSetupStep < int(std::size(QuickSetupSteps)))
					begin_listening(quick_setup_selection(quickSetupStep), -1);
				else
				{
					isListeningForInput = ListenState::False;
					if (quickSetupActive)
					{
						quickSetupActive = false;
						quickSetupComplete = true;
					}
				}
			}
			return;
		}

		ImGui::OpenPopup("Listening for Input");
		if (ImGui::BeginPopupModal("Listening for Input", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			if (quickSetupActive && quickSetupStep < int(std::size(QuickSetupSteps)))
			{
				const auto& step = QuickSetupSteps[quickSetupStep];
				ImGui::TextDisabled("Quick Setup  |  Step %d of %d", quickSetupStep + 1, int(std::size(QuickSetupSteps)));
				ImGui::SeparatorText(step.title);
				ImGui::TextWrapped("%s", step.prompt);
				if (!quickSetupCandidate && !quickSetupTimedOut)
				{
					const auto remaining = std::chrono::duration<float>(quickSetupCaptureDeadline - std::chrono::steady_clock::now()).count();
					if (remaining <= 0.f)
						quickSetupTimedOut = true;
					else
					{
						ImGui::ProgressBar(remaining / 6.f, ImVec2(320.f, 0.f), std::format("{:.1f} seconds", remaining).c_str());
						ImGui::TextDisabled("Only the first deliberate input will be proposed.");
					}
				}
			}
			else
				ImGui::Text("Press any input to bind to %s", bindingName.c_str());
			ImGui::Spacing();
			if (quickSetupActive && quickSetupCandidate)
			{
				ImGui::SeparatorText("Confirm input");
				ImGui::Text("Detected: %s", quickSetupCandidate->displayName().c_str());
				if (const auto* device = manager.deviceForBinding(*quickSetupCandidate))
					ImGui::TextDisabled("Device: %s", SDL_GetJoystickName(device->joystick));
				ImGui::TextWrapped("Confirm this input before Quick Setup moves to the next control.");
				if (ImGui::Button("Use this input"))
				{
					auto& bindings = action_for(bindTarget).bindings();
					std::erase_if(bindings, [](const InputBinding& existing) { return !existing.isKeyboard(); });
					action_for(bindTarget).add(*quickSetupCandidate);
					quickSetupCandidate.reset();
					++quickSetupStep;
					unsavedChanges = true;
					isListeningForInput = ListenState::WaitForBindButtonRelease;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Try again"))
				{
					begin_listening(bindTarget, -1);
					ImGui::CloseCurrentPopup();
				}
			}
			else if (quickSetupActive && quickSetupTimedOut)
			{
				ImGui::TextWrapped("No deliberate input was detected. This step was not skipped.");
				if (ImGui::Button("Try this step again"))
				{
					begin_listening(bindTarget, -1);
					ImGui::CloseCurrentPopup();
				}
			}
			else
			{
				ImGui::TextDisabled(quickSetupActive ? "Escape to cancel Quick Setup" : "Escape to cancel, Delete to clear");
				if (HandleNewBinding())
					unsavedChanges = true;
			}

			if (quickSetupActive && (quickSetupCandidate || quickSetupTimedOut))
			{
				ImGui::TextDisabled("Escape to cancel Quick Setup");
				if (ImGui::IsKeyPressed(ImGuiKey_Escape))
				{
					restore_quick_setup_backup();
					isListeningForInput = ListenState::False;
					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::EndPopup();
		}
	}

	void draw_quick_setup_complete(bool& dialogOpen)
	{
		if (!quickSetupComplete)
			return;
		ImGui::OpenPopup("Test your driving controls");
		if (!ImGui::BeginPopupModal("Test your driving controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			return;

		ImGui::TextWrapped("Move the wheel, accelerator and brake. If everything responds correctly, save and drive.");
		ImGui::Spacing();
		const float steering = action_for({ Vol, int(ADChannel::Steering) }).getState().currentValue;
		const float accelerator = action_for({ Vol, int(ADChannel::Acceleration) }).getState().currentValue;
		const float brake = action_for({ Vol, int(ADChannel::Brake) }).getState().currentValue;
		ImGui::Text("Steering");
		ImGui::SameLine();
		ImGui::ProgressBar(std::clamp((steering + 1.0f) * 0.5f, 0.0f, 1.0f), ImVec2(280.0f, 0),
			std::format("{:.2f}", steering).c_str());
		ImGui::Text("Accelerator");
		ImGui::SameLine();
		ImGui::ProgressBar(std::clamp(accelerator, 0.0f, 1.0f), ImVec2(280.0f, 0),
			std::format("{:.2f}", accelerator).c_str());
		ImGui::Text("Brake");
		ImGui::SameLine();
		ImGui::ProgressBar(std::clamp(brake, 0.0f, 1.0f), ImVec2(280.0f, 0),
			std::format("{:.2f}", brake).c_str());

		ImGui::Spacing();
		if (ImGui::Button("Save & Drive"))
		{
			if (InputManager::instance.saveBindingIni(Module::BindingsIniPath))
			{
				quickSetupBackup.clear();
				quickSetupComplete = false;
				unsavedChanges = false;
				dialogOpen = false;
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Start Over"))
		{
			restore_quick_setup_backup();
			start_quick_setup();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			restore_quick_setup_backup();
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

public:
	void render(bool overlayEnabled) override
	{
		if (!Overlay::IsBindingDialogActive)
			return;

		auto& manager = InputManager::instance;

		auto padType = SDL_GAMEPAD_TYPE_XBOX360;
		if (auto* primary = manager.getPrimaryGamepad())
			padType = SDL_GetGamepadType(primary);

		bool dialogOpen = true;

		// Sized from the font rather than from the screen, so it tracks what is
		// in it instead of how large the monitor is. The action list is the tall
		// part and scrolls; nothing else grows.
		const Overlay::ContentRect content = Overlay::content_rect();
		const float fontSize = ImGui::GetFontSize();

		float width = fontSize * 27.0f;
		float height = fontSize * 26.0f;

		if (width > content.width * 0.9f)
			width = content.width * 0.9f;
		if (height > content.height * 0.9f)
			height = content.height * 0.9f;

		ImGui::SetNextWindowPos(ImVec2(content.x + (content.width * 0.5f), content.y + (content.height * 0.5f)),
			ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);

		ImGui::OpenPopup("Input Bindings");
		if (ImGui::BeginPopupModal("Input Bindings", &dialogOpen, ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove))
		{
			if (ImGui::Button("Quick Setup"))
				start_quick_setup();

			ImGui::SameLine();
			if (ImGui::Button(unsavedChanges ? "Save bindings*##save" : "Save bindings##save"))
				if (manager.saveBindingIni(Module::BindingsIniPath))
					unsavedChanges = false;

			ImGui::SameLine();

			if (ImGui::Button("Load bindings"))
				if (manager.readBindingIni(Module::BindingsIniPath))
					unsavedChanges = false;

			// Two lines are reserved below: the unsaved note and the button row.
			const float footerHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetTextLineHeightWithSpacing();

			ImGui::BeginChild("##body", ImVec2(0, -footerHeight));
			if (ImGui::BeginTabBar("##sections"))
			{
				if (ImGui::BeginTabItem("Bindings"))
				{
					// The action list is the only part that can outgrow the
					// dialog, so it is the only part that scrolls.
					if (ImGui::BeginChild("##actions", ImVec2(fontSize * 9.0f, 0), ImGuiChildFlags_Borders))
						draw_action_list();
					ImGui::EndChild();

					ImGui::SameLine();

					if (ImGui::BeginChild("##editor", ImVec2(0, 0)))
						draw_binding_editor(padType);
					ImGui::EndChild();

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Controllers"))
				{
					draw_controllers();
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Force Feedback"))
				{
					draw_force_feedback();
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Options"))
				{
					draw_options();
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}
			ImGui::EndChild();

			// Kept on its own line whether or not there are changes, so the
			// buttons below don't shift as it appears.
			ImGui::TextDisabled("%s", unsavedChanges ? "Note: you have unsaved changes!" : "");

			if (ImGui::Button("Return to game"))
				dialogOpen = false;

			ImGui::SameLine();

			if (ImGui::Button(!confirmingReset ? "Reset to default##clear" : "Are you sure?##clear"))
			{
				if (!confirmingReset)
				{
					confirmingReset = true;
				}
				else
				{
					unsavedChanges = true;
					Settings::SteeringDeadZone = 0.2f;
					setting_changed(Settings::SteeringDeadZone);
					Settings::BypassGameSensitivity = false;
					setting_changed(Settings::BypassGameSensitivity);
					manager.setupDefaultBindings();
					if (auto* controller = manager.getPrimaryGamepad())
						manager.setupGamepad(controller);
					confirmingReset = false;
				}
			}

			// Back/B leaves the dialog, but only while nothing is being bound -
			// otherwise the press meant for a binding closes the screen instead.
			if (isListeningForInput == ListenState::False)
			{
				if ((manager.switch_overlay & (1 << int(SwitchId::Back) | 1 << int(SwitchId::B))) != 0)
					dialogOpen = false;
			}

			draw_listening_popup();
			draw_quick_setup_complete(dialogOpen);

			ImGui::EndPopup();
		}

		// Outside the popup so a change still reaches the INI on the frame the
		// dialog is closed.
		flush_settings();

		if (!dialogOpen)
		{
			Overlay::IsBindingDialogActive = false;
			Overlay::RequestMouseHide = true;
			confirmingReset = false;
		}
	}

	static InputBindingsUI instance;
};
InputBindingsUI InputBindingsUI::instance;
