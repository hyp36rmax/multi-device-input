#include "research_telemetry_replay.hpp"
#include "road2_policy.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <type_traits>

using namespace HYP36RSignalState;
namespace Road2 = HYP36RRoad2;

namespace
{
	Inputs input(uint64_t frame, std::array<uint32_t, 4> surfaces,
		float left = 0.0f, float right = 0.0f, float rise = 0.0f)
	{
		Inputs i{};
		i.frameId = frame;
		i.surfaces = surfaces;
		i.effectLeft = left;
		i.effectRight = right;
		i.effectCombined = left > right ? left : right;
		i.effectRise = rise;
		i.gripState = GripState::Load;
		return i;
	}

	void settle(PassiveObserver& observer, Road2::PassivePolicy& policy,
		uint64_t& frame, uint32_t raw)
	{
		for (unsigned n = 0; n < 6; ++n)
			policy.evaluate(observer.observe(input(++frame, { raw, raw, raw, raw })));
		assert(policy.frame().spatial.referenceEstablished);
		assert(policy.frame().spatial.referenceRaw == raw);
	}

	void assert_replay_equal(const HYP36RResearchReplay::Result& a,
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
	static_assert(Road2::PolicyVersion == 1);
	static_assert(std::is_trivially_copyable_v<Road2::Frame>);
	static_assert(!std::is_convertible_v<Road2::Frame, float>);
	static_assert(!std::is_convertible_v<Road2::ContinuousActivity, float>);
	static_assert(sizeof(Road2::Frame) <= 512);

	PassiveObserver observer;
	Road2::PassivePolicy policy;
	uint64_t frame = 0;
	settle(observer, policy, frame, 2);

	// B01: stable uniform local control does not manufacture Road activity.
	const auto b01 = policy.evaluate(observer.observe(input(++frame, { 2, 2, 2, 2 }, 0.21f, 0.21f)));
	assert(b01.policyVersion == Road2::PolicyVersion);
	assert(b01.spatial.topology == Road2::OccupancyTopology::Uniform);
	assert(b01.spatial.coverage == Road2::Coverage::Reference);
	assert(b01.activity.state == Road2::ActivityState::Inactive);
	assert(b01.activity.exclusion == Road2::ExclusionReason::NoSurfaceContext);

	// B02: partial paired occupancy preserves group 1/3 independently.
	const auto b02 = policy.evaluate(observer.observe(input(++frame, { 2, 2048, 2, 2048 }, 0.0f, 0.08f)));
	assert(b02.spatial.topology == Road2::OccupancyTopology::PairedSides);
	assert(b02.spatial.coverage == Road2::Coverage::Partial);
	assert(b02.spatial.group02.differingFromReference == 0);
	assert(b02.spatial.group13.differingFromReference == 2);
	assert(b02.transition.active && b02.transition.changedMask == 0xA);
	assert(b02.activity.state == Road2::ActivityState::ContinuousCandidate);

	// The other established side grouping remains independently representable.
	const auto group02 = policy.evaluate(observer.observe(input(++frame, { 8192, 2, 8192, 2 }, 0.02f, 0.04f)));
	assert(group02.spatial.group02.differingFromReference == 2);
	assert(group02.spatial.group13.differingFromReference == 0);

	// B03: broad/full occupancy is context, not an intensity multiplier.
	const auto b03 = policy.evaluate(observer.observe(input(++frame,
		{ 1024, 1024, 1024, 1024 }, 0.01f, 0.01f)));
	assert(b03.spatial.topology == Road2::OccupancyTopology::Uniform);
	assert(b03.spatial.coverage == Road2::Coverage::Broad);
	assert(b03.spatial.differingFromReference == 4);
	assert(b03.activity.rightEvidence == 0.01f);

	// B04/B05: persistent activity remains independent from partial/broad context.
	const auto b04 = policy.evaluate(observer.observe(input(++frame,
		{ 4, 8192, 4, 8192 }, 0.02f, 0.08f)));
	assert(b04.spatial.coverage == Road2::Coverage::Broad);
	assert(b04.activity.state == Road2::ActivityState::ContinuousCandidate);
	const auto b05 = policy.evaluate(observer.observe(input(++frame,
		{ 4, 4, 4, 4 }, 0.20f, 0.20f)));
	assert(b05.spatial.coverage == Road2::Coverage::Broad);
	assert(b05.activity.state == Road2::ActivityState::ContinuousCandidate);
	// No scalar comparison asserts B05 is stronger than B04.

	// B06: re-entry timing is retained as a surface event, separate from grip.
	const auto b06Mixed = policy.evaluate(observer.observe(input(++frame,
		{ 8, 2048, 8, 2048 }, 0.05f, 0.10f)));
	assert(b06Mixed.transition.active);
	const auto b06Return = policy.evaluate(observer.observe(input(++frame,
		{ 2, 2, 2, 2 }, 0.0f, 0.0f)));
	assert(b06Return.transition.active && b06Return.transition.changedCount == 4);
	assert(b06Return.spatial.coverage == Road2::Coverage::Reference);

	// R2-A grip changes alone cannot create Road activity.
	auto gripOnly = input(++frame, { 2, 2, 2, 2 }, 0.1f, 0.1f);
	gripOnly.gripState = GripState::Bite;
	const auto bite = policy.evaluate(observer.observe(gripOnly));
	assert(bite.activity.state == Road2::ActivityState::Inactive);

	// R2-C gear and collision identities exclude native effects from Road.
	auto gearInput = input(++frame, { 4, 4, 4, 4 }, KnownGearFixture::Left,
		KnownGearFixture::Right, KnownGearFixture::Rise);
	gearInput.currentGear = 4;
	gearInput.previousGear = 3;
	gearInput.gearTransition = true;
	gearInput.existingImpact = KnownGearFixture::ExistingImpact;
	const auto gear = policy.evaluate(observer.observe(gearInput));
	assert(gear.activity.state == Road2::ActivityState::EventExcluded);
	assert(gear.activity.exclusion == Road2::ExclusionReason::GearEvent);

	auto collisionInput = input(++frame, { 4, 4, 4, 4 }, 0.525f, 0.0753045f, 0.298658f);
	collisionInput.existingImpact = 0.2f;
	const auto collision = policy.evaluate(observer.observe(collisionInput));
	assert(collision.activity.state == Road2::ActivityState::EventExcluded);
	assert(collision.activity.exclusion == Road2::ExclusionReason::CollisionCandidate);

	auto unknownEventInput = input(++frame, { 4, 4, 4, 4 }, 0.2f, 0.0f, 0.2f);
	const auto unknownEvent = policy.evaluate(observer.observe(unknownEventInput));
	assert(unknownEvent.activity.exclusion == Road2::ExclusionReason::UnknownNativeEvent);

	// Unknown raw identity remains raw and receives no material-derived strength.
	const auto unknownRaw = policy.evaluate(observer.observe(input(++frame,
		{ 999, 2, 999, 2 }, 0.0f, 0.0f)));
	assert(unknownRaw.spatial.group02.raw[0] == 999);
	assert(unknownRaw.spatial.group02.raw[1] == 999);
	assert(unknownRaw.spatial.coverage == Road2::Coverage::Partial);

	// Invalid, stale and non-finite observations fail closed.
	auto staleSignal = observer.snapshot(frame + 5, 2);
	const auto stale = policy.evaluate(staleSignal);
	assert(stale.meta.validity == Validity::Stale);
	assert(stale.activity.state == Road2::ActivityState::Unavailable);

	auto unsupportedInput = input(++frame, { 2, 2, 2, 2 });
	unsupportedInput.sourceSupported = false;
	const auto unsupported = policy.evaluate(observer.observe(unsupportedInput));
	assert(unsupported.meta.validity == Validity::UnsupportedSource);

	auto unsupportedSchema = observer.observe(input(++frame, { 2, 2, 2, 2 }));
	unsupportedSchema.schemaVersion = 999;
	const auto schemaRejected = policy.evaluate(unsupportedSchema);
	assert(schemaRejected.meta.validity == Validity::Unavailable);
	assert(schemaRejected.meta.fallback == FallbackReason::UnsupportedSource);

	auto lowConfidence = observer.observe(input(++frame, { 2, 2, 2, 2 }));
	lowConfidence.surfaces.meta.confidence = Confidence::Possible;
	const auto confidenceRejected = policy.evaluate(lowConfidence);
	assert(confidenceRejected.meta.validity == Validity::Unavailable);
	assert(confidenceRejected.activity.exclusion == Road2::ExclusionReason::LowConfidence);

	auto nonfiniteInput = input(++frame, { 4, 4, 4, 4 });
	nonfiniteInput.effectRight = std::numeric_limits<float>::quiet_NaN();
	const auto nonfinite = policy.evaluate(observer.observe(nonfiniteInput));
	assert(nonfinite.activity.meta.validity == Validity::Nonfinite);
	assert(nonfinite.activity.state == Road2::ActivityState::Unavailable);

	// Provenance remains derived-with-evidence while source observations remain
	// untouched in Signal State.
	observer.reset();
	policy.reset();
	frame = 100;
	settle(observer, policy, frame, 2);
	const auto provenance = policy.evaluate(observer.observe(input(++frame,
		{ 2, 2048, 2, 2048 }, 0.01f, 0.03f)));
	assert(provenance.meta.source == Source::DerivedEvidenceSupported);
	assert(observer.frame().surfaces.meta.source == Source::NativeSurfaceState);
	assert(observer.frame().nativeEffect.meta.source == Source::RestoredNativeEffect);

	// Clean-state determinism.
	PassiveObserver observerA, observerB;
	Road2::PassivePolicy policyA, policyB;
	uint64_t frameA = 0, frameB = 0;
	settle(observerA, policyA, frameA, 2);
	settle(observerB, policyB, frameB, 2);
	const auto deterministicA = policyA.evaluate(observerA.observe(input(7,
		{ 2, 2048, 2, 2048 }, 0.02f, 0.08f)));
	const auto deterministicB = policyB.evaluate(observerB.observe(input(7,
		{ 2, 2048, 2, 2048 }, 0.02f, 0.08f)));
	assert(deterministicA.spatial.coverage == deterministicB.spatial.coverage);
	assert(deterministicA.transition.changedMask == deterministicB.transition.changedMask);
	assert(deterministicA.activity.state == deterministicB.activity.state);

	// Exact v1 equivalence through final request.
	const HYP36RResearchReplay::Channels raw{ 0.42f, -0.07f, 0.11f };
	const HYP36RResearchReplay::Channels gains{ 1.2f, 0.8f, 1.0f };
	const auto before = HYP36RResearchReplay::replay(raw, gains, 0.75f);
	policyA.evaluate(observerA.observe(input(8, { 4, 8192, 4, 8192 }, 0.02f, 0.08f)));
	const auto after = HYP36RResearchReplay::replay(raw, gains, 0.75f);
	assert_replay_equal(before, after);
	assert(before.preDrive * -0.85f == after.preDrive * -0.85f);

	// Source-level boundary: Signal State and Road policy execute only after the
	// sole v1 drive call and are never read back into force composition.
	const auto hookPath = std::filesystem::path(__FILE__).parent_path().parent_path() /
		"src" / "hooks_forcefeedback.cpp";
	std::ifstream hookFile(hookPath);
	assert(hookFile.good());
	const std::string source((std::istreambuf_iterator<char>(hookFile)),
		std::istreambuf_iterator<char>());
	const auto drivePos = source.find("WheelForceFeedback::drive(hardwareForce);");
	const auto signalPos = source.find("HYP36RSignalState::update(signalInputs);");
	const auto roadPos = source.find("HYP36RRoad2::evaluate(HYP36RSignalState::frame());");
	assert(drivePos < signalPos && signalPos < roadPos);
	assert(source.find("HYP36RRoad2::frame()") == std::string::npos);
	assert(source.find("WheelForceFeedback::drive(", drivePos + 1) == std::string::npos);

	const auto start = std::chrono::steady_clock::now();
	for (uint64_t n = 0; n < 200000; ++n)
		policyA.evaluate(observerA.observe(input(1000 + n,
			{ 2, 2048, 2, 2048 }, 0.02f, static_cast<float>(n & 255u) / 255.0f)));
	assert(std::chrono::steady_clock::now() > start);
	return 0;
}
