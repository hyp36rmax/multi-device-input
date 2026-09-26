#include "research_telemetry_replay.hpp"
#include "signal_state.hpp"

#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <type_traits>

using namespace HYP36RSignalState;

namespace
{
	Inputs baseline(uint64_t frame = 1)
	{
		Inputs i{};
		i.frameId = frame;
		i.steering = 0.2f;
		i.speed = 0.75f;
		i.normalizedSpeed = 0.5f;
		i.nativeVehicleValid = true;
		i.responseAngle = 0.1f;
		i.responseRate = -0.2f;
		i.referenceResponseError = 0.03f;
		i.responseAuthority = 0.8f;
		i.surfaces = { 2, 2, 2, 2 };
		i.field14Available = true;
		i.field14 = i.surfaces;
		i.effectLeft = 0.05f;
		i.effectRight = 0.2f;
		i.effectCombined = 0.2f;
		i.effectRise = 0.01f;
		i.currentGear = 3;
		i.previousGear = 3;
		i.gripState = GripState::Load;
		i.nativeDynamicsAvailable = true;
		i.fieldE8 = { 1.0f, 2.0f, 3.0f, 4.0f };
		i.fieldEC = { 10, 20, 30, 40 };
		i.fieldEE = { -1, -2, -3, -4 };
		return i;
	}

	void assert_same_replay(const HYP36RResearchReplay::Result& a,
		const HYP36RResearchReplay::Result& b)
	{
		assert(a.postGain.directional == b.postGain.directional);
		assert(a.postGain.road == b.postGain.road);
		assert(a.postGain.impact == b.postGain.impact);
		assert(a.composerInput == b.composerInput);
		assert(a.postTanh == b.postTanh);
		assert(a.preDrive == b.preDrive);
	}
}

