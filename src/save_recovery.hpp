#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace SaveRecovery
{
	std::string FileSha256(const std::filesystem::path& path);
	enum class Reason { PreUnlockAll };

	struct Point
	{
		std::string id;
		std::string createdUtc;
		std::string reason;
		std::string source;
		int activeLicence = -1;
		bool valid = false;
		std::string error;
	};

	struct Result
	{
		bool ok = false;
		Point point;
		std::string error;
	};

	// The gate must prove OutRun's save writer is idle for the entire synchronous
	// call. No production caller supplies this gate in E3A; a false gate fails closed.
	class Service
	{
	public:
		Service(std::filesystem::path source, std::function<bool()> saveIdleGate);
		Result CreateRestorePoint(Reason reason, int activeLicence = -1) const;
		Point ValidateRestorePoint(const std::string& id) const;
		std::vector<Point> ListRestorePoints() const;
		std::filesystem::path storageRoot() const;
	private:
		std::filesystem::path source_;
		std::function<bool()> saveIdleGate_;
	};
}
