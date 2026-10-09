#include "package_identity.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

int main()
{
	using namespace PackageIdentity;
	constexpr std::string_view sha = "a1b2c3d";
	const std::string development = "OutRun-2006-C2C-Multi-Input-v1.5.0-dev-a1b2c3d";
	const std::string release = "OutRun-2006-C2C-Multi-Input-v1.5.0";
	const std::string releaseCandidate = "OutRun-2006-C2C-Multi-Input-v1.5.0-RC";

	assert(artifact_name("1.5.0-dev", sha) == development);
	assert(validate(development, "1.5.0-dev", sha));
	assert(artifact_name("1.5.0", sha) == release);
	assert(validate(release, "1.5.0", sha));
	assert(release_candidate_name("1.5.0") == releaseCandidate);
	assert(validate_release_candidate(releaseCandidate, "1.5.0"));
	assert(release_candidate_name("1.5.0-dev").empty());

	assert(!validate(development, "1.5.0", sha));
	assert(!validate("OutRun-2006-C2C-Multi-Input-v1.0.0-dev-a1b2c3d", "1.5.0-dev", sha));
	assert(!validate("OutRun-2006-C2C-Multi-Input-v1.5.0-dev-BADSHA!", "1.5.0-dev", sha));
	assert(artifact_name("1.5-dev", sha).empty());
	assert(artifact_name("v1.5.0", sha).empty());
	assert(artifact_name("1.5.0-dev", "ABC1234").empty());
}
