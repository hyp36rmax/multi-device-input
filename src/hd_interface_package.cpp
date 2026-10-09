#include "hd_interface_package.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <optional>
#include <set>
#include <string_view>
#include <vector>

#include <miniz.h>

namespace HdInterface
{
namespace
{
constexpr std::string_view TexturesPrefix = "textures/";
constexpr std::string_view KnownWrapper = "OR2-HD-GUI-v0.26.09a/";
constexpr std::uint64_t MaximumArchiveContents = 8ull * 1024 * 1024 * 1024;
constexpr std::uint64_t MaximumSingleFile = 512ull * 1024 * 1024;

struct PlannedFile
{
	mz_uint index = 0;
	std::filesystem::path relative;
	std::uint64_t size = 0;
};

struct Plan
{
	PackageResult result;
	std::vector<PlannedFile> files;
};

std::string normalize(std::string name)
{
	std::replace(name.begin(), name.end(), '\\', '/');
	while (name.starts_with("./"))
		name.erase(0, 2);
	return name;
}

bool unsafe_path(std::string_view path)
{
	if (path.empty() || path.front() == '/' || path.find(':') != std::string_view::npos)
		return true;

	std::size_t begin = 0;
	while (begin <= path.size())
	{
		const auto end = path.find('/', begin);
		const auto component = path.substr(begin,
			end == std::string_view::npos ? path.size() - begin : end - begin);
		if (component == "..")
			return true;
		if (end == std::string_view::npos)
			break;
		begin = end + 1;
	}
	return false;
}

bool is_directory_name(std::string_view path)
{
	return !path.empty() && path.back() == '/';
}

bool extension_is_dds(const std::filesystem::path& path)
{
	std::string extension = path.extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(),
		[](unsigned char c) { return char(std::tolower(c)); });
	return extension == ".dds";
}

Plan build_plan(mz_zip_archive& zip, const std::filesystem::path& gameRoot)
{
	Plan plan;
	std::set<std::filesystem::path> destinations;
	bool hasLoadDirectory = false;
	bool hasDds = false;
	std::optional<std::string> selectedWrapper;

	const mz_uint count = mz_zip_reader_get_num_files(&zip);
	for (mz_uint index = 0; index < count; ++index)
	{
		mz_zip_archive_file_stat stat{};
		if (!mz_zip_reader_file_stat(&zip, index, &stat))
		{
			plan.result.error = "Could not read an archive entry.";
			return plan;
		}

		std::string name = normalize(stat.m_filename);
		if (unsafe_path(name))
		{
			plan.result.error = "The archive contains an unsafe path.";
			return plan;
		}

		std::string wrapper;
		std::string relative;
		if (name.starts_with(TexturesPrefix))
			relative = name;
		else if (name.starts_with(KnownWrapper) &&
			std::string_view(name).substr(KnownWrapper.size()).starts_with(TexturesPrefix))
		{
			wrapper = std::string(KnownWrapper);
			relative = name.substr(KnownWrapper.size());
		}
		else
		{
			// The release includes a root Readme.txt. It is attribution/documentation,
			// not game content, so only the authoritative textures tree is installed.
			if (name == "Readme.txt" || name == std::string(KnownWrapper) + "Readme.txt")
				continue;
			plan.result.error = "The archive has an unexpected root layout.";
			return plan;
		}

		if (!selectedWrapper)
			selectedWrapper = wrapper;
		else if (*selectedWrapper != wrapper)
		{
			plan.result.error = "The archive mixes wrapped and unwrapped texture paths.";
			return plan;
		}

		if (relative == "textures/load/" || relative.starts_with("textures/load/"))
			hasLoadDirectory = true;
		if (is_directory_name(relative))
			continue;
		if (!std::string_view(relative).starts_with("textures/load/"))
		{
			plan.result.error = "The archive contains files outside textures/load.";
			return plan;
		}
		if (stat.m_uncomp_size > MaximumSingleFile ||
			plan.result.uncompressedBytes > MaximumArchiveContents - stat.m_uncomp_size)
		{
			plan.result.error = "The archive expands beyond the supported safety limit.";
			return plan;
		}

		const std::filesystem::path destination = std::filesystem::path(relative).lexically_normal();
		if (!destinations.emplace(destination).second)
		{
			plan.result.error = "The archive contains duplicate destination paths.";
			return plan;
		}

		plan.files.push_back({ index, destination, stat.m_uncomp_size });
		plan.result.uncompressedBytes += stat.m_uncomp_size;
		hasDds = hasDds || extension_is_dds(destination);
		std::error_code existenceError;
		const bool exists = std::filesystem::exists(gameRoot / destination, existenceError);
		if (existenceError)
		{
			plan.result.error = "Could not inspect the destination texture folder.";
			return plan;
		}
		if (exists)
			++plan.result.collisionCount;
	}

	if (!hasLoadDirectory || !hasDds || plan.files.empty())
	{
		plan.result.error = "The archive does not contain the expected textures/load package.";
		return plan;
	}

	plan.result.success = true;
	plan.result.collisions = plan.result.collisionCount != 0;
	plan.result.fileCount = plan.files.size();
	plan.result.wrapper = selectedWrapper.value_or("");
	return plan;
}

bool replace_file(const std::filesystem::path& source, const std::filesystem::path& destination,
	std::error_code& error)
{
	const bool exists = std::filesystem::exists(destination, error);
	if (error)
		return false;
	if (exists)
		std::filesystem::remove(destination, error);
	if (error)
		return false;
	std::filesystem::rename(source, destination, error);
	return !error;
}
}

