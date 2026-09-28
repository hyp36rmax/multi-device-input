#include "research_telemetry_replay.hpp"
#include "road2_presentation.hpp"

#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <type_traits>

using namespace HYP36RSignalState;
namespace Road2 = HYP36RRoad2;
namespace Road2P = HYP36RRoad2Presentation;

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
		Road2P::PassivePrototype& direct, Road2P::PassivePrototype& conditioned,
		uint64_t& frame, uint32_t raw)
	{
		for (unsigned n = 0; n < 6; ++n)
		{
			const auto& state = observer.observe(input(++frame, { raw, raw, raw, raw }));
			const auto& road = policy.evaluate(state);
			direct.evaluate(road);
			conditioned.evaluate(road);
		}
	}

	void replay_equal(const HYP36RResearchReplay::Result& a,
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
	static_assert(Road2P::PresentationVersion == 1);
	static_assert(std::is_trivially_copyable_v<Road2P::Frame>);
	static_assert(!std::is_convertible_v<Road2P::Frame, float>);
	static_assert(!std::is_convertible_v<Road2P::ContinuousEnvelope, float>);
	static_assert(!std::is_convertible_v<Road2P::SpatialImbalance, float>);
	static_assert(sizeof(Road2P::Frame) <= 256);

	PassiveObserver observer;
	Road2::PassivePolicy policy;
	Road2P::PassivePrototype direct(Road2P::Candidate::Direct);
	Road2P::PassivePrototype conditioned(Road2P::Candidate::Conditioned3);
	uint64_t frame = 0;
	settle(observer, policy, direct, conditioned, frame, 2);

	// B01 stable control: native effects alone cannot manufacture Road.
	const auto& b01Policy = policy.evaluate(observer.observe(input(++frame,
		{ 2, 2, 2, 2 }, 0.21f, 0.21f)));
	const auto b01 = direct.evaluate(b01Policy);
	assert(!b01.continuous.active);
	assert(!b01.spatial.asymmetric);
	assert(!b01.transition.active);

	// B02 paired partial context: continuous, neutral-group imbalance and
	// transition identity remain independent.
	const auto& b02Policy = policy.evaluate(observer.observe(input(++frame,
		{ 2, 2048, 2, 2048 }, 0.02f, 0.08f)));
	const auto b02Direct = direct.evaluate(b02Policy);
	const auto b02Conditioned = conditioned.evaluate(b02Policy);
	assert(b02Direct.continuous.active);
	assert(b02Direct.continuous.normalized > 0.0f && b02Direct.continuous.normalized <= 1.0f);
	assert(b02Direct.spatial.group02Context == 0.0f);
	assert(b02Direct.spatial.group13Context == 1.0f);
	assert(b02Direct.spatial.neutralGroupAxis == -1.0f);
	assert(b02Direct.transition.active && b02Direct.transition.changedMask == 0xA);
	assert(b02Conditioned.transition.changedMask == b02Direct.transition.changedMask);
	assert(std::abs(b02Conditioned.spatial.neutralGroupAxis) <
		std::abs(b02Direct.spatial.neutralGroupAxis));

	// B03 broad uniform context is not automatically stronger than partial.
	const auto& b03Policy = policy.evaluate(observer.observe(input(++frame,
		{ 1024, 1024, 1024, 1024 }, 0.01f, 0.01f)));
	const auto b03 = direct.evaluate(b03Policy);
	assert(b03.spatial.group02Context == 1.0f && b03.spatial.group13Context == 1.0f);
	assert(!b03.spatial.asymmetric);
	assert(b03.continuous.normalized < b02Direct.continuous.normalized);

	// B04/B05 retain bounded activity under partial/mixed and broad/uniform
	// conditions without a material multiplier.
	const auto b04 = direct.evaluate(policy.evaluate(observer.observe(input(++frame,
		{ 4, 8192, 4, 8192 }, 0.02f, 0.08f))));
	assert(b04.continuous.active && b04.spatial.asymmetric);
	assert(b04.spatial.categoricalDifference);
	assert(b04.spatial.neutralGroupAxis == 0.0f);
	const auto b05 = direct.evaluate(policy.evaluate(observer.observe(input(++frame,
		{ 4, 4, 4, 4 }, 0.20f, 0.20f))));
	assert(b05.continuous.active && !b05.spatial.asymmetric);
	assert(b05.continuous.normalized <= 1.0f);

	// B06 transition stays explicit and grip state never enters presentation.
	auto b06Input = input(++frame, { 8, 2048, 8, 2048 }, 0.05f, 0.10f);
	b06Input.gripState = GripState::Bite;
	const auto b06 = direct.evaluate(policy.evaluate(observer.observe(b06Input)));
	assert(b06.transition.active);
	assert(b06.transition.changedCount == 4);
	assert(b06.spatial.asymmetric);

	// Gear and collision candidates remain zeroed event exclusions.
	auto gearInput = input(++frame, { 4, 4, 4, 4 }, KnownGearFixture::Left,
		KnownGearFixture::Right, KnownGearFixture::Rise);
	gearInput.gearTransition = true;
	gearInput.currentGear = 4;
	gearInput.previousGear = 3;
	gearInput.existingImpact = KnownGearFixture::ExistingImpact;
	const auto gear = direct.evaluate(policy.evaluate(observer.observe(gearInput)));
	assert(!gear.continuous.active && gear.continuous.normalized == 0.0f);

	auto collisionInput = input(++frame, { 4, 4, 4, 4 }, 0.525f, 0.0753045f, 0.298658f);
	collisionInput.existingImpact = 0.2f;
	const auto collision = direct.evaluate(policy.evaluate(observer.observe(collisionInput)));
	assert(!collision.continuous.active && collision.continuous.normalized == 0.0f);

	// Invalid policy fails closed with finite, bounded zero components.
	auto staleState = observer.snapshot(frame + 5, 2);
	const auto stale = direct.evaluate(policy.evaluate(staleState));
	assert(stale.meta.validity == Validity::Stale);
	assert(stale.continuous.normalized == 0.0f);
	assert(stale.spatial.neutralGroupAxis == 0.0f);
	assert(std::isfinite(stale.continuous.normalized));

	// Candidate determinism from identical clean histories.
	PassiveObserver observerA, observerB;
	Road2::PassivePolicy policyA, policyB;
	Road2P::PassivePrototype modelA(Road2P::Candidate::Conditioned3);
	Road2P::PassivePrototype modelB(Road2P::Candidate::Conditioned3);
	Road2P::PassivePrototype unusedA(Road2P::Candidate::Direct);
	Road2P::PassivePrototype unusedB(Road2P::Candidate::Direct);
	uint64_t frameA = 0, frameB = 0;
	settle(observerA, policyA, unusedA, modelA, frameA, 2);
	settle(observerB, policyB, unusedB, modelB, frameB, 2);
	const auto resultA = modelA.evaluate(policyA.evaluate(observerA.observe(input(7,
		{ 2, 2048, 2, 2048 }, 0.02f, 0.08f))));
	const auto resultB = modelB.evaluate(policyB.evaluate(observerB.observe(input(7,
		{ 2, 2048, 2, 2048 }, 0.02f, 0.08f))));
	assert(resultA.continuous.normalized == resultB.continuous.normalized);
	assert(resultA.spatial.neutralGroupAxis == resultB.spatial.neutralGroupAxis);

	// Exact Reference+ v1 equivalence.
	const HYP36RResearchReplay::Channels raw{ 0.42f, -0.07f, 0.11f };
	const HYP36RResearchReplay::Channels gains{ 1.2f, 0.8f, 1.0f };
	const auto before = HYP36RResearchReplay::replay(raw, gains, 0.75f);
	modelA.evaluate(policyA.evaluate(observerA.observe(input(8,
		{ 4, 8192, 4, 8192 }, 0.02f, 0.08f))));
	const auto after = HYP36RResearchReplay::replay(raw, gains, 0.75f);
	replay_equal(before, after);
	assert(before.preDrive * -0.85f == after.preDrive * -0.85f);

	// R4.2H promotes the validated Direct candidate into the active Road-only
	// path; its output is selected before the sole DirectInput drive call.
	const auto hookPath = std::filesystem::path(__FILE__).parent_path().parent_path() /
		"src" / "hooks_forcefeedback.cpp";
	std::ifstream hookFile(hookPath);
	assert(hookFile.good());
	const std::string hookSource((std::istreambuf_iterator<char>(hookFile)),
		std::istreambuf_iterator<char>());
	assert(hookSource.find("road2_presentation") != std::string::npos);
	const auto presentationPosition = hookSource.find("RuntimeRoadPresentation.evaluate(");
	const auto activePosition = hookSource.find("HYP36RRoad2Active::evaluate(");
	const auto drivePosition = hookSource.find("WheelForceFeedback::drive(hardwareForce);");
	assert(presentationPosition < activePosition && activePosition < drivePosition);

	const auto start = std::chrono::steady_clock::now();
	for (uint64_t n = 0; n < 200000; ++n)
		modelA.evaluate(policyA.evaluate(observerA.observe(input(1000 + n,
			{ 2, 2048, 2, 2048 }, 0.02f, static_cast<float>(n & 255u) / 255.0f))));
	assert(std::chrono::steady_clock::now() > start);
	return 0;
}
