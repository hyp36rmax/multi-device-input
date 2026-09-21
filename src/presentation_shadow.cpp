#include "presentation_shadow.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>

#include <spdlog/spdlog.h>

#include "settings.hpp"

namespace Settings
{
	Setting<std::string> PresentationProfile{ "Developer", "PresentationProfile", "Reference",
		"Passive HYP36R presentation research profile." };
	Setting<float> PresentationPresence{ "Developer", "PresentationPresence", 1.0f,
		"Passive global Presence request." };
	Setting<float> PresentationContrast{ "Developer", "PresentationContrast", 0.0f,
		"Passive eligible M5 secondary-information Contrast request." };
	Setting<float> PresentationSecondaryBudget{ "Developer", "PresentationSecondaryBudget", 0.05f,
		"Passive linear secondary budget as a fraction of primary authority." };

	namespace
	{
		struct HidePresentationSettings
		{
			HidePresentationSettings()
			{
				PresentationProfile.hidden(true);
				PresentationPresence.hidden(true);
				PresentationContrast.hidden(true);
				PresentationSecondaryBudget.hidden(true);
			}
		} hidePresentationSettings;
	}
}

namespace HYP36RPresentation
{
	namespace
	{
		struct Configuration
		{
			Profile profile = Profile::Reference;
			float presence = 1.0f;
			float contrast = 0.0f;
			float budgetFraction = 0.05f;
			bool fallback = false;
		};

		Configuration configuration{};
		Frame current{};

		std::string_view trim_ascii_whitespace(std::string_view value)
		{
			while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())))
				value.remove_prefix(1);
			while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
				value.remove_suffix(1);
			return value;
		}

		bool equals_ascii_case_insensitive(std::string_view left, std::string_view right)
		{
			if (left.size() != right.size())
				return false;
			for (size_t index = 0; index < left.size(); ++index)
			{
				const auto a = static_cast<unsigned char>(left[index]);
				const auto b = static_cast<unsigned char>(right[index]);
				if (std::tolower(a) != std::tolower(b))
					return false;
			}
			return true;
		}

		bool supported_presence(float value)
		{
			return value == 1.0f || value == 1.1f || value == 1.2f ||
				value == 1.3f || value == 1.4f;
		}

		bool supported_contrast(float value)
		{
			return value == 0.0f || value == 1.0f || value == 2.0f ||
				value == 4.0f || value == 8.0f;
		}

		SoftwareRegion classify_region(float presence)
		{
			if (presence <= 1.3f)
				return SoftwareRegion::Comfort;
			if (presence <= 1.6f)
				return SoftwareRegion::HeadroomAware;
			if (presence < 2.0f)
				return SoftwareRegion::CompressionRisk;
			return SoftwareRegion::SaturationRisk;
		}

		float finite_or_zero(float value)
		{
			return std::isfinite(value) ? value : 0.0f;
		}
	}

	void initialize()
	{
		Configuration next{};
		const auto profile = trim_ascii_whitespace(Settings::PresentationProfile.get());
		const float presence = Settings::PresentationPresence.get();
		const float contrast = Settings::PresentationContrast.get();
		const float budget = Settings::PresentationSecondaryBudget.get();
		const bool valid = equals_ascii_case_insensitive(profile, "Reference") &&
			std::isfinite(presence) && supported_presence(presence) &&
			std::isfinite(contrast) && supported_contrast(contrast) &&
			std::isfinite(budget) && budget >= 0.0f && budget <= 0.10f;

		if (valid)
		{
			next.presence = presence;
			next.contrast = contrast;
			next.budgetFraction = budget;
		}
		else
		{
			next.fallback = true;
		}
		configuration = next;
		reset();
		spdlog::info("HYP36R presentation shadow: profile={} presence={:.2f} contrast={:g} budget={:.3f}{}",
			profile_name(configuration.profile), configuration.presence,
			configuration.contrast, configuration.budgetFraction,
			configuration.fallback ? " fallback=Reference" : "");
	}

	const Frame& evaluate(const Inputs& rawInputs)
	{
		Frame next{};
		next.profile = configuration.profile;
		next.presence = configuration.presence;
		next.contrast = configuration.contrast;
		next.secondaryBudgetFraction = configuration.budgetFraction;
		next.fallbackActive = configuration.fallback;
		next.softwareRegion = classify_region(configuration.presence);
		next.directionalPrimary = finite_or_zero(rawInputs.directionalPrimary);
		next.roadRequest = finite_or_zero(rawInputs.road);
		next.impactRequest = finite_or_zero(rawInputs.impact);
		next.vibrationRequest = finite_or_zero(rawInputs.vibration);

		const float legacyLimit = std::abs(finite_or_zero(rawInputs.legacyDirectional));
		if (rawInputs.secondaryEligible && !configuration.fallback)
			next.secondaryRaw = finite_or_zero(rawInputs.directionalSecondaryRaw);
		next.secondaryRequested = next.secondaryRaw * configuration.contrast;

		const float budget = configuration.budgetFraction * std::abs(next.directionalPrimary);
		const float budgeted = (std::clamp)(next.secondaryRequested, -budget, budget);
		next.secondaryBudgetActive = budgeted != next.secondaryRequested;

		float semanticDirectional = next.directionalPrimary + budgeted;
		if (next.directionalPrimary == 0.0f ||
			semanticDirectional * next.directionalPrimary < 0.0f)
		{
			semanticDirectional = next.directionalPrimary;
		}

		// The Legacy boundary constrains only the added secondary information. It
		// must never reduce an already-authoritative M4 primary at Contrast zero.
		const float legacyBounded = std::abs(next.directionalPrimary) > legacyLimit
			? next.directionalPrimary
			: (std::clamp)(semanticDirectional, -legacyLimit, legacyLimit);
		next.legacyBoundaryActive = legacyBounded != semanticDirectional;
		next.secondaryPermitted = legacyBounded - next.directionalPrimary;
		next.directionalRequest = finite_or_zero(legacyBounded * configuration.presence);

		current = next;
		return current;
	}

	void reset()
	{
		current = {};
		current.profile = configuration.profile;
		current.presence = configuration.presence;
		current.contrast = configuration.contrast;
		current.secondaryBudgetFraction = configuration.budgetFraction;
		current.fallbackActive = configuration.fallback;
		current.softwareRegion = classify_region(configuration.presence);
	}

	const Frame& frame() { return current; }
	const char* profile_name(Profile) { return "Reference"; }

	const char* software_region_name(SoftwareRegion region)
	{
		switch (region) {
		case SoftwareRegion::HeadroomAware: return "headroom-aware";
		case SoftwareRegion::CompressionRisk: return "compression-risk";
		case SoftwareRegion::SaturationRisk: return "saturation-risk";
		default: return "comfort";
		}
	}
}
