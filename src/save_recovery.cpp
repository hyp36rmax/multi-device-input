#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <wincrypt.h>

#include "save_recovery.hpp"
#include "resource.h"

#include <json/json.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace SaveRecovery
{
	namespace fs = std::filesystem;
	namespace
	{
		constexpr int Schema = 1;
		const char* reasonName(Reason reason)
		{
			switch (reason) { case Reason::PreUnlockAll: return "PRE_UNLOCK_ALL"; }
			throw std::runtime_error("unknown restore-point reason");
		}

		class Crypto
		{
		public:
			HCRYPTPROV provider = 0;
			Crypto()
			{
				if (!CryptAcquireContextW(&provider, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
					throw std::runtime_error("cannot initialize SHA-256 provider");
			}
			~Crypto() { CryptReleaseContext(provider, 0); }
			Crypto(const Crypto&) = delete;
			Crypto& operator=(const Crypto&) = delete;
		};

		std::string hex(const BYTE* data, size_t length)
		{
			std::ostringstream stream;
			stream << std::hex << std::setfill('0');
			for (size_t i = 0; i < length; ++i) stream << std::setw(2) << unsigned(data[i]);
			return stream.str();
		}

		std::string hashFile(const fs::path& path, Crypto& crypto)
		{
			std::ifstream input(path, std::ios::binary);
			if (!input) throw std::runtime_error("cannot read snapshot file");
			HCRYPTHASH hash = 0;
			if (!CryptCreateHash(crypto.provider, CALG_SHA_256, 0, 0, &hash))
				throw std::runtime_error("cannot create SHA-256 hash");
			try
			{
				std::array<char, 65536> buffer{};
				while (input.read(buffer.data(), buffer.size()) || input.gcount())
				{
					if (!CryptHashData(hash, reinterpret_cast<const BYTE*>(buffer.data()),
						static_cast<DWORD>(input.gcount()), 0))
						throw std::runtime_error("SHA-256 update failed");
				}
				if (input.bad()) throw std::runtime_error("snapshot file read failed");
				std::array<BYTE, 32> digest{};
				DWORD length = static_cast<DWORD>(digest.size());
				if (!CryptGetHashParam(hash, HP_HASHVAL, digest.data(), &length, 0) || length != digest.size())
					throw std::runtime_error("SHA-256 finalization failed");
				CryptDestroyHash(hash);
				return hex(digest.data(), digest.size());
			}
			catch (...) { CryptDestroyHash(hash); throw; }
		}

		bool safeId(const std::string& id)
		{
			return !id.empty() && id.size() <= 80 && std::all_of(id.begin(), id.end(), [](unsigned char c) {
				return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || c == '-';
			});
		}

		std::string utcNow()
		{
			SYSTEMTIME now{};
			GetSystemTime(&now);
			char result[40]{};
			sprintf_s(result, "%04u-%02u-%02uT%02u:%02u:%02u.%03uZ", now.wYear, now.wMonth,
				now.wDay, now.wHour, now.wMinute, now.wSecond, now.wMilliseconds);
			return result;
		}

		std::string newId(Crypto& crypto)
		{
			std::array<BYTE, 16> random{};
			if (!CryptGenRandom(crypto.provider, static_cast<DWORD>(random.size()), random.data()))
				throw std::runtime_error("cannot generate restore-point ID");
			SYSTEMTIME now{};
			GetSystemTime(&now);
			char timestamp[32]{};
			sprintf_s(timestamp, "%04u%02u%02u-%02u%02u%02u-%03u", now.wYear, now.wMonth,
				now.wDay, now.wHour, now.wMinute, now.wSecond, now.wMilliseconds);
			return std::string(timestamp) + "-" + hex(random.data(), random.size());
		}

		std::string relativeName(const fs::path& relative)
		{
			if (relative.empty() || relative.is_absolute()) throw std::runtime_error("unsafe relative path");
			for (auto& segment : relative)
				if (segment == ".." || segment == "." || segment.empty())
					throw std::runtime_error("unsafe relative path");
			return relative.generic_string();
		}

		Json::Value inventory(const fs::path& root, Crypto& crypto)
		{
			if (!fs::is_directory(root) || fs::is_symlink(fs::symlink_status(root)))
				throw std::runtime_error("SaveGame root is missing or redirected");
			Json::Value files(Json::arrayValue);
			for (const auto& entry : fs::recursive_directory_iterator(root))
			{
				const auto status = entry.symlink_status();
				if (fs::is_symlink(status) || (!fs::is_directory(status) && !fs::is_regular_file(status)))
					throw std::runtime_error("unsupported redirected or special save entry");
				if (!fs::is_regular_file(status)) continue;
				Json::Value file;
				file["path"] = relativeName(entry.path().lexically_relative(root));
				file["size"] = Json::UInt64(entry.file_size());
				file["sha256"] = hashFile(entry.path(), crypto);
				files.append(file);
			}
			return files;
		}

		std::set<std::string> directories(const fs::path& root)
		{
			if (!fs::is_directory(root) || fs::is_symlink(fs::symlink_status(root)))
				throw std::runtime_error("SaveGame root is missing or redirected");
			std::set<std::string> names;
			for (const auto& entry : fs::recursive_directory_iterator(root))
			{
				const auto status = entry.symlink_status();
				if (fs::is_symlink(status) || (!fs::is_directory(status) && !fs::is_regular_file(status)))
					throw std::runtime_error("unsupported redirected or special save entry");
				if (fs::is_directory(status))
					names.insert(relativeName(entry.path().lexically_relative(root)));
			}
			return names;
		}

		Json::Value directoryRecords(const std::set<std::string>& names)
		{
			Json::Value result(Json::arrayValue);
			for (const auto& name : names) result.append(name);
			return result;
		}

		std::set<std::string> normalizedDirectories(const Json::Value& records)
		{
			if (!records.isArray()) throw std::runtime_error("invalid directory inventory");
			std::set<std::string> result;
			for (const auto& item : records)
			{
				if (!item.isString()) throw std::runtime_error("invalid directory record");
				const auto name = item.asString();
				if (name.find('\\') != std::string::npos || name.find(':') != std::string::npos ||
					relativeName(fs::path(name)) != name || !result.insert(name).second)
					throw std::runtime_error("unsafe or duplicate directory record");
			}
			return result;
		}

		std::map<std::string, std::pair<Json::UInt64, std::string>> normalized(const Json::Value& files)
		{
			if (!files.isArray()) throw std::runtime_error("invalid file inventory");
			std::map<std::string, std::pair<Json::UInt64, std::string>> result;
			for (const auto& file : files)
			{
				if (!file.isObject() || !file["path"].isString() || !file["size"].isUInt64() || !file["sha256"].isString())
					throw std::runtime_error("invalid file record");
				const auto name = file["path"].asString();
				if (name.find('\\') != std::string::npos || relativeName(fs::path(name)) != name || name.find(':') != std::string::npos)
					throw std::runtime_error("unsafe file record path");
				const auto digest = file["sha256"].asString();
				if (digest.size() != 64 || !std::all_of(digest.begin(), digest.end(), [](char c) {
					return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
				})) throw std::runtime_error("invalid file hash");
				if (!result.emplace(name, std::make_pair(file["size"].asUInt64(), digest)).second)
					throw std::runtime_error("duplicate file record");
			}
			return result;
		}

		Point validateDirectory(const fs::path& directory, const std::string& id,
			const fs::path& expectedSource, Crypto& crypto)
		{
			Point point;
			point.id = id;
			try
			{
				if (!safeId(id) || fs::is_symlink(fs::symlink_status(directory)) || !fs::is_directory(directory))
					throw std::runtime_error("restore point is missing or redirected");
				if (fs::is_symlink(fs::symlink_status(directory / "metadata.json")))
					throw std::runtime_error("metadata is redirected");
				std::ifstream file(directory / "metadata.json", std::ios::binary);
				if (!file || fs::file_size(directory / "metadata.json") > 4 * 1024 * 1024)
					throw std::runtime_error("metadata is missing or too large");
				Json::Value metadata;
				Json::CharReaderBuilder reader;
				std::string errors;
				if (!Json::parseFromStream(reader, file, &metadata, &errors) || !metadata.isObject())
					throw std::runtime_error("metadata is corrupt");
				if (!metadata["schema"].isInt() || metadata["schema"].asInt() != Schema ||
					!metadata["id"].isString() || metadata["id"].asString() != id ||
					!metadata["createdUtc"].isString() || !metadata["reason"].isString() ||
					metadata["reason"].asString() != reasonName(Reason::PreUnlockAll) ||
					!metadata["source"].isString() || !metadata["activeLicence"].isInt())
					throw std::runtime_error("unsupported or incomplete metadata");
				point.createdUtc = metadata["createdUtc"].asString();
				point.reason = metadata["reason"].asString();
				point.source = metadata["source"].asString();
				if (point.source != fs::weakly_canonical(expectedSource).string())
					throw std::runtime_error("restore point belongs to another SaveGame root");
				point.activeLicence = metadata["activeLicence"].asInt();
				if (point.activeLicence < -1 || point.activeLicence > 3)
					throw std::runtime_error("invalid licence index");
				if (normalized(metadata["files"]) != normalized(inventory(directory / "SaveGame", crypto)))
					throw std::runtime_error("snapshot file inventory or hashes differ");
				if (normalizedDirectories(metadata["directories"]) != directories(directory / "SaveGame"))
					throw std::runtime_error("snapshot directory inventory differs");
				point.valid = true;
			}
			catch (const std::exception& error) { point.error = error.what(); }
			return point;
		}
	}

	Service::Service(fs::path source, std::function<bool()> saveIdleGate)
		: source_(std::move(source)), saveIdleGate_(std::move(saveIdleGate)) {}

	std::string FileSha256(const fs::path& path)
	{
		Crypto crypto;
		return hashFile(path, crypto);
	}

	fs::path Service::storageRoot() const
	{
		return source_.parent_path() / "MultiInput" / "SaveRecovery";
	}

	Result Service::CreateRestorePoint(Reason reason, int activeLicence) const
	{
		Result result;
		fs::path temporary;
		try
		{
			if (!saveIdleGate_ || !saveIdleGate_()) throw std::runtime_error("safe save-idle phase not established");
			if (activeLicence < -1 || activeLicence > 3) throw std::runtime_error("invalid active licence");
			Crypto crypto;
			const auto before = normalized(inventory(source_, crypto));
			const auto beforeDirectories = directories(source_);
			if (!saveIdleGate_()) throw std::runtime_error("save-idle phase ended");
			const auto storage = storageRoot();
			fs::create_directories(storage);
			const auto id = newId(crypto);
			temporary = storage / (id + ".incomplete");
			const auto published = storage / id;
			if (fs::exists(temporary) || fs::exists(published)) throw std::runtime_error("restore-point ID collision");
			fs::create_directory(temporary);
			fs::copy(source_, temporary / "SaveGame", fs::copy_options::recursive);
			if (!saveIdleGate_() || before != normalized(inventory(source_, crypto)) ||
				before != normalized(inventory(temporary / "SaveGame", crypto)) ||
				beforeDirectories != directories(source_) ||
				beforeDirectories != directories(temporary / "SaveGame"))
				throw std::runtime_error("source changed or snapshot copy differs");
			Json::Value metadata;
			metadata["schema"] = Schema;
			metadata["id"] = id;
			metadata["createdUtc"] = utcNow();
			metadata["reason"] = reasonName(reason);
			metadata["source"] = fs::weakly_canonical(source_).string();
			metadata["activeLicence"] = activeLicence;
			metadata["build"] = MODULE_VERSION_STR;
			metadata["files"] = inventory(temporary / "SaveGame", crypto);
			metadata["directories"] = directoryRecords(beforeDirectories);
			std::ofstream out(temporary / "metadata.json", std::ios::binary | std::ios::trunc);
			if (!out) throw std::runtime_error("cannot write restore-point metadata");
			Json::StreamWriterBuilder writer;
			out << Json::writeString(writer, metadata);
			out.close();
			if (!out) throw std::runtime_error("cannot finish restore-point metadata");
			HANDLE metadataHandle = CreateFileW((temporary / "metadata.json").c_str(), GENERIC_WRITE,
				FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (metadataHandle == INVALID_HANDLE_VALUE)
				throw std::runtime_error("cannot reopen restore-point metadata for flush");
			const bool flushed = FlushFileBuffers(metadataHandle) != 0;
			CloseHandle(metadataHandle);
			if (!flushed) throw std::runtime_error("cannot flush restore-point metadata");
			const auto validated = validateDirectory(temporary, id, source_, crypto);
			if (!validated.valid) throw std::runtime_error(validated.error);
			if (!saveIdleGate_() || before != normalized(inventory(source_, crypto)) ||
				beforeDirectories != directories(source_))
				throw std::runtime_error("source changed before publication");
			fs::rename(temporary, published);
			temporary.clear();
			result.point = validateDirectory(published, id, source_, crypto);
			result.ok = result.point.valid;
			if (!result.ok) result.error = result.point.error;
		}
		catch (const std::exception& error)
		{
			result.error = error.what();
			// Leave an incomplete directory for diagnostics; it is never listed.
		}
		return result;
	}

	Point Service::ValidateRestorePoint(const std::string& id) const
	{
		if (!safeId(id)) return Point{ id, {}, {}, {}, -1, false, "invalid restore-point ID" };
		try { Crypto crypto; return validateDirectory(storageRoot() / id, id, source_, crypto); }
		catch (const std::exception& error) { return Point{ id, {}, {}, {}, -1, false, error.what() }; }
	}

	std::vector<Point> Service::ListRestorePoints() const
	{
		std::vector<Point> points;
		try
		{
			if (!fs::exists(storageRoot())) return points;
			for (const auto& entry : fs::directory_iterator(storageRoot()))
			{
				const auto id = entry.path().filename().string();
				if (safeId(id) && entry.is_directory() && !entry.is_symlink())
					points.push_back(ValidateRestorePoint(id));
			}
			std::sort(points.begin(), points.end(), [](const Point& a, const Point& b) {
				return a.id > b.id;
			});
		}
		catch (const std::exception&) { /* Do not expose incomplete discovery as valid. */ }
		return points;
	}
}
