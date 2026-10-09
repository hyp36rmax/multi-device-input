#include "car_identity.hpp"

#include <array>
#include <format>

namespace CarIdentity
{
	namespace
	{
		constexpr std::array<std::string_view, 30> PlayerCarNames{
			"Ferrari F50 (Intermediate A)",
			"Ferrari Dino 246 GTS (Novice)",
			"Ferrari 288 GTO (Intermediate B)",
			"Ferrari 512 BB (Professional)",
			"Ferrari 365 GTS/4 Daytona (Novice)",
			"Ferrari Enzo Ferrari (Professional)",
			"Ferrari Testarossa (Intermediate B)",
			"Ferrari 360 Spider (Intermediate A)",
			"Ferrari F40 (Professional)",
			"Ferrari 250 GTO (Professional)",
			"Ferrari F355 Spider (Intermediate A)",
			"Ferrari 328 GTS (Intermediate B)",
			"Ferrari F430 (Professional)",
			"Ferrari 550 Barchetta (Professional)",
			"Ferrari SuperAmerica (Intermediate A)",
			"Ferrari F50 (OutRun)",
			"Ferrari Dino 246 GTS (OutRun)",
			"Ferrari 288 GTO (OutRun)",
			"Ferrari 512 BB (OutRun)",
			"Ferrari 365 GTS/4 Daytona (OutRun)",
			"Ferrari Enzo Ferrari (OutRun)",
			"Ferrari Testarossa (OutRun)",
			"Ferrari 360 Spider (OutRun)",
			"Ferrari F40 (OutRun)",
			"Ferrari 250 GTO (OutRun)",
			"Ferrari F355 Spider (OutRun)",
			"Ferrari 328 GTS (OutRun)",
			"Ferrari F430 Spider (OutRun)",
			"Ferrari 550 Barchetta (OutRun)",
			"Ferrari SuperAmerica (OutRun)"
		};
	}

	std::string_view friendly_name(int carId) noexcept
	{
		if (carId < FirstPlayerCarId || carId > LastPlayerCarId) return {};
		return PlayerCarNames[static_cast<std::size_t>(carId)];
	}

	std::string display_name(int carId)
	{
		const auto friendly = friendly_name(carId);
		return friendly.empty() ? std::format("Car #{}", carId) : std::string(friendly);
	}

	std::string display_name(bool gameStateAvailable, int carId)
	{
		return gameStateAvailable ? display_name(carId) : std::string(UnavailableName);
	}

	std::string overlay_label(bool gameStateAvailable, int carId)
	{
		return std::format("Car: {}", display_name(gameStateAvailable, carId));
	}
}
