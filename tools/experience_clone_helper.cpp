#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "experience_licence.hpp"
#include "save_recovery.hpp"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

static const char* codeName(ExperienceLicence::Code code)
{
	switch (code)
	{
	case ExperienceLicence::Code::Created: return "CREATED";
	case ExperienceLicence::Code::Reused: return "REUSED";
	case ExperienceLicence::Code::UnsafePhase: return "UNSAFE_PHASE";
	case ExperienceLicence::Code::UnsupportedExecutable: return "UNSUPPORTED_EXECUTABLE";
	case ExperienceLicence::Code::SourceInvalid: return "SOURCE_INVALID";
	case ExperienceLicence::Code::NoFreeSlot: return "NO_FREE_SLOT";
	case ExperienceLicence::Code::BackupFailed: return "BACKUP_FAILED";
	case ExperienceLicence::Code::DestinationChanged: return "DESTINATION_CHANGED";
	case ExperienceLicence::Code::ValidationFailed: return "VALIDATION_FAILED";
	case ExperienceLicence::Code::StaleManagedSlot: return "STALE_MANAGED_SLOT";
	case ExperienceLicence::Code::IoFailure: return "IO_FAILURE";
	}
	return "UNKNOWN";
}

static void status(const fs::path& root, const std::string& message)
{
	const auto folder = root.parent_path() / "MultiInput" / "ExperienceLicences";
	fs::create_directories(folder);
	std::ofstream out(folder / "clone-run88-status.txt", std::ios::binary | std::ios::trunc);
	out << message << '\n';
}

int wmain(int argc, wchar_t** argv)
{
	if (argc != 5) return 2;
	const auto exe = fs::path(argv[1]);
	const auto root = fs::path(argv[2]);
	const auto pid = wcstoul(argv[3], nullptr, 10);
	const auto sourceSlot = int(wcstol(argv[4], nullptr, 10));
	try
	{
		const auto managed = exe.parent_path() / "_E2ManagedTest";
		const auto expectedRoot = managed / "SaveGame";
		if (fs::is_symlink(fs::symlink_status(managed)) ||
			fs::is_symlink(fs::symlink_status(root)) ||
			fs::weakly_canonical(root) != fs::weakly_canonical(expectedRoot) ||
			SaveRecovery::FileSha256(exe) !=
			"68ceb386829066f8455b9d027320af962584321f3e2e8a79c72841495a6134c3")
		{
			status(root, "E3B Run #88 FAILED: MANAGED_TEST root or supported EXE validation failed");
			return 3;
		}
		HANDLE game = OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(pid));
		if (!game)
		{
			status(root, "E3B Run #88 FAILED: could not monitor game process exit");
			return 4;
		}
		const auto wait = WaitForSingleObject(game, 30 * 60 * 1000);
		if (wait != WAIT_OBJECT_0)
		{
			CloseHandle(game);
			status(root, "E3B Run #88 FAILED: game did not exit within 30 minutes");
			return 5;
		}
		const auto source = root / ("License" + std::to_string(sourceSlot + 1) + ".dat");
		std::string before;
		try { before = SaveRecovery::FileSha256(source); } catch (...) {}
		ExperienceLicence::Service service(exe, root, [game] {
			return WaitForSingleObject(game, 0) == WAIT_OBJECT_0;
		});
		const auto result = service.CloneActiveLicenceToFreeSlot(sourceSlot);
		std::string after;
		try { after = SaveRecovery::FileSha256(source); } catch (...) {}
		CloseHandle(game);
		const bool sourceIntact = !before.empty() && before == after;
		std::string report = "E3B Run #88 result=" + std::string(codeName(result.code)) +
			" sourceSlot=" + std::to_string(result.sourceSlot + 1) +
			" destinationSlot=" + (result.destinationSlot >= 0 ? std::to_string(result.destinationSlot + 1) : "none") +
			" restorePoint=" + (result.restorePointId.empty() ? "none" : result.restorePointId) +
			" sourceIntegrity=" + (sourceIntact ? "unchanged" : "FAILED") +
			" detail=" + result.error;
		status(root, report);
		return result.success() && sourceIntact ? 0 : 6;
	}
	catch (const std::exception& error)
	{
		try { status(root, std::string("E3B Run #88 FAILED: ") + error.what()); } catch (...) {}
		return 7;
	}
}
