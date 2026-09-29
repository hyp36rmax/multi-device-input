#include "car_identity.hpp"

#include <array>
#include <format>

namespace CarIdentity
{
	namespace
	{
		constexpr std::array<std::string_view, 15> PlayerCarNames{
			"F50",
			"Dino 246 GTS",
			"288 GTO",
			"512 BB",
			"365 GTS/4 Daytona",
			"Enzo Ferrari",
			"Testarossa",
			"360 Spider",
			"F40",
			"250 GTO",
			"F355 Spider",
			"328 GTS",
			"F430",
			"550 Barchetta",
			"SuperAmerica"
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
		return friendly.empty() ? std::format("Car {}", carId) : std::string(friendly);
	}
}
