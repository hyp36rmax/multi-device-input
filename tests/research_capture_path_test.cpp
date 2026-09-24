#include "research_capture_path.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

using HYP36RResearchPath::sanitize_scenario;

int main()
{
	assert(sanitize_scenario("R1_SCHEMA_UAT") == "R1_SCHEMA_UAT");
	assert(sanitize_scenario("Road test 01") == "Road_test_01");
	assert(sanitize_scenario("../../outside") == "outside");
	assert(sanitize_scenario("C:\\absolute\\path") == "C_absolute_path");
	assert(sanitize_scenario("CON") == "SCENARIO_CON");
	assert(sanitize_scenario("") == "UNSPECIFIED");
}
