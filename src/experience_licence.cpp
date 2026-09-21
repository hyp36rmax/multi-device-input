#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "experience_licence.hpp"
#include "resource.h"

#include <json/json.h>
#include <array>
#include <cstring>
#include <fstream>
#include <stdexcept>

namespace ExperienceLicence
{
	namespace fs = std::filesystem;
	namespace
	{
		constexpr char SupportedExe[] = "68ceb386829066f8455b9d027320af962584321f3e2e8a79c72841495a6134c3";
		constexpr size_t PayloadSize = 0x40c;
		constexpr size_t FileSize = PayloadSize + 4;
		constexpr size_t OccupiedFlagOffset = 4 + 0x3f4;
		constexpr size_t CommonPayloadSize = 0x10be4;

		fs::path slotPath(const fs::path& root, int slot)
		{
			return root / ("License" + std::to_string(slot + 1) + ".dat");
		}

		std::array<unsigned char, FileSize> readLicence(const fs::path& path)
		{
			if (fs::is_symlink(fs::symlink_status(path)) || fs::file_size(path) != FileSize)
				throw std::runtime_error("licence file is redirected or has the wrong size");
			std::array<unsigned char, FileSize> bytes{};
			std::ifstream input(path, std::ios::binary);
			if (!input.read(reinterpret_cast<char*>(bytes.data()), bytes.size()))
				throw std::runtime_error("licence file cannot be read");
			const unsigned length = unsigned(bytes[0]) | (unsigned(bytes[1]) << 8) |
				(unsigned(bytes[2]) << 16) | (unsigned(bytes[3]) << 24);
			if (length != PayloadSize || !(bytes[OccupiedFlagOffset] & 1))
				throw std::runtime_error("licence header or native occupancy bit is invalid");
			return bytes;
		}

		Json::Value loadMetadata(const fs::path& path)
		{
			std::ifstream input(path, std::ios::binary);
			if (!input || fs::is_symlink(fs::symlink_status(path)) || fs::file_size(path) > 65536)
				throw std::runtime_error("Experience metadata is missing, redirected or oversized");
			Json::Value root;
			Json::CharReaderBuilder reader;
			std::string errors;
			if (!Json::parseFromStream(reader, input, &root, &errors) || !root.isObject())
				throw std::runtime_error("Experience metadata is corrupt");
			return root;
		}

