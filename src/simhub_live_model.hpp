#pragma once
#include <algorithm>
#include <string_view>
#include "../tools/simhub/simhub_external_contract.hpp"
namespace SimHubLiveModel {
struct NativeSnapshot { bool inGame=false; float steeringInput=0; int carId=-1; int stageId=-1; float roadActivity=0; float impactIntensity=0; };
inline void apply_native(SimHubOutRun2006::TelemetryPacket& p, const NativeSnapshot& s, std::string_view carName, bool fresh) noexcept {
	const bool running = fresh && s.inGame;
	p.IsSessionRunning=running; p.IsSessionPaused=0; p.IsUserInControl=running;
	p.SpeedKmh=0; SimHubOutRun2006::WriteUtf8Z(p.Gear,sizeof(p.Gear),"");
	p.SteeringInput=running?std::clamp(s.steeringInput,-1.0f,1.0f):0;
	p.CarID=running?s.carId:-1; SimHubOutRun2006::WriteUtf8Z(p.CarName,sizeof(p.CarName),running&&!carName.empty()?carName.data():"");
	p.StageID=running?s.stageId:-1; p.RoadActivity=running?std::clamp(s.roadActivity,0.0f,1.0f):0; p.ImpactIntensity=running?std::clamp(s.impactIntensity,0.0f,1.0f):0;
}
}
