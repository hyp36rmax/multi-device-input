#pragma once

#include <string>
#include <string_view>

namespace CarIdentity
{
	inline constexpr int FirstPlayerCarId = 0;
	inline constexpr int LastPlayerCarId = 14;

	// Returns an empty view for unknown IDs. Call display_name() for UI/reporting.
	std::string_view friendly_name(int carId) noexcept;
	std::string display_name(int carId);
}
