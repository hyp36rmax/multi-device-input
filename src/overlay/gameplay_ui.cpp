#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <vector>
#include <imgui.h>

#include "overlay.hpp"
#include "native_unlock.hpp"
#include "plugin.hpp"

// Player actions belong here, separate from the diagnostic Debug tab.
class GameplayWindow : public OverlayWindow
{
	bool hasResult = false;
	NativeUnlock::Result result;

	static const char* player_message(NativeUnlock::Code code)
	{
		switch (code)
		{
		case NativeUnlock::Code::Success: return "Unlocked \xe2\x9c\x93";
		case NativeUnlock::Code::NoActiveLicence: return "Select a profile before using Unlock All.";
		case NativeUnlock::Code::UnsupportedExecutable: return "Unlock All is unavailable with this game executable.";
		case NativeUnlock::Code::InvalidNativeContext: return "Return to the main menu and try again.";
		case NativeUnlock::Code::InvocationFailed:
		case NativeUnlock::Code::VerificationFailed: return "Unlock All could not be applied.";
		}
		return "Unlock All could not be applied.";
	}

public:
	Kind kind() const override { return Kind::Tab; }
	const char* name() const override { return "Gameplay"; }
	int order() const override { return 1; }
	void init() override {}

	void render(bool) override
	{
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Unlock All Content");
		ImGui::SameLine();
		if (ImGui::Button("Unlock##content"))
			ImGui::OpenPopup("Unlock All Content##confirm");
		ImGui::SameLine();
		ImGui::TextDisabled("\xe2\x93\x98");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30.0f);
			ImGui::TextUnformatted("Unlocks all game content on the currently active profile.\n\n"
				"To preserve your existing progress, create or select a new profile before "
				"unlocking. If you save afterward, the unlocked state will be saved to that profile.");
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}

		if (ImGui::BeginPopupModal("Unlock All Content##confirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::TextUnformatted("Unlock all content on the current profile?\n\n"
				"The unlocked state can become permanent if you later save this profile.");
			ImGui::Spacing();
			if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
			if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
			ImGui::SameLine();
			if (ImGui::Button("Unlock##confirm"))
			{
				result = NativeUnlock::UnlockAllContent();
				hasResult = true;
				spdlog::info("E4 native unlock: activeLicence={}, supportedExe={}, nativeInvoked={}, "
					"verified={}, result={}", result.activeLicence, result.supportedExecutable,
					result.invoked, result.verified, NativeUnlock::CodeName(result.code));
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (hasResult)
		{
			ImGui::Spacing();
			ImGui::TextWrapped("%s", player_message(result.code));
		}
	}

	static GameplayWindow instance;
};

GameplayWindow GameplayWindow::instance;
