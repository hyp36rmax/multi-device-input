#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")

#include "hd_interface_installer.hpp"

#include <atomic>
#include <filesystem>
#include <format>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

#include <spdlog/spdlog.h>

#include "hd_interface_package.hpp"
#include "plugin.hpp"

namespace HdInterface
{
namespace
{
std::mutex StateMutex;
InstallerSnapshot Current;
std::filesystem::path CurrentGameRoot;
std::filesystem::path DownloadedArchive;
std::atomic_bool WorkerRunning = false;
std::atomic_bool InstallCompleted = false;

std::string sha256_file(const std::filesystem::path& path)
{
	BCRYPT_ALG_HANDLE algorithm = nullptr;
	BCRYPT_HASH_HANDLE hash = nullptr;
	DWORD objectSize = 0;
	DWORD hashSize = 0;
	DWORD received = 0;
	std::vector<UCHAR> object;
	std::vector<UCHAR> digest;

	if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM,
		nullptr, 0)) ||
		!BCRYPT_SUCCESS(BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
			reinterpret_cast<PUCHAR>(&objectSize), sizeof(objectSize), &received, 0)) ||
		!BCRYPT_SUCCESS(BCryptGetProperty(algorithm, BCRYPT_HASH_LENGTH,
			reinterpret_cast<PUCHAR>(&hashSize), sizeof(hashSize), &received, 0)))
	{
		if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
		return {};
	}

	object.resize(objectSize);
	digest.resize(hashSize);
	if (!BCRYPT_SUCCESS(BCryptCreateHash(algorithm, &hash, object.data(), objectSize,
		nullptr, 0, 0)))
	{
		BCryptCloseAlgorithmProvider(algorithm, 0);
		return {};
	}

	std::ifstream input(path, std::ios::binary);
	std::vector<char> buffer(1024 * 1024);
	while (input)
	{
		input.read(buffer.data(), buffer.size());
		const auto count = input.gcount();
		if (count > 0 && !BCRYPT_SUCCESS(BCryptHashData(hash,
			reinterpret_cast<PUCHAR>(buffer.data()), static_cast<ULONG>(count), 0)))
		{
			input.setstate(std::ios::badbit);
			break;
		}
	}

	const bool finished = input.eof() && BCRYPT_SUCCESS(BCryptFinishHash(hash,
		digest.data(), static_cast<ULONG>(digest.size()), 0));
	BCryptDestroyHash(hash);
	BCryptCloseAlgorithmProvider(algorithm, 0);
	if (!finished)
		return {};

	std::ostringstream hex;
	hex << std::hex << std::setfill('0');
	for (const UCHAR byte : digest)
		hex << std::setw(2) << unsigned(byte);
	return hex.str();
}

void set_state(InstallerState state, std::string message = {}, std::size_t collisions = 0)
{
	std::scoped_lock lock(StateMutex);
	Current = { state, collisions, std::move(message) };
}

void remove_download()
{
	std::error_code error;
	if (!DownloadedArchive.empty())
		std::filesystem::remove(DownloadedArchive, error);
}

void install_download(bool allowOverwrite)
{
	set_state(InstallerState::Installing, "Installing HD Interface...");
	spdlog::info("HD Interface: extracting {} to {}", ReleaseAsset, CurrentGameRoot.string());
	const PackageResult result = install_package(DownloadedArchive, CurrentGameRoot, allowOverwrite);
	if (!result.success)
	{
		spdlog::error("HD Interface: extraction failed: {}", result.error);
		set_state(InstallerState::Failed, "Installation failed. Check OutRun2006Tweaks.log for details.");
		remove_download();
		return;
	}

	spdlog::info("HD Interface: installed {} files ({} bytes), wrapper='{}'",
		result.fileCount, result.uncompressedBytes, result.wrapper);
	if (!is_installed(CurrentGameRoot))
	{
		spdlog::error("HD Interface: post-install verification failed");
		set_state(InstallerState::Failed,
			"Installation verification failed. Check OutRun2006Tweaks.log for details.");
		remove_download();
		return;
	}
	spdlog::info("HD Interface: installation verified");
	set_state(InstallerState::Installed, "HD Interface installed. Restart required.");
	InstallCompleted = true;
	remove_download();
}

