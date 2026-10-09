#include "../tools/simhub/simhub_external_contract.hpp"

#include <cassert>
#include <cmath>
#include <cstring>

int main()
{
    using namespace SimHubOutRun2006;

    static_assert(sizeof(TelemetryPacket) == 215);
    assert(Contract::GameSignature == 0x007D179Du);
    assert(Contract::TelemetrySignature == 0x87FEBB5Au);
    assert(Contract::LayoutMajorVersion == 1);
    assert(Contract::LayoutMinorVersion == 0);
    assert(Contract::DefaultPort == 30777);

    auto packet = CreateDefaultPacket();
    ApplySyntheticTelemetry(packet, 4.25);
    assert(packet.CarID == 27);
    assert(std::strcmp(packet.CarName, "Ferrari F430 Spider (OutRun)") == 0);
    assert(packet.CarName[127] == '\0');
    assert(packet.StageID == 0);
    assert(packet.SpeedKmh >= 0.0f && packet.SpeedKmh <= 120.0f);
    assert(packet.SteeringInput >= -1.0f && packet.SteeringInput <= 1.0f);
    assert(packet.RoadActivity >= 0.0f && packet.RoadActivity <= 1.0f);
    assert(packet.ImpactIntensity >= 0.0f && packet.ImpactIntensity <= 1.0f);

    ApplySyntheticTelemetry(packet, 0.0);
    assert(packet.SpeedKmh == 0.0f);
    assert(std::strcmp(packet.Gear, "N") == 0);
    assert(packet.ImpactIntensity == 1.0f);
    ApplySyntheticTelemetry(packet, 12.0);
    assert(packet.SpeedKmh == 120.0f);
    ApplySyntheticTelemetry(packet, 24.0);
    assert(packet.SpeedKmh == 0.0f);
    return 0;
}
