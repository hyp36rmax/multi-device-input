#include "overlay/device_diagnostics_help.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <set>
#include <string_view>

int main()
{
	using namespace DeviceDiagnosticsHelp;
	static_assert(RequiredSections.size() == 9);
	static_assert(QuickSetupControls.size() == 12);
	static_assert(ForceControls.size() == 16);

	std::set<std::string_view> ids;
	const auto validate = [&ids](const Entry& entry)
	{
		assert(!entry.id.empty());
		assert(!entry.label.empty());
		assert(!entry.description.empty());
		assert(entry.description.size() <= (entry.id == "export-report" ? 320 : 120));
		assert(ids.insert(entry.id).second);
	};
	for (const auto& entry : RequiredSections)
		validate(entry);
	for (const auto& entry : ForceControls)
		validate(entry);
	for (const auto& entry : QuickSetupControls)
		validate(entry);
	assert(ExportReport.description.find("Documents -> HYP36rforce Device Diagnostics -> Exports") != std::string_view::npos);
	assert(ExportReport.description.find("Open Exports Folder") != std::string_view::npos);

	return 0;
}
