#include "research_telemetry_replay.hpp"
#include "road2_periodic_shadow.hpp"

#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <type_traits>

namespace Road2P = HYP36RRoad2Presentation;
namespace Periodic = HYP36RRoad2Periodic;

namespace
{
	Road2P::Frame road(uint64_t frame, float amplitude, bool active = true)
	{
		Road2P::Frame value{};
		value.frameId = frame;
		value.meta.validity = HYP36RSignalState::Validity::Valid;
		value.meta.confidence = HYP36RSignalState::Confidence::Moderate;
		value.continuous.normalized = amplitude;
		value.continuous.active = active;
		return value;
	}

	Periodic::TimingInput timing(uint64_t frame, float dt, float speed, uint64_t session = 1)
	{
		return { session, frame, dt, speed };
	}
}

int main()
{
	static_assert(Periodic::ContractVersion == 1);
	static_assert(std::is_trivially_copyable_v<Periodic::Frame>);
	static_assert(!std::is_convertible_v<Periodic::Frame, float>);
	static_assert(sizeof(Periodic::Frame) <= 192);

	Periodic::PassivePeriodicRequest model;
	double timeSum = 0.0, distanceSum = 0.0;
	for (uint64_t n = 1; n <= 6000; ++n)
	{
		const auto result = model.evaluate(road(n, 0.4f), timing(n, 1.0f / 60.0f, 0.5f));
		assert(result.valid && result.available);
		assert(std::abs(result.timePostBound) <= 0.4f + 1.0e-6f);
		assert(std::abs(result.distancePostBound) <= 0.4f + 1.0e-6f);
		timeSum += result.timePostBound;
		distanceSum += result.distancePostBound;
	}
	assert(std::abs(timeSum / 6000.0) < 1.0e-4);
	assert(std::abs(distanceSum / 6000.0) < 1.0e-4);

	// Distance phase stops with the car; time phase remains the explicit
	// comparison model. Neither can emit amplitude without Road authority.
	const auto moving = model.evaluate(road(6001, 0.4f), timing(6001, 1.0f / 60.0f, 0.5f));
	const auto stopped = model.evaluate(road(6002, 0.4f), timing(6002, 1.0f / 60.0f, 0.0f));
	assert(stopped.distancePhase == moving.distancePhase);
	assert(stopped.timePhase != moving.timePhase);

	const auto normal = model.evaluate(road(6003, 0.0f, false), timing(6003, 1.0f / 60.0f, 0.5f));
	assert(!normal.valid && normal.timePostBound == 0.0f && normal.distancePostBound == 0.0f);
	assert(normal.resetReason == Periodic::ResetReason::RoadInactive);

	// Timing, source, session and non-finite failures all fail closed.
	assert(!model.evaluate(road(6004, 0.4f), timing(6004, 0.0f, 0.5f)).valid);
	assert(!model.evaluate(road(6005, 0.4f), timing(6005, -0.1f, 0.5f)).valid);
	assert(!model.evaluate(road(6006, 0.4f), timing(6006, 0.5f, 0.5f)).valid);
	auto invalidRoad = road(6007, std::numeric_limits<float>::quiet_NaN());
	assert(!model.evaluate(invalidRoad, timing(6007, 1.0f / 60.0f, 0.5f)).valid);
	auto unavailable = road(6008, 0.4f);
	unavailable.meta.validity = HYP36RSignalState::Validity::Stale;
	assert(!model.evaluate(unavailable, timing(6008, 1.0f / 60.0f, 0.5f)).valid);
	const auto newSession = model.evaluate(road(1, 0.4f), timing(1, 1.0f / 60.0f, 0.5f, 2));
	assert(newSession.resetReason == Periodic::ResetReason::SessionChanged);

	// Exact current-Force replay equivalence: the passive result is not consumed.
	const HYP36RResearchReplay::Channels raw{ 0.42f, -0.07f, 0.11f };
	const HYP36RResearchReplay::Channels gains{ 1.2f, 0.8f, 1.0f };
	const auto before = HYP36RResearchReplay::replay(raw, gains, 0.75f);
	model.evaluate(road(2, 0.8f), timing(2, 1.0f / 60.0f, 0.9f, 2));
	const auto after = HYP36RResearchReplay::replay(raw, gains, 0.75f);
	assert(before.postGain.directional == after.postGain.directional);
	assert(before.postGain.road == after.postGain.road);
	assert(before.postGain.impact == after.postGain.impact);
	assert(before.composerInput == after.composerInput);
	assert(before.postTanh == after.postTanh);
	assert(before.preDrive == after.preDrive);

	const auto hookPath = std::filesystem::path(__FILE__).parent_path().parent_path() /
		"src" / "hooks_forcefeedback.cpp";
	std::ifstream hookFile(hookPath);
	const std::string hookSource((std::istreambuf_iterator<char>(hookFile)), {});
	assert(hookSource.find("road2_periodic_shadow") == std::string::npos);
	assert(hookSource.find("HYP36RRoad2Periodic") == std::string::npos);

	const auto start = std::chrono::steady_clock::now();
	for (uint64_t n = 0; n < 200000; ++n)
		model.evaluate(road(n, static_cast<float>(n & 255u) / 255.0f),
			timing(n, 1.0f / 60.0f, static_cast<float>(n & 63u) / 63.0f, 3));
	assert(std::chrono::steady_clock::now() > start);
	return 0;
}
