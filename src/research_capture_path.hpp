#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace HYP36RResearchPath
{
	inline std::string sanitize_scenario(std::string_view scenario)
	{
		std::string result;
		result.reserve(scenario.size());
		for (const unsigned char c : scenario)
		{
			if (std::isalnum(c) || c == '-' || c == '_')
				result.push_back(static_cast<char>(c));
			else if (std::isspace(c) && !result.empty() && result.back() != '_')
				result.push_back('_');
			else if (!result.empty() && result.back() != '_')
				result.push_back('_');
		}
		while (!result.empty() && result.back() == '_')
			result.pop_back();
		if (result.empty())
			result = "UNSPECIFIED";
		if (result.size() > 64)
			result.resize(64);

		std::string upper = result;
		std::transform(upper.begin(), upper.end(), upper.begin(),
			[](unsigned char c) { return static_cast<char>(std::toupper(c)); });
		const bool reserved = upper == "CON" || upper == "PRN" || upper == "AUX" ||
			upper == "NUL" || (upper.size() == 4 &&
				(upper.rfind("COM", 0) == 0 || upper.rfind("LPT", 0) == 0) &&
				upper[3] >= '1' && upper[3] <= '9');
		if (reserved)
			result.insert(0, "SCENARIO_");
		return result;
	}
}
