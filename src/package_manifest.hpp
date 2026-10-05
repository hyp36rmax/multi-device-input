#pragma once

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace PackageManifest
{
	inline constexpr std::array<std::string_view, 6> ExpectedFiles{
		"LICENSE.md",
		"OR2006C2C.exe",
		"OutRun2006Tweaks.ini",
		"OutRun2006Tweaks.lods.ini",
		"README.md",
		"dinput8.dll"
	};

	inline bool validate(std::vector<std::string> files)
	{
		std::sort(files.begin(), files.end());
		if (files.size() != ExpectedFiles.size()) return false;
		for (std::size_t i = 0; i < files.size(); ++i)
			if (files[i] != ExpectedFiles[i]) return false;
		return true;
	}
}
