#pragma once

#include <imgui.h>
#include <array>
#include <deque>
#include <mutex>
#include <sstream>
#include "game_addrs.hpp"
#include "overlay.hpp"

inline size_t maxNotifications = 5;
inline std::chrono::seconds displayDuration = std::chrono::seconds(10);

inline ImVec2 notificationSize = { 600, 100 };
inline float notificationSpacing = 10.0f;
inline float notificationTextScale = 1.5f;

class Notifications
{
private:
	struct Notification
	{
		std::string message;
		std::chrono::time_point<std::chrono::steady_clock> timestamp;
		int minDisplaySeconds;
		std::function<void()> onMouseClick;
		bool startupCard;
	};

	std::deque<Notification> notifications;
	std::mutex notificationsMutex;

public:
	void add(const std::string& message, int minDisplaySeconds = 0,
		std::function<void()> onMouseClick = nullptr, bool startupCard = false)
	{
		std::lock_guard<std::mutex> lock(notificationsMutex);
		notifications.push_back({ message, std::chrono::steady_clock::now(), minDisplaySeconds, onMouseClick, startupCard });

		if (notifications.size() > maxNotifications)
			notifications.pop_front();
	}

	void render()
	{
		// Remove expired notifications
		auto now = std::chrono::steady_clock::now();
		{
			std::lock_guard<std::mutex> lock(notificationsMutex);

			while (!notifications.empty())
			{
				auto& front = notifications.front();

				auto duration = displayDuration;
				if (front.minDisplaySeconds > 0)
					duration = std::chrono::seconds(front.minDisplaySeconds);

				if (now - front.timestamp <= duration) // notif time hasn't elapsed yet?
					break;

				notifications.pop_front();
			}
		}

		if (Game::is_in_game())
		{
			if (Overlay::NotifyHideMode == Overlay::NotifyHideMode_AllRaces)
				return;

			if (Overlay::NotifyHideMode == Overlay::NotifyHideMode_OnlineRaces &&
				*Game::SumoNet_CurNetDriver && (*Game::SumoNet_CurNetDriver)->is_in_lobby() &&
				(*Game::game_mode == 3 || *Game::game_mode == 4))
				return;
		}

		if (!Overlay::NotifyEnable)
			return;

		// Latest notification goes against the right edge of the game's content,
		// which letterboxing pulls inward.
		const Overlay::ContentRect content = Overlay::content_rect();

		float startX = content.x + content.width - notificationSize.x - 10.f;  // 10px padding from the right
		float curY = (content.height / 4.0f) - (notifications.size() * (notificationSize.y + notificationSpacing) / 2.0f);

		std::lock_guard<std::mutex> lock(notificationsMutex);

		for (size_t i = 0; i < notifications.size(); ++i)
		{
			auto windowSize = notificationSize;
			const auto& notification = notifications[i];

			ImGui::SetNextWindowPos(ImVec2(startX, curY));

			std::string windowName = "Notification " + std::to_string(i);
			ImGui::Begin(windowName.c_str(), nullptr, ImGuiWindowFlags_NoDecoration |
				ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);

            // Check for click on the notification
            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                if (notification.onMouseClick)
                    notification.onMouseClick();

			if (notification.startupCard)
			{
				// This five-line identity has its own measured layout. Other notifications
				// keep the existing size, scale, and wrapping behavior below.
				std::array<std::string, 5> lines{};
				std::istringstream stream(notification.message);
				for (auto& line : lines)
					std::getline(stream, line);

				const float uiScale = Overlay::GlobalFontScale;
				const float sidePadding = 10.0f * uiScale;
				const float topPadding = 7.0f * uiScale;
				const float bottomPadding = 8.0f * uiScale;
				const std::array<float, 5> scales{ 1.20f, 1.00f, 1.00f, 1.05f, 0.90f };
				const std::array<float, 5> gaps{ 2.0f, 4.0f, 5.0f, 4.0f, 0.0f };
				std::array<ImVec2, 5> textSizes{};
				float naturalWidth = notificationSize.x;
				for (size_t line = 0; line < lines.size(); ++line)
				{
					ImGui::SetWindowFontScale(scales[line]);
					naturalWidth = max(naturalWidth, ImGui::CalcTextSize(lines[line].c_str()).x +
						2.0f * sidePadding);
				}
				const float width = min(naturalWidth, min(notificationSize.x * 1.20f,
					content.width - 20.0f));
				float height = topPadding + bottomPadding;
				for (size_t line = 0; line < lines.size(); ++line)
				{
					ImGui::SetWindowFontScale(scales[line]);
					textSizes[line] = ImGui::CalcTextSize(lines[line].c_str(), nullptr, false,
						width - (2.0f * sidePadding));
					height += textSizes[line].y + gaps[line] * uiScale;
				}

				ImGui::SetWindowSize(ImVec2(width, height));
				ImGui::SetWindowPos(ImVec2(content.x + content.width - width - 10.0f, curY));
				curY += height + notificationSpacing;

				float y = topPadding;
				for (size_t line = 0; line < lines.size(); ++line)
				{
					ImGui::SetWindowFontScale(scales[line]);
					const float x = max(sidePadding, (width - textSizes[line].x) * 0.5f);
					ImGui::SetCursorPos(ImVec2(x, y));
					ImGui::PushTextWrapPos(width - sidePadding);
					if (line == lines.size() - 1)
						ImGui::TextDisabled("%s", lines[line].c_str());
					else
						ImGui::TextWrapped("%s", lines[line].c_str());
					ImGui::PopTextWrapPos();
					y += textSizes[line].y + gaps[line] * uiScale;
				}
				ImGui::SetWindowFontScale(1.0f);
				ImGui::End();
				continue;
			}

			ImGui::SetWindowFontScale(notificationTextScale);

			// Split the message into lines
			std::vector<std::string> lines;
			if (notification.message.find("---") == std::string::npos)
				lines.push_back(notification.message); // notificiation doesn't have splitter, treat it as all single line for centering
			else
			{
				std::istringstream messageStream(notification.message);
				std::string line;
				while (std::getline(messageStream, line)) {
					lines.push_back(line);
				}
			}

			// Calculate total height for all lines
			float totalTextHeight = 0.0f;
			for (const auto& singleLine : lines) {
				ImVec2 lineSize = ImGui::CalcTextSize(singleLine.c_str(), nullptr, true, windowSize.x - 20.0f);
				totalTextHeight += lineSize.y;
			}
			totalTextHeight += (lines.size() - 1) * ImGui::GetStyle().ItemSpacing.y;

			// Adjust window height if necessary
			if (totalTextHeight + 40.0f > windowSize.y)
				windowSize.y = totalTextHeight + 40.0f;

			ImGui::SetWindowSize(windowSize);

			curY += windowSize.y + notificationSpacing;

			// Center text block vertically
			float paddingY = 5.0f;
			float currentYOffset = (windowSize.y - totalTextHeight) / 2.0f;
			currentYOffset = max(currentYOffset, paddingY);

			for (const auto& singleLine : lines) {
				// Calculate individual line size
				ImVec2 lineSize = ImGui::CalcTextSize(singleLine.c_str(), nullptr, true, windowSize.x - 20.0f);

				// Center line horizontally
				if (singleLine != "---")
				{
					float paddingX = 10.0f;
					float offsetX = (windowSize.x - lineSize.x) / 2.0f;
					offsetX = max(offsetX, paddingX);

					ImGui::SetCursorPos(ImVec2(offsetX, currentYOffset));
					ImGui::TextWrapped("%s", singleLine.c_str());
				}
				else
				{
					ImGui::SetCursorPos(ImVec2(0, currentYOffset + (paddingY * 3)));
					ImGui::Separator();
				}

				// Move down for the next line, including spacing
				currentYOffset += lineSize.y + ImGui::GetStyle().ItemSpacing.y;
			}

			ImGui::End();
		}
	}

	static Notifications instance;
};
