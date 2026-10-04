#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace HYP36RResearchPath
{
	inline constexpr std::string_view TelemetryFolder = "Telemetry";
	inline constexpr std::string_view GeneralCaptureFolder = "General Capture";
	inline constexpr std::string_view ResearchFolder = "Research";

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

	inline bool is_windows_reserved_name(std::string_view value)
	{
		std::string upper(value);
		std::transform(upper.begin(), upper.end(), upper.begin(),
			[](unsigned char c) { return static_cast<char>(std::toupper(c)); });
		const auto dot = upper.find('.');
		if (dot != std::string::npos)
			upper.resize(dot);
		return upper == "CON" || upper == "PRN" || upper == "AUX" || upper == "NUL" ||
			(upper.size() == 4 &&
				(upper.rfind("COM", 0) == 0 || upper.rfind("LPT", 0) == 0) &&
				upper[3] >= '1' && upper[3] <= '9');
	}

	// Preserve player-facing spaces and punctuation unless Windows forbids them.
	// Empty results are intentionally returned as empty so optional filename
	// components can be omitted instead of receiving a placeholder identity.
	inline std::string sanitize_filename_component(std::string_view value)
	{
		std::string result;
		result.reserve(value.size());
		for (const unsigned char c : value)
		{
			if (c < 32 || c == '<' || c == '>' || c == ':' || c == '"' ||
				c == '/' || c == '\\' || c == '|' || c == '?' || c == '*')
				result.push_back('_');
			else
				result.push_back(static_cast<char>(c));
		}
		while (!result.empty() && (result.back() == ' ' || result.back() == '.'))
			result.pop_back();
		while (!result.empty() && result.front() == ' ')
			result.erase(result.begin());
		if (is_windows_reserved_name(result))
			result.insert(0, "_");
		return result;
	}

	inline std::string general_capture_stem(std::string_view car,
		std::string_view startingStage, std::string_view localDate,
		std::string_view localTime)
	{
		std::vector<std::string> components{ "General Capture" };
		for (const auto value : { car, startingStage })
		{
			auto component = sanitize_filename_component(value);
			if (!component.empty())
				components.push_back(std::move(component));
		}
		components.emplace_back(localDate);
		components.emplace_back(localTime);

		std::string stem;
		for (const auto& component : components)
		{
			if (!stem.empty())
				stem += " - ";
			stem += component;
		}
		return stem;
	}

	inline std::string collision_safe_stem(std::string_view baseStem, unsigned suffix)
	{
		return suffix <= 1 ? std::string(baseStem) :
			std::string(baseStem) + " (" + std::to_string(suffix) + ")";
	}
}
