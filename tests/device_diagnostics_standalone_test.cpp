#include "device_diagnostics/device_diagnostics_model.hpp"

#include <cassert>

int main()
{
	using namespace DeviceDiagnostics;
	SafetyController safety;
	const auto now = std::chrono::steady_clock::now();
	assert(!safety.begin(true, true, now));
	safety.authorized = true;
	assert(!safety.begin(false, true, now));
	assert(safety.begin(true, true, now));
	assert(safety.bounded_magnitude(100) == 2000);
	assert(safety.bounded_magnitude(10) == 1000);
	assert(safety.bounded_magnitude(20) == safety.bounded_magnitude(100));
	assert(!effective_right(false, false));
	assert(effective_right(false, true));
	assert(effective_right(true, false));
	assert(!effective_right(true, true));
	static_assert(VisibleFfbActions.size() == 3);
	static_assert(VisibleFfbActions[0] == "Test Left" && VisibleFfbActions[1] == "Test Right" && VisibleFfbActions[2] == "Hold to Shake");
	static_assert(BackendObservationTime == std::chrono::seconds(10));
	static_assert(CompatibilityUpdatePeriod == std::chrono::milliseconds(66));
	static_assert(CompatibilitySignalPercent.front() == 0 && CompatibilitySignalPercent.back() == 0);
	const auto legacyOneAxis=simulate_compatibility(CompatibilityStrategy::LegacyRecreation,1);
	const auto legacyTwoAxis=simulate_compatibility(CompatibilityStrategy::LegacyRecreation,2);
	const auto dynamicOneAxis=simulate_compatibility(CompatibilityStrategy::PersistentDynamic,1);
	const auto dynamicTwoAxis=simulate_compatibility(CompatibilityStrategy::PersistentDynamic,2);
	assert(legacyOneAxis.state==ResultState::Completed&&legacyTwoAxis.state==ResultState::Completed);
	assert(legacyOneAxis.createCount==int(CompatibilitySignalPercent.size())&&legacyOneAxis.updateCount==0);
	assert(dynamicOneAxis.createCount==1&&dynamicOneAxis.updateCount==int(CompatibilitySignalPercent.size())-1);
	assert(dynamicTwoAxis.createCount==1&&dynamicTwoAxis.actuatorAxes==2);
	assert(legacyOneAxis.peakMagnitude==dynamicOneAxis.peakMagnitude&&legacyOneAxis.rmsMagnitude==dynamicOneAxis.rmsMagnitude);
	assert(dynamicOneAxis.averageIntervalMs==66.0&&dynamicOneAxis.jitterMs==0.0&&dynamicOneAxis.zeroTimeMs>0.0);
	auto rejected=simulate_compatibility(CompatibilityStrategy::PersistentDynamic,1,4);
	assert(rejected.state==ResultState::Failed&&classify_compatibility(rejected)==CompatibilityClassification::DynamicRejected);
	auto accepted=dynamicOneAxis;assert(classify_compatibility(accepted)==CompatibilityClassification::DynamicAcceptedUnverified);
	accepted.physical=PhysicalConfirmation::Yes;assert(classify_compatibility(accepted)==CompatibilityClassification::DynamicAccepted);
	accepted.physical=PhysicalConfirmation::No;assert(classify_compatibility(accepted)==CompatibilityClassification::DynamicAcceptedIneffective);
	assert(exports_path("D:/OneDrive/Documents") == std::filesystem::path("D:/OneDrive/Documents/HYP36rforce Device Diagnostics/Exports"));
	assert(safety.must_stop(false, true, true, now));
	safety.stop();
	assert(safety.begin(true, true, now));
	assert(safety.must_stop(true, false, true, now));
	safety.stop();
	assert(safety.begin(true, true, now));
	assert(safety.must_stop(true, true, false, now));
	safety.stop();
	assert(safety.begin(true, true, now));
	assert(safety.must_stop(true, true, true, now + MaximumRunTime));

	QuickSetupController quick;
	assert(quick.saved.size() == 8);
	quick.saved[0] = CapturedInput{ "old", "Existing Wheel", "Axis 0" };
	quick.start(now);
	assert(quick.active && quick.step == 0);
	assert(quick.deadline - now == std::chrono::seconds(6));
	assert(!quick.continue_step(now));
	quick.observe({ "wheel-a", "Wheel interface A", "Axis 0", true });
	assert(!quick.continue_step(now));
	quick.retry(now);
	assert(!quick.candidate && !quick.timedOut);
	quick.observe({ "wheel-b", "Wheel interface B", "Axis 1", false });
	assert(quick.continue_step(now));
	assert(quick.saved[0]->deviceId == "wheel-b");
	quick.skip(now);
	assert(quick.step == 2 && !quick.saved[1]);
	quick.back(now);
	assert(quick.step == 1);
	quick.update(now + std::chrono::seconds(7));
	assert(quick.timedOut && !quick.candidate);
	quick.cancel();
	assert(!quick.active && quick.saved[0]->deviceId == "old");

	const std::vector<std::string> none;
	assert(resolve_ffb_device(none, {}).state == FfbResolution::NotFound);
	const std::vector<std::string> one{ "wheel-a" };
	const auto automatic = resolve_ffb_device(one, {});
	assert(automatic.state == FfbResolution::AutoSelected && automatic.index == 0);
	const std::vector<std::string> two{ "wheel-a", "wheel-b" };
	const auto restored = resolve_ffb_device(two, "wheel-b");
	assert(restored.state == FfbResolution::Restored && restored.index == 1);
	assert(resolve_ffb_device(two, {}).state == FfbResolution::SelectionRequired);
	assert(resolve_ffb_device(one, "missing").state == FfbResolution::AutoSelected);
	RedetectLifecycle recovery{true,true,true,true,true,true,true};
	assert(recovery.effectStopped && recovery.deviceReleased && recovery.enumerationRefreshed);
	assert(recovery.identityResolved && recovery.capabilitiesValidated && recovery.acquired && recovery.zeroForce);
	ShakeController shake;
	shake.begin(now);
	assert(shake.active && !shake.right);
	assert(!shake.update(now + ShakeController::HalfPeriod - std::chrono::milliseconds(1)));
	assert(shake.update(now + ShakeController::HalfPeriod) == true);
	assert(shake.update(now + ShakeController::HalfPeriod * 2) == false);
	shake.stop();
	assert(!shake.active && !shake.update(now + std::chrono::seconds(1)));

	assert(sanitize_filename_component("Fanatec Podium Wheel Base DD2") == "Fanatec_Podium_Wheel_Base_DD2");
	assert(sanitize_filename_component("Thrustmaster: T300RS / Racing Wheel") == "Thrustmaster_T300RS_Racing_Wheel");
	assert(sanitize_filename_component("<>:\"/\\|?*") == "No_Wheel_Detected");
	return 0;
}
