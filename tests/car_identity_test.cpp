#include "car_identity.hpp"

#include <array>
#include <cassert>
#include <string_view>

int main()
{
	constexpr std::array<std::string_view, 30> expected{
		"Ferrari F50 (Intermediate A)", "Ferrari Dino 246 GTS (Novice)", "Ferrari 288 GTO (Intermediate B)", "Ferrari 512 BB (Professional)", "Ferrari 365 GTS/4 Daytona (Novice)",
		"Ferrari Enzo Ferrari (Professional)", "Ferrari Testarossa (Intermediate B)", "Ferrari 360 Spider (Intermediate A)", "Ferrari F40 (Professional)", "Ferrari 250 GTO (Professional)",
		"Ferrari F355 Spider (Intermediate A)", "Ferrari 328 GTS (Intermediate B)", "Ferrari F430 (Professional)", "Ferrari 550 Barchetta (Professional)", "Ferrari SuperAmerica (Intermediate A)",
		"Ferrari F50 (OutRun)", "Ferrari Dino 246 GTS (OutRun)", "Ferrari 288 GTO (OutRun)", "Ferrari 512 BB (OutRun)", "Ferrari 365 GTS/4 Daytona (OutRun)",
		"Ferrari Enzo Ferrari (OutRun)", "Ferrari Testarossa (OutRun)", "Ferrari 360 Spider (OutRun)", "Ferrari F40 (OutRun)", "Ferrari 250 GTO (OutRun)",
		"Ferrari F355 Spider (OutRun)", "Ferrari 328 GTS (OutRun)", "Ferrari F430 Spider (OutRun)", "Ferrari 550 Barchetta (OutRun)", "Ferrari SuperAmerica (OutRun)"
	};
	for (int id = 0; id < static_cast<int>(expected.size()); ++id)
	{
		assert(CarIdentity::friendly_name(id) == expected[static_cast<std::size_t>(id)]);
		assert(CarIdentity::display_name(id) == expected[static_cast<std::size_t>(id)]);
	}
	assert(CarIdentity::friendly_name(-1).empty());
	assert(CarIdentity::friendly_name(30).empty());
	assert(CarIdentity::display_name(30) == "Car #30");
	assert(CarIdentity::display_name(false, 0) == "Car unavailable");
	assert(CarIdentity::overlay_label(true, 12) == "Car: Ferrari F430 (Professional)");
	assert(CarIdentity::overlay_label(true, 27) == "Car: Ferrari F430 Spider (OutRun)");
	assert(CarIdentity::display_name(5) != CarIdentity::display_name(20));
	assert(CarIdentity::display_name(12) != CarIdentity::display_name(27));
}
