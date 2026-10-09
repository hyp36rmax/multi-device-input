#include "product_identity.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <string>
#include <string_view>

int main()
{
	static_assert(ProductIdentity::ReleaseVersion == "1.5.0");
	static_assert(ProductIdentity::ForceName == "HYP36rforce FFB");
	static_assert(!ProductIdentity::IsDevelopmentBuild);
	assert(ProductIdentity::Version == ProductIdentity::ReleaseVersion);
	assert(ProductIdentity::BuildCommit.size() >= 7);
	assert(ProductIdentity::ShortCommit == ProductIdentity::BuildCommit.substr(0, 7));
	assert(ProductIdentity::Version.find("2.0.0") == std::string_view::npos);
	const auto startup = ProductIdentity::startup_notification("0.6.1.0");
	assert(startup == "OutRun 2006 C2C Multi Input\nv1.5.0\n\nHYP36rforce FFB + Multi Input by hyp36rmax\nBased on OutRun2006Tweaks v0.6.1.0 by emoose");
	assert(startup.find("Multi-Input") == std::string::npos);
}
