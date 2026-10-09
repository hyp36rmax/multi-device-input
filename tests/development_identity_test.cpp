#include "product_identity.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <string_view>

int main()
{
	constexpr std::string_view prefix = "2.0.0-rc-dev+";
	static_assert(ProductIdentity::ReleaseVersion == "2.0.0-rc-dev");
	static_assert(ProductIdentity::ForceName == "HYP36rforce FFB");
	static_assert(ProductIdentity::IsDevelopmentBuild);
	assert(ProductIdentity::Version.starts_with(prefix));
	assert(ProductIdentity::BuildCommit.size() >= 7);
	assert(ProductIdentity::ShortCommit == ProductIdentity::BuildCommit.substr(0, 7));
	assert(ProductIdentity::Version.substr(prefix.size()) ==
		ProductIdentity::BuildCommit.substr(0, 7));
	assert(ProductIdentity::Version.find("1.5.0-dev") == std::string_view::npos);
}
