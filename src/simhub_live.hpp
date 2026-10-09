#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

#include "settings.hpp"
#include "simhub_live_model.hpp"

namespace Settings
{
	extern Setting<bool> SimHubTelemetry;
}

namespace SimHubLive
{
	using NativeSnapshot = SimHubLiveModel::NativeSnapshot;

	enum class RegistrationState : std::uint8_t
	{
		NotAttempted,
		SimHubNotInstalled,
		DefinitionMissing,
		Registered,
		AlreadyCurrent,
		Failed
	};

	struct Diagnostics
	{
		RegistrationState registration = RegistrationState::NotAttempted;
		std::uint64_t packetCount = 0;
		std::uint64_t sendErrors = 0;
		double packetRate = 0.0;
		double secondsSinceLastTransmission = -1.0;
		bool telemetryValid = false;
		std::string registrationMessage;
	};

	void initialize(const std::filesystem::path& installationDirectory);
	void publish(const NativeSnapshot& snapshot) noexcept;
	void shutdown();
	Diagnostics diagnostics();
	const char* registration_state_name(RegistrationState state) noexcept;
}
