#include "device_diagnostics_report.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>

int main()
{
	using namespace DeviceDiagnosticsReport;
	const auto directory = std::filesystem::temp_directory_path() / "hyp36r-device-diagnostics-report-test";
	std::filesystem::remove_all(directory);
	Report report{ "2026-10-09T12:34:56Z", {
		{ "Device discovery", Status::Completed, "One device found.", { "Wheel & pedals" } },
		{ "Input test", Status::Untested, "No test was recorded.", {} },
		{ "API errors", Status::Unavailable, "No retained error history.", { "Do not infer success." } },
		{ "Failure sample", Status::Failed, "A quoted \"error\" was recorded.", {} }
	} };
	const auto result = write(directory, report);
	assert(result.success);
	assert(std::filesystem::exists(result.textPath));
	assert(std::filesystem::exists(result.jsonPath));
	std::ifstream textFile(result.textPath);
	std::ifstream jsonFile(result.jsonPath);
	const std::string text{ std::istreambuf_iterator<char>(textFile), {} };
	const std::string json{ std::istreambuf_iterator<char>(jsonFile), {} };
	assert(text.find("Device discovery [completed]") != std::string::npos);
	assert(text.find("Input test [untested]") != std::string::npos);
	assert(json.find("HYP36R_DEVICE_DIAGNOSTICS_V1") != std::string::npos);
	assert(json.find("\\\"error\\\"") != std::string::npos);
	assert(json.find("\"status\": \"unavailable\"") != std::string::npos);
	const auto second = write(directory, report);
	assert(second.success);
	assert(second.textPath != result.textPath);
	assert(second.jsonPath != result.jsonPath);
	const auto named = write_named(directory, report, "Fanatec_DD2_2026-10-09_214530");
	assert(named.success);
	assert(named.textPath.filename() == "Fanatec_DD2_2026-10-09_214530.txt");
	assert(named.jsonPath.filename() == "Fanatec_DD2_2026-10-09_214530.json");
	const auto collision = write_named(directory, report, "Fanatec_DD2_2026-10-09_214530");
	assert(collision.success);
	assert(collision.textPath.filename() == "Fanatec_DD2_2026-10-09_214530_02.txt");
	assert(collision.jsonPath.filename() == "Fanatec_DD2_2026-10-09_214530_02.json");
	textFile.close();
	jsonFile.close();
	std::filesystem::remove_all(directory);
	return 0;
}
