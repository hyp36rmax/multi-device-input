#include "device_diagnostics_identity.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

int main()
{
	using namespace DeviceDiagnosticsIdentity;
	static_assert(Name == "HYP36rforce Device Diagnostics");
	static_assert(Version == "0.2.0-dev.6");
	static_assert(IsDevelopmentBuild);
	static_assert(ReportSchema == "HYP36R_DEVICE_DIAGNOSTICS_V1");
	assert(BuildCommit.size() >= 7);
	assert(ShortCommit.size() == 7);
	assert(ShortCommit == BuildCommit.substr(0, 7));
}
