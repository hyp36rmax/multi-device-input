#include "hd_interface_package.hpp"

#include <miniz.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace
{
struct Entry
{
	std::string name;
	std::string contents;
};

fs::path unique_temp(std::string_view name)
{
	return fs::temp_directory_path() /
		("hyp36r-hd-test-" + std::string(name) + "-" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
}

void make_zip(const fs::path& path, const std::vector<Entry>& entries)
{
	mz_zip_archive zip{};
	assert(mz_zip_writer_init_file(&zip, path.string().c_str(), 0));
	for (const auto& entry : entries)
		assert(mz_zip_writer_add_mem(&zip, entry.name.c_str(), entry.contents.data(),
			entry.contents.size(), MZ_BEST_SPEED));
	assert(mz_zip_writer_finalize_archive(&zip));
	assert(mz_zip_writer_end(&zip));
}

std::vector<Entry> valid_entries(std::string wrapper = {})
{
	return {
		{ wrapper + "textures/load/README.md", "community package" },
		{ wrapper + "textures/load/spr_test/ABC_64x64.dds", "dds" },
	};
}
}

int main()
{
	const fs::path root = unique_temp("root");
	const fs::path zip = unique_temp("valid.zip");
	fs::create_directories(root);
	make_zip(zip, valid_entries());

	// Clean installs create textures/ and textures/load/ from nothing.
	auto result = HdInterface::inspect_package(zip, root);
	assert(result.success && !result.collisions && result.wrapper.empty());
	result = HdInterface::install_package(zip, root, false);
	assert(result.success);
	assert(fs::is_regular_file(root / "textures/load/spr_test/ABC_64x64.dds"));
	assert(fs::is_regular_file(HdInterface::installation_marker(root)));
	assert(HdInterface::is_installed(root));

	// A pre-existing textures/ directory without load/ is also a clean install.
	const fs::path texturesOnlyRoot = unique_temp("textures-only-root");
	fs::create_directories(texturesOnlyRoot / "textures");
	assert(HdInterface::install_package(zip, texturesOnlyRoot, false).success);
	assert(fs::is_regular_file(texturesOnlyRoot / "textures/load/spr_test/ABC_64x64.dds"));

	// Existing unrelated texture content is preserved.
	std::ofstream(root / "textures/load/unrelated.dds") << "keep";
	const fs::path mergeZip = unique_temp("merge.zip");
	make_zip(mergeZip, {{ "textures/load/spr_other/DEF_64x64.dds", "new" }});
	assert(HdInterface::install_package(mergeZip, root, false).success);
	assert(fs::is_regular_file(root / "textures/load/unrelated.dds"));

	// Same-path content asks for confirmation and is not overwritten beforehand.
	result = HdInterface::inspect_package(zip, root);
	assert(result.success && result.collisions && result.collisionCount == 2);
	const auto blocked = HdInterface::install_package(zip, root, false);
	assert(!blocked.success && blocked.collisions);
	assert(HdInterface::install_package(zip, root, true).success);

	// The one known release wrapper is stripped; arbitrary wrappers are rejected.
	const fs::path wrapped = unique_temp("wrapped.zip");
	make_zip(wrapped, valid_entries("OR2-HD-GUI-v0.26.09a/"));
	const fs::path wrappedRoot = unique_temp("wrapped-root");
	fs::create_directories(wrappedRoot);
	result = HdInterface::inspect_package(wrapped, wrappedRoot);
	assert(result.success && result.wrapper == "OR2-HD-GUI-v0.26.09a/");
	assert(HdInterface::install_package(wrapped, wrappedRoot, false).success);
	assert(fs::is_regular_file(wrappedRoot / "textures/load/spr_test/ABC_64x64.dds"));
	const fs::path unknown = unique_temp("unknown.zip");
	make_zip(unknown, valid_entries("SomeOtherRelease/"));
	assert(!HdInterface::inspect_package(unknown, root).success);

	// Invalid ZIPs, unexpected roots and traversal attempts fail closed.
	const fs::path invalid = unique_temp("invalid.zip");
	std::ofstream(invalid) << "not a zip";
	assert(!HdInterface::inspect_package(invalid, root).success);
	const fs::path unexpected = unique_temp("unexpected.zip");
	make_zip(unexpected, {{ "other/file.dds", "no" }});
	assert(!HdInterface::inspect_package(unexpected, root).success);
	const fs::path traversal = unique_temp("traversal.zip");
	make_zip(traversal, {{ "textures/load/../escape.dds", "no" }});
	assert(!HdInterface::inspect_package(traversal, root).success);
	const fs::path unwritableRoot = unique_temp("root-is-file");
	std::ofstream(unwritableRoot) << "not a directory";
	assert(!HdInterface::install_package(zip, unwritableRoot, true).success);

	fs::remove_all(root);
	fs::remove_all(texturesOnlyRoot);
	fs::remove_all(wrappedRoot);
	fs::remove(zip);
	fs::remove(mergeZip);
	fs::remove(wrapped);
	fs::remove(unknown);
	fs::remove(invalid);
	fs::remove(unexpected);
	fs::remove(traversal);
	fs::remove(unwritableRoot);
}
