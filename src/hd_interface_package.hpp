#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace HdInterface
{
inline constexpr char SourceRepository[] = "https://github.com/envido32/OR2006Sprites";
inline constexpr char ReleaseTag[] = "v0.26.09a";
inline constexpr char ReleaseAsset[] = "OR2-HD-GUI-v0.26.09a.zip";
inline constexpr char DownloadHost[] = "github.com";
inline constexpr wchar_t DownloadPath[] = L"/envido32/OR2006Sprites/releases/download/v0.26.09a/OR2-HD-GUI-v0.26.09a.zip";
inline constexpr std::uint64_t ReleaseAssetSize = 325966152;
inline constexpr char ReleaseAssetSha256[] = "6bc9d6037b635d9e643f1a95cb82ccf98c90feb51da0193e0c51e6b700087878";

struct PackageResult
{
	bool success = false;
	bool collisions = false;
	std::size_t fileCount = 0;
	std::size_t collisionCount = 0;
	std::uint64_t uncompressedBytes = 0;
	std::string wrapper;
	std::string error;
};

PackageResult inspect_package(const std::filesystem::path& archive,
	const std::filesystem::path& gameRoot);
PackageResult install_package(const std::filesystem::path& archive,
	const std::filesystem::path& gameRoot, bool allowOverwrite);
bool is_installed(const std::filesystem::path& gameRoot);
std::filesystem::path installation_marker(const std::filesystem::path& gameRoot);
}
