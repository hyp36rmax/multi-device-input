#include "ffb_output_observer.hpp"

#include <cassert>

using namespace HYP36RFFBOutput;

int main()
{
	Observer observer;
	observer.set_enabled(true);
	observer.record({ .finalRequestedForce = 0.5f, .requestedMagnitude = 5000,
		.requestedDirection = 9000, .effect = EffectType::Constant,
		.strategy = Strategy::TwoAxisPolar, .actuatorAxes = 2, .deviceReady = true,
		.operation = Operation::SetParameters, .result = 0, .timestampUs = 100000,
		.requestedIntervalUs = 16000, .persistentUpdate = true });
	auto frame = observer.frame();
	assert(frame.finalRequestedAvailable && frame.finalRequestedForce == 0.5f);
	assert(frame.requestedMagnitude == 5000 && frame.requestedDirection == 9000);
	assert(frame.strategy == Strategy::TwoAxisPolar && frame.persistentUpdateCount == 1);
	assert(frame.recordedDirectInputOperations == 1 && frame.failedDirectInputOperations == 0);

	observer.record({ .finalRequestedForce = -0.25f, .requestedMagnitude = -2500,
		.requestedDirection = 1, .effect = EffectType::Constant,
		.strategy = Strategy::OneAxisCartesianRecreation, .actuatorAxes = 1,
		.deviceReady = true, .operation = Operation::Start, .result = 0,
		.timestampUs = 167000, .requestedIntervalUs = 66000, .recreation = true });
	frame = observer.frame();
	assert(frame.submissionIntervalUs == 67000 && frame.timingJitterUs == 1000);
	assert(frame.timingClass == TimingClass::Comparable);
	assert(frame.recreationCount == 1);
	observer.record({ .timestampUs = 160000 });
	assert(observer.frame().submissionIntervalUs == 0);
	assert(observer.frame().timingClass == TimingClass::NonMonotonic);

	observer.record({ .finalRequestedForce = 0.12f, .requestedMagnitude = 1200,
		.requestedDirection = 1, .effect = EffectType::Triangle,
		.strategy = Strategy::PeriodicPersistent, .actuatorAxes = 1,
		.deviceReady = true, .operation = Operation::SetParameters, .result = -1,
		.timestampUs = 180000, .persistentUpdate = true });
	frame = observer.frame();
	assert(frame.effect == EffectType::Triangle && frame.result == -1);
	assert(frame.persistentUpdateCount == 2);
	assert(frame.failedDirectInputOperations == 1);

	observer.record_api(Operation::Acquire, -2, 181000, EffectType::Constant,
		Strategy::TwoAxisPolar, 2, false);
	assert(observer.frame().operation == Operation::Acquire && observer.frame().result == -2);
	assert(observer.frame().recordedDirectInputOperations == 4);
	assert(observer.frame().failedDirectInputOperations == 2);
	observer.safety(SafetyState::Watchdog);
	assert(observer.frame().watchdogShutdownCount == 1);

	for (size_t i = 0; i < Observer::Capacity + 4; ++i)
		observer.record({ .requestedMagnitude = static_cast<int32_t>(i), .timestampUs = 200000 + i });
	assert(observer.frame().bufferedSamples == Observer::Capacity);
	assert(observer.frame().droppedSamples == 9);
	const auto retained = observer.retained_events();
	assert(retained.size() == Observer::Capacity);
	assert(retained.front().requestedMagnitude == 4);
	assert(retained.back().requestedMagnitude == static_cast<int32_t>(Observer::Capacity + 3));
	for (size_t i = 1; i < retained.size(); ++i)
		assert(retained[i - 1].timestampUs <= retained[i].timestampUs);

	Observer timing;
	timing.set_enabled(true);
	timing.record({ .operation = Operation::SetParameters, .timestampUs = 100000,
		.requestedIntervalUs = 66000 });
	timing.record({ .operation = Operation::SetParameters, .timestampUs = 500000,
		.requestedIntervalUs = 66000 });
	assert(timing.frame().submissionIntervalUs == 400000);
	assert(timing.frame().timingClass == TimingClass::IdleGap);
	assert(timing.frame().timingJitterUs == 334000);

	const auto beforeDisabled = observer.frame().persistentUpdateCount;
	observer.set_enabled(false);
	observer.record({ .persistentUpdate = true });
	assert(!observer.frame().enabled);
	assert(observer.frame().persistentUpdateCount == beforeDisabled);
	assert(observer.frame().recordedDirectInputOperations == 4);
	assert(name(EffectType::Bump) == "Bump");
	assert(name(Strategy::OneShotBump) == "One-Shot Bump");
	assert(name(TimingClass::IdleGap) == "Idle Gap");
	return 0;
}