std::filesystem::path installation_marker(const std::filesystem::path& gameRoot)
{
	return gameRoot / "textures" / "load" / ".hyp36r-hd-interface-v0.26.09a";
}

bool is_installed(const std::filesystem::path& gameRoot)
{
	std::error_code error;
	// The marker records installer provenance, but is not enough by itself: a
	// player may later remove part of the package. These two package-owned files
	// are the inexpensive structural signature already used to recognize manual
	// installs of this exact community compilation.
	const bool readme = std::filesystem::is_regular_file(gameRoot / "textures/load/README.md", error);
	if (error || !readme)
		return false;
	return std::filesystem::is_regular_file(gameRoot /
		"textures/load/spr_sprani_sumo_vsload_Exst/0_24B1E81A_128x128.dds", error) && !error;
}

PackageResult inspect_package(const std::filesystem::path& archive,
	const std::filesystem::path& gameRoot)
{
	mz_zip_archive zip{};
	if (!mz_zip_reader_init_file(&zip, archive.string().c_str(), 0))
		return { .error = "The downloaded file is not a readable ZIP archive." };

	Plan plan = build_plan(zip, gameRoot);
	mz_zip_reader_end(&zip);
	return plan.result;
}

PackageResult install_package(const std::filesystem::path& archive,
	const std::filesystem::path& gameRoot, bool allowOverwrite)
{
	mz_zip_archive zip{};
	if (!mz_zip_reader_init_file(&zip, archive.string().c_str(), 0))
		return { .error = "The downloaded file is not a readable ZIP archive." };

	Plan plan = build_plan(zip, gameRoot);
	if (!plan.result.success || (plan.result.collisions && !allowOverwrite))
	{
		mz_zip_reader_end(&zip);
		if (plan.result.collisions && !allowOverwrite)
			plan.result.success = false;
		return plan.result;
	}

	std::error_code error;
	const auto space = std::filesystem::space(gameRoot, error);
	if (error || space.available < plan.result.uncompressedBytes)
	{
		mz_zip_reader_end(&zip);
		plan.result.success = false;
		plan.result.error = error ? "Could not check free disk space." : "Not enough free disk space.";
		return plan.result;
	}

	for (const PlannedFile& file : plan.files)
	{
		const auto destination = gameRoot / file.relative;
		const std::filesystem::path temporary = destination.wstring() + L".hyp36r-installing";
		std::filesystem::create_directories(destination.parent_path(), error);
		size_t extractedSize = 0;
		void* extracted = error ? nullptr : mz_zip_reader_extract_to_heap(&zip, file.index,
			&extractedSize, 0);
		bool written = extracted != nullptr && extractedSize == file.size;
		if (written)
		{
			std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
			output.write(static_cast<const char*>(extracted),
				static_cast<std::streamsize>(extractedSize));
			written = bool(output);
		}
		mz_free(extracted);
		if (!written || !replace_file(temporary, destination, error))
		{
			std::filesystem::remove(temporary, error);
			mz_zip_reader_end(&zip);
			plan.result.success = false;
			plan.result.error = "Could not write the texture package to the game folder.";
			return plan.result;
		}
	}
	mz_zip_reader_end(&zip);

	std::ofstream marker(installation_marker(gameRoot), std::ios::trunc);
	marker << "source=" << SourceRepository << '\n'
		<< "release=" << ReleaseTag << '\n'
		<< "asset=" << ReleaseAsset << '\n'
		<< "sha256=" << ReleaseAssetSha256 << '\n';
	if (!marker)
	{
		plan.result.success = false;
		plan.result.error = "Textures were extracted, but installation could not be finalized.";
	}
	return plan.result;
}
}
