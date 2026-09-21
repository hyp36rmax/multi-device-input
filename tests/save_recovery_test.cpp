#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "save_recovery.hpp"
#include <json/json.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

static void check(bool condition, const char* message)
{
	if (!condition) throw std::runtime_error(message);
}

static void write(const fs::path& path, const std::string& bytes)
{
	fs::create_directories(path.parent_path());
	std::ofstream out(path, std::ios::binary);
	out << bytes;
	check(bool(out), "test fixture write failed");
}

int main()
{
	const auto unique = std::to_string(GetCurrentProcessId()) + "-" +
		std::to_string(GetTickCount64());
	const auto base = fs::temp_directory_path() / ("e3a-recovery-test-" + unique);
	try
	{
		const auto source = base / "_E2ManagedTest" / "SaveGame";
		write(source / "common.dat", "common");
		write(source / "License1.dat", "first");
		write(source / "License2.dat", "second");
		write(source / "rankings.dat", "ranking");
		write(source / "ghosts" / "ghost.dat", "ghost");
		write(source / "empty.dat", "");
		fs::create_directories(source / "empty-directory");
		const auto sourceStamp = fs::last_write_time(source / "License1.dat");
		bool idle = false;
		SaveRecovery::Service service(source, [&] { return idle; });
		check(!service.CreateRestorePoint(SaveRecovery::Reason::PreUnlockAll).ok,
			"unsafe phase accepted");
		check(service.ListRestorePoints().empty(), "unsafe phase published backup");
		int gateChecks = 0;
		SaveRecovery::Service interrupted(source, [&] { return ++gateChecks < 3; });
		check(!interrupted.CreateRestorePoint(SaveRecovery::Reason::PreUnlockAll).ok,
			"interrupted copy accepted");
		check(service.ListRestorePoints().empty(), "interrupted copy published backup");
		idle = true; // Offline disposable fixture: no OutRun process or writer exists.
		const auto first = service.CreateRestorePoint(SaveRecovery::Reason::PreUnlockAll, 0);
		check(first.ok, "first snapshot failed");
		const auto second = service.CreateRestorePoint(SaveRecovery::Reason::PreUnlockAll, 0);
		check(second.ok && first.point.id != second.point.id, "snapshot ID collision");
		check(service.ListRestorePoints().size() == 2, "published snapshot discovery failed");
		check(service.ValidateRestorePoint(first.point.id).valid, "snapshot validation failed");
		check(fs::file_size(source / "empty.dat") == 0, "source empty file changed");
		check(fs::file_size(source / "License1.dat") == 5, "source licence changed");
		check(fs::last_write_time(source / "License1.dat") == sourceStamp,
			"source licence timestamp changed");
		check(fs::exists(service.storageRoot() / first.point.id / "SaveGame" / "empty-directory"),
			"empty directory not copied");
		const auto incomplete = service.storageRoot() / "deadbeef.incomplete";
		fs::create_directory(incomplete);
		check(service.ListRestorePoints().size() == 2, "incomplete point listed");
		{
			std::ifstream in(service.storageRoot() / second.point.id / "metadata.json");
			Json::Value metadata;
			Json::CharReaderBuilder reader;
			std::string errors;
			check(Json::parseFromStream(reader, in, &metadata, &errors), "metadata fixture read failed");
			metadata["schema"] = 999;
			Json::StreamWriterBuilder writer;
			write(service.storageRoot() / second.point.id / "metadata.json", Json::writeString(writer, metadata));
			check(!service.ValidateRestorePoint(second.point.id).valid, "unsupported schema accepted");
			metadata["schema"] = 1;
			write(service.storageRoot() / second.point.id / "metadata.json", Json::writeString(writer, metadata));
		}
		check(service.ValidateRestorePoint(second.point.id).valid, "valid metadata not recovered");
		const auto point = service.storageRoot() / first.point.id;
		write(point / "SaveGame" / "License1.dat", "corrupt");
		check(!service.ValidateRestorePoint(first.point.id).valid, "hash mismatch accepted");
		check(service.ValidateRestorePoint(second.point.id).valid, "other restore point affected");
		fs::remove(point / "SaveGame" / "License2.dat");
		check(!service.ValidateRestorePoint(first.point.id).valid, "missing file accepted");
		write(point / "metadata.json", "not json");
		check(!service.ValidateRestorePoint(first.point.id).valid, "corrupt metadata accepted");
		check(!service.ValidateRestorePoint("../SaveGame").valid, "path traversal accepted");
		check(!service.ValidateRestorePoint(first.point.id + ".incomplete").valid,
			"incomplete point accepted");
		fs::remove(service.storageRoot() / second.point.id / "SaveGame" / "empty-directory");
		check(!service.ValidateRestorePoint(second.point.id).valid, "missing empty directory accepted");
		fs::create_directory(service.storageRoot() / second.point.id / "SaveGame" / "empty-directory");
		write(source / "ghosts" / "ghost.dat", "changed");
		check(service.ValidateRestorePoint(second.point.id).valid,
			"source mutation should not alter published snapshot");
		fs::remove_all(base);
		std::cout << "E3A offline disposable-root tests passed\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "E3A test failed: " << error.what() << '\n';
		// Preserve failed disposable fixture for inspection.
		return 1;
	}
}