void run_download()
{
	try
	{
	spdlog::info("HD Interface: source={} release={} asset={} expectedSize={}",
		SourceRepository, ReleaseTag, ReleaseAsset, ReleaseAssetSize);
	if (!Util::HttpDownloadFile(DownloadHost, DownloadPath, DownloadedArchive, 443))
	{
		spdlog::error("HD Interface: download failed for {}/{}", ReleaseTag, ReleaseAsset);
		set_state(InstallerState::Failed, "Download failed. Check your connection and try again.");
		remove_download();
		WorkerRunning = false;
		return;
	}

	std::error_code error;
	const auto downloadedSize = std::filesystem::file_size(DownloadedArchive, error);
	spdlog::info("HD Interface: download completed ({} bytes)", error ? 0 : downloadedSize);
	if (error || downloadedSize != ReleaseAssetSize)
	{
		spdlog::error("HD Interface: unexpected download size (expected {}, got {})",
			ReleaseAssetSize, error ? 0 : downloadedSize);
		set_state(InstallerState::Failed, "The downloaded package could not be validated.");
		remove_download();
		WorkerRunning = false;
		return;
	}
	const std::string digest = sha256_file(DownloadedArchive);
	if (digest != ReleaseAssetSha256)
	{
		spdlog::error("HD Interface: SHA-256 validation failed (expected {}, got {})",
			ReleaseAssetSha256, digest.empty() ? "unavailable" : digest);
		set_state(InstallerState::Failed, "The downloaded package could not be validated.");
		remove_download();
		WorkerRunning = false;
		return;
	}
	spdlog::info("HD Interface: SHA-256 verified: {}", digest);

	const PackageResult inspection = inspect_package(DownloadedArchive, CurrentGameRoot);
	if (!inspection.success)
	{
		spdlog::error("HD Interface: archive validation failed: {}", inspection.error);
		set_state(InstallerState::Failed, "The downloaded package has an unexpected layout.");
		remove_download();
		WorkerRunning = false;
		return;
	}

	spdlog::info("HD Interface: archive valid; files={}, bytes={}, collisions={}, wrapper='{}'",
		inspection.fileCount, inspection.uncompressedBytes, inspection.collisionCount,
		inspection.wrapper);
	if (inspection.collisions)
	{
		set_state(InstallerState::AwaitingOverwrite,
			"Existing interface textures were found. Installing HD Interface may replace those files.",
			inspection.collisionCount);
		WorkerRunning = false;
		return;
	}

	install_download(false);
	WorkerRunning = false;
	}
	catch (const std::exception& exception)
	{
		spdlog::error("HD Interface: unexpected installation failure: {}", exception.what());
		set_state(InstallerState::Failed, "Installation failed. Check OutRun2006Tweaks.log for details.");
		remove_download();
		WorkerRunning = false;
	}
	catch (...)
	{
		spdlog::error("HD Interface: unexpected installation failure");
		set_state(InstallerState::Failed, "Installation failed. Check OutRun2006Tweaks.log for details.");
		remove_download();
		WorkerRunning = false;
	}
}
}

void initialize(const std::filesystem::path& gameRoot)
{
	CurrentGameRoot = gameRoot;
	if (is_installed(gameRoot))
	{
		set_state(InstallerState::Installed);
		spdlog::info("HD Interface: detected existing installation");
	}
	else
	{
		set_state(InstallerState::NotInstalled);
		spdlog::info("HD Interface: no valid installation detected");
	}
}

void refresh_installation_state()
{
	const InstallerSnapshot before = snapshot();
	if (before.state != InstallerState::Installed && before.state != InstallerState::NotInstalled)
		return;

	const bool installed = is_installed(CurrentGameRoot);
	if (installed && before.state == InstallerState::NotInstalled)
	{
		set_state(InstallerState::Installed);
		spdlog::info("HD Interface: detected existing installation");
	}
	else if (!installed && before.state == InstallerState::Installed)
	{
		set_state(InstallerState::NotInstalled);
		spdlog::warn("HD Interface: required installation files are missing");
	}
}

void start_install(const std::filesystem::path& gameRoot)
{
	if (WorkerRunning.exchange(true))
		return;

	CurrentGameRoot = gameRoot;
	std::error_code error;
	const auto tempDirectory = std::filesystem::temp_directory_path(error);
	if (error)
	{
		WorkerRunning = false;
		set_state(InstallerState::Failed, "Could not create a temporary download file.");
		return;
	}
	DownloadedArchive = tempDirectory /
		std::format("HYP36R-HD-Interface-{}-{}.zip", GetCurrentProcessId(), GetTickCount64());
	set_state(InstallerState::Downloading, "Downloading HD Interface...");
	std::thread(run_download).detach();
}

void continue_after_collision()
{
	if (snapshot().state != InstallerState::AwaitingOverwrite || WorkerRunning.exchange(true))
		return;
	std::thread([]
	{
		try
		{
			install_download(true);
		}
		catch (const std::exception& exception)
		{
			spdlog::error("HD Interface: unexpected extraction failure: {}", exception.what());
			set_state(InstallerState::Failed, "Installation failed. Check OutRun2006Tweaks.log for details.");
			remove_download();
		}
		catch (...)
		{
			spdlog::error("HD Interface: unexpected extraction failure");
			set_state(InstallerState::Failed, "Installation failed. Check OutRun2006Tweaks.log for details.");
			remove_download();
		}
		WorkerRunning = false;
	}).detach();
}

void cancel_collision()
{
	if (snapshot().state != InstallerState::AwaitingOverwrite)
		return;
	remove_download();
	set_state(InstallerState::NotInstalled);
}

InstallerSnapshot snapshot()
{
	std::scoped_lock lock(StateMutex);
	return Current;
}

bool consume_completed_install()
{
	return InstallCompleted.exchange(false);
}

void finish_setting_enable(bool success)
{
	if (success)
	{
		spdlog::info("HD Interface: UITextureReplacement enabled and saved");
		return;
	}

	std::error_code error;
	std::filesystem::remove(installation_marker(CurrentGameRoot), error);
	spdlog::error("HD Interface: textures extracted, but UITextureReplacement could not be saved");
	set_state(InstallerState::Failed,
		"Textures were installed, but the HD Interface setting could not be saved.");
}
}
