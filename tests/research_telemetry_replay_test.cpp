#include "research_telemetry_replay.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>

using namespace HYP36RResearchReplay;

static bool near(float a, float b)
{
	return std::abs(a - b) < 0.000001f;
}

int main()
{
	const Channels raw{ -0.30f, 0.05f, 0.10f };
	const auto identity = replay(raw, { 1.0f, 1.0f, 1.0f }, 0.75f);
	assert(near(identity.composerInput, -0.15f));
	assert(near(identity.postTanh, std::tanh(-0.15f)));
	assert(near(identity.preDrive, std::tanh(-0.15f) * 0.75f));

	// Offline ceiling sweeps alter presentation only. Raw vehicle/Force state
	// remains the same input for every counterfactual replay.
	for (float gain : { 1.0f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f, 1.75f, 2.0f })
	{
		const auto steering = replay(raw, { gain, 1.0f, 1.0f }, 0.75f);
		const auto road = replay(raw, { 1.0f, gain, 1.0f }, 0.75f);
		const auto impact = replay(raw, { 1.0f, 1.0f, gain }, 0.75f);
		const auto all = replay(raw, { gain, gain, gain }, 0.75f);
		assert(near(steering.postGain.directional, raw.directional * gain));
		assert(near(road.postGain.road, raw.road * gain));
		assert(near(impact.postGain.impact, raw.impact * gain));
		assert(std::isfinite(all.preDrive));
	}

	const std::array<unsigned, 4> before{ 1, 1, 1, 1 };
	const auto changed = surface_changes(before, { 2, 1, 3, 1 });
	assert(changed[0] && !changed[1] && changed[2] && !changed[3]);
}
