#pragma once

#include <array>
#include <string_view>

namespace RoadDetailModeUi
{
inline constexpr std::string_view Label = "Road Detail Mode";
inline constexpr std::array Choices{
	std::string_view("Classic"),
	std::string_view("Enhanced"),
};
inline constexpr std::array CanonicalValues{
	std::string_view("REFERENCE_PLUS"),
	std::string_view("ROAD2_EXPERIMENTAL"),
};
inline constexpr std::string_view Tooltip =
	"Choose how road surfaces feel. Classic preserves the original feedback; "
	"Enhanced adds more detailed surface texture.";
}
