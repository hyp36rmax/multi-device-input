#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <set>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

int main()
{
	const auto root = std::filesystem::path(SIMHUB_DASHBOARD_SOURCE_DIR);
	const auto path = root / "HYP36rforce Digital Dash" / "HYP36rforce Digital Dash.djson";
	const auto metadataPath = root / "HYP36rforce Digital Dash" / "HYP36rforce Digital Dash.djson.metadata";
	const auto versionPath = root / "VERSION";
	std::ifstream input(path);
	assert(input);
	std::ostringstream buffer;
	buffer << input.rdbuf();
	const std::string json = buffer.str();
	std::ifstream metadataInput(metadataPath);
	std::ostringstream metadataBuffer;
	metadataBuffer << metadataInput.rdbuf();
	const std::string metadata = metadataBuffer.str();
	std::ifstream versionInput(versionPath);
	std::string version;
	std::getline(versionInput, version);
	assert(metadataInput && versionInput);
	assert(json.find("\"BaseWidth\": 1920") != std::string::npos);
	assert(json.find("\"BaseHeight\": 1080") != std::string::npos);
	assert(json.find("SimHubVersion\": \"9.13.2") != std::string::npos);
	assert(json.find("HYP36rforce Digital Dash") == std::string::npos); // identity belongs to metadata sidecar
	assert(json.find("\"DashboardDebugManager\"") != std::string::npos);
	assert(json.find("\"Screens\"") != std::string::npos);
	assert(json.find("\"Items\"") != std::string::npos);
	assert(json.find("\"BlinkPhasisInverted\": false") != std::string::npos);
	assert(json.find("\"RenderingSkip\": 0") != std::string::npos);
	assert(json.find("\"MinimumRefreshIntervalMS\": 0.0") != std::string::npos);
	assert(json.find("TargetPropertyName") == std::string::npos);
	assert(json.find("\"Variables\"") == std::string::npos);
	for (const char* property : { "Custom_SteeringInput", "Custom_CarID", "Custom_CarName",
		"Custom_StageID", "Custom_RoadActivity", "Custom_ImpactIntensity" })
		assert(json.find(property) != std::string::npos);
	assert(json.find("SpeedKmh") != std::string::npos);
	assert(json.find("$prop('Gear')") != std::string::npos);
	assert(json.find("Custom_RPM") == std::string::npos);
	assert(json.find("Ferrari") == std::string::npos);
	assert(json.find("SEGA") == std::string::npos);
	assert(json.find("Telemetry State: Unverified") != std::string::npos);
	for (const char* staticText : { "HYP36rforce", "SPEED", "GEAR", "STEERING INPUT" })
		assert(json.find(staticText) != std::string::npos);
	assert(json.find("\"Text\": \"--\"") != std::string::npos);
	assert(metadata.find("\"Title\": \"HYP36rforce Digital Dash\"") != std::string::npos);
	assert(metadata.find("\"Width\": 1920.0") != std::string::npos);
	assert(metadata.find("\"Height\": 1080.0") != std::string::npos);
	assert(metadata.find("\"DashboardVersion\": \"" + version + "\"") != std::string::npos);
	assert(json.find("\"DashboardVersion\": \"" + version + "\"") != std::string::npos);

	std::set<std::string> names;
	std::size_t cursor = 0;
	const std::string marker = "\"Name\": \"";
	while ((cursor = json.find(marker, cursor)) != std::string::npos)
	{
		cursor += marker.size();
		const auto end = json.find('"', cursor);
		assert(end != std::string::npos);
		assert(names.emplace(json.substr(cursor, end - cursor)).second);
		cursor = end + 1;
	}
	assert(names.size() >= 40);
}
