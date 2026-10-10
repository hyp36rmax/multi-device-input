#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

int main()
{
	const auto root = std::filesystem::path(SIMHUB_DASHBOARD_SOURCE_DIR);
	const auto path = root / "HYP36rforce Digital Dash" / "HYP36rforce Digital Dash.djson";
	std::ifstream input(path);
	assert(input);
	std::ostringstream buffer;
	buffer << input.rdbuf();
	const std::string json = buffer.str();
	assert(json.find("\"BaseWidth\": 1920") != std::string::npos);
	assert(json.find("\"BaseHeight\": 1080") != std::string::npos);
	assert(json.find("SimHubVersion\": \"9.13.2") != std::string::npos);
	assert(json.find("HYP36rforce Digital Dash") == std::string::npos); // identity belongs to metadata sidecar
	for (const char* property : { "Custom_SteeringInput", "Custom_CarID", "Custom_CarName",
		"Custom_StageID", "Custom_RoadActivity", "Custom_ImpactIntensity" })
		assert(json.find(property) != std::string::npos);
	assert(json.find("SpeedKmh") != std::string::npos);
	assert(json.find("$prop('Gear')") != std::string::npos);
	assert(json.find("Custom_RPM") == std::string::npos);
	assert(json.find("Ferrari") == std::string::npos);
	assert(json.find("SEGA") == std::string::npos);
	assert(json.find("Telemetry State: Unverified") != std::string::npos);
}
