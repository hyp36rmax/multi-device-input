#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace SimHubOutRun2006
{
    struct Contract
    {
        static constexpr const char* DefinitionUniqueId = "cca64189-7ad3-4852-89e3-6fcc790bf02a";
        static constexpr std::uint32_t GameSignature = 0x007D179Du;
        static constexpr std::uint32_t TelemetrySignature = 0x87FEBB5Au;
        static constexpr std::uint16_t LayoutMajorVersion = 1;
        static constexpr std::uint16_t LayoutMinorVersion = 0;
        static constexpr std::size_t ExpectedPacketLength = 215;
        static constexpr const char* DefaultHost = "127.0.0.1";
        static constexpr std::uint16_t DefaultPort = 30777;
    };

    inline void WriteUtf8Z(char* destination, std::size_t destinationSize, const char* value)
    {
        if (destinationSize == 0)
            return;

        std::memset(destination, 0, destinationSize);
        if (value == nullptr)
            return;

        const auto count = (std::min)(destinationSize - 1, std::strlen(value));
        if (count != 0)
            std::memcpy(destination, value, count);
        destination[count] = '\0';
    }

#pragma pack(push, 1)
    struct TelemetryPacket
    {
        std::uint32_t GameSignature;
        std::uint32_t TelemetrySignature;
        std::uint16_t LayoutMajorVersion;
        std::uint16_t LayoutMinorVersion;
        std::uint64_t EmitterInstanceId;
        std::uint8_t PacketId;
        std::uint64_t PacketsCounter;
        std::uint8_t IsSessionRunning;
        std::uint8_t IsSessionPaused;
        std::uint64_t SessionId;
        std::uint8_t IsReplay;
        std::uint8_t IsUserInControl;
        std::uint8_t IsAIInControl;
        std::uint8_t IsSpectator;
        double SessionTimeSeconds;
        std::uint32_t PhysicsDiscontinuityCounter;
        float SpeedKmh;
        char Gear[8];
        float SteeringInput;
        std::int32_t CarID;
        char CarName[128];
        std::int32_t StageID;
        float RoadActivity;
        float ImpactIntensity;
    };
#pragma pack(pop)

    static_assert(sizeof(TelemetryPacket) == Contract::ExpectedPacketLength);
    static_assert(offsetof(TelemetryPacket, SpeedKmh) == 55);
    static_assert(offsetof(TelemetryPacket, Gear) == 59);
    static_assert(offsetof(TelemetryPacket, SteeringInput) == 67);
    static_assert(offsetof(TelemetryPacket, CarID) == 71);
    static_assert(offsetof(TelemetryPacket, CarName) == 75);
    static_assert(offsetof(TelemetryPacket, StageID) == 203);
    static_assert(offsetof(TelemetryPacket, RoadActivity) == 207);
    static_assert(offsetof(TelemetryPacket, ImpactIntensity) == 211);

    inline TelemetryPacket CreateDefaultPacket()
    {
        TelemetryPacket packet{};
        packet.GameSignature = Contract::GameSignature;
        packet.TelemetrySignature = Contract::TelemetrySignature;
        packet.LayoutMajorVersion = Contract::LayoutMajorVersion;
        packet.LayoutMinorVersion = Contract::LayoutMinorVersion;
        return packet;
    }

    inline void ApplySyntheticTelemetry(TelemetryPacket& packet, double sessionTimeSeconds)
    {
        constexpr double Pi = 3.14159265358979323846;
        const double speedCycle = std::fmod((std::max)(0.0, sessionTimeSeconds), 24.0);
        packet.SpeedKmh = static_cast<float>(speedCycle <= 12.0
            ? speedCycle * 10.0
            : (24.0 - speedCycle) * 10.0);

        constexpr std::array<const char*, 6> Gears{ "N", "1", "2", "3", "4", "5" };
        const auto gearIndex = static_cast<std::size_t>(sessionTimeSeconds / 2.0) % Gears.size();
        WriteUtf8Z(packet.Gear, sizeof(packet.Gear), Gears[gearIndex]);

        packet.SteeringInput = static_cast<float>(std::sin(sessionTimeSeconds * (2.0 * Pi / 8.0)));
        packet.CarID = 27;
        WriteUtf8Z(packet.CarName, sizeof(packet.CarName), "Ferrari F430 Spider (OutRun)");
        packet.StageID = 0;
        packet.RoadActivity = static_cast<float>(0.5 + 0.5 * std::sin(sessionTimeSeconds * (2.0 * Pi / 3.0)));

        const double impactPhase = std::fmod((std::max)(0.0, sessionTimeSeconds), 5.0);
        packet.ImpactIntensity = impactPhase < 0.15
            ? static_cast<float>(1.0 - (impactPhase / 0.15))
            : 0.0f;
    }
}
