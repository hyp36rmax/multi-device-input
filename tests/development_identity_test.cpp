#include "product_identity.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
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
}
