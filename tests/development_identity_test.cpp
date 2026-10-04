#include "product_identity.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <string_view>

int main()
{
	constexpr std::string_view prefix = "1.0.0-dev+";
	static_assert(ProductIdentity::ReleaseVersion == "1.0.0-dev");
	static_assert(ProductIdentity::ForceName == "HYP36rforce FFB");
	assert(ProductIdentity::Version.starts_with(prefix));
	assert(ProductIdentity::BuildCommit.size() >= 7);
	assert(ProductIdentity::Version.substr(prefix.size()) ==
		ProductIdentity::BuildCommit.substr(0, 7));
	assert(ProductIdentity::Version.find("2.0.0") == std::string_view::npos);
}
