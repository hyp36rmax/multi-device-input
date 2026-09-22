#include "force_character_presentation.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <initializer_list>
#include <limits>

using namespace HYP36RForceCharacter;

static bool near(float a, float b)
{
	return std::abs(a - b) < 0.000001f;
}

int main()
{
	constexpr Channels raw{ -0.32f, 0.08f, -0.12f };
	const auto identity = apply(raw, {});
	assert(identity.directional == raw.directional);
	assert(identity.road == raw.road);
	assert(identity.impact == raw.impact);
	assert(near(std::tanh(identity.directional + identity.road + identity.impact) * 0.6f,
		std::tanh(raw.directional + raw.road + raw.impact) * 0.6f));
	assert(DefaultPercent == 100);
	assert(MaximumPercent == 100);
	assert(!HasIndependentWheelVibration);

	const auto noSteering = apply(raw, { 0, 100, 100 });
	assert(noSteering.directional == 0.0f && noSteering.road == raw.road && noSteering.impact == raw.impact);
	const auto noRoad = apply(raw, { 100, 0, 100 });
	assert(noRoad.directional == raw.directional && noRoad.road == 0.0f && noRoad.impact == raw.impact);
	const auto noImpact = apply(raw, { 100, 100, 0 });
	assert(noImpact.directional == raw.directional && noImpact.road == raw.road && noImpact.impact == 0.0f);

	const auto half = apply(raw, { 50, 50, 50 });
	assert(near(half.directional, raw.directional * 0.5f));
	assert(near(half.road, raw.road * 0.5f));
	assert(near(half.impact, raw.impact * 0.5f));
	for (int percent : { 0, 50, 75, 100, 110, 120, 130, 140, 150 })
	{
		const auto output = apply(raw, { percent, percent, percent });
		const float gain = float(clamp_percent(percent)) / 100.0f;
		assert(near(output.directional, raw.directional * gain));
		assert(near(output.road, raw.road * gain));
		assert(near(output.impact, raw.impact * gain));
		assert(std::isfinite(std::tanh(output.directional + output.road + output.impact)));
		assert(output.directional <= 0.0f && output.road >= 0.0f && output.impact <= 0.0f);
	}
	assert(clamp_percent(-25) == 0);
	assert(clamp_percent(175) == 100);
	const auto nonfinite = apply({ std::numeric_limits<float>::quiet_NaN(),
		std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity() }, {});
	assert(nonfinite.directional == 0.0f && nonfinite.road == 0.0f && nonfinite.impact == 0.0f);
}
