#pragma once

#include "save_recovery.hpp"
#include <filesystem>
#include <functional>
#include <string>

namespace ExperienceLicence
{
	enum class Code
	{
		Created, Reused, UnsafePhase, UnsupportedExecutable, SourceInvalid,
		NoFreeSlot, BackupFailed, DestinationChanged, ValidationFailed,
		StaleManagedSlot, IoFailure
	};

	struct Result
	{
		Code code = Code::IoFailure;
		int sourceSlot = -1;
		int destinationSlot = -1;
		std::string restorePointId;
		std::string error;
		bool success() const { return code == Code::Created || code == Code::Reused; }
	};

	// E3B's safe offline transaction. The gate must attest that OutRun is not
	// running and cannot write SaveGame for the full synchronous call.
	class Service
	{
	public:
		Service(std::filesystem::path exe, std::filesystem::path saveGame,
			std::function<bool()> gameClosedGate);
		int GetActiveLicence() const;
		Result CloneActiveLicenceToFreeSlot(int activeSlot) const;
	private:
		std::filesystem::path exe_;
		std::filesystem::path saveGame_;
		std::function<bool()> gameClosedGate_;
	};
}