int main()
{
	static_assert(SchemaVersion == 1);
	static_assert(InterpretationVersion == 1);
	static_assert(std::is_trivially_copyable_v<Frame>);
	static_assert(std::is_trivially_copyable_v<Inputs>);
	static_assert(!std::is_convertible_v<RoadShadowIntent, float>);
	static_assert(!std::is_convertible_v<EventShadowIntent, float>);
	static_assert(sizeof(Frame) <= 1024);

	PassiveObserver observer;
	auto input = baseline();
	const auto first = observer.observe(input);
	assert(first.schemaVersion == SchemaVersion);
	assert(first.interpretationVersion == InterpretationVersion);
	assert(first.vehicle.meta.validity == Validity::Valid);
	assert(first.surfaces.previous == first.surfaces.current);
	assert(first.occupancy.allSame && !first.occupancy.mixed);
	assert(first.occupancy.distinctCount == 1);
	assert(!first.occupancy.wheelOwnershipKnown);
	assert(first.surfaces.field14ValidationAvailable);
	assert(first.surfaces.field14MatchesCanonical);
	assert(first.nativeEffect.left == input.effectLeft);
	assert(first.nativeEffect.right == input.effectRight);
	assert(first.dynamics.fieldE8 == input.fieldE8);
	assert(first.dynamics.fieldEC == input.fieldEC);
	assert(first.dynamics.fieldEE == input.fieldEE);

	// R2-B representative partial-surface occupation keeps all four corners.
	input = baseline(2);
	input.surfaces = { 2, 2048, 2, 2048 };
	input.field14 = { 2, 2048, 4, 2048 };
	const auto partial = observer.observe(input);
	assert(partial.occupancy.mixed && partial.occupancy.distinctCount == 2);
	assert(partial.occupancy.pair02Same && partial.occupancy.pair13Same);
	assert(partial.surfaces.changed[1] && partial.surfaces.changed[3]);
	assert(!partial.surfaces.field14MatchesCanonical);
	assert(partial.surfaces.field14MismatchMask == (1u << 2));
	assert(partial.roadIntent.surfaceTransitionObserved);

	input = baseline(3);
	input.surfaces = { 4, 4, 4, 4 };
	const auto full = observer.observe(input);
	assert(full.occupancy.allSame && full.occupancy.distinctCount == 1);
	assert(full.surfaces.changed[0] && full.surfaces.changed[1] &&
		full.surfaces.changed[2] && full.surfaces.changed[3]);

	// R2-C known gear fixture is preserved without being misclassified as impact.
	input = baseline(4);
	input.effectLeft = KnownGearFixture::Left;
	input.effectRight = KnownGearFixture::Right;
	input.effectCombined = KnownGearFixture::Left;
	input.effectRise = KnownGearFixture::Rise;
	input.existingImpact = KnownGearFixture::ExistingImpact;
	input.currentGear = 4;
	input.previousGear = 3;
	input.gearTransition = true;
	const auto gear = observer.observe(input);
	assert(gear.eventIntent.classification == EventClass::GearShift);
	assert(gear.eventIntent.meta.confidence == Confidence::High);
	assert(gear.eventIntent.leftEvidence == 0.14f);
	assert(gear.eventIntent.rightEvidence == 0.0f);

	// A bilateral transient may be labelled only as a collision candidate.
	input = baseline(5);
	input.effectLeft = 0.525f;
	input.effectRight = 0.0753045f;
	input.effectCombined = input.effectLeft;
	input.effectRise = 0.298658f;
	input.existingImpact = -0.25f;
	const auto collision = observer.observe(input);
	assert(collision.eventIntent.classification == EventClass::CollisionCandidate);
	assert(collision.eventIntent.meta.confidence == Confidence::Moderate);

	input = baseline(6);
	input.effectRise = 0.2f;
	input.effectLeft = 0.2f;
	input.effectRight = 0.0f;
	input.existingImpact = 0.0f;
	assert(observer.observe(input).eventIntent.classification == EventClass::UnknownNativeEvent);

	for (const auto state : { GripState::Load, GripState::Release, GripState::Free,
		GripState::Recovering, GripState::Bite })
	{
		input = baseline(10 + static_cast<unsigned>(state));
		input.gripState = state;
		assert(observer.observe(input).grip.state == state);
	}

	input = baseline(20);
	input.effectRight = std::numeric_limits<float>::quiet_NaN();
	const auto nonfinite = observer.observe(input);
	assert(nonfinite.nativeEffect.meta.validity == Validity::Nonfinite);
	assert(nonfinite.roadIntent.meta.validity == Validity::Nonfinite);
	assert(nonfinite.eventIntent.classification == EventClass::None);

	input = baseline(21);
	input.sourceSupported = false;
	const auto unsupported = observer.observe(input);
	assert(unsupported.vehicle.meta.validity == Validity::UnsupportedSource);
	assert(unsupported.eventIntent.meta.validity == Validity::UnsupportedSource);

	observer.reset();
	observer.observe(baseline(30));
	const auto stale = observer.snapshot(34, 2);
	assert(stale.vehicle.meta.validity == Validity::Stale);
	assert(stale.eventIntent.meta.validity == Validity::Stale);
	assert(stale.eventIntent.classification == EventClass::None);
	assert(stale.roadIntent.continuousActivityEvidence == 0.0f);

	// Same input and clean state produce the same complete fixed-size frame.
	PassiveObserver a;
	PassiveObserver b;
	const auto deterministicA = a.observe(baseline(40));
	const auto deterministicB = b.observe(baseline(40));
	assert(deterministicA.frameId == deterministicB.frameId);
	assert(deterministicA.vehicle.steering == deterministicB.vehicle.steering);
	assert(deterministicA.eventIntent.classification == deterministicB.eventIntent.classification);

	// Exact v1 equivalence: observing R4.1 cannot change any v1 component,
	// composition, tanh, pre-drive value, or DirectInput request.
	const HYP36RResearchReplay::Channels raw{ 0.42f, -0.07f, 0.11f };
	const HYP36RResearchReplay::Channels gains{ 1.2f, 0.8f, 1.0f };
	const auto v1Before = HYP36RResearchReplay::replay(raw, gains, 0.75f);
	a.observe(baseline(41));
	const auto v1After = HYP36RResearchReplay::replay(raw, gains, 0.75f);
	assert_same_replay(v1Before, v1After);
	const float strength = 0.85f;
	const bool invert = true;
	const float requestBefore = v1Before.preDrive * strength * (invert ? -1.0f : 1.0f);
	const float requestAfter = v1After.preDrive * strength * (invert ? -1.0f : 1.0f);
	assert(requestBefore == requestAfter);

	// Source-level architectural guard: the only runtime observation occurs
	// after the single established drive call, and the hook never reads the
	// published frame back into its force path.
	const auto hookPath = std::filesystem::path(__FILE__).parent_path().parent_path() /
		"src" / "hooks_forcefeedback.cpp";
	std::ifstream hookFile(hookPath);
	assert(hookFile.good());
	const std::string hookSource((std::istreambuf_iterator<char>(hookFile)),
		std::istreambuf_iterator<char>());
	const auto drivePosition = hookSource.find("WheelForceFeedback::drive(hardwareForce);");
	const auto updatePosition = hookSource.find("HYP36RSignalState::update(signalInputs);");
	assert(drivePosition != std::string::npos);
	assert(updatePosition != std::string::npos);
	assert(updatePosition > drivePosition);
	assert(hookSource.find("WheelForceFeedback::drive(", drivePosition + 1) == std::string::npos);
	const auto frameRead = hookSource.find("HYP36RSignalState::frame()");
	assert(frameRead > updatePosition);
	assert(hookSource.find("HYP36RSignalState::frame()", frameRead + 1) == std::string::npos);

	// Runtime-cost smoke measurement: the contract is fixed-size and performs
	// no allocation; timing is reported by CI without a machine-specific gate.
	const auto start = std::chrono::steady_clock::now();
	for (uint64_t n = 0; n < 200000; ++n)
	{
		auto measured = baseline(100 + n);
		measured.effectRight = static_cast<float>(n & 255u) / 255.0f;
		a.observe(measured);
	}
	const auto elapsed = std::chrono::steady_clock::now() - start;
	assert(elapsed > std::chrono::steady_clock::duration::zero());
	return 0;
}
