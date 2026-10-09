#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#include "simhub_live.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <fstream>
#include <mutex>
#include <thread>

#include <spdlog/spdlog.h>

#include "car_identity.hpp"
#include "../tools/simhub/simhub_external_contract.hpp"

namespace Settings
{
	Setting<bool> SimHubTelemetry{ "Gameplay", "SimHubTelemetry", false,
		"Sends live game telemetry to SimHub for bass shakers, dashboards, and other external devices." };
}

namespace SimHubLive
{
	using Clock = std::chrono::steady_clock;
	namespace
	{
		constexpr auto SendPeriod = std::chrono::microseconds(16667);
		constexpr auto StaleAfter = std::chrono::milliseconds(250);
		constexpr const wchar_t* DefinitionName = L"OutRun 2006 C2C Multi Input.simdef";

		std::mutex stateMutex;
		NativeSnapshot latest{};
		Clock::time_point latestAt{};
		Diagnostics status{};
		std::thread sender;
		std::atomic<bool> stopping{ false };
		std::atomic<bool> initialized{ false };
		std::atomic<bool> enabled{ false };
		std::filesystem::path installDirectory;

		RegistrationState register_definition(std::string& message)
		{
			const auto definition = installDirectory / DefinitionName;
			if (!std::filesystem::exists(definition))
			{
				message = "Packaged .simdef was not found";
				return RegistrationState::DefinitionMissing;
			}

			wchar_t localAppData[MAX_PATH]{};
			const DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
			if (length == 0 || length >= MAX_PATH)
			{
				message = "LOCALAPPDATA is unavailable";
				return RegistrationState::Failed;
			}

			const auto simHub = std::filesystem::path(localAppData) / L"SimHub";
			if (!std::filesystem::exists(simHub))
			{
				message = "SimHub is not installed for this user";
				return RegistrationState::SimHubNotInstalled;
			}

			const auto registrations = simHub / L"ExternalSims" / L"Registrations";
			const auto link = registrations /
				L"{cca64189-7ad3-4852-89e3-6fcc790bf02a}.shlink";
			const auto desired = std::filesystem::absolute(definition).wstring();
			try
			{
				std::filesystem::create_directories(registrations);
				if (std::filesystem::exists(link))
				{
					std::wifstream existing(link);
					std::wstring current;
					std::getline(existing, current);
					if (current == desired)
					{
						message = "Registration already points to this installation";
						return RegistrationState::AlreadyCurrent;
					}
				}
				std::wofstream output(link, std::ios::trunc);
				output << desired;
				if (!output)
					throw std::runtime_error("registration write failed");
				message = "Registered packaged SimHub definition";
				return RegistrationState::Registered;
			}
			catch (const std::exception& error)
			{
				message = error.what();
				return RegistrationState::Failed;
			}
		}

