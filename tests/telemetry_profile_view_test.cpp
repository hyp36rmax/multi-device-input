#include "telemetry_profile_view.hpp"

#include <cassert>
#include <string_view>

int main()
{
	using namespace HYP36RTelemetryView;
	static_assert(TelemetrySchemaName == "HYP36R_RESEARCH_II_R1_AER_OUTPUT_V11");
	static_assert(TelemetryColumnCount == 294);

	const auto reference = resolve(0, "Directional", false);
	assert(reference.profile == ForceProfile::ReferencePlus);
	assert(std::string_view(force_profile_name(reference.profile)) == "Reference+");
	assert(reference.showReferenceSettings && !reference.showAerSettings);
	assert(!reference.showSurfaceSettings);

	const auto arcade = resolve(1, "Directional", false);
	assert(arcade.profile == ForceProfile::ArcadeExperience);
	assert(std::string_view(force_profile_name(arcade.profile)) == "Arcade Experience");
	assert(!arcade.showReferenceSettings && arcade.showAerSettings);
	assert(!arcade.showSurfaceSettings);

	assert(resolve(0, "Surface", true).showSurfaceSettings);
	assert(resolve(1, "SURFACE", true).showSurfaceSettings);
	assert(!resolve(1, "Surface", false).showSurfaceSettings);
	assert(!resolve(1, "Directional", true).showSurfaceSettings);
	assert(force_profile_from_selection(-1) == ForceProfile::ReferencePlus);
	assert(force_profile_from_selection(99) == ForceProfile::ReferencePlus);
	assert(std::string_view(force_profile_name(false)) == "Reference+");
	assert(std::string_view(force_profile_name(true)) == "Arcade Experience");
}
