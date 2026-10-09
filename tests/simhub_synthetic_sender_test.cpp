#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>

#include "../tools/simhub/simhub_external_contract.hpp"

#include <cassert>
#include <cmath>
#include <cstring>

int main()
{
    using namespace SimHubOutRun2006;

    static_assert(sizeof(TelemetryPacket) == 215);
    assert(Contract::DefaultPort == 30777);
    assert(std::strcmp(Contract::DefaultHost, "127.0.0.1") == 0);

    TelemetryPacket first = CreateDefaultPacket();
    first.EmitterInstanceId = 0x123456789ABCDEF0ull;
    first.SessionId = 0x0FEDCBA987654321ull;
    first.PacketsCounter = 41;
    first.IsSessionRunning = 1;
    first.IsUserInControl = 1;
    first.SessionTimeSeconds = 4.25;
    ApplySyntheticTelemetry(first, first.SessionTimeSeconds);

    assert(first.GameSignature == 0x007D179Du);
    assert(first.TelemetrySignature == 0x87FEBB5Au);
    assert(first.LayoutMajorVersion == 1 && first.LayoutMinorVersion == 0);
    assert(first.EmitterInstanceId != 0 && first.SessionId != 0);
    assert(first.CarID == 27);
    assert(std::strcmp(first.CarName, "Ferrari F430 Spider (OutRun)") == 0);
    assert(first.CarName[127] == '\0');
    assert(first.StageID == 0);
    assert(first.SpeedKmh >= 0.0f && first.SpeedKmh <= 120.0f);
    assert(first.SteeringInput >= -1.0f && first.SteeringInput <= 1.0f);
    assert(first.RoadActivity >= 0.0f && first.RoadActivity <= 1.0f);
    assert(first.ImpactIntensity >= 0.0f && first.ImpactIntensity <= 1.0f);

    TelemetryPacket second = first;
    ++second.PacketsCounter;
    second.SessionTimeSeconds += 1.0 / 60.0;
    ApplySyntheticTelemetry(second, second.SessionTimeSeconds);
    assert(second.PacketsCounter == first.PacketsCounter + 1);
    assert(second.EmitterInstanceId == first.EmitterInstanceId);
    assert(second.SessionId == first.SessionId);

    WSADATA winsockData{};
    assert(WSAStartup(MAKEWORD(2, 2), &winsockData) == 0);
    const SOCKET receiver = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    const SOCKET transmitter = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    assert(receiver != INVALID_SOCKET && transmitter != INVALID_SOCKET);

    DWORD timeoutMs = 2000;
    assert(setsockopt(receiver, SOL_SOCKET, SO_RCVTIMEO,
        reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs)) == 0);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(Contract::DefaultPort);
    assert(inet_pton(AF_INET, Contract::DefaultHost, &address.sin_addr) == 1);
    assert(bind(receiver, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == 0);

    const int sent = sendto(transmitter, reinterpret_cast<const char*>(&second), sizeof(second), 0,
        reinterpret_cast<const sockaddr*>(&address), sizeof(address));
    assert(sent == sizeof(second));

    TelemetryPacket received{};
    sockaddr_in source{};
    int sourceLength = sizeof(source);
    const int receivedBytes = recvfrom(receiver, reinterpret_cast<char*>(&received), sizeof(received), 0,
        reinterpret_cast<sockaddr*>(&source), &sourceLength);
    assert(receivedBytes == Contract::ExpectedPacketLength);
    assert(std::memcmp(&received, &second, sizeof(second)) == 0);

    closesocket(transmitter);
    closesocket(receiver);
    WSACleanup();
    return 0;
}
