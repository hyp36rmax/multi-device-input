#pragma once

namespace FFBTestPolicy
{
	// DirectInput nominal force is 10,000. Direction tests use a fixed,
	// short-lived 20% request regardless of the live driving Strength setting.
	constexpr long DirectionTestMagnitude = 2000;

	constexpr bool reverse_direction(float direction, bool invert)
	{
		return (direction < 0.0f) != invert;
	}
}
