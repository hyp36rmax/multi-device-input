#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "experience_licence.hpp"
#include "save_recovery.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;
static void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }

static void licence(const fs::path& path, char name)
{
	std::array<unsigned char, 0x410> bytes{};
	bytes[0] = 0x0c; bytes[1] = 0x04;
	bytes[4] = static_cast<unsigned char>(name);
	bytes[4 + 0x3f4] = 1; // Native licence occupancy bit.
	std::ofstream out(path, std::ios::binary);
	out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
	check(bool(out), "fixture licence write failed");
}

static void common(const fs::path& path)
{
	std::array<unsigned char, 0x10be8> bytes{};
	bytes[0] = 0xe4; bytes[1] = 0x0b; bytes[2] = 0x01;
	// First payload dword is native selected licence index 0.
	std::ofstream out(path, std::ios::binary);
	out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
	check(bool(out), "fixture common write failed");
}

int main(int argc, char** argv)
{
	if (argc != 2) { std::cerr << "Expected supported OR2006C2C.exe path\n"; return 2; }
	const auto exe = fs::path(argv[1]);
	const auto base = fs::temp_directory_path() /
		("e3b-licence-test-" + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(GetTickCount64()));
	try
	{
		check(SaveRecovery::FileSha256(exe) ==
			"68ceb386829066f8455b9d027320af962584321f3e2e8a79c72841495a6134c3",
			"CI executable differs from validated E2 executable");
		const auto root = base / "_E2ManagedTest" / "SaveGame";
		fs::create_directories(root);
		common(root / "common.dat");
		licence(root / "License1.dat", 'A');
		bool closed = false;
		ExperienceLicence::Service service(exe, root, [&] { return closed; });
		check(service.CloneActiveLicenceToFreeSlot(0).code == ExperienceLicence::Code::UnsafePhase,
			"game-open phase accepted");
		check(!fs::exists(root / "License2.dat"), "unsafe phase wrote a licence");
		closed = true; // Offline disposable fixture: no game process exists.
		const auto sourceHash = SaveRecovery::FileSha256(root / "License1.dat");
		const auto created = service.CloneActiveLicenceToFreeSlot(0);
		check(created.code == ExperienceLicence::Code::Created && created.destinationSlot == 1,
			"native-slot clone failed");
		check(!created.restorePointId.empty(), "backup precondition was skipped");
		SaveRecovery::Service recovery(root, [] { return true; });
		check(recovery.ValidateRestorePoint(created.restorePointId).valid, "pre-clone backup invalid");
		check(SaveRecovery::FileSha256(root / "License1.dat") == sourceHash,
			"source licence changed during clone");
		check(SaveRecovery::FileSha256(root / "License2.dat") == sourceHash,
			"destination differs from source");
		check(service.CloneActiveLicenceToFreeSlot(0).code == ExperienceLicence::Code::Reused,
			"matching managed slot not reused");
		licence(root / "License2.dat", 'B');
		check(SaveRecovery::FileSha256(root / "License1.dat") == sourceHash,
			"destination edit changed source");
		check(service.CloneActiveLicenceToFreeSlot(0).code == ExperienceLicence::Code::StaleManagedSlot,
			"changed managed slot was claimed as original clone");
		const auto full = base / "full" / "SaveGame";
		fs::create_directories(full);
		common(full / "common.dat");
		for (int i = 0; i < 4; ++i) licence(full / ("License" + std::to_string(i + 1) + ".dat"), 'A' + i);
		ExperienceLicence::Service fullService(exe, full, [] { return true; });
		check(fullService.CloneActiveLicenceToFreeSlot(0).code == ExperienceLicence::Code::NoFreeSlot,
			"all four occupied slots not rejected");
		check(!fs::exists(full.parent_path() / "MultiInput"), "full-slot case created a backup");
		const auto invalidExe = base / "wrong.exe";
		{ std::ofstream out(invalidExe, std::ios::binary); out << "wrong"; }
		ExperienceLicence::Service wrong(invalidExe, full, [] { return true; });
		check(wrong.CloneActiveLicenceToFreeSlot(0).code == ExperienceLicence::Code::UnsupportedExecutable,
			"unknown executable accepted");
		fs::remove_all(base);
		std::cout << "E3B disposable native-slot tests passed\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "E3B test failed: " << error.what() << '\n';
		return 1;
	}
}
