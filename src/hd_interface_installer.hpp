#pragma once

#include <cstddef>
#include <filesystem>
#include <string>

namespace HdInterface
{
enum class InstallerState
{
	NotInstalled,
	Downloading,
	Installing,
	AwaitingOverwrite,
	Installed,
	Failed,
};

struct InstallerSnapshot
{
	InstallerState state = InstallerState::NotInstalled;
	std::size_t collisionCount = 0;
	std::string message;
};

void initialize(const std::filesystem::path& gameRoot);
void refresh_installation_state();
void start_install(const std::filesystem::path& gameRoot);
void continue_after_collision();
void cancel_collision();
InstallerSnapshot snapshot();
bool consume_completed_install();
void finish_setting_enable(bool success);
}