		void sender_loop()
		{
			WSADATA wsa{};
			if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
			{
				std::scoped_lock lock(stateMutex);
				status.sendErrors++;
				return;
			}
			SOCKET socketHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			sockaddr_in destination{};
			destination.sin_family = AF_INET;
			destination.sin_port = htons(SimHubOutRun2006::Contract::DefaultPort);
			inet_pton(AF_INET, SimHubOutRun2006::Contract::DefaultHost, &destination.sin_addr);

			auto packet = SimHubOutRun2006::CreateDefaultPacket();
			packet.EmitterInstanceId = (std::uint64_t(GetCurrentProcessId()) << 32) ^
				std::uint64_t(Clock::now().time_since_epoch().count());
			if (packet.EmitterInstanceId == 0) packet.EmitterInstanceId = 1;
			std::uint64_t sessionSerial = 0;
			std::uint32_t discontinuities = 0;
			bool previousRunning = false;
			int previousStage = -1;
			auto sessionStart = Clock::now();
			auto rateStart = sessionStart;
			std::uint64_t ratePackets = 0;
			auto next = Clock::now();

			while (!stopping.load(std::memory_order_relaxed))
			{
				next += SendPeriod;
				if (!enabled.load(std::memory_order_relaxed))
				{
					previousRunning = false;
					std::this_thread::sleep_until(next);
					continue;
				}

				NativeSnapshot state;
				Clock::time_point stateAt;
				{
					std::scoped_lock lock(stateMutex);
					state = latest;
					stateAt = latestAt;
				}
				const auto now = Clock::now();
				const bool fresh = stateAt.time_since_epoch().count() != 0 && now - stateAt <= StaleAfter;
				const bool running = fresh && state.inGame;
				if (running && (!previousRunning || state.stageId != previousStage))
				{
					sessionSerial++;
					discontinuities++;
					sessionStart = now;
				}

				packet.PacketId++;
				packet.PacketsCounter++;
				const auto name = running ? CarIdentity::friendly_name(state.carId) : std::string_view{};
				SimHubLiveModel::apply_native(packet, state, name, fresh);
				packet.SessionId = running ? sessionSerial : 0;
				packet.IsReplay = 0;
				packet.IsAIInControl = 0;
				packet.IsSpectator = 0;
				packet.SessionTimeSeconds = running ? std::chrono::duration<double>(now - sessionStart).count() : 0.0;
				packet.PhysicsDiscontinuityCounter = discontinuities;

				const int result = socketHandle == INVALID_SOCKET ? SOCKET_ERROR : sendto(socketHandle,
					reinterpret_cast<const char*>(&packet), sizeof(packet), 0,
					reinterpret_cast<const sockaddr*>(&destination), sizeof(destination));
				{
					std::scoped_lock lock(stateMutex);
					status.telemetryValid = running;
					if (result == sizeof(packet))
					{
						status.packetCount++;
						status.secondsSinceLastTransmission = 0.0;
						ratePackets++;
					}
					else status.sendErrors++;
					const auto rateElapsed = std::chrono::duration<double>(now - rateStart).count();
					if (rateElapsed >= 1.0)
					{
						status.packetRate = ratePackets / rateElapsed;
						ratePackets = 0;
						rateStart = now;
					}
				}
				previousRunning = running;
				previousStage = running ? state.stageId : -1;
				std::this_thread::sleep_until(next);
			}
			if (socketHandle != INVALID_SOCKET) closesocket(socketHandle);
			WSACleanup();
		}
	}

	void initialize(const std::filesystem::path& installationDirectory)
	{
		if (initialized.exchange(true)) return;
		installDirectory = installationDirectory;
		enabled = Settings::SimHubTelemetry.get();
		Settings::SimHubTelemetry.watch([] { enabled = Settings::SimHubTelemetry.get(); });
		status.registration = register_definition(status.registrationMessage);
		spdlog::info("SimHub telemetry: {} ({})", registration_state_name(status.registration), status.registrationMessage);
		stopping = false;
		sender = std::thread(sender_loop);
	}

	void publish(const NativeSnapshot& snapshot) noexcept
	{
		if (!initialized.load(std::memory_order_relaxed)) return;
		enabled = Settings::SimHubTelemetry.get();
		if (!enabled.load(std::memory_order_relaxed)) return;
		std::unique_lock lock(stateMutex, std::try_to_lock);
		if (!lock.owns_lock()) return;
		latest = snapshot;
		latestAt = Clock::now();
	}

	void shutdown()
	{
		if (!initialized.exchange(false)) return;
		stopping = true;
		if (sender.joinable()) sender.join();
	}

	Diagnostics diagnostics()
	{
		std::scoped_lock lock(stateMutex);
		return status;
	}

	const char* registration_state_name(RegistrationState state) noexcept
	{
		switch (state)
		{
		case RegistrationState::NotAttempted: return "not attempted";
		case RegistrationState::SimHubNotInstalled: return "SimHub not installed";
		case RegistrationState::DefinitionMissing: return "definition missing";
		case RegistrationState::Registered: return "registered";
		case RegistrationState::AlreadyCurrent: return "registered (current)";
		case RegistrationState::Failed: return "failed";
		}
		return "unknown";
	}
}