		void createFileNoOverwrite(const fs::path& path, const void* bytes, size_t length)
		{
			HANDLE handle = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
				FILE_ATTRIBUTE_NORMAL, nullptr);
			if (handle == INVALID_HANDLE_VALUE) throw std::runtime_error("destination appeared or cannot be created");
			DWORD written = 0;
			const bool ok = WriteFile(handle, bytes, static_cast<DWORD>(length), &written, nullptr) &&
				written == length && FlushFileBuffers(handle);
			CloseHandle(handle);
			if (!ok) throw std::runtime_error("destination write or flush failed");
		}
	}

	Service::Service(fs::path exe, fs::path saveGame, std::function<bool()> gameClosedGate)
		: exe_(std::move(exe)), saveGame_(std::move(saveGame)), gameClosedGate_(std::move(gameClosedGate)) {}

	int Service::GetActiveLicence() const
	{
		const auto path = saveGame_ / "common.dat";
		if (fs::is_symlink(fs::symlink_status(path)) || fs::file_size(path) != CommonPayloadSize + 4)
			throw std::runtime_error("common.dat is redirected or has the wrong size");
		std::array<unsigned char, 8> bytes{};
		std::ifstream input(path, std::ios::binary);
		if (!input.read(reinterpret_cast<char*>(bytes.data()), bytes.size()))
			throw std::runtime_error("common.dat cannot be read");
		const auto length = unsigned(bytes[0]) | (unsigned(bytes[1]) << 8) |
			(unsigned(bytes[2]) << 16) | (unsigned(bytes[3]) << 24);
		if (length != CommonPayloadSize) throw std::runtime_error("common.dat header is invalid");
		const auto index = unsigned(bytes[4]) | (unsigned(bytes[5]) << 8) |
			(unsigned(bytes[6]) << 16) | (unsigned(bytes[7]) << 24);
		if (index > 3) throw std::runtime_error("no valid active native licence");
		return static_cast<int>(index);
	}

	Result Service::CloneActiveLicenceToFreeSlot(int activeSlot) const
	{
		Result result;
		result.sourceSlot = activeSlot;
		try
		{
			if (!gameClosedGate_ || !gameClosedGate_())
				return { Code::UnsafePhase, activeSlot, -1, {}, "game-closed phase not established" };
			if (SaveRecovery::FileSha256(exe_) != SupportedExe)
				return { Code::UnsupportedExecutable, activeSlot, -1, {}, "unsupported OutRun executable" };
			if (activeSlot < 0 || activeSlot > 3)
				return { Code::SourceInvalid, activeSlot, -1, {}, "invalid active licence index" };
			try
			{
				if (GetActiveLicence() != activeSlot)
					return { Code::SourceInvalid, activeSlot, -1, {}, "requested slot is not the native selected licence" };
			}
			catch (const std::exception& error)
				{ return { Code::SourceInvalid, activeSlot, -1, {}, error.what() }; }
			const auto source = slotPath(saveGame_, activeSlot);
			std::array<unsigned char, FileSize> original{};
			try { original = readLicence(source); }
			catch (const std::exception& error)
				{ return { Code::SourceInvalid, activeSlot, -1, {}, error.what() }; }
			const auto sourceHash = SaveRecovery::FileSha256(source);
			const auto metadataDir = saveGame_.parent_path() / "MultiInput" / "ExperienceLicences";
			const auto metadataPath = metadataDir / "metadata.json";
			if (fs::exists(metadataPath))
			{
				try
				{
					const auto metadata = loadMetadata(metadataPath);
					const int destination = metadata["destinationSlot"].asInt();
					if (metadata["schema"].asInt() != 1 || destination < 0 || destination > 3 ||
						!metadata["destinationHash"].isString() || !fs::exists(slotPath(saveGame_, destination)) ||
						SaveRecovery::FileSha256(slotPath(saveGame_, destination)) != metadata["destinationHash"].asString())
						return { Code::StaleManagedSlot, activeSlot, destination, {}, "managed slot was replaced or changed" };
					return { Code::Reused, activeSlot, destination, metadata["restorePointId"].asString(), {} };
				}
				catch (const std::exception& error)
					{ return { Code::StaleManagedSlot, activeSlot, -1, {}, error.what() }; }
			}
			int freeSlot = -1;
			for (int slot = 0; slot < 4; ++slot)
				if (slot != activeSlot && !fs::exists(slotPath(saveGame_, slot))) { freeSlot = slot; break; }
			if (freeSlot < 0) return { Code::NoFreeSlot, activeSlot, -1, {}, "all native licence slots are occupied" };
			if (!gameClosedGate_()) return { Code::UnsafePhase, activeSlot, freeSlot, {}, "game-closed phase ended" };
			SaveRecovery::Service recovery(saveGame_, gameClosedGate_);
			const auto point = recovery.CreateRestorePoint(SaveRecovery::Reason::PreUnlockAll, activeSlot);
			if (!point.ok || !recovery.ValidateRestorePoint(point.point.id).valid)
				return { Code::BackupFailed, activeSlot, freeSlot, {}, point.error.empty() ? "restore-point validation failed" : point.error };
			result.restorePointId = point.point.id;
			result.destinationSlot = freeSlot;
			if (!gameClosedGate_() || SaveRecovery::FileSha256(source) != sourceHash ||
				fs::exists(slotPath(saveGame_, freeSlot)))
				return { Code::DestinationChanged, activeSlot, freeSlot, result.restorePointId,
					"source changed or destination was occupied after backup" };
			const auto destinationPath = slotPath(saveGame_, freeSlot);
			createFileNoOverwrite(destinationPath, original.data(), original.size());
			if (SaveRecovery::FileSha256(source) != sourceHash ||
				SaveRecovery::FileSha256(destinationPath) != sourceHash ||
				readLicence(destinationPath) != original)
				return { Code::ValidationFailed, activeSlot, freeSlot, result.restorePointId,
					"source or destination failed byte-level validation" };
			fs::create_directories(metadataDir);
			Json::Value metadata;
			metadata["schema"] = 1;
			metadata["sourceSlot"] = activeSlot;
			metadata["destinationSlot"] = freeSlot;
			metadata["sourceHash"] = sourceHash;
			metadata["destinationHash"] = sourceHash;
			metadata["restorePointId"] = result.restorePointId;
			metadata["build"] = MODULE_VERSION_STR;
			Json::StreamWriterBuilder writer;
			const auto serialized = Json::writeString(writer, metadata);
			createFileNoOverwrite(metadataPath, serialized.data(), serialized.size());
			result.code = Code::Created;
			return result;
		}
		catch (const std::exception& error)
		{
			result.code = Code::IoFailure;
			result.error = error.what();
			return result;
		}
	}
}
