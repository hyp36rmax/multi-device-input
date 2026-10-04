#include "research_capture_path.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

using HYP36RResearchPath::sanitize_scenario;
using HYP36RResearchPath::sanitize_filename_component;
using HYP36RResearchPath::general_capture_stem;
using HYP36RResearchPath::collision_safe_stem;

int main()
{
	assert(sanitize_scenario("R1_SCHEMA_UAT") == "R1_SCHEMA_UAT");
	assert(sanitize_scenario("Road test 01") == "Road_test_01");
	assert(sanitize_scenario("../../outside") == "outside");
	assert(sanitize_scenario("C:\\absolute\\path") == "C_absolute_path");
	assert(sanitize_scenario("CON") == "SCENARIO_CON");
	assert(sanitize_scenario("") == "UNSPECIFIED");

	assert(HYP36RResearchPath::TelemetryFolder == "Telemetry");
	assert(HYP36RResearchPath::GeneralCaptureFolder == "General Capture");
	assert(HYP36RResearchPath::ResearchFolder == "Research");
	assert(general_capture_stem("F40", "Deep Lake", "2026-10-04", "154327") ==
		"General Capture - F40 - Deep Lake - 2026-10-04 - 154327");
	assert(general_capture_stem("F40", "", "2026-10-04", "154327") ==
		"General Capture - F40 - 2026-10-04 - 154327");
	assert(general_capture_stem("", "", "2026-10-04", "154327") ==
		"General Capture - 2026-10-04 - 154327");
	assert(general_capture_stem("F40", "Deep Lake", "2026-10-04", "154327").find("UNSPECIFIED") == std::string::npos);
	assert(sanitize_filename_component("Dino 246 GTS") == "Dino 246 GTS");
	assert(sanitize_filename_component("Bad:/\\Name*?. ") == "Bad___Name__");
	assert(sanitize_filename_component("CON") == "_CON");
	assert(sanitize_filename_component("... ").empty());
	assert(collision_safe_stem("Capture", 1) == "Capture");
	assert(collision_safe_stem("Capture", 2) == "Capture (2)");
}
