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

	observer.record({ .finalRequestedForce = -0.25f, .requestedMagnitude = -2500,
		.requestedDirection = 1, .effect = EffectType::Constant,
		.strategy = Strategy::OneAxisCartesianRecreation, .actuatorAxes = 1,
		.deviceReady = true, .operation = Operation::Start, .result = 0,
		.timestampUs = 167000, .requestedIntervalUs = 66000, .recreation = true });
	frame = observer.frame();
	assert(frame.submissionIntervalUs == 67000 && frame.timingJitterUs == 1000);
	assert(frame.recreationCount == 1);
	observer.record({ .timestampUs = 160000 });
	assert(observer.frame().submissionIntervalUs == 0);

	observer.record({ .finalRequestedForce = 0.12f, .requestedMagnitude = 1200,
		.requestedDirection = 1, .effect = EffectType::Triangle,
		.strategy = Strategy::PeriodicPersistent, .actuatorAxes = 1,
		.deviceReady = true, .operation = Operation::SetParameters, .result = -1,
		.timestampUs = 180000, .persistentUpdate = true });
	frame = observer.frame();
	assert(frame.effect == EffectType::Triangle && frame.result == -1);
	assert(frame.persistentUpdateCount == 2);

	observer.record_api(Operation::Acquire, -2, 181000, EffectType::Constant,
		Strategy::TwoAxisPolar, 2, false);
	assert(observer.frame().operation == Operation::Acquire && observer.frame().result == -2);
	observer.safety(SafetyState::Watchdog);
	assert(observer.frame().watchdogShutdownCount == 1);

	for (size_t i = 0; i < Observer::Capacity + 4; ++i)
		observer.record({ .timestampUs = 200000 + i });
	assert(observer.frame().bufferedSamples == Observer::Capacity);
	assert(observer.frame().droppedSamples >= 4);

	const auto beforeDisabled = observer.frame().persistentUpdateCount;
	observer.set_enabled(false);
	observer.record({ .persistentUpdate = true });
	assert(!observer.frame().enabled);
	assert(observer.frame().persistentUpdateCount == beforeDisabled);
	assert(name(EffectType::Bump) == "Bump");
	assert(name(Strategy::OneShotBump) == "One-Shot Bump");
	return 0;
}
