#pragma once

#include <string>
#include <string_view>

namespace CarIdentity
{
	inline constexpr int FirstPlayerCarId = 0;
	inline constexpr int LastPlayerCarId = 29;
	inline constexpr std::string_view UnavailableName = "Car unavailable";

	// Returns an empty view for unknown IDs. Call display_name() for UI/reporting.
	std::string_view friendly_name(int carId) noexcept;
	std::string display_name(int carId);
	std::string display_name(bool gameStateAvailable, int carId);
	std::string overlay_label(bool gameStateAvailable, int carId);
}
