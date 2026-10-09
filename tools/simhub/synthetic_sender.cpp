#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include "simhub_external_contract.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <thread>

namespace
{
    std::atomic_bool Running{ true };

    BOOL WINAPI HandleConsoleSignal(DWORD signal)
    {
        if (signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT || signal == CTRL_CLOSE_EVENT)
        {
            Running = false;
            return TRUE;
        }
        return FALSE;
    }

    std::uint64_t CreateNonzeroId()
    {
        std::random_device randomDevice;
        std::mt19937_64 generator(randomDevice());
        std::uint64_t value = 0;
        while (value == 0)
            value = generator();
        return value;
    }
}

int main()
{
    using namespace SimHubOutRun2006;

    SetConsoleCtrlHandler(HandleConsoleSignal, TRUE);

    WSADATA winsockData{};
    if (WSAStartup(MAKEWORD(2, 2), &winsockData) != 0)
    {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    const SOCKET socketHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socketHandle == INVALID_SOCKET)
    {
        std::cerr << "socket() failed: " << WSAGetLastError() << '\n';
        WSACleanup();
        return 1;
    }

    sockaddr_in destination{};
    destination.sin_family = AF_INET;
    destination.sin_port = htons(Contract::DefaultPort);
    if (inet_pton(AF_INET, Contract::DefaultHost, &destination.sin_addr) != 1)
    {
        std::cerr << "Invalid destination address\n";
        closesocket(socketHandle);
        WSACleanup();
        return 1;
    }

    TelemetryPacket packet = CreateDefaultPacket();
    packet.EmitterInstanceId = CreateNonzeroId();
    packet.SessionId = CreateNonzeroId();
    packet.IsSessionRunning = 1;
    packet.IsUserInControl = 1;

    std::cout << "HYP36rforce SimHub synthetic telemetry (not live game data)\n"
              << "Destination: " << Contract::DefaultHost << ':' << Contract::DefaultPort << '\n'
              << "Packet size: " << sizeof(packet) << " bytes\n"
              << "Target rate: 60 Hz\n"
              << "Press Ctrl+C to stop cleanly.\n\n";

    const auto sessionStart = std::chrono::steady_clock::now();
    auto nextPacket = sessionStart;
    auto nextDiagnostic = sessionStart + std::chrono::seconds(1);
    std::uint64_t sendErrors = 0;

    while (Running)
    {
        const auto now = std::chrono::steady_clock::now();
        packet.SessionTimeSeconds = std::chrono::duration<double>(now - sessionStart).count();
        ++packet.PacketsCounter;
        ApplySyntheticTelemetry(packet, packet.SessionTimeSeconds);

        const int sent = sendto(socketHandle,
            reinterpret_cast<const char*>(&packet), static_cast<int>(sizeof(packet)), 0,
            reinterpret_cast<const sockaddr*>(&destination), static_cast<int>(sizeof(destination)));
        if (sent != static_cast<int>(sizeof(packet)))
        {
            ++sendErrors;
            std::cerr << "sendto() failed: " << WSAGetLastError() << '\n';
        }

        if (now >= nextDiagnostic)
        {
            const double elapsed = std::chrono::duration<double>(now - sessionStart).count();
            const double rate = elapsed > 0.0 ? static_cast<double>(packet.PacketsCounter) / elapsed : 0.0;
            std::cout << "Packets: " << packet.PacketsCounter
                      << " | Rate: " << std::fixed << std::setprecision(1) << rate << " Hz"
                      << " | Send errors: " << sendErrors << '\n';
            nextDiagnostic += std::chrono::seconds(1);
        }

        nextPacket += std::chrono::microseconds(16667);
        std::this_thread::sleep_until(nextPacket);
    }

    closesocket(socketHandle);
    WSACleanup();
    std::cout << "Stopped cleanly after " << packet.PacketsCounter << " packets.\n";
    return sendErrors == 0 ? 0 : 2;
}
