#pragma once

#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace DeviceDiagnosticsReport
{
	enum class Status { Completed, Failed, Unavailable, Untested, Cancelled, Inconclusive };

	struct Section
	{
		std::string name;
		Status status = Status::Untested;
		std::string summary;
		std::vector<std::string> details;
	};

	struct Report
	{
		std::string generatedUtc;
		std::vector<Section> sections;
		std::vector<std::pair<std::string, std::string>> summary;
	};

	struct Result
	{
		bool success = false;
		std::filesystem::path textPath;
		std::filesystem::path jsonPath;
		std::string error;
	};

	inline const char* status_name(Status status)
	{
		switch (status)
		{
		case Status::Completed: return "completed";
		case Status::Failed: return "failed";
		case Status::Unavailable: return "unavailable";
		case Status::Untested: return "untested";
		case Status::Cancelled: return "cancelled";
		case Status::Inconclusive: return "inconclusive";
		}
		return "unavailable";
	}

	inline std::string utc_timestamp(bool filenameSafe = false)
	{
		const auto now = std::chrono::system_clock::now();
		const std::time_t value = std::chrono::system_clock::to_time_t(now);
		std::tm utc{};
#ifdef _WIN32
		gmtime_s(&utc, &value);
#else
		gmtime_r(&value, &utc);
#endif
		std::ostringstream out;
		out << std::put_time(&utc, filenameSafe ? "%Y%m%d_%H%M%S" : "%Y-%m-%dT%H:%M:%SZ");
		return out.str();
	}

	inline std::string json_escape(std::string_view value)
	{
		std::string escaped;
		for (const unsigned char c : value)
		{
			switch (c)
			{
			case '\\': escaped += "\\\\"; break;
			case '"': escaped += "\\\""; break;
			case '\n': escaped += "\\n"; break;
			case '\r': escaped += "\\r"; break;
			case '\t': escaped += "\\t"; break;
			default:
				if (c < 0x20)
				{
					char buffer[7]{};
					std::snprintf(buffer, sizeof(buffer), "\\u%04x", c);
					escaped += buffer;
				}
				else escaped.push_back(char(c));
			}
		}
		return escaped;
	}

	inline Result write_named(const std::filesystem::path& directory, const Report& report, const std::string& requestedStem);

	inline Result write(const std::filesystem::path& directory, const Report& report)
	{
		return write_named(directory, report, "HYP36rforce-Device-Diagnostics-" + utc_timestamp(true));
	}

	inline Result write_named(const std::filesystem::path& directory, const Report& report, const std::string& requestedStem)
	{
		Result result;
		std::error_code error;
		std::filesystem::create_directories(directory, error);
		if (error)
		{
			result.error = "Could not create the Device Diagnostics report folder.";
			return result;
		}

		const std::string stem = requestedStem.empty() ? "HYP36rforce-Device-Diagnostics-" + utc_timestamp(true) : requestedStem;
		for (int suffix = 0;; ++suffix)
		{
			std::ostringstream unique;
			unique << stem;
			if (suffix) unique << '_' << std::setw(2) << std::setfill('0') << suffix + 1;
			const std::string uniqueStem = unique.str();
			result.textPath = directory / (uniqueStem + ".txt");
			result.jsonPath = directory / (uniqueStem + ".json");
			if (!std::filesystem::exists(result.textPath) && !std::filesystem::exists(result.jsonPath))
				break;
		}
		std::ofstream text(result.textPath, std::ios::trunc);
		std::ofstream json(result.jsonPath, std::ios::trunc);
		if (!text || !json)
		{
			result.error = "Could not create both Device Diagnostics report files.";
			return result;
		}

		text << "HYP36rforce Device Diagnostics\nGenerated (UTC): " << report.generatedUtc << "\n\n";
		if (!report.summary.empty())
		{
			text << "DEVICE DIAGNOSTICS SUMMARY\n";
			for (const auto& [label, value] : report.summary)
				text << label << ": " << value << "\n";
			text << "\n";
		}
		json << "{\n  \"schema\": \"HYP36R_DEVICE_DIAGNOSTICS_V1\",\n"
			<< "  \"generated_utc\": \"" << json_escape(report.generatedUtc) << "\",\n"
			<< "  \"summary\": {";
		for (size_t index = 0; index < report.summary.size(); ++index)
		{
			if (index) json << ", ";
			json << "\"" << json_escape(report.summary[index].first) << "\": \""
				<< json_escape(report.summary[index].second) << "\"";
		}
		json << "},\n  \"sections\": [\n";
		for (size_t index = 0; index < report.sections.size(); ++index)
		{
			const auto& section = report.sections[index];
			text << section.name << " [" << status_name(section.status) << "]\n" << section.summary << "\n";
			for (const auto& detail : section.details)
				text << "- " << detail << "\n";
			text << "\n";

			json << "    {\"name\": \"" << json_escape(section.name) << "\", \"status\": \""
				<< status_name(section.status) << "\", \"summary\": \"" << json_escape(section.summary)
				<< "\", \"details\": [";
			for (size_t detail = 0; detail < section.details.size(); ++detail)
			{
				if (detail) json << ", ";
				json << "\"" << json_escape(section.details[detail]) << "\"";
			}
			json << "]}" << (index + 1 == report.sections.size() ? "\n" : ",\n");
		}
		json << "  ]\n}\n";
		text.flush();
		json.flush();
		if (!text || !json)
		{
			result.error = "A Device Diagnostics report could not be written completely.";
			return result;
		}
		result.success = true;
		return result;
	}
}
