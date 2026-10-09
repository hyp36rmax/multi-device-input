#pragma once

#include <cctype>
#include <string>
#include <string_view>

namespace PackageIdentity
{
	inline constexpr std::string_view Prefix = "OutRun-2006-C2C-Multi-Input-v";

	inline bool valid_version(std::string_view version) noexcept
	{
		if (version.ends_with("-dev"))
			version.remove_suffix(4);
		int components = 0;
		std::size_t digits = 0;
		for (char c : version)
		{
			if (c == '.')
			{
				if (digits == 0 || components >= 2) return false;
				++components;
				digits = 0;
			}
			else if (std::isdigit(static_cast<unsigned char>(c)))
				++digits;
			else
				return false;
		}
		return components == 2 && digits > 0;
	}

	inline bool valid_short_sha(std::string_view shortSha) noexcept
	{
		if (shortSha.size() != 7) return false;
		for (char c : shortSha)
			if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
		return true;
	}

	inline std::string artifact_name(std::string_view version, std::string_view shortSha)
	{
		if (!valid_version(version)) return {};
		std::string name(Prefix);
		name += version;
		if (version.ends_with("-dev"))
		{
			if (!valid_short_sha(shortSha)) return {};
			name += '-';
			name += shortSha;
		}
		return name;
	}

	inline bool validate(std::string_view candidate, std::string_view authoritativeVersion,
		std::string_view shortSha) noexcept
	{
		return candidate == artifact_name(authoritativeVersion, shortSha);
	}

	inline std::string release_candidate_name(std::string_view version)
	{
		if (!valid_version(version) || version.ends_with("-dev")) return {};
		std::string name(Prefix);
		name += version;
		name += "-RC";
		return name;
	}

	inline bool validate_release_candidate(std::string_view candidate,
		std::string_view authoritativeVersion) noexcept
	{
		return candidate == release_candidate_name(authoritativeVersion);
	}
}
