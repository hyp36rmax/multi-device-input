#include "simhub_live_model.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
int main() {
	auto p=SimHubOutRun2006::CreateDefaultPacket();
	SimHubLiveModel::NativeSnapshot s{true,1.4f,27,14,1.4f,-0.2f};
	SimHubLiveModel::apply_native(p,s,"Ferrari F430 Spider (OutRun)",true);
	assert(p.IsSessionRunning&&p.IsUserInControl); assert(p.SpeedKmh==0&&p.Gear[0]=='\0');
	assert(p.SteeringInput==1&&p.CarID==27&&p.StageID==14); assert(std::strcmp(p.CarName,"Ferrari F430 Spider (OutRun)")==0);
	assert(p.RoadActivity==1&&p.ImpactIntensity==0);
	SimHubLiveModel::apply_native(p,s,"ignored",false);
	assert(!p.IsSessionRunning&&!p.IsUserInControl&&p.CarID==-1&&p.StageID==-1&&p.CarName[0]=='\0');
}
