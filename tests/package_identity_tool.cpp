#include "package_identity.hpp"
#include "package_manifest.hpp"
#include "product_identity.hpp"

#include <iostream>

int main(int argc, char** argv)
{
	if (argc >= 2 && std::string_view(argv[1]) == "--manifest")
	{
		std::vector<std::string> files;
		for (int i = 2; i < argc; ++i) files.emplace_back(argv[i]);
		if (!PackageManifest::validate(std::move(files))) return 4;
		std::cout << "manifest-ok";
		return 0;
	}
	if (argc == 2 && std::string_view(argv[1]) == "--rc")
	{
		const std::string name = PackageIdentity::release_candidate_name(ProductIdentity::ReleaseVersion);
		if (name.empty() || !PackageIdentity::validate_release_candidate(
			name, ProductIdentity::ReleaseVersion)) return 5;
		std::cout << name;
		return 0;
	}
	if (argc != 2)
		return 2;
	const std::string name = PackageIdentity::artifact_name(ProductIdentity::ReleaseVersion, argv[1]);
	if (name.empty() || !PackageIdentity::validate(name, ProductIdentity::ReleaseVersion, argv[1]))
		return 3;
	std::cout << name;
	return 0;
}
