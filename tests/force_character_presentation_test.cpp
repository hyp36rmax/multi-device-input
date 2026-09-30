#include "force_character_presentation.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <string>

using namespace HYP36RForceCharacter;

static bool near(float a, float b)
{
	return std::abs(a - b) < 0.000001f;
}

int main()
{
	static_assert(SurfaceTooltip == "Adjusts how strongly road textures, bumps, and changes in driving surface are felt through the wheel.");
	constexpr Channels raw{ -0.32f, 0.08f, -0.12f };
	const auto identity = apply(raw, {});
	assert(identity.directional == raw.directional);
	assert(identity.road == raw.road);
	assert(identity.impact == raw.impact);
	assert(near(std::tanh(identity.directional + identity.road + identity.impact) * 0.6f,
		std::tanh(raw.directional + raw.road + raw.impact) * 0.6f));
	assert(DefaultPercent == 100);
	assert(SteeringMaximumPercent == 130);
	assert(RoadMaximumPercent == 200);
	assert(ImpactMaximumPercent == 150);
	assert(PlayerUiMaximumPercent == 100);
	assert(!HasIndependentWheelVibration);
	const auto reference = reference_plus();
	assert(reference.steeringLoad == 100 && reference.roadDetail == 100 && reference.impact == 100);
	assert(recommended_player_percent(SteeringMaximumPercent) == 77);
	assert(recommended_player_percent(RoadMaximumPercent) == 50);
	assert(recommended_player_percent(ImpactMaximumPercent) == 67);
	assert(from_player_percent(77, SteeringMaximumPercent) == 100);
	assert(from_player_percent(50, RoadMaximumPercent) == 100);
	assert(from_player_percent(67, ImpactMaximumPercent) == 100);
	assert(from_player_percent(100, SteeringMaximumPercent) == 130);
	assert(from_player_percent(100, RoadMaximumPercent) == 200);
	assert(from_player_percent(100, ImpactMaximumPercent) == 150);
	assert(from_player_percent(-1, SteeringMaximumPercent) == 0);
	assert(from_player_percent(101, SteeringMaximumPercent) == 130);
	assert(to_player_percent(100, SteeringMaximumPercent) == 77);
	assert(to_player_percent(100, RoadMaximumPercent) == 50);
	assert(to_player_percent(100, ImpactMaximumPercent) == 67);
	assert(to_player_percent(999, ImpactMaximumPercent) == 100);
	const auto physicalFixture = to_player_configuration(100, 100, 100, 100, 50, true);
	assert(physicalFixture.ffbStrengthPercent == 100);
	assert(physicalFixture.steeringLoadPercent == 77);
	assert(physicalFixture.roadDetailPercent == 50);
	assert(physicalFixture.impactPercent == 67);
	assert(physicalFixture.enhancedRoad);
	assert(physicalFixture.surfacePercent == 50);
	assert(!to_player_configuration(100, 100, 100, 100, 50, false).enhancedRoad);
	for (int canonical : { 0, 65, 100, 130 })
		assert(to_player_configuration(100, canonical, 100, 100, 50, true).steeringLoadPercent ==
			to_player_percent(canonical, SteeringMaximumPercent));
	for (int canonical : { 0, 100, 200 })
		assert(to_player_configuration(100, 100, canonical, 100, 50, true).roadDetailPercent ==
			to_player_percent(canonical, RoadMaximumPercent));
	for (int canonical : { 0, 100, 150 })
		assert(to_player_configuration(100, 100, 100, canonical, 50, true).impactPercent ==
			to_player_percent(canonical, ImpactMaximumPercent));

	// Persistence remains the canonical integer multiplier. Displaying a value
	// never rewrites it, and save/load cycles therefore cannot introduce UI
	// rounding drift. Only an intentional player edit performs reverse mapping.
	for (const auto [value, maximum] : { std::pair{ 0, 130 }, std::pair{ 37, 130 },
		std::pair{ 100, 130 }, std::pair{ 130, 130 }, std::pair{ 73, 200 },
		std::pair{ 100, 200 }, std::pair{ 170, 200 }, std::pair{ 200, 200 },
		std::pair{ 41, 150 }, std::pair{ 100, 150 }, std::pair{ 130, 150 },
		std::pair{ 150, 150 } })
	{
		int persisted = value;
		for (int cycle = 0; cycle < 5; ++cycle)
		{
			const std::string serialized = std::to_string(persisted);
			persisted = clamp_percent(std::stoi(serialized), maximum);
			(void)to_player_percent(persisted, maximum);
		}
		assert(persisted == value);
	}

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
		assert(near(output.directional, raw.directional *
			(float(clamp_percent(percent, SteeringMaximumPercent)) / 100.0f)));
		assert(near(output.road, raw.road *
			(float(clamp_percent(percent, RoadMaximumPercent)) / 100.0f)));
		assert(near(output.impact, raw.impact *
			(float(clamp_percent(percent, ImpactMaximumPercent)) / 100.0f)));
		assert(std::isfinite(std::tanh(output.directional + output.road + output.impact)));
		assert(output.directional <= 0.0f && output.road >= 0.0f && output.impact <= 0.0f);
	}
	assert(clamp_percent(-25, SteeringMaximumPercent) == 0);
	assert(clamp_percent(175, SteeringMaximumPercent) == 130);
	const auto ceilings = apply(raw, { 999, 999, 999 });
	assert(near(ceilings.directional, raw.directional * 1.30f));
	assert(near(ceilings.road, raw.road * 2.00f));
	assert(near(ceilings.impact, raw.impact * 1.50f));
	const auto nonfinite = apply({ std::numeric_limits<float>::quiet_NaN(),
		std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity() }, {});
	assert(nonfinite.directional == 0.0f && nonfinite.road == 0.0f && nonfinite.impact == 0.0f);
}
