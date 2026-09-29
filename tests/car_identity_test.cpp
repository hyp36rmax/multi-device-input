#include "car_identity.hpp"

#include <array>
#include <cassert>
#include <string_view>

int main()
{
	constexpr std::array<std::string_view, 15> expected{
		"F50", "Dino 246 GTS", "288 GTO", "512 BB", "365 GTS/4 Daytona",
		"Enzo Ferrari", "Testarossa", "360 Spider", "F40", "250 GTO",
		"F355 Spider", "328 GTS", "F430", "550 Barchetta", "SuperAmerica"
	};
	for (int id = 0; id < static_cast<int>(expected.size()); ++id)
	{
		assert(CarIdentity::friendly_name(id) == expected[static_cast<std::size_t>(id)]);
		assert(CarIdentity::display_name(id) == expected[static_cast<std::size_t>(id)]);
	}
	assert(CarIdentity::friendly_name(-1).empty());
	assert(CarIdentity::friendly_name(15).empty());
	assert(CarIdentity::display_name(99) == "Car 99");
}
