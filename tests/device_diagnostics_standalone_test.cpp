#include "device_diagnostics/device_diagnostics_model.hpp"

#include <cassert>

int main()
{
	using namespace DeviceDiagnostics;
	static_assert(requested_nominal_magnitude(20)==2000);
	static_assert(requested_nominal_magnitude(40)==4000);
	static_assert(requested_nominal_magnitude(60)==6000);
	static_assert(requested_nominal_magnitude(80)==8000);
	static_assert(requested_nominal_magnitude(100)==10000);
	static_assert(safety_limited_magnitude(20)==2000);
	static_assert(safety_limited_magnitude(100)==2000);
	static_assert(delivery_request(1).right&&delivery_request(1).magnitudePercent==5);
	static_assert(!delivery_request(8).right&&delivery_request(8).magnitudePercent==10);
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
	static_assert(loaded_fixture_valid());
	static_assert(loaded_sequence_duration() == std::chrono::milliseconds(1386));
	static_assert(loaded_complete_repetitions(std::chrono::seconds(15)) == 10);
	static_assert(LoadedSteeringPreparationTime == std::chrono::seconds(6));
	static_assert(LoadedSteeringMaximumRunTime == std::chrono::seconds(15));
	static_assert(LoadedSteeringActivityThreshold == 2500);
	static_assert(LoadedSteeringSignalPercent[LoadedTransitionBegin] == 18);
	static_assert(LoadedSteeringSignalPercent[LoadedTransitionEnd] == 20);
	for (const int request : LoadedSteeringSignalPercent) assert(request >= 0 && request <= PhysicalOutputCeilingPercent);
	const auto loadedLegacy = simulate_loaded_steering(CompatibilityStrategy::LegacyRecreation, 1);
	const auto loadedDynamic = simulate_loaded_steering(CompatibilityStrategy::PersistentDynamic, 1);
	assert(loadedLegacy.state == ResultState::Completed && loadedDynamic.state == ResultState::Completed);
	assert(loadedLegacy.peakMagnitude == 2000 && loadedDynamic.peakMagnitude == 2000);
	assert(loadedLegacy.rmsMagnitude == loadedDynamic.rmsMagnitude);
	assert(loadedLegacy.apiResults.size() == LoadedSteeringSignalPercent.size());
	assert(loadedDynamic.updateCount == int(LoadedSteeringSignalPercent.size()) - 1);
	assert(simulate_loaded_steering(CompatibilityStrategy::PersistentDynamic, 1, 6).state == ResultState::Failed);
	LoadedSafetyController loadedSafety;
	assert(!loadedSafety.begin(true,true,true,now));
	loadedSafety.authorized=true;assert(!loadedSafety.begin(false,true,true,now));
	loadedSafety.authorized=true;assert(!loadedSafety.begin(true,false,true,now));
	loadedSafety.authorized=true;assert(loadedSafety.begin(true,true,true,now));
	loadedSafety.beat(now+std::chrono::seconds(14));
	assert(!loadedSafety.must_stop(true,true,true,now+std::chrono::seconds(14)));
	assert(loadedSafety.must_stop(true,true,true,now+LoadedSteeringMaximumRunTime));
	loadedSafety.stop();assert(!loadedSafety.running&&!loadedSafety.authorized);
	loadedSafety.authorized=true;assert(loadedSafety.begin(true,true,true,now));assert(loadedSafety.must_stop(false,true,true,now));loadedSafety.stop();
	loadedSafety.authorized=true;assert(loadedSafety.begin(true,true,true,now));assert(loadedSafety.must_stop(true,false,true,now));loadedSafety.stop();
	loadedSafety.authorized=true;assert(loadedSafety.begin(true,true,true,now));assert(loadedSafety.must_stop(true,true,false,now));loadedSafety.stop();
	loadedSafety.authorized=true;assert(loadedSafety.begin(true,true,true,now));assert(loadedSafety.must_stop(true,true,true,now+WatchdogTimeout));loadedSafety.stop();
	static_assert((flag(LoadedSensation::Smooth)|flag(LoadedSensation::Pulsing))==3u);
	static_assert((flag(LoadedObservationPhase::Loading)|flag(LoadedObservationPhase::RampDown))==17u);
	static_assert(LoadedPhysicalResponseUnsureId!=LoadedSensationUnsureId&&LoadedPhysicalResponseUnsureId!=LoadedObservationTimingUnsureId&&LoadedSensationUnsureId!=LoadedObservationTimingUnsureId);
	uint32_t phases=select_observation_phase(0,LoadedObservationPhase::Loading,true);
	phases=select_observation_phase(phases,LoadedObservationPhase::Recovery,true);assert(phases==(flag(LoadedObservationPhase::Loading)|flag(LoadedObservationPhase::Recovery)));
	phases=select_observation_phase(phases,LoadedObservationPhase::Unsure,true);assert(phases==flag(LoadedObservationPhase::Unsure));
	phases=select_observation_phase(phases,LoadedObservationPhase::RampDown,true);assert(phases==flag(LoadedObservationPhase::RampDown));
	phases=select_observation_phase(phases,LoadedObservationPhase::RampDown,false);assert(phases==0);
	assert(steering_axis_from_control("Axis 3 (positive)")==3);assert(!steering_axis_from_control("Button 3"));
	LoadedSteeringActivity steeringActivity;steeringActivity.select("wheel",1,100);
	steeringActivity.observe("pedals",1,true,30000);assert(!steeringActivity.ready());
	steeringActivity.observe("wheel",1,true,100+LoadedSteeringActivityThreshold-1);assert(!steeringActivity.ready());
	steeringActivity.observe("wheel",1,true,100+LoadedSteeringActivityThreshold);assert(steeringActivity.ready());
	steeringActivity.select("wheel",2,50);assert(!steeringActivity.ready());
	steeringActivity.observe("wheel",2,true,50+LoadedSteeringActivityThreshold);assert(steeringActivity.ready());
	steeringActivity.observe("wheel",2,false,0);assert(!steeringActivity.ready());
	LoadedSteeringStage resetStage=LoadedSteeringStage::DynamicFeedback;CompatibilityStrategy resetPending=CompatibilityStrategy::PersistentDynamic;
	auto resetSimLegacy=loadedLegacy,resetSimDynamic=loadedDynamic,resetPhysicalLegacy=loadedLegacy,resetPhysicalDynamic=loadedDynamic;
	resetPhysicalLegacy.physical=PhysicalConfirmation::Yes;resetPhysicalDynamic.sensations=flag(LoadedSensation::Pulsing);
	reset_loaded_steering_session(resetStage,resetPending,resetSimLegacy,resetSimDynamic,resetPhysicalLegacy,resetPhysicalDynamic);
	assert(resetStage==LoadedSteeringStage::Idle&&resetPending==CompatibilityStrategy::LegacyRecreation);
	assert(resetPhysicalLegacy.state==ResultState::Untested&&resetPhysicalLegacy.physical==PhysicalConfirmation::Untested&&resetPhysicalDynamic.sensations==0);
	const auto legacyOneAxis=simulate_compatibility(CompatibilityStrategy::LegacyRecreation,1);
	const auto legacyTwoAxis=simulate_compatibility(CompatibilityStrategy::LegacyRecreation,2);
	const auto dynamicOneAxis=simulate_compatibility(CompatibilityStrategy::PersistentDynamic,1);
	const auto dynamicTwoAxis=simulate_compatibility(CompatibilityStrategy::PersistentDynamic,2);
	assert(legacyOneAxis.state==ResultState::Completed&&legacyTwoAxis.state==ResultState::Completed);
	assert(legacyOneAxis.createCount==int(CompatibilitySignalPercent.size())&&legacyOneAxis.updateCount==0);
	assert(legacyOneAxis.startCount==int(CompatibilitySignalPercent.size()));
	assert(legacyOneAxis.stopCount==int(CompatibilitySignalPercent.size()));
	assert(legacyOneAxis.releaseCount==int(CompatibilitySignalPercent.size()));
	assert(legacyOneAxis.replacementCount==int(CompatibilitySignalPercent.size())-1);
	assert(legacyOneAxis.finalCleanupCount==1);
	assert(dynamicOneAxis.createCount==1&&dynamicOneAxis.updateCount==int(CompatibilitySignalPercent.size())-1);
	assert(dynamicOneAxis.stopCount==1&&dynamicOneAxis.releaseCount==1&&dynamicOneAxis.finalCleanupCount==1);
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
	assert(quick.saved.size() == 7);
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
	QuickFfbCheck quickFfb;quickFfb.begin(now);assert(!quickFfb.countdown_complete(now+std::chrono::seconds(2)));assert(quickFfb.countdown_complete(now+std::chrono::seconds(3)));
	quickFfb.start_output(now);assert(!quickFfb.output_complete(now+std::chrono::milliseconds(1199)));assert(quickFfb.output_complete(now+std::chrono::milliseconds(1200)));
	quickFfb.answer(QuickFfbResponse::NotConfirmed);assert(quickFfb.stage==QuickFfbStage::RetryChoice);quickFfb.retry();assert(quickFfb.stage==QuickFfbStage::Offer);quickFfb.skip();assert(quickFfb.response==QuickFfbResponse::NotTested&&quickFfb.stage==QuickFfbStage::Complete);
	assert(delivery_progress(DeliveryStage::Idle)==0.0f);assert(delivery_progress(DeliveryStage::Countdown)==2.0f/8.0f);assert(delivery_progress(DeliveryStage::LegacyFeedback)==3.0f/8.0f);assert(delivery_progress(DeliveryStage::SafetyInterval)==4.0f/8.0f);assert(delivery_progress(DeliveryStage::DynamicFeedback)==6.0f/8.0f);assert(delivery_progress(DeliveryStage::Results)==1.0f);assert(delivery_progress(DeliveryStage::Legacy,CompatibilitySignalPercent.size())==3.0f/8.0f);

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
	const auto exactRestored=resolve_exact_ffb_device(two,"wheel-b");
	assert(exactRestored.state==FfbResolution::Restored&&exactRestored.index==1);
	assert(resolve_exact_ffb_device(two,"missing").state==FfbResolution::NotFound);
	assert(resolve_exact_ffb_device(one,"missing").state==FfbResolution::NotFound);
	assert(resolve_exact_ffb_device(one,{}).state==FfbResolution::SelectionRequired);
	RedetectLifecycle recovery{true,true,true,true,true,true,true,true};
	assert(recovery.effectStopped && recovery.deviceReleased && recovery.inputRefreshed && recovery.enumerationRefreshed);
	assert(recovery.identityResolved && recovery.capabilitiesValidated && recovery.acquired && recovery.zeroForce);
	std::vector<std::string> lifecycle;
	const auto redetectReady=execute_redetect_lifecycle(
		[&]{lifecycle.push_back("release-native");},
		[&]{lifecycle.push_back("refresh-sdl");return true;},
		[&]{lifecycle.push_back("acquire-native");return true;});
	assert(redetectReady==RedetectExecutionResult::Ready);
	assert((lifecycle==std::vector<std::string>{"release-native","refresh-sdl","acquire-native"}));
	lifecycle.clear();
	const auto inputFailed=execute_redetect_lifecycle(
		[&]{lifecycle.push_back("release-native");},
		[&]{lifecycle.push_back("refresh-sdl");return false;},
		[&]{lifecycle.push_back("acquire-native");return true;});
	assert(inputFailed==RedetectExecutionResult::InputRefreshFailed);
	assert((lifecycle==std::vector<std::string>{"release-native","refresh-sdl"}));
	lifecycle.clear();
	const auto nativeFailed=execute_redetect_lifecycle(
		[&]{lifecycle.push_back("release-native");},
		[&]{lifecycle.push_back("refresh-sdl");return true;},
		[&]{lifecycle.push_back("acquire-native");return false;});
	assert(nativeFailed==RedetectExecutionResult::NativeRefreshFailed);
	assert((lifecycle==std::vector<std::string>{"release-native","refresh-sdl","acquire-native"}));
	int effectCreates=0;
	const auto reacquireFailed=prepare_authorized_output_lifecycle([]{return false;},[]{return true;});
	if(reacquireFailed==AuthorizedOutputPreparation::Ready)++effectCreates;
	assert(reacquireFailed==AuthorizedOutputPreparation::ReacquireFailed&&effectCreates==0);
	const auto actuatorsFailed=prepare_authorized_output_lifecycle([]{return true;},[]{return false;});
	if(actuatorsFailed==AuthorizedOutputPreparation::Ready)++effectCreates;
	assert(actuatorsFailed==AuthorizedOutputPreparation::ActuatorEnableFailed&&effectCreates==0);
	std::vector<std::string> outputLifecycle;
	const auto outputReady=prepare_authorized_output_lifecycle(
		[&]{outputLifecycle.push_back("reacquire");return true;},
		[&]{outputLifecycle.push_back("enable-actuators");return true;});
	if(outputReady==AuthorizedOutputPreparation::Ready){outputLifecycle.push_back("create-effect");++effectCreates;}
	assert((outputLifecycle==std::vector<std::string>{"reacquire","enable-actuators","create-effect"}));
	assert(effectCreates==1);
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
