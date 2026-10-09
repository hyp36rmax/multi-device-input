#include "package_manifest.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

int main()
{
	using PackageManifest::validate;
	std::vector<std::string> valid{
		"dinput8.dll", "README.md", "OutRun2006Tweaks.lods.ini",
		"OR2006C2C.exe", "LICENSE.md", "OutRun2006Tweaks.ini", "RELEASE_NOTES.md"
	};
	assert(validate(valid));

	auto missingLods = valid;
	missingLods.erase(std::find(missingLods.begin(), missingLods.end(), "OutRun2006Tweaks.lods.ini"));
	assert(!validate(missingLods));

	auto userIniInstead = missingLods;
	userIniInstead.emplace_back("OutRun2006Tweaks.user.ini");
	assert(!validate(userIniInstead));

	auto research = valid;
	research.emplace_back("docs/research/SIMHUB_INTEGRATION_RESEARCH.md");
	assert(!validate(research));

	auto symbols = valid;
	symbols.emplace_back("dinput8.pdb");
	assert(!validate(symbols));
}
